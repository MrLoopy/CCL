// HLS-related includes
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
#include <iostream>
#include "kernels.hpp"

static void read_input_int(unsigned int* in, hls::stream<unsigned int>& inStream, int size) {
    for (int i = 0; i < size; i++) {
      inStream << in[i];
    }
}
static void read_input_float(float* in, hls::stream<float>& inStream, int size) {
    for (int i = 0; i < size; i++) {
      inStream << in[i];
    }
}
static void write_labels(unsigned int* out, int m_labels[MAX_COMPONENT_NODES], int size, int m_lookup[MAX_NODES]) {
  for (int i = 0; i < size; i++) {
    if(m_lookup[i] < 0)
      out[i] = 0;
    else
      out[i] = m_labels[m_lookup[i]];
  }
}

static void filter_memory(hls::stream<unsigned int>& edge_from_stream, hls::stream<unsigned int>& edge_to_stream, hls::stream<float>& scores_stream, int m_num_edges, int m_graph[MAX_COMPONENT_NODES][MAX_CONNECTIONS], node_information m_info[MAX_COMPONENT_NODES], int m_lookup[MAX_NODES], int& m_graph_size) {
  const float cutoff = 0.5;
  int from_large, to_large, from_small, to_small;
  float score;
  for (int i = 0; i < m_num_edges; i++) {
    // read nodes and score of current edge
    from_large = edge_from_stream.read();
    to_large = edge_to_stream.read();
    score = scores_stream.read();
    // check if score > cutoff -> true edge
    if(score > cutoff){
      // fill lookup and determain from_small and to_small
      if(m_lookup[from_large] < 0){
        from_small = m_graph_size;
        m_lookup[from_large] = m_graph_size;
        m_graph_size++;
      }
      else{
        from_small = m_lookup[from_large];
      }
      if(m_lookup[to_large] < 0){
        to_small = m_graph_size;
        m_lookup[to_large] = m_graph_size;
        m_graph_size++;
      }
      else{
        to_small = m_lookup[to_large];
      }

      // add to-node to from-list and increase from-counter
      m_graph[from_small][m_info[from_small].connections] = to_small;
      m_info[from_small].connections++;
      // add from-node to to-list and increase to-counter
      m_graph[to_small][m_info[to_small].connections] = from_small;
      m_info[to_small].connections++;
    }
  }
}

static void compute_core(int m_graph[MAX_COMPONENT_NODES][MAX_CONNECTIONS], node_information m_info[MAX_COMPONENT_NODES], int m_num_nodes, int m_labels[MAX_COMPONENT_NODES]){

  int current_label = 1;
  int component[MAX_COMPONENT_SIZE];
  for (int i = 0; i < MAX_COMPONENT_SIZE; i++){
    component[i] = -1;
  }
  int current_component_size = 0;
  int processed_nodes = 0;
  int next_node = -1;

  for (int node = 0; node < m_num_nodes; node++){
    // node without any connections -> label 0 and processed // should not happen anymore since lookup is used
    if(m_info[node].connections == 0){
      std::cout << std::endl << "[KERNEL] node processed with no connections" << std::endl << std::endl;
      m_labels[node] = 0;
      m_info[node].processed = true;
    }
    // node with connections that has not been processed yet -> new component
    else if (!m_info[node].processed){
      // new component needs to reset parameters
      for (int i = 0; i < MAX_COMPONENT_SIZE; i++){
        component[i] = -1;
      }
      current_component_size = 0;
      processed_nodes = 0;
      // first node gets added to component
      component[current_component_size] = node;
      current_component_size++;
      
      // main loop to iterate through the component
      while (current_component_size != processed_nodes){
        // get next node of the component that has not yet been processed
        next_node = component[processed_nodes];
        // add all connections of current node to component if not already processed <- to many fills for highly connected components?
        for (int i = 0; i < m_info[next_node].connections; i++){
          if(!m_info[m_graph[next_node][i]].processed && current_component_size < MAX_COMPONENT_SIZE){
            component[current_component_size] = m_graph[next_node][i];
            current_component_size++;
          }
        }
        // after all connections of node have been added, the node is done and can be marked as processed
        m_info[next_node].processed = true;
        processed_nodes++;
      }

      // after component is fully set up, we can export the labels from it
      for (int i = 0; i < current_component_size; i++){
        m_labels[component[i]] = current_label;
      }
      current_label++;
    }
  }
}

extern "C" {
  void CCL(unsigned int* in_edge_from, unsigned int* in_edge_to, float* in_scores, unsigned int* out_labels, int num_edges, int num_nodes) {
    static hls::stream<unsigned int> inStream_edge_from("input_stream1");
    static hls::stream<unsigned int> inStream_edge_to("input_stream2");
    static hls::stream<float> inStream_score("input_stream3");
    
    #pragma HLS INTERFACE m_axi port = in_edge_from   bundle=gmem0
    #pragma HLS INTERFACE m_axi port = in_edge_from   bundle=gmem1
    #pragma HLS INTERFACE m_axi port = in_scores      bundle=gmem2
    #pragma HLS INTERFACE m_axi port = out_labels   bundle=gmem0

    static int graph_connections[MAX_COMPONENT_NODES][MAX_CONNECTIONS];
    static node_information graph_info[MAX_COMPONENT_NODES];
    static int labels[MAX_COMPONENT_NODES];
    static int lookup[MAX_NODES];
    #pragma HLS bind_storage variable=graph_connections type=RAM_T2P impl=bram
    #pragma HLS bind_storage variable=graph_info type=RAM_T2P impl=bram
    #pragma HLS bind_storage variable=labels type=RAM_T2P impl=bram
    #pragma HLS bind_storage variable=lookup type=RAM_T2P impl=bram

    for (int i = 0; i < MAX_NODES; i++)
      lookup[i] = -1;
    static int graph_size = 0;

    // #pragma HLS dataflow
    // dataflow pragma instruct compiler to run all functions in parallel -> problem because graph needs to be finished, before computation can start
    read_input_int(in_edge_from, inStream_edge_from, num_edges);
    read_input_int(in_edge_to, inStream_edge_to, num_edges);
    read_input_float(in_scores, inStream_score, num_edges);

    filter_memory(inStream_edge_from, inStream_edge_to, inStream_score, num_edges, graph_connections, graph_info, lookup, graph_size);

    compute_core(graph_connections, graph_info, graph_size, labels);

    write_labels(out_labels, labels, num_nodes, lookup);
  }
}




