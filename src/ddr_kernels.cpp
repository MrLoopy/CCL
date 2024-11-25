// HLS-related includes
#include "ap_int.h"
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
#include <iostream>
#include "ddr_kernels.hpp"

static void filter_memory(float m_cutoff, hls::vector<uint32_t, 16>* full_graph, ap_uint<512>* full_graph_cons, hls::vector<float, 16>* m_scores, unsigned int m_num_nodes,
                          unsigned int* m_graph, ap_uint<512>* graph_cons, unsigned int& m_graph_size, bool ctrl) {

  ap_uint<8> connections = 0;
  bool new_row = true;
  m_graph_size = 1; // has to start at 1, cause 0 indicates that no index has been given yet
  unsigned int row = 0;
  
  ap_uint<512> multi_full_con = 0;
  ap_uint<512> multi_graph_con = 0;
  ap_uint<8> single_full_con = 0;

  hls::vector<float, 16> multi_scores;
  #pragma HLS array_partition variable=multi_scores dim=1 complete
  multi_scores = hls::vector<float, 16>(0.0);
  hls::vector<uint32_t, 16> multi_edges;
  #pragma HLS array_partition variable=multi_edges dim=1 complete
  multi_edges = hls::vector<uint32_t, 16>(0.0);

  unsigned int con_iterations = m_num_nodes / 64; // number of 512-bit-values that need to be read
  unsigned int con_residue = m_num_nodes % 64; // number of 8-bit-values that are missing after the last complete 512-bit-value
  // unsigned int max_con = 64;
  unsigned int scr_iterations = 0;
  unsigned int scr_residue = 0;
  // unsigned int max_scr = 16;

  // ###########################################
  {
  //   unsigned int height = 8;
  //   unsigned int width = 4;

  //   midStream << height * (2 * width + 2) + 12; // 8 * (2 * 4 + 2) + 12 = 92

  //   multi_full_con = full_graph_cons[0];
  //   for(unsigned int i = 0; i < height; i++){
  //     midStream << 2 * width + 1;
  //     midStream << (unsigned int)multi_full_con.range((i + 1) * 8 - 1, i * 8);
  //     multi_edges = full_graph[i * MAX_FULL_GRAPH_BLOCKS];
  //     multi_scores = m_scores[i * MAX_FULL_GRAPH_BLOCKS];
  //     for(unsigned int j = 0; j < width; j++)
  //       midStream << multi_edges[j];
  //     for(unsigned int j = 0; j < width; j++){
  //       if(multi_scores[j] > m_cutoff) midStream << 1;
  //       else midStream << 0;
  //     }
  //   }
  }
  // ###########################################

  // unsigned int reached_c = 0;
  // unsigned int reached_k = 0;
  // unsigned int reached_s = 0;
  // unsigned int reached_i = 0;
  // unsigned int reached_hs = 0;
  // unsigned int reached_nr = 0;
  // unsigned int reached_wcs = 0;
  // unsigned int reached_wcm = 0;

  for (unsigned int c = 0; c < con_iterations + 1 ; c++){
    // reached_c++;
    // max_con = 64;
    // if(c == con_iterations){
    //   max_con = con_residue;
    // }
    multi_full_con = full_graph_cons[c];
    multi_graph_con = 0;
    for (unsigned int k = 0; k < 64 ; k++){ // iterate through 64 8-bit values inside the 512-bit variable
      if(c == con_iterations && k == con_residue){
        break;
      }
      // reached_k++;

      single_full_con = multi_full_con.range((k + 1) * 8 - 1, k * 8);
      if(single_full_con > 0){
        connections = 0;
        new_row = true;
        row = c * 64 + k;

        scr_iterations = single_full_con / 16;
        scr_residue = single_full_con % 16;
        for (unsigned int s = 0; s < scr_iterations + 1 ; s++){
          // reached_s++;
          // max_scr = 16;
          // if(s == scr_iterations){
          //   max_scr = scr_residue;
          // }
          multi_scores = m_scores[row * MAX_FULL_GRAPH_BLOCKS + s];
          multi_edges = full_graph[row * MAX_FULL_GRAPH_BLOCKS + s];
          for (unsigned int i = 0; i < 16 ; i++){
            if(s == scr_iterations && i == scr_residue){
              break;
            }
            // reached_i++;
            if(multi_scores[i] > m_cutoff){
              // reached_hs++;
              m_graph[row * MAX_EDGES + connections] = multi_edges[i];
              connections++;
              if(new_row){
                // reached_nr++;
                new_row = false;
                m_graph_size++;
              }
            }
          }
        }
        if(!new_row){
          // reached_wcs++;
          multi_graph_con.range((k + 1) * 8 - 1, k * 8) = connections;
        }
      }
    }
    // reached_wcm++;
    graph_cons[c] = multi_graph_con;
  }

  ctrl = true;

  // ###########################################
  {
    // multi_edges[0] = m_num_nodes;
    // multi_edges[1] = con_iterations;
    // multi_edges[2] = con_residue;
    // multi_edges[3] = reached_c;
    // multi_edges[4] = reached_k;
    // multi_edges[5] = reached_s;
    // multi_edges[6] = reached_i;
    // multi_edges[7] = reached_hs;
    // multi_edges[8] = reached_nr;
    // multi_edges[9] = reached_wcs;
    // multi_edges[10] = reached_wcm;
    // multi_edges[11] = 0;
    // multi_edges[12] = 0;
    // multi_edges[13] = 0;
    // multi_edges[14] = 0;
    // full_graph[0] = multi_edges;
  }
  {
  //   midStream << 11;

  //   midStream << m_num_nodes;
  //   midStream << con_iterations;
  //   midStream << con_residue;
  //   midStream << reached_c;
  //   midStream << reached_k;

  //   midStream << reached_s;
  //   midStream << reached_i;
  //   midStream << reached_hs;
  //   midStream << reached_nr;
  //   midStream << reached_wcs;

  //   midStream << reached_wcm;
  }
  // ###########################################

}

static void compute_core(unsigned int* m_graph, ap_uint<512>* graph_cons, unsigned int m_num_nodes, hls::stream<unsigned int>& outStream, bool ctrl){

  while(!ctrl){;}

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
  unsigned int pos_multi = 0;
  unsigned int pos_single = 0;
  unsigned int con_iterations = m_num_nodes / 64; // number of 512-bit-values that need to be read
  unsigned int con_residue = m_num_nodes % 64; // number of 8-bit-values that are missing after the last complete 512-bit-value

  // ###########################################
  {
    // unsigned int size = midStream.read();
    // for (unsigned int i = 0; i < size; i++)
    //   outStream << midStream.read();

    // unsigned int height = 8;
    // unsigned int width = 4;

    // multi_graph_con = graph_cons[0];
    // for(unsigned int i = 0; i < height; i++){
    //   outStream << width + 1;
    //   outStream << (unsigned int)multi_graph_con.range((i + 1) * 8 - 1, i * 8);
    //   for(unsigned int j = 0; j < width; j++)
    //     outStream << m_graph[i * MAX_EDGES + j];
    // }
  }
  // ###########################################

  for (unsigned int c = 0; c < con_iterations + 1 ; c++){
    multi_graph_con = graph_cons[c];
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
        // new component needs to reset parameters
        compute_reset_component:
        for (unsigned int i = 0; i < MAX_COMPONENT_SIZE; i++){
          component[i] = 0;
        }
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
            compute_connections:
            for(unsigned int i = 0 ; i < next_node_cons; i++){
              #pragma HLS loop_tripcount min=1 avg=2 max=MAX_EDGES
              if(!processed[m_graph[next_node * MAX_EDGES + i]] && current_component_size < MAX_COMPONENT_SIZE){
                bool new_node = true;
                compute_check_component:
                for(unsigned int j = 0 ; j < current_component_size; j++){
                  #pragma HLS loop_tripcount min=1 avg=4 max=MAX_COMPONENT_SIZE
                    if(component[j] == m_graph[next_node * MAX_EDGES + i]){
                      new_node = false;
                    }
                }
                if(new_node){
                  component[current_component_size] = m_graph[next_node * MAX_EDGES + i];
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
            unsigned int* io_graph, ap_uint<512>* io_graph_cons,
            unsigned int* out_components, unsigned int num_nodes, float cutoff) {
    
    // hls::vector<float, 16>* in_scores

    #pragma HLS INTERFACE m_axi port = in_full_graph      bundle=gmem0
    #pragma HLS INTERFACE m_axi port = in_full_graph_cons bundle=gmem1
    #pragma HLS INTERFACE m_axi port = in_scores          bundle=gmem2
    #pragma HLS INTERFACE m_axi port = io_graph           bundle=gmem3
    #pragma HLS INTERFACE m_axi port = io_graph_cons      bundle=gmem4
    #pragma HLS INTERFACE m_axi port = out_components     bundle=gmem5

    static hls::stream<unsigned int> outStream_components("output_stream_components");
    // static hls::stream<unsigned int> midStream_temp("midStream");
    static unsigned int graph_size = 0;
    #pragma HLS STREAM variable=graph_size type=pipo
    static bool control = false;
    #pragma HLS STREAM variable=control type=pipo

    #pragma HLS dataflow
    filter_memory(cutoff, in_full_graph, in_full_graph_cons, in_scores, num_nodes, io_graph, io_graph_cons, graph_size, control);
    compute_core(io_graph, io_graph_cons, num_nodes, outStream_components, control);
    write_components(out_components, outStream_components, num_nodes);

  }
}




