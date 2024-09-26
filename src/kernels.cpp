// HLS-related includes
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
#include <iostream>
#include "kernels.hpp"

static void read_input_float(float* in, hls::stream<float>& inStream, unsigned int size) {
    for (unsigned int i = 0; i < size * 2; i++) {
      inStream << in[i];
    }
}

static void queue_components(unsigned int m_labels[MAX_TOTAL_NODES], hls::stream<unsigned int>& outStream, unsigned int size, unsigned int* m_lookup) {
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

static void filter_memory(unsigned int* full_graph, hls::stream<float>& scores_stream, unsigned int m_num_nodes,
                          unsigned int* m_graph, unsigned int* m_lookup, unsigned int* m_lookup_filter, unsigned int& m_graph_size) {

  unsigned int temp_connections[MAX_TRUE_NODES];
  for (unsigned int i = 0; i < MAX_TRUE_NODES; i++)
    temp_connections[i] = 0;

  const float cutoff = 0.5;
  unsigned int new_from, new_to;
  float score;
  m_graph_size = 1;
  // for each node in the full graph / for each row in the graph-table
  for (unsigned int row = 0; row < m_num_nodes; row++){
    // for each connection of that node
    for (unsigned int i = 0; i < full_graph[row * MAX_FULL_GRAPH_EDGES]; i++){
      score = scores_stream.read();
      if(score > cutoff){
        // put row in lookup and get out new index
        if(m_lookup_filter[row] == 0){
          m_lookup_filter[row] = m_graph_size;
          m_lookup[row] = m_graph_size;
          new_from = m_graph_size;
          m_graph_size++;
        }
        else
          new_from = m_lookup_filter[row];
        // put i in lookup and get out new index
        if(m_lookup_filter[full_graph[row * MAX_FULL_GRAPH_EDGES + 1 + i]] == 0){
          m_lookup_filter[full_graph[row * MAX_FULL_GRAPH_EDGES + 1 + i]] = m_graph_size;
          m_lookup[full_graph[row * MAX_FULL_GRAPH_EDGES + 1 + i]] = m_graph_size;
          new_to = m_graph_size;
          m_graph_size++;
        }
        else
          new_to = m_lookup_filter[full_graph[row * MAX_FULL_GRAPH_EDGES + 1 + i]];

        // Add new indices to the filtered graph
        m_graph[new_from * MAX_EDGES + 1 + temp_connections[new_from]] = new_to;
        temp_connections[new_from]++;
        // m_graph[new_to * MAX_EDGES + 1 + temp_connections[new_to]] = new_from;
        // temp_connections[new_to]++;
      }
    }
  }
  for (unsigned int i = 0; i < m_graph_size; i++)
    m_graph[i * MAX_EDGES] = temp_connections[i];

}

static void compute_core(unsigned int* m_graph, unsigned int m_num_nodes, unsigned int m_labels[MAX_TRUE_NODES]){

  unsigned int component[MAX_COMPONENT_SIZE];
  unsigned int processed[MAX_TRUE_NODES];
  for (unsigned int i = 0; i < MAX_TRUE_NODES; i++)
    processed[i] = false;
  unsigned int current_component_size = 0;
  unsigned int processed_nodes = 0;
  unsigned int next_node = 0;
  unsigned int current_label = 1;

  for (unsigned int row = 0; row < m_num_nodes; row++){

    if(m_graph[row * MAX_EDGES] == 0){
      m_labels[row] = 0;
      processed[row] = true;
    }
    // node with connections that has not been processed yet -> new component
    else if (!processed[row]){
      // new component needs to reset parameters
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

        // Check if this node has already been processed by other core that works on the same track
          // Conflict handler -> transfer current component from one core to the other

        // add all connections of current node to component if not already processed <- to many fills for highly connected components?
        for(unsigned int i = 0 ; i < m_graph[next_node * MAX_EDGES] ; i++){
          if(!processed[m_graph[next_node * MAX_EDGES + 1 + i]] && current_component_size < MAX_COMPONENT_SIZE){
            component[current_component_size] = m_graph[next_node * MAX_EDGES + 1 + i];
            current_component_size++;
          }
        }
        // after all connections of node have been added, the node is done and can be marked as processed
        processed[next_node] = true;
        processed_nodes++;
      }

      // after component is fully set up, we can export the labels from it      <<<<<   Output each component directly (with PIPO of MAX_COMPONENT_SIZE)
      for (unsigned int i = 0; i < current_component_size; i++)
        m_labels[component[i]] = current_label;
      current_label++;
    }
  }
}

extern "C" {
  void CCL(unsigned int* in_full_graph, float* in_scores, unsigned int* io_graph, unsigned int* io_lookup, unsigned int* io_lookup_filter, unsigned int* out_labels, unsigned int num_edges, unsigned int num_nodes) {
    static hls::stream<unsigned int> inStream_edge_from("input_stream_from");
    static hls::stream<unsigned int> inStream_edge_to("input_stream_to");
    static hls::stream<float> inStream_score("input_stream_score");
    static hls::stream<unsigned int> outStream_labels("output_stream_labels");
    
    #pragma HLS INTERFACE m_axi port = in_full_graph    bundle=gmem0 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = in_scores        bundle=gmem1 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = io_graph         bundle=gmem2 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = io_lookup        bundle=gmem3 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = io_lookup_filter bundle=gmem4 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = out_labels       bundle=gmem5 max_widen_bitwidth=512

    static unsigned int labels[MAX_TRUE_NODES];
    #pragma HLS STREAM variable=labels type=pipo

    static unsigned int graph_size;
    #pragma HLS STREAM variable=graph_size type=pipo

    #pragma HLS dataflow
    read_input_float(in_scores, inStream_score, num_edges);

    filter_memory(in_full_graph, inStream_score, num_nodes, io_graph, io_lookup, io_lookup_filter, graph_size);

    compute_core(io_graph, graph_size, labels);

    queue_components(labels, outStream_labels, num_nodes, io_lookup);

    write_labels(out_labels, outStream_labels, num_nodes);
  }
}




