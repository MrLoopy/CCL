// HLS-related includes
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
// #include <iostream>
#include "test_kernels.hpp"

static void filter_scores(float* m_scores, unsigned int m_num_nodes, float* m_graph, unsigned int* m_lookup, unsigned int& m_graph_size){

  const float cutoff = 0.5;
  m_graph_size = 0;
  bool true_row = false;
  
  filter_rows:
  for (unsigned int row = 0; row < m_num_nodes; row++){
    true_row = false;
    for (unsigned int i = 0; i < MAX_FULL_GRAPH_EDGES; i++)
      if(m_scores[row * MAX_FULL_GRAPH_EDGES + i] > cutoff)
        true_row = true;
    if(true_row){
      for (unsigned int i = 0; i < MAX_FULL_GRAPH_EDGES; i++)
        m_graph[m_graph_size * MAX_FULL_GRAPH_EDGES + i] = m_scores[row * MAX_FULL_GRAPH_EDGES + i];
      m_lookup[m_graph_size] = row;
      m_graph_size++;
    }
  }
  m_lookup[m_graph_size] = 0;
}

static void compute_connections(float* m_graph, unsigned int m_num_nodes, hls::stream<unsigned int>& outStream){

  const float cutoff = 0.5;
  unsigned int current_component_size = 0;

  for (unsigned int row = 0; row < m_num_nodes; row++){
    current_component_size = 0;
    for (unsigned int i = 0; i < MAX_FULL_GRAPH_EDGES; i++)
      if(m_graph[row * MAX_FULL_GRAPH_EDGES + i] > cutoff)
        current_component_size++;
    outStream << current_component_size;
  }
  // outStream << 0;
}

static void write_number_connections(unsigned int* m_lookup, hls::stream<unsigned int>& outStream, unsigned int m_num_nodes, unsigned int* out) {

  unsigned int next_node = m_lookup[0];
  unsigned int lookup_node = 1;
  unsigned int connections = 0;

  for (unsigned int node = 0; node < m_num_nodes; node++){
    if(next_node == node){
      connections = outStream.read();
      out[node] = connections;
      next_node = m_lookup[lookup_node];
      lookup_node++;
    }
    else
      out[node] = 0;
  }
}

extern "C" {
  void PIPE_TEST(float* in_scores, float* io_graph, unsigned int* io_lookup, unsigned int* out_components, unsigned int num_nodes) {
    static hls::stream<unsigned int> outStream_components("output_stream_components");
    
    #pragma HLS INTERFACE m_axi port = in_scores        bundle=gmem1 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = io_graph         bundle=gmem2 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = io_lookup        bundle=gmem3 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = out_components   bundle=gmem5 max_widen_bitwidth=512

    static unsigned int graph_size;
    #pragma HLS STREAM variable=graph_size type=pipo

    #pragma HLS dataflow
    filter_scores(in_scores, num_nodes, io_graph, io_lookup, graph_size);

    compute_connections(io_graph, graph_size, outStream_components);

    write_number_connections(io_lookup, outStream_components, num_nodes, out_components);
  }
}




