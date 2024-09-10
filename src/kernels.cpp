// HLS-related includes
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
// #include <iostream>
#include "kernels.hpp"

static void read_input_int(unsigned int* in, hls::stream<unsigned int>& inStream, unsigned int size) {
    for (unsigned int i = 0; i < size; i++) {
      inStream << in[i];
    }
}
static void read_input_float(float* in, hls::stream<float>& inStream, unsigned int size) {
    for (unsigned int i = 0; i < size; i++) {
      inStream << in[i];
    }
}

static void queue_components(unsigned int m_labels[MAX_TOTAL_NODES], hls::stream<unsigned int>& outStream, unsigned int size, unsigned int m_lookup[MAX_TOTAL_NODES]) {
  for (unsigned int i = 0; i < size; i++) {
    if(m_lookup[i] == 0)
      outStream << 0;
    else
      outStream << m_labels[m_lookup[i]];
  }
}

static void write_labels(unsigned int* out, hls::stream<unsigned int>& outStream, unsigned int size) {
  for (unsigned int i = 0; i < size; i++) {
    out[i] = outStream.read();
  }
}

static void filter_memory(hls::stream<unsigned int>& edge_from_stream, hls::stream<unsigned int>& edge_to_stream, hls::stream<float>& scores_stream,
                          unsigned int m_num_edges, unsigned int m_graph[MAX_TRUE_NODES][MAX_EDGES], unsigned int m_connections[MAX_TRUE_NODES],
                          unsigned int m_lookup[MAX_TOTAL_NODES], unsigned int& m_graph_size) {

  unsigned int temp_lookup[MAX_TOTAL_NODES];
  for (unsigned int i = 0; i < MAX_TOTAL_NODES; i++)
    temp_lookup[i] = 0;
  unsigned int temp_connections[MAX_TRUE_NODES];
  for (unsigned int i = 0; i < MAX_TRUE_NODES; i++)
    temp_connections[i] = false;
  m_graph_size = 1;

  const float cutoff = 0.5;
  unsigned int from_large, to_large, from_small, to_small;
  float score;
  for (unsigned int i = 0; i < m_num_edges; i++) {
    // read nodes and score of current edge
    from_large = edge_from_stream.read();
    to_large = edge_to_stream.read();
    score = scores_stream.read();
    // check if score > cutoff -> true edge
    if(score > cutoff){
      // fill lookup and determain from_small and to_small
      if(temp_lookup[from_large] == 0){
        from_small = m_graph_size;
        temp_lookup[from_large] = m_graph_size;
        m_graph_size++;
      }
      else{
        from_small = temp_lookup[from_large];
      }
      if(temp_lookup[to_large] == 0){
        to_small = m_graph_size;
        temp_lookup[to_large] = m_graph_size;
        m_graph_size++;
      }
      else{
        to_small = temp_lookup[to_large];
      }

      // add to-node to from-list and increase from-counter
      m_graph[from_small][temp_connections[from_small]] = to_small;
      temp_connections[from_small]++;
      // add from-node to to-list and increase to-counter
      m_graph[to_small][temp_connections[to_small]] = from_small;
      temp_connections[to_small]++;
    }
  }
  for (unsigned int i = 0; i < MAX_TOTAL_NODES; i++)
    m_lookup[i] = temp_lookup[i];
  for (unsigned int i = 0; i < MAX_TRUE_NODES; i++)
    m_connections[i] = temp_connections[i];
}

static void compute_core(unsigned int m_graph[MAX_TRUE_NODES][MAX_EDGES], unsigned int m_connections[MAX_TRUE_NODES], unsigned int m_num_nodes,
                          unsigned int m_labels[MAX_TRUE_NODES]){

  unsigned int current_label = 1;
  unsigned int component[MAX_COMPONENT_SIZE];
  for (int i = 0; i < MAX_COMPONENT_SIZE; i++)
    component[i] = 0;
  unsigned int processed[MAX_TRUE_NODES];
  for (unsigned int i = 0; i < MAX_TRUE_NODES; i++)
    processed[i] = false;
  unsigned int current_component_size = 0;
  unsigned int processed_nodes = 0;
  unsigned int next_node = 0;

  for (unsigned int node = 0; node < m_num_nodes; node++){
    // node without any connections -> label 0 and processed // should not happen anymore since lookup is used
    if(m_connections[node] == 0){
      // std::cout << std::endl << "[KERNEL] node processed with no connections" << std::endl << std::endl;
      m_labels[node] = 0;
      processed[node] = true;
    }
    // node with connections that has not been processed yet -> new component
    else if (!processed[node]){
      // new component needs to reset parameters
      for (unsigned int i = 0; i < MAX_COMPONENT_SIZE; i++){
        component[i] = 0;
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
        // Check if this node has already been processed by other core that works on the same track
        // if(m_info[next_node].processed)
          // Conflict handler -> transfer current component from one core to the other
        // add all connections of current node to component if not already processed <- to many fills for highly connected components?
        for (unsigned int i = 0; i < m_connections[next_node]; i++){
          if(!processed[m_graph[next_node][i]] && current_component_size < MAX_COMPONENT_SIZE){
            component[current_component_size] = m_graph[next_node][i];
            current_component_size++;
          }
        }
        // after all connections of node have been added, the node is done and can be marked as processed
        processed[next_node] = true;
        processed_nodes++;
      }

      // after component is fully set up, we can export the labels from it
      for (unsigned int i = 0; i < current_component_size; i++){
        m_labels[component[i]] = current_label;
      }
      current_label++;
    }
  }

}

extern "C" {
  void CCL(unsigned int* in_edge_from, unsigned int* in_edge_to, float* in_scores, unsigned int* out_labels, unsigned int num_edges, unsigned int num_nodes) {
    static hls::stream<unsigned int> inStream_edge_from("input_stream_from");
    static hls::stream<unsigned int> inStream_edge_to("input_stream_to");
    static hls::stream<float> inStream_score("input_stream_score");
    static hls::stream<unsigned int> outStream_labels("output_stream_labels");
    
    #pragma HLS INTERFACE m_axi port = in_edge_from   bundle=gmem0
    #pragma HLS INTERFACE m_axi port = in_edge_from   bundle=gmem1
    #pragma HLS INTERFACE m_axi port = in_scores      bundle=gmem2
    #pragma HLS INTERFACE m_axi port = out_labels     bundle=gmem0

    static unsigned int graph[MAX_TRUE_NODES][MAX_EDGES];
    static unsigned int graph_connections[MAX_TRUE_NODES];
    static unsigned int labels[MAX_TRUE_NODES];
    static unsigned int lookup[MAX_TOTAL_NODES];
    #pragma HLS STREAM variable=graph type=pipo
    #pragma HLS STREAM variable=graph_connections type=pipo
    #pragma HLS STREAM variable=labels type=pipo
    #pragma HLS STREAM variable=lookup type=pipo
    // #pragma HLS bind_storage variable=graph type=RAM_T2P impl=bram
    // #pragma HLS bind_storage variable=graph_connections type=RAM_T2P impl=bram
    // #pragma HLS bind_storage variable=labels type=RAM_T2P impl=bram
    // #pragma HLS bind_storage variable=lookup type=RAM_T2P impl=bram

    static unsigned int graph_size;
    #pragma HLS STREAM variable=graph_size type=pipo

    #pragma HLS dataflow
    read_input_int(in_edge_from, inStream_edge_from, num_edges);
    read_input_int(in_edge_to, inStream_edge_to, num_edges);
    read_input_float(in_scores, inStream_score, num_edges);

    filter_memory(inStream_edge_from, inStream_edge_to, inStream_score, num_edges, graph, graph_connections, lookup, graph_size);

    compute_core(graph, graph_connections, graph_size, labels);

    queue_components(labels, outStream_labels, num_nodes, lookup);

    write_labels(out_labels, outStream_labels, num_nodes);
  }
}




