// HLS-related includes
#include "ap_int.h"
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
// #include <iostream>
#include "ddr_kernels.hpp"

static void filter_memory(float m_cutoff, hls::vector<uint32_t, 16>* full_graph, ap_uint<512>* full_graph_cons, hls::vector<float, 16>* m_scores, unsigned int m_num_nodes,
                          unsigned int* m_graph, ap_uint<512>* graph_cons, unsigned int* m_node_list, unsigned int& m_graph_size) {

  ap_uint<8> connections = 0;
  bool new_row = true;
  m_graph_size = 0;
  unsigned int row = 0;
  
  ap_uint<512> multi_full_con = 0;
  ap_uint<512> multi_graph_con = 0;
  ap_uint<8> single_full_con = 0;

  hls::vector<float, 16> multi_scores;
  hls::vector<uint32_t, 16> multi_edges;
  bool mask[16];
  ap_uint<8> prefix_sum[16];
  #pragma HLS array_partition variable=multi_scores dim=1 complete
  #pragma HLS array_partition variable=multi_edges dim=1 complete
  #pragma HLS array_partition variable=mask dim=1 complete
  #pragma HLS array_partition variable=prefix_sum dim=1 complete

  // unsigned int pos_multi = 0;
  // unsigned int pos_single = 0;

  unsigned int con_iterations = m_num_nodes / 64; // number of 512-bit-values that need to be read
  unsigned int con_residue = m_num_nodes % 64; // number of 8-bit-values that are missing after the last complete 512-bit-value
  unsigned int max_con = 64;
  unsigned int scr_iterations = 0;
  unsigned int scr_residue = 0;
  unsigned int max_scr = 16;
  
  for (unsigned int c = 0; c < con_iterations + 1 ; c++){
    max_con = 64;
    if(c == con_iterations)
      max_con = con_residue;
    multi_full_con = full_graph_cons[c];
    multi_graph_con = 0;
    for (unsigned int k = 0; k < max_con ; k++){ // iterate through 64 8-bit values inside the 512-bit variable
      single_full_con = multi_full_con.range((k + 1) * 8 - 1, k * 8);
      if(single_full_con > 0){
        connections = 0;
        new_row = true;
        row = c * 64 + k;
        scr_iterations = single_full_con / 16;
        scr_residue = single_full_con % 16;
        for (unsigned int s = 0; s < scr_iterations + 1 ; s++){
          max_scr = 16;
          if(s == scr_iterations)
            max_scr = scr_residue;
          multi_scores = m_scores[s];
          multi_edges = full_graph[s];
          for (unsigned int i = 0; i < max_scr ; i++){
            #pragma HLS unroll
            if(multi_scores[i] > m_cutoff)
              mask[i] = true;
            else
              mask[i] = false;
          }
          for(unsigned int i = 0; i < max_scr ; i++){
            #pragma HLS unroll
            prefix_sum[i] = 0;
          }
          for(unsigned int i = 0; i < max_scr ; i++){
            #pragma HLS unroll
            for(unsigned int j = 0; j < i + 1 ; j++){
              #pragma HLS unroll
              prefix_sum[i] += mask[j];
            }
          }
          // prefix_sum[0]  = mask[0];
          // prefix_sum[1]  = mask[0] + mask[1];
          // prefix_sum[2]  = mask[0] + mask[1] + mask[2];
          // prefix_sum[3]  = mask[0] + mask[1] + mask[2] + mask[3];
          // prefix_sum[4]  = mask[0] + mask[1] + mask[2] + mask[3] + mask[4];
          // prefix_sum[5]  = mask[0] + mask[1] + mask[2] + mask[3] + mask[4] + mask[5];
          // prefix_sum[6]  = mask[0] + mask[1] + mask[2] + mask[3] + mask[4] + mask[5] + mask[6];
          // prefix_sum[7]  = mask[0] + mask[1] + mask[2] + mask[3] + mask[4] + mask[5] + mask[6] + mask[7];
          // prefix_sum[8]  = mask[0] + mask[1] + mask[2] + mask[3] + mask[4] + mask[5] + mask[6] + mask[7] + mask[8];
          // prefix_sum[9]  = mask[0] + mask[1] + mask[2] + mask[3] + mask[4] + mask[5] + mask[6] + mask[7] + mask[8] + mask[9];
          // prefix_sum[10] = mask[0] + mask[1] + mask[2] + mask[3] + mask[4] + mask[5] + mask[6] + mask[7] + mask[8] + mask[9] + mask[10];
          // prefix_sum[11] = mask[0] + mask[1] + mask[2] + mask[3] + mask[4] + mask[5] + mask[6] + mask[7] + mask[8] + mask[9] + mask[10] + mask[11];
          // prefix_sum[12] = mask[0] + mask[1] + mask[2] + mask[3] + mask[4] + mask[5] + mask[6] + mask[7] + mask[8] + mask[9] + mask[10] + mask[11] + mask[12];
          // prefix_sum[13] = mask[0] + mask[1] + mask[2] + mask[3] + mask[4] + mask[5] + mask[6] + mask[7] + mask[8] + mask[9] + mask[10] + mask[11] + mask[12] + mask[13];
          // prefix_sum[14] = mask[0] + mask[1] + mask[2] + mask[3] + mask[4] + mask[5] + mask[6] + mask[7] + mask[8] + mask[9] + mask[10] + mask[11] + mask[12] + mask[13] + mask[14];
          // prefix_sum[15] = mask[0] + mask[1] + mask[2] + mask[3] + mask[4] + mask[5] + mask[6] + mask[7] + mask[8] + mask[9] + mask[10] + mask[11] + mask[12] + mask[13] + mask[14] + mask[15];
          for (unsigned int i = 0; i < max_scr ; i++){
            #pragma HLS unroll
            if(mask[i])
              m_graph[row * MAX_EDGES + connections + prefix_sum[i] - 1] = multi_edges[i];
          }
          if(max_scr > 0){
            connections += prefix_sum[max_scr - 1];

            if(new_row && prefix_sum[max_scr - 1] > 0){
              new_row = false;
              m_node_list[m_graph_size] = row;
              m_graph_size++;
            }
          }
/* 
1 0 1 1 0 1 0 1
  1 0 1 1 0 1 0 1
    1 0 1 1 0 1 0 1
      1 0 1 1 0 1 0 1
        1 0 1 1 0 1 0 1
          1 0 1 1 0 1 0 1
            1 0 1 1 0 1 0 1
              1 0 1 1 0 1 0 1
1 1 2 3 3 4 4 5

1 0 1 1 0 0 0 0
  1 0 1 1 0 0 0 0
    1 0 1 1 0 0 0 0
      1 0 1 1 0 0 0 0
        1 0 1 1 0 0 0 0
          1 0 1 1 0 0 0 0
            1 0 1 1 0 0 0 0
              1 0 1 1 0 0 0 0
1 1 2 3 3 3 3 3 
 */
          // ###########################################################################################################
          // shuffle logic
          // bool running = true;
          // uint8_t first_false = 17;
          // uint8_t first_true = 17;
          // while(running){
          //   for (uint8_t i = 0; i < max_scr ; i++){
          //     if(first_false > max_scr)
          //       if(!mask[i])
          //         first_false = i;
          //     else if(first_true > max_scr)
          //       if(mask[i])
          //         first_true = i;
          //   }
          // }
          // ###########################################################################################################
        }
        if(!new_row)
          multi_graph_con.range((k + 1) * 8 - 1, k * 8) = connections;
      }
    }
    graph_cons[c] = multi_graph_con;
  }
}

static void compute_core(unsigned int* m_graph, ap_uint<512>* graph_cons, unsigned int m_num_nodes, hls::stream<unsigned int>& outStream, unsigned int* m_node_list){

  static unsigned int component[MAX_COMPONENT_SIZE];
  static unsigned int processed[MAX_TRUE_NODES];
  compute_reset_processed:
  for (unsigned int i = 0; i < MAX_TRUE_NODES; i++){
    processed[i] = false;
  }
  unsigned int current_component_size = 0;
  unsigned int processed_nodes = 0;
  unsigned int next_node = 0;
  ap_uint<8> next_node_cons = 0;

  unsigned int row = 0;
  unsigned int max_con = 64;
  ap_uint<512> multi_graph_con = 0;
  ap_uint<8> single_graph_con = 0;
  unsigned int pos_multi = 0;
  unsigned int pos_single = 0;
  unsigned int con_iterations = m_num_nodes / 64; // number of 512-bit-values that need to be read
  unsigned int con_residue = m_num_nodes % 64; // number of 8-bit-values that are missing after the last complete 512-bit-value

  for (unsigned int c = 0; c < con_iterations + 1 ; c++){
    max_con = 64;
    if(c == con_iterations)
      max_con = con_residue;
    multi_graph_con = graph_cons[c];
    for (unsigned int k = 0; k < max_con ; k++){
      single_graph_con = multi_graph_con.range((k + 1) * 8 - 1, k * 8);
      row = c * 64 + k;
      if(single_graph_con == 0)
        processed[row] = true;
      // node with connections that has not been processed yet -> new component
      else if (!processed[row]){
        // new component needs to reset parameters
        compute_reset_component:
        for (unsigned int i = 0; i < MAX_COMPONENT_SIZE; i++)
          component[i] = 0;
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
          if(next_node_cons > 0) // avoid for-loop 0 times
            compute_connections:
            for(unsigned int i = 0 ; i < next_node_cons; i++){
              #pragma HLS loop_tripcount min=1 avg=2 max=MAX_EDGES
              if(!processed[m_graph[next_node * MAX_EDGES + i]] && current_component_size < MAX_COMPONENT_SIZE){
                bool new_node = true;
                compute_check_component:
                for(unsigned int j = 0 ; j < current_component_size; j++){
                  #pragma HLS loop_tripcount min=1 avg=4 max=MAX_COMPONENT_SIZE
                    if(component[j] == m_graph[next_node * MAX_EDGES + i])
                      new_node = false;
                }
                if(new_node){
                  component[current_component_size] = m_graph[next_node * MAX_EDGES + i];
                  current_component_size++;
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
            unsigned int* io_graph, ap_uint<512>* io_graph_cons, unsigned int* io_node_list,
            unsigned int* out_components, unsigned int num_nodes, float cutoff) {
    
    // hls::vector<float, 16>* in_scores

    #pragma HLS INTERFACE m_axi port = in_full_graph      bundle=gmem0
    #pragma HLS INTERFACE m_axi port = in_full_graph_cons bundle=gmem1
    #pragma HLS INTERFACE m_axi port = in_scores          bundle=gmem2
    #pragma HLS INTERFACE m_axi port = io_graph           bundle=gmem3
    #pragma HLS INTERFACE m_axi port = io_graph_cons      bundle=gmem4
    #pragma HLS INTERFACE m_axi port = io_node_list       bundle=gmem5
    #pragma HLS INTERFACE m_axi port = out_components     bundle=gmem6

    static hls::stream<unsigned int> outStream_components("output_stream_components");
    static unsigned int graph_size;
    #pragma HLS STREAM variable=graph_size type=pipo

    #pragma HLS dataflow
    filter_memory(cutoff, in_full_graph, in_full_graph_cons, in_scores, num_nodes, io_graph, io_graph_cons, io_node_list, graph_size);
    compute_core(io_graph, io_graph_cons, graph_size, outStream_components, io_node_list);
    write_components(out_components, outStream_components, num_nodes);

  }
}




