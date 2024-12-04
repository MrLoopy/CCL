// HLS-related includes
#include "ap_int.h"
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
// #include <iostream>
#include "kernels.hpp"

static void in_ram_wrapper(uint16_t* in_graph, uint16_t* ram_graph, uint16_t* in_cons, uint16_t* ram_cons, bool* in_scores, bool* ram_scores){
  for (unsigned int i = 0; i < MAX_TOTAL_NODES; i++)
    ram_cons[i] = in_cons[i];
  for (unsigned int i = 0; i < MAX_TOTAL_NODES * MAX_FULL_GRAPH_EDGES; i++){
    ram_graph[i] = in_graph[i];
    ram_scores[i] = in_scores[i];
  }
}
static void out_ram_wrapper(uint16_t* out_components, uint16_t* ram_components){
  for (unsigned int i = 0; i < MAX_TRUE_NODES + MAX_COMPONENTS; i++)
    out_components[i] = ram_components[i];
}

static void compute_direct(float m_cutoff, uint16_t* full_graph, uint16_t* full_graph_cons, bool* scores, hls::stream<uint16_t>& outStream, uint16_t num_nodes){

  uint16_t component[MAX_COMPONENT_SIZE];
  uint16_t current_component_size = 0;
  uint16_t processed_nodes = 0;
  uint16_t next_node = 0;
  uint16_t potential_node = 0;
  bool found_component = true;
  bool new_row = true;
  bool processed[MAX_TOTAL_NODES];
  direct_reset_processed:
  for(unsigned int i = 0; i < MAX_TOTAL_NODES; i++)
    processed[i] = false;

  direct_rows:
  for(unsigned int row = 0; row < num_nodes; row++){
    if(full_graph_cons[row] > 0 && !processed[row]){
      if(found_component){
        found_component = false;
        new_row = true;
        current_component_size = 0;
        processed_nodes = 0;
      }
      direct_nodes:
      for (unsigned int i = 0; i < full_graph_cons[row]; i++)
        if(scores[row * MAX_FULL_GRAPH_EDGES + i] > m_cutoff){
          found_component = true;
          if(new_row){
            new_row = false;
            component[current_component_size] = row;
            current_component_size++;
          }
          component[current_component_size] = full_graph[row * MAX_FULL_GRAPH_EDGES + i];
          current_component_size++;
        }
      if(found_component){
        // row already processed
        processed[row] = true;
        processed_nodes++;

        direct_while:
        while(current_component_size != processed_nodes){
          next_node = component[processed_nodes];
          direct_connections:
          for(unsigned int i = 0 ; i < full_graph_cons[next_node]; i++){
            potential_node = full_graph[next_node * MAX_FULL_GRAPH_EDGES + i];
            if(scores[next_node * MAX_FULL_GRAPH_EDGES + i] > m_cutoff 
            && !processed[potential_node] 
            && current_component_size < MAX_COMPONENT_SIZE){
              bool new_node = true;
              direct_check_component:
              for(unsigned int j = 0 ; j < current_component_size; j++){
                if(component[j] == potential_node)
                  new_node = false;
              }
              if(new_node){
                component[current_component_size] = potential_node;
                current_component_size++;
              }
            }
          }
          processed[next_node] = true;
          processed_nodes++;
        }
        // write output
        outStream << current_component_size;
        direct_write_output:
        for (unsigned int i = 0; i < current_component_size; i++)
          outStream << component[i];
      }
    }
  }
  outStream << 0;
}

static void write_components(uint16_t* out, hls::stream<uint16_t>& outStream) {

  bool stream_running = true;
  uint16_t stream_size = 1; // position 0 in the Output will be the size of the total output. Therefore size needs to start at 1
  uint16_t component_size = 0;

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
            uint16_t* in_full_graph, uint16_t* in_full_graph_cons, bool* in_scores,
            uint16_t* out_components, uint16_t num_nodes, float cutoff) {
    
    #pragma HLS dataflow

    #pragma HLS INTERFACE m_axi port = in_full_graph      bundle=gmem0
    #pragma HLS INTERFACE m_axi port = in_full_graph_cons bundle=gmem1
    #pragma HLS INTERFACE m_axi port = in_scores          bundle=gmem2
    #pragma HLS INTERFACE m_axi port = out_components     bundle=gmem3
    
    static uint16_t full_graph[MAX_TOTAL_NODES * MAX_FULL_GRAPH_EDGES]; // 8192 * 256 = 2M -> 4MB
    #pragma HLS bind_storage variable=full_graph type=RAM_T2P impl=URAM
    #pragma HLS STREAM variable=full_graph type=pipo
    static uint16_t full_graph_cons[MAX_TOTAL_NODES]; // 8192 -> 16kB
    #pragma HLS bind_storage variable=full_graph_cons type=RAM_T2P impl=BRAM
    #pragma HLS STREAM variable=full_graph_cons type=pipo
    static bool scores[MAX_TOTAL_NODES * MAX_FULL_GRAPH_EDGES]; // 8192 * 256 = 2M -> 256kB
    #pragma HLS bind_storage variable=scores type=RAM_T2P impl=URAM
    #pragma HLS STREAM variable=scores type=pipo
    static uint16_t components[MAX_TRUE_NODES + MAX_COMPONENTS]; // 576
    #pragma HLS bind_storage variable=components type=RAM_2P impl=LUTRAM
    #pragma HLS STREAM variable=components type=pipo
    
    static hls::stream<uint16_t> outStream_components("output_stream_components");

    in_ram_wrapper(in_full_graph, full_graph, in_full_graph_cons, full_graph_cons, in_scores, scores);

    compute_direct(cutoff, full_graph, full_graph_cons, scores, outStream_components, num_nodes);
    write_components(components, outStream_components);

    out_ram_wrapper(out_components, components);

  }
}




