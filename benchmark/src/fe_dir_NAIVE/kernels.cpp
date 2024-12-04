// HLS-related includes
#include "ap_int.h"
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
// #include <iostream>
#include "ddr_kernels.hpp"

static void compute_direct( float m_cutoff, unsigned int* full_graph, unsigned int* full_graph_cons, float* m_scores,
                        hls::stream<unsigned int>& outStream, unsigned int num_nodes){

  unsigned int component[MAX_COMPONENT_SIZE];
  unsigned int current_component_size = 0;
  unsigned int processed_nodes = 0;
  unsigned int next_node = 0;
  bool found_component = true;
  bool new_row = true;
  bool processed[MAX_TOTAL_NODES];
  direct_reset_processed:
  for(unsigned int i = 0; i < MAX_TOTAL_NODES; i++)
    processed[i] = false;

  for (unsigned int row = 0; row < num_nodes ; row++){
    if(full_graph_cons[row] > 0 && !processed[row]){
      if(found_component){
        found_component = false;
        new_row = true;
        current_component_size = 0;
        processed_nodes = 0;
      }
      for (unsigned int i = 0; i < full_graph_cons[row] ; i++){
        if(m_scores[row * MAX_FULL_GRAPH_EDGES + i] > m_cutoff){
          found_component = true;
          if(new_row){
            new_row = false;
            component[current_component_size] = row;
            current_component_size++;
          }
          component[current_component_size] = full_graph[row * MAX_FULL_GRAPH_EDGES + i];
          current_component_size++;
        }
      }
      if(found_component){
        processed[row] = true;
        processed_nodes++;
        while(current_component_size != processed_nodes){
          next_node = component[processed_nodes];
          for (unsigned int i = 0; i < full_graph_cons[next_node] ; i++){
            if( m_scores[next_node * MAX_FULL_GRAPH_EDGES + i] > m_cutoff
                && !processed[full_graph[next_node * MAX_FULL_GRAPH_EDGES + i]]
                && current_component_size < MAX_COMPONENT_SIZE){
              bool new_node = true;
              direct_check_component:
              for(unsigned int j = 0 ; j < current_component_size; j++){
                if(component[j] == full_graph[next_node * MAX_FULL_GRAPH_EDGES + i])
                  new_node = false;
              }
              if(new_node){
                component[current_component_size] = full_graph[next_node * MAX_FULL_GRAPH_EDGES + i];
                current_component_size++;
              }
            }
          }
          processed[next_node] = true;
          processed_nodes++;
        }
        outStream << current_component_size;
        direct_write_output:
        for (unsigned int i = 0; i < current_component_size; i++)
          outStream << component[i];
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
            unsigned int* in_full_graph, unsigned int* in_full_graph_cons, float* in_scores,
            unsigned int* out_components, unsigned int num_nodes, float cutoff) {
    
    #pragma HLS dataflow

    #pragma HLS INTERFACE m_axi port = in_full_graph      bundle=gmem0
    #pragma HLS INTERFACE m_axi port = in_full_graph_cons bundle=gmem1
    #pragma HLS INTERFACE m_axi port = in_scores          bundle=gmem2
    #pragma HLS INTERFACE m_axi port = out_components     bundle=gmem3
    
    static hls::stream<unsigned int> outStream_components("output_stream_components");

    compute_direct(cutoff, in_full_graph, in_full_graph_cons, in_scores, outStream_components, num_nodes);

    write_components(out_components, outStream_components, num_nodes);

  }
}




