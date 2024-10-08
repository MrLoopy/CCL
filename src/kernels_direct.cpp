// HLS-related includes
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
// #include <iostream>
#include "kernels.hpp"

static void compute_direct(unsigned int* full_graph, float* scores, unsigned int* processed, unsigned int* out_components, unsigned int num_nodes){
  // use lookup_filter as processed, has same length = MAX_TOTAL_NODES

  unsigned int component[MAX_COMPONENT_SIZE];
  unsigned int current_component_size = 0;
  unsigned int processed_nodes = 0;
  unsigned int next_node = 0;
  unsigned int stream_size = 1;
  float cutoff = 0.5;
  bool found_component = true;
  bool new_row = true;

  direct_rows: for(unsigned int row = 0; row < MAX_TOTAL_NODES; row++)
  if(row < num_nodes){
    if(full_graph[row * MAX_FULL_GRAPH_EDGES] > 0 && processed[row] == 0){
      if(found_component){
        found_component = false;
        new_row = true;
        current_component_size = 0;
        processed_nodes = 0;
        direct_reset: for(unsigned int i = 0; i < MAX_COMPONENT_SIZE; i++){
          #pragma HLS unroll
          component[i] = 4294967295;
        }
      }
      direct_nodes: for (unsigned int i = 0; i < MAX_FULL_GRAPH_EDGES - 1; i++)
      if(i < full_graph[row * MAX_FULL_GRAPH_EDGES])
        if(scores[row * MAX_FULL_GRAPH_EDGES + i] > cutoff){
          found_component = true;
          bool new_node = true;
          direct_check_row: for(unsigned int j = 0 ; j < MAX_COMPONENT_SIZE; j++){
            #pragma HLS unroll
            if(j < current_component_size){
              if(component[j] == row)
                new_row = false;
              if(component[j] == full_graph[row * MAX_FULL_GRAPH_EDGES + 1 + i])
                new_node = false;
            }
          }
          if(new_row){
            component[current_component_size] = row;
            current_component_size++;
          }
          if(new_node){
            component[current_component_size] = full_graph[row * MAX_FULL_GRAPH_EDGES + 1 + i];
            current_component_size++;
          }
        }
      if(found_component){
        // row already processed
        processed[row] = 1;
        processed_nodes++;

        direct_while: while(current_component_size != processed_nodes){
          next_node = component[processed_nodes];
          direct_connections: for(unsigned int i = 0 ; i < MAX_FULL_GRAPH_EDGES - 1; i++)
          if(i < full_graph[next_node * MAX_FULL_GRAPH_EDGES]){
            if(scores[next_node * MAX_FULL_GRAPH_EDGES + i] > cutoff 
            && processed[full_graph[next_node * MAX_FULL_GRAPH_EDGES + 1 + i]] == 0 
            && current_component_size < MAX_COMPONENT_SIZE){
              bool new_node = true;
              direct_check_component: for(unsigned int j = 0 ; j < MAX_COMPONENT_SIZE; j++){
                #pragma HLS unroll
                if(j < current_component_size)
                  if(component[j] == full_graph[next_node * MAX_FULL_GRAPH_EDGES + 1 + i])
                    new_node = false;
              }
              if(new_node){
                component[current_component_size] = full_graph[next_node * MAX_FULL_GRAPH_EDGES + 1 + i];
                current_component_size++;
              }
            }
          }
          processed[next_node] = 1;
          processed_nodes++;
        }
        // write output
        out_components[stream_size] = current_component_size;
        stream_size++;
        direct_write_output: for (unsigned int i = 0; i < MAX_COMPONENT_SIZE; i++)
        if(i < current_component_size){
          out_components[stream_size] = component[i];
          stream_size++;
        }
      }
    }
  }
  out_components[0] = stream_size;
}

extern "C" {
  void CCL(unsigned int* in_full_graph, float* in_scores, unsigned int* io_graph, unsigned int* io_lookup, unsigned int* io_lookup_filter, unsigned int* out_components, unsigned int num_edges, unsigned int num_nodes) {
    
    #pragma HLS INTERFACE m_axi port = in_full_graph    bundle=gmem0 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = in_scores        bundle=gmem1 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = io_graph         bundle=gmem2 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = io_lookup        bundle=gmem3 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = io_lookup_filter bundle=gmem4 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = out_components   bundle=gmem5 max_widen_bitwidth=512

    #pragma HLS dataflow
    compute_direct(in_full_graph, in_scores, io_lookup_filter, out_components, num_nodes);
  }
}




