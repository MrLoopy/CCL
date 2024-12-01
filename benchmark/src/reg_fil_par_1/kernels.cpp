// HLS-related includes
#include "ap_int.h"
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
#include <iostream>
#include "large_kernels.hpp"

static void filter_memory(float m_cutoff, hls::vector<uint32_t, 16>* full_graph, ap_uint<512>* full_graph_cons, hls::vector<float, 16>* m_scores, unsigned int m_num_nodes,
                          hls::vector<uint32_t, 16>* m_graph_0, ap_uint<512>* graph_cons_0, hls::stream<bool>& ctrl_0
                          // hls::vector<uint32_t, 16>* m_graph_1, ap_uint<512>* graph_cons_1, hls::stream<bool>& ctrl_1
                          // hls::vector<uint32_t, 16>* m_graph_2, ap_uint<512>* graph_cons_2, hls::stream<bool>& ctrl_2,
                          // hls::vector<uint32_t, 16>* m_graph_3, ap_uint<512>* graph_cons_3, hls::stream<bool>& ctrl_3
                          // hls::vector<uint32_t, 16>* m_graph_4, ap_uint<512>* graph_cons_4, hls::stream<bool>& ctrl_4,
                          // hls::vector<uint32_t, 16>* m_graph_5, ap_uint<512>* graph_cons_5, hls::stream<bool>& ctrl_5,
                          // hls::vector<uint32_t, 16>* m_graph_6, ap_uint<512>* graph_cons_6, hls::stream<bool>& ctrl_6,
                          // hls::vector<uint32_t, 16>* m_graph_7, ap_uint<512>* graph_cons_7, hls::stream<bool>& ctrl_7
                          ) {

  ap_uint<8> connections = 0;
  bool new_row = true;
  unsigned int row = 0;
  
  ap_uint<512> multi_full_con = 0;
  ap_uint<512> multi_graph_con = 0;
  ap_uint<8> single_full_con = 0;

  hls::vector<float, 16> multi_scores;
  multi_scores = hls::vector<float, 16>(0.0);
  hls::vector<uint32_t, 16> multi_edges;
  multi_edges = hls::vector<uint32_t, 16>(0);
  hls::vector<uint32_t, 16> multi_nodes;
  multi_nodes = hls::vector<uint32_t, 16>(0);
  bool mask[16];
  ap_uint<8> prefix_sum[16];
  #pragma HLS array_partition variable=multi_scores complete
  #pragma HLS array_partition variable=multi_edges complete
  #pragma HLS array_partition variable=multi_nodes complete
  #pragma HLS array_partition variable=mask complete
  #pragma HLS array_partition variable=prefix_sum complete

  unsigned int con_iterations = m_num_nodes / 64; // number of 512-bit-values that need to be read
  unsigned int con_residue = m_num_nodes % 64; // number of 8-bit-values that are missing after the last complete 512-bit-value
  unsigned int scr_iterations = 0;
  // unsigned int scr_residue = 0;

  filter_rows_outer:
  for (unsigned int c = 0; c < con_iterations + 1 ; c++){
    #pragma HLS loop_tripcount min=290000/64 avg=335000/64 max=MAX_TOTAL_NODES/64
    multi_full_con = full_graph_cons[c];
    multi_graph_con = 0;
    filter_rows_inner:
    for (unsigned int k = 0; k < 64 ; k++){ // iterate through 64 8-bit values inside the 512-bit variable
      if(c == con_iterations && k == con_residue){
        break;
      }

      single_full_con = multi_full_con.range((k + 1) * 8 - 1, k * 8);
      if(single_full_con > 0){
        connections = 0;
        new_row = true;
        row = c * 64 + k;

        scr_iterations = single_full_con / 16;
        // scr_residue = single_full_con % 16;
        filter_columns_outer:
        for (unsigned int s = 0; s < scr_iterations + 1 ; s++){
          #pragma HLS loop_tripcount min=1 avg=1 max=MAX_FULL_GRAPH_EDGES/16
          multi_scores = m_scores[row * MAX_FULL_GRAPH_BLOCKS + s];
          multi_edges = full_graph[row * MAX_FULL_GRAPH_BLOCKS + s];
          filter_mask:
          for (unsigned int i = 0; i < 16 ; i++){
            #pragma HLS unroll
            // bit mask
            if(multi_scores[i] > m_cutoff)
              mask[i] = true;
            else
              mask[i] = false;
          }
          filter_prefix_sum:
          for (unsigned int i = 0; i < 16 ; i++){
            #pragma HLS unroll
            // calc prefix_sum
            prefix_sum[i] = 0;
            filter_prefix_sum_inner:
            for(unsigned int j = 0; j < i + 1 ; j++){
              #pragma HLS unroll
              prefix_sum[i] += mask[j];
            }
            // store indices
            if(mask[i])
              multi_nodes[connections + prefix_sum[i] - 1] = multi_edges[i];
          }
          // increase connections by calculated value
          connections += prefix_sum[15];
          // set bool to write #connections later
          if(prefix_sum[15] > 0 && new_row){
            new_row = false;
          }
        }
        if(!new_row){
          multi_graph_con.range((k + 1) * 8 - 1, k * 8) = connections;
          m_graph_0[row] = multi_nodes;
          // m_graph_1[row] = multi_nodes;
          // m_graph_2[row] = multi_nodes;
          // m_graph_3[row] = multi_nodes;
          // m_graph_4[row] = multi_nodes;
          // m_graph_5[row] = multi_nodes;
          // m_graph_6[row] = multi_nodes;
          // m_graph_7[row] = multi_nodes;
        }
      }
    }
    graph_cons_0[c] = multi_graph_con;
    // graph_cons_1[c] = multi_graph_con;
    // graph_cons_2[c] = multi_graph_con;
    // graph_cons_3[c] = multi_graph_con;
    // graph_cons_4[c] = multi_graph_con;
    // graph_cons_5[c] = multi_graph_con;
    // graph_cons_6[c] = multi_graph_con;
    // graph_cons_7[c] = multi_graph_con;
  }
  ctrl_0 << true;
  // ctrl_1 << true;
  // ctrl_2 << true;
  // ctrl_3 << true;
  // ctrl_4 << true;
  // ctrl_5 << true;
  // ctrl_6 << true;
  // ctrl_7 << true;
}

static void compute_core(hls::vector<uint32_t, 16>* m_graph, ap_uint<512>* graph_cons, unsigned int m_num_nodes, hls::stream<unsigned int>& outStream, hls::stream<bool>& ctrl){

  static unsigned int component[MAX_COMPONENT_SIZE];
  static bool processed[MAX_TOTAL_NODES];
  compute_reset_processed:
  for (unsigned int i = 0; i < MAX_TOTAL_NODES; i++){
    processed[i] = false;
  }
  unsigned int current_component_size = 0;
  unsigned int processed_nodes = 0;
  unsigned int next_node = 0;
  ap_uint<8> next_node_cons = 0;

  unsigned int row = 0;
  ap_uint<512> multi_graph_con = 0;
  ap_uint<8> single_graph_con = 0;
  hls::vector<uint32_t, 16> multi_nodes;
  multi_nodes = hls::vector<uint32_t, 16>(0);
  unsigned int pos_multi = 0;
  unsigned int pos_single = 0;
  unsigned int con_iterations = m_num_nodes / 64; // number of 512-bit-values that need to be read
  unsigned int con_residue = m_num_nodes % 64; // number of 8-bit-values that are missing after the last complete 512-bit-value

  bool temp = ctrl.read();

  compute_rows_outer:
  for (unsigned int c = 0; c < con_iterations + 1 ; c++){
    multi_graph_con = graph_cons[c];
    compute_rows_inner:
    for (unsigned int k = 0; k < 64 ; k++){
      if(c == con_iterations && k == con_residue){
        break;
      }
      single_graph_con = multi_graph_con.range((k + 1) * 8 - 1, k * 8);
      row = c * 64 + k;

      if(single_graph_con == 0){
        processed[row] = true;
      }
      // node with connections that has not been processed yet -> new component
      else if (!processed[row]){
        current_component_size = 0;
        processed_nodes = 0;

        // first node gets added to component
        component[current_component_size] = row;
        current_component_size++;

        // main loop to iterate through the component
        while (current_component_size != processed_nodes){
          // get next node of the component that has not yet been processed
          next_node = component[processed_nodes];

          // add all connections of current node to component if not already processed
          pos_multi = next_node / 64;
          pos_single = next_node % 64;
          next_node_cons = graph_cons[pos_multi].range((pos_single + 1) * 8 - 1, pos_single * 8);
          if(next_node_cons > 0){
            multi_nodes = m_graph[next_node];
            compute_connections:
            for(unsigned int i = 0 ; i < next_node_cons; i++){
              #pragma HLS loop_tripcount min=1 avg=2 max=MAX_EDGES
              if(!processed[multi_nodes[i]] && current_component_size < MAX_COMPONENT_SIZE){
                bool new_node = true;
                compute_check_component:
                for(unsigned int j = 0 ; j < current_component_size; j++){
                  #pragma HLS loop_tripcount min=1 avg=4 max=MAX_COMPONENT_SIZE
                    if(component[j] == multi_nodes[i]){
                      new_node = false;
                    }
                }
                if(new_node){
                  component[current_component_size] = multi_nodes[i];
                  current_component_size++;
                }
              }
            }
          }
          // after all connections of node have been added, the node is done and can be marked as processed
          processed[next_node] = true;
          processed_nodes++;
        }

        // after component is fully set up, we can export its size and indices
        outStream << current_component_size;
        compute_write_output:
        for (unsigned int i = 0; i < current_component_size; i++){
          #pragma HLS loop_tripcount min=1 avg=8 max=MAX_COMPONENT_SIZE
          outStream << component[i];
        }
      }
    }
  }
  outStream << 0;
}

/* static void sub_core(
                      hls::vector<uint32_t, 16>* m_graph, ap_uint<512>* graph_cons, hls::stream<unsigned int>& outStream, hls::stream<bool>& ctrl,
                      unsigned int* component, bool* processed, unsigned int start, unsigned int end){

  core_reset_processed:
  for (unsigned int i = 0; i < MAX_TOTAL_NODES; i++)
    processed[i] = false;

  unsigned int current_component_size = 0;
  unsigned int processed_nodes = 0;
  unsigned int next_node = 0;
  ap_uint<8> next_node_cons = 0;

  unsigned int row = 0;
  ap_uint<512> multi_graph_con = 0;
  ap_uint<8> single_graph_con = 0;
  hls::vector<uint32_t, 16> multi_nodes;
  multi_nodes = hls::vector<uint32_t, 16>(0);
  unsigned int pos_multi = 0;
  unsigned int pos_single = 0;
  unsigned int con_iterations = end / 64;
  unsigned int con_residue = end % 64;
  unsigned int start_iterations = start / 64;
  bool duplicate = false;

  bool temp = ctrl.read();

  core_rows_outer:
  for (unsigned int c = start_iterations; c < con_iterations + 1 ; c++){
    #pragma HLS loop_tripcount min=150000 / 64 avg=MAX_TOTAL_NODES / ( 64 * 2) max=MAX_TOTAL_NODES / 64
    multi_graph_con = graph_cons[c];
    core_rows_inner:
    for (unsigned int k = 0; k < 64 ; k++){
      if(c == con_iterations && k == con_residue){
        break;
      }
      single_graph_con = multi_graph_con.range((k + 1) * 8 - 1, k * 8);
      row = c * 64 + k;

      if(single_graph_con == 0){
        processed[row] = true;
      }
      // node with connections that has not been processed yet -> new component
      else if (!processed[row]){
        current_component_size = 0;
        processed_nodes = 0;

        // first node gets added to component
        component[current_component_size] = row;
        current_component_size++;

        // main loop to iterate through the component
        while (current_component_size != processed_nodes){
          // get next node of the component that has not yet been processed
          next_node = component[processed_nodes];
          if(next_node < start)
            duplicate = true;

          // add all connections of current node to component if not already processed
          pos_multi = next_node / 64;
          pos_single = next_node % 64;
          next_node_cons = graph_cons[pos_multi].range((pos_single + 1) * 8 - 1, pos_single * 8);
          if(next_node_cons > 0){
            multi_nodes = m_graph[next_node];
            core_connections:
            for(unsigned int i = 0 ; i < next_node_cons; i++){
              #pragma HLS loop_tripcount min=1 avg=2 max=MAX_EDGES
              if(!processed[multi_nodes[i]] && current_component_size < MAX_COMPONENT_SIZE){
                bool new_node = true;
                core_check_component:
                for(unsigned int j = 0 ; j < current_component_size; j++){
                  #pragma HLS loop_tripcount min=1 avg=4 max=MAX_COMPONENT_SIZE
                    if(component[j] == multi_nodes[i]){
                      new_node = false;
                    }
                }
                if(new_node){
                  component[current_component_size] = multi_nodes[i];
                  current_component_size++;
                }
              }
            }
          }
          // after all connections of node have been added, the node is done and can be marked as processed
          processed[next_node] = true;
          processed_nodes++;
        }
        if(duplicate)
          duplicate = false;
        else{
          outStream << current_component_size;
          core_write_output:
          for (unsigned int i = 0; i < current_component_size; i++){
            #pragma HLS loop_tripcount min=1 avg=8 max=MAX_COMPONENT_SIZE
            outStream << component[i];
          }
        }
      }
    }
  }
  outStream << 0;
} */

/*static void merge_streams(hls::stream<unsigned int>& outStream,
                          hls::stream<unsigned int>& stream_0, hls::stream<unsigned int>& stream_1
                          // hls::stream<unsigned int>& stream_2, hls::stream<unsigned int>& stream_3
                          // hls::stream<unsigned int>& stream_4, hls::stream<unsigned int>& stream_5,
                          // hls::stream<unsigned int>& stream_6, hls::stream<unsigned int>& stream_7
                          ) {

  bool str_0 = true;
  bool str_1 = true;
  bool str_2 = false;
  bool str_3 = false;
  bool str_4 = false;
  bool str_5 = false;
  bool str_6 = false;
  bool str_7 = false;
  bool strms = true;
  unsigned int component_size = 0;

  merge_while:
  while(strms){
    if(stream_0.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_0.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_0 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5  && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_0:
        for (unsigned int i = 0; i < component_size; i++){
          #pragma HLS loop_tripcount min=2 avg=8 max=MAX_COMPONENT_SIZE
          outStream << stream_0.read();
        }
      }
    }
    if(stream_1.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_1.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_1 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5  && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_1:
        for (unsigned int i = 0; i < component_size; i++){
          #pragma HLS loop_tripcount min=2 avg=8 max=MAX_COMPONENT_SIZE
          outStream << stream_1.read();
        }
      }
    }
/*    if(stream_2.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_2.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_2 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5  && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_2:
        for (unsigned int i = 0; i < component_size; i++){
          #pragma HLS loop_tripcount min=2 avg=8 max=MAX_COMPONENT_SIZE
          outStream << stream_2.read();
        }
      }
    }
    if(stream_3.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_3.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_3 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5  && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_3:
        for (unsigned int i = 0; i < component_size; i++){
          #pragma HLS loop_tripcount min=2 avg=8 max=MAX_COMPONENT_SIZE
          outStream << stream_3.read();
        }
      }
    }
/*     if(stream_4.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_4.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_4 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5  && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_4:
        for (unsigned int i = 0; i < component_size; i++){
          #pragma HLS loop_tripcount min=2 avg=8 max=MAX_COMPONENT_SIZE
          outStream << stream_4.read();
        }
      }
    }
    if(stream_5.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_5.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_5 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5  && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_5:
        for (unsigned int i = 0; i < component_size; i++){
          #pragma HLS loop_tripcount min=2 avg=8 max=MAX_COMPONENT_SIZE
          outStream << stream_5.read();
        }
      }
    }
    if(stream_6.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_6.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_6 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5  && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_6:
        for (unsigned int i = 0; i < component_size; i++){
          #pragma HLS loop_tripcount min=2 avg=8 max=MAX_COMPONENT_SIZE
          outStream << stream_6.read();
        }
      }
    }
    if(stream_7.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_7.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_7 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5  && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_7:
        for (unsigned int i = 0; i < component_size; i++){
          #pragma HLS loop_tripcount min=2 avg=8 max=MAX_COMPONENT_SIZE
          outStream << stream_7.read();
        }
      }
    }
 */
//   }
// }

static void write_components(unsigned int* out, hls::stream<unsigned int>& outStream, unsigned int size) {

  bool stream_running = true;
  unsigned int stream_size = 1; // position 0 in the Output will be the size of the total output. Therefore size needs to start at 1
  unsigned int component_size = 0;
  out_while:
  while(stream_running)
    if(outStream.size() > 0){
      // every first number of each component is the size of the component;
      component_size = outStream.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        out[0] = stream_size;
        stream_running = false;
        break;
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        out[stream_size] = component_size;
        stream_size++;
        out_write_nodes:
        for (unsigned int i = 0; i < component_size; i++){
          #pragma HLS loop_tripcount min=2 avg=8 max=MAX_COMPONENT_SIZE
          out[stream_size] = outStream.read();
          stream_size++;
        }
      }
    }
}

extern "C" {

  void CCL(
            hls::vector<uint32_t, 16>* in_full_graph, ap_uint<512>* in_full_graph_cons,  hls::vector<float, 16>* in_scores,
            hls::vector<uint32_t, 16>* io_graph_0, ap_uint<512>* io_graph_cons_0,
            // hls::vector<uint32_t, 16>* io_graph_1, ap_uint<512>* io_graph_cons_1,
            // hls::vector<uint32_t, 16>* io_graph_2, ap_uint<512>* io_graph_cons_2,
            // hls::vector<uint32_t, 16>* io_graph_3, ap_uint<512>* io_graph_cons_3,
            // hls::vector<uint32_t, 16>* io_graph_4, ap_uint<512>* io_graph_cons_4,
            // hls::vector<uint32_t, 16>* io_graph_5, ap_uint<512>* io_graph_cons_5,
            // hls::vector<uint32_t, 16>* io_graph_6, ap_uint<512>* io_graph_cons_6,
            // hls::vector<uint32_t, 16>* io_graph_7, ap_uint<512>* io_graph_cons_7,
            unsigned int* out_components, unsigned int num_nodes, float cutoff) {
    
    #pragma HLS INTERFACE m_axi port = in_full_graph      bundle=gmem0
    #pragma HLS INTERFACE m_axi port = in_full_graph_cons bundle=gmem1
    #pragma HLS INTERFACE m_axi port = in_scores          bundle=gmem2
    #pragma HLS INTERFACE m_axi port = io_graph_0         bundle=gmem3
    #pragma HLS INTERFACE m_axi port = io_graph_cons_0    bundle=gmem4
    // #pragma HLS INTERFACE m_axi port = io_graph_1         bundle=gmem5
    // #pragma HLS INTERFACE m_axi port = io_graph_cons_1    bundle=gmem6
    // #pragma HLS INTERFACE m_axi port = io_graph_2         bundle=gmem7
    // #pragma HLS INTERFACE m_axi port = io_graph_cons_2    bundle=gmem8
    // #pragma HLS INTERFACE m_axi port = io_graph_3         bundle=gmem9
    // #pragma HLS INTERFACE m_axi port = io_graph_cons_3    bundle=gmem10
    // #pragma HLS INTERFACE m_axi port = io_graph_4         bundle=gmem11
    // #pragma HLS INTERFACE m_axi port = io_graph_cons_4    bundle=gmem12
    // #pragma HLS INTERFACE m_axi port = io_graph_5         bundle=gmem13
    // #pragma HLS INTERFACE m_axi port = io_graph_cons_5    bundle=gmem14
    // #pragma HLS INTERFACE m_axi port = io_graph_6         bundle=gmem15
    // #pragma HLS INTERFACE m_axi port = io_graph_cons_6    bundle=gmem16
    // #pragma HLS INTERFACE m_axi port = io_graph_7         bundle=gmem17
    // #pragma HLS INTERFACE m_axi port = io_graph_cons_7    bundle=gmem18
    #pragma HLS INTERFACE m_axi port = out_components     bundle=gmem5

    static hls::stream<unsigned int> outStream_components("output_stream_components");
    static hls::stream<bool> ctrl_0("ctrl_0");
    // static hls::stream<unsigned int> out_0("out_0");
    // static hls::stream<bool> ctrl_1("ctrl_1");
    // static hls::stream<unsigned int> out_1("out_1");
    // static hls::stream<bool> ctrl_2("ctrl_2");
    // static hls::stream<unsigned int> out_2("out_2");
    // static hls::stream<bool> ctrl_3("ctrl_3");
    // static hls::stream<unsigned int> out_3("out_3");
    // static hls::stream<bool> ctrl_4("ctrl_4");
    // static hls::stream<unsigned int> out_4("out_4");
    // static hls::stream<bool> ctrl_5("ctrl_5");
    // static hls::stream<unsigned int> out_5("out_5");
    // static hls::stream<bool> ctrl_6("ctrl_6");
    // static hls::stream<unsigned int> out_6("out_6");
    // static hls::stream<bool> ctrl_7("ctrl_7");
    // static hls::stream<unsigned int> out_7("out_7");

    #pragma HLS dataflow
    filter_memory(cutoff, in_full_graph, in_full_graph_cons, in_scores, num_nodes,
                  io_graph_0, io_graph_cons_0, ctrl_0
                  // io_graph_1, io_graph_cons_1, ctrl_1,
                  // io_graph_2, io_graph_cons_2, ctrl_2,
                  // io_graph_3, io_graph_cons_3, ctrl_3
                  // io_graph_4, io_graph_cons_4, ctrl_4,
                  // io_graph_5, io_graph_cons_5, ctrl_5,
                  // io_graph_6, io_graph_cons_6, ctrl_6,
                  // io_graph_7, io_graph_cons_7, ctrl_7
                  );

    // static const unsigned int factor = 4;

    // static unsigned int component_0[MAX_COMPONENT_SIZE];
    // static bool processed_0[MAX_TOTAL_NODES];
    // static unsigned int component_1[MAX_COMPONENT_SIZE];
    // static bool processed_1[MAX_TOTAL_NODES];
    // static unsigned int component_2[MAX_COMPONENT_SIZE];
    // static bool processed_2[MAX_TOTAL_NODES];
    // static unsigned int component_3[MAX_COMPONENT_SIZE];
    // static bool processed_3[MAX_TOTAL_NODES];
    // static unsigned int component_4[MAX_COMPONENT_SIZE];
    // static bool processed_4[MAX_TOTAL_NODES];
    // static unsigned int component_5[MAX_COMPONENT_SIZE];
    // static bool processed_5[MAX_TOTAL_NODES];
    // static unsigned int component_6[MAX_COMPONENT_SIZE];
    // static bool processed_6[MAX_TOTAL_NODES];
    // static unsigned int component_7[MAX_COMPONENT_SIZE];
    // static bool processed_7[MAX_TOTAL_NODES];

    // sub_core(io_graph_0, io_graph_cons_0, out_0, ctrl_0, component_0, processed_0, 0, (num_nodes * 1 / factor) - ((num_nodes * 1 / factor) % 64));
    // sub_core(io_graph_1, io_graph_cons_1, out_1, ctrl_1, component_1, processed_1, (num_nodes * 1 / factor) - ((num_nodes * 1 / factor) % 64), (num_nodes * 2 / factor) - ((num_nodes * 2 / factor) % 64));
    // sub_core(io_graph_2, io_graph_cons_2, out_2, ctrl_2, component_2, processed_2, (num_nodes * 2 / factor) - ((num_nodes * 2 / factor) % 64), (num_nodes * 3 / factor) - ((num_nodes * 3 / factor) % 64));
    // sub_core(io_graph_3, io_graph_cons_3, out_3, ctrl_3, component_3, processed_3, (num_nodes * 3 / factor) - ((num_nodes * 3 / factor) % 64), num_nodes);
    // sub_core(io_graph_4, io_graph_cons_4, out_4, ctrl_4, component_4, processed_4, (num_nodes * 4 / factor) - ((num_nodes * 4 / factor) % 64), (num_nodes * 5 / factor) - ((num_nodes * 5 / factor) % 64));
    // sub_core(io_graph_5, io_graph_cons_5, out_5, ctrl_5, component_5, processed_5, (num_nodes * 5 / factor) - ((num_nodes * 5 / factor) % 64), (num_nodes * 6 / factor) - ((num_nodes * 6 / factor) % 64));
    // sub_core(io_graph_6, io_graph_cons_6, out_6, ctrl_6, component_6, processed_6, (num_nodes * 6 / factor) - ((num_nodes * 6 / factor) % 64), (num_nodes * 7 / factor) - ((num_nodes * 7 / factor) % 64));
    // sub_core(io_graph_7, io_graph_cons_7, out_7, ctrl_7, component_7, processed_7, (num_nodes * 7 / factor) - ((num_nodes * 7 / factor) % 64), num_nodes);

    // merge_streams(outStream_components, out_0, out_1, out_2, out_3/* , out_4, out_5, out_6, out_7 */);

    compute_core(io_graph_0, io_graph_cons_0, num_nodes, outStream_components, ctrl_0);

    write_components(out_components, outStream_components, num_nodes);

  }
}




