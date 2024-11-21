// HLS-related includes
#include "ap_int.h"
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
// #include <iostream>
#include "ddr_kernels.hpp"

static void filter_memory(float m_cutoff, unsigned int* full_graph, ap_uint<512>* full_graph_cons, float* m_scores, unsigned int m_num_nodes,
                          unsigned int* m_graph, ap_uint<512>* graph_cons, unsigned int* m_lookup, unsigned int* m_lookup_filter, unsigned int& m_graph_size) {

  ap_uint<8> connections = 0;
  unsigned int new_from, new_to;
  m_graph_size = 1; // has to start at 1, cause 0 indicates that no index has been given yet
  unsigned int row = 0;
  unsigned int max_iter = 64;
  
  ap_uint<512> multi_full_con = 0;
  ap_uint<8> single_full_con = 0;
  unsigned int pos_multi = 0;
  unsigned int pos_single = 0;

  unsigned int con_iterations = m_num_nodes / 64; // number of 512-bit-values that need to be read
  unsigned int con_residue = m_num_nodes % 64; // number of 8-bit-values that are missing after the last complete 512-bit-value

  for (unsigned int c = 0; c < con_iterations + 1 ; c++){
    max_iter = 64;
    if(c == con_iterations)
      max_iter = con_residue;
    multi_full_con = full_graph_cons[c];
    for (unsigned int k = 0; k < max_iter ; k++){ // iterate through 64 8-bit values inside the 512-bit variable
      single_full_con = multi_full_con.range((k + 1) * 8 - 1, k * 8);
      if(single_full_con > 0){
        connections = 0;
        new_from = 0;
        row = c * 64 + k;
        for (unsigned int i = 0; i < single_full_con; i++)
          if(m_scores[row * MAX_FULL_GRAPH_EDGES + i] > m_cutoff){
            // put row in lookup and get out new index
            if(new_from == 0){
              if(m_lookup_filter[row] == 0){
                m_lookup_filter[row] = m_graph_size;
                m_lookup[m_graph_size] = row;
                new_from = m_graph_size;
                m_graph_size++;
              }
              else
                new_from = m_lookup_filter[row];
            }
            // put i in lookup and get out new index
            if(m_lookup_filter[full_graph[row * MAX_FULL_GRAPH_EDGES + i]] == 0){
              m_lookup_filter[full_graph[row * MAX_FULL_GRAPH_EDGES + i]] = m_graph_size;
              m_lookup[m_graph_size] = full_graph[row * MAX_FULL_GRAPH_EDGES + i];
              new_to = m_graph_size;
              m_graph_size++;
            }
            else
              new_to = m_lookup_filter[full_graph[row * MAX_FULL_GRAPH_EDGES + i]];

            // Add new indices to the filtered graph
            m_graph[new_from * MAX_EDGES + connections] = new_to;
            connections++;
          }
        if(new_from != 0){
          pos_multi = new_from / 64;
          pos_single = new_from % 64;
          graph_cons[pos_multi].range((pos_single + 1) * 8 - 1, pos_single * 8) = connections;
        }
      }
    }
  }
}

static void compute_core(unsigned int* m_graph, ap_uint<512>* graph_cons, unsigned int m_num_nodes, hls::stream<unsigned int>& outStream, unsigned int* m_lookup){

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
  unsigned int max_iter = 64;
  ap_uint<512> multi_graph_con = 0;
  ap_uint<8> single_graph_con = 0;
  unsigned int pos_multi = 0;
  unsigned int pos_single = 0;
  unsigned int con_iterations = m_num_nodes / 64; // number of 512-bit-values that need to be read
  unsigned int con_residue = m_num_nodes % 64; // number of 8-bit-values that are missing after the last complete 512-bit-value

  for (unsigned int c = 0; c < con_iterations + 1 ; c++){
    max_iter = 64;
    if(c == con_iterations)
      max_iter = con_residue;
    multi_graph_con = graph_cons[c];
    for (unsigned int k = 0; k < max_iter ; k++){
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
          outStream << m_lookup[component[i]];
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

  void CCL( unsigned int* in_full_graph, ap_uint<512>* in_full_graph_cons, float* in_scores, unsigned int* io_graph, ap_uint<512>* io_graph_cons, unsigned int* io_lookup, unsigned int* io_lookup_filter, unsigned int* out_components, unsigned int num_nodes, float cutoff) {
    
    #pragma HLS INTERFACE m_axi port = in_full_graph      bundle=gmem0
    #pragma HLS INTERFACE m_axi port = in_full_graph_cons bundle=gmem1
    #pragma HLS INTERFACE m_axi port = in_scores          bundle=gmem2
    #pragma HLS INTERFACE m_axi port = io_graph           bundle=gmem3
    #pragma HLS INTERFACE m_axi port = io_graph_cons      bundle=gmem4
    #pragma HLS INTERFACE m_axi port = io_lookup          bundle=gmem5
    #pragma HLS INTERFACE m_axi port = io_lookup_filter   bundle=gmem6
    #pragma HLS INTERFACE m_axi port = out_components     bundle=gmem7

    static hls::stream<unsigned int> outStream_components("output_stream_components");
    static unsigned int graph_size;
    #pragma HLS STREAM variable=graph_size type=pipo

    #pragma HLS dataflow
    filter_memory(cutoff, in_full_graph, in_full_graph_cons, in_scores, num_nodes, io_graph, io_graph_cons, io_lookup, io_lookup_filter, graph_size);
    compute_core(io_graph, io_graph_cons, graph_size, outStream_components, io_lookup);
    write_components(out_components, outStream_components, num_nodes);

  }
}




