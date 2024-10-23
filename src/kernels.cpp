// HLS-related includes
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
// #include <iostream>
#include "kernels.hpp"

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

static void filter_memory(unsigned int* full_graph, float* m_scores, unsigned int m_num_nodes,
                          unsigned int* m_graph, unsigned int* m_lookup, unsigned int* m_lookup_filter, unsigned int& m_graph_size) {

  unsigned int connections = 0;
  const float cutoff = 0.5;
  unsigned int new_from, new_to;
  m_graph_size = 1; // has to start at 1, cause 0 indicates that no index has been given yet
  
  // for each node in the full graph / for each row in the graph-table
  filter_rows:
  for (unsigned int row = 0; row < m_num_nodes; row++){
    #pragma HLS loop_tripcount min=290000 avg=335000 max=MAX_TOTAL_NODES
    connections = 0;
    new_from = 0;
    // for each connection of that node
    if(full_graph[row * MAX_FULL_GRAPH_EDGES] > 0){ // avoid for-loop 0 times
      filter_nodes:
      for (unsigned int i = 0; i < full_graph[row * MAX_FULL_GRAPH_EDGES]; i++){
        #pragma HLS loop_tripcount min=1 avg=5 max=MAX_FULL_GRAPH_EDGES
        if(m_scores[row * MAX_FULL_GRAPH_EDGES + i] > cutoff){
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
          if(m_lookup_filter[full_graph[row * MAX_FULL_GRAPH_EDGES + 1 + i]] == 0){
            m_lookup_filter[full_graph[row * MAX_FULL_GRAPH_EDGES + 1 + i]] = m_graph_size;
            m_lookup[m_graph_size] = full_graph[row * MAX_FULL_GRAPH_EDGES + 1 + i];
            new_to = m_graph_size;
            m_graph_size++;
          }
          else
            new_to = m_lookup_filter[full_graph[row * MAX_FULL_GRAPH_EDGES + 1 + i]];

          // Add new indices to the filtered graph
          m_graph[new_from * MAX_EDGES + 1 + connections] = new_to;
          connections++;
        }
      }
      if(new_from != 0)
        m_graph[new_from * MAX_EDGES] = connections;
    }
  }
}

static void compute_core(unsigned int* m_graph, unsigned int m_num_nodes, hls::stream<unsigned int>& outStream, unsigned int* m_lookup){

  static unsigned int component[MAX_COMPONENT_SIZE];
  static unsigned int processed[MAX_TRUE_NODES];
  compute_reset_processed:
  for (unsigned int i = 0; i < MAX_TRUE_NODES; i++){
    processed[i] = false;
  }
  unsigned int current_component_size = 0;
  unsigned int processed_nodes = 0;
  unsigned int next_node = 0;

  compute_rows:
  for (unsigned int row = 0; row < m_num_nodes; row++){
    #pragma HLS loop_tripcount min=31000 avg=MAX_TRUE_NODES max=MAX_TRUE_NODES // avg=35846
    if(m_graph[row * MAX_EDGES] == 0)
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

        // Check if this node has already been processed by other core that works on the same track
          // Conflict handler -> transfer current component from one core to the other

        // add all connections of current node to component if not already processed <- to many fills for highly connected components?
        if(m_graph[next_node * MAX_EDGES] > 0) // avoid for-loop 0 times
          compute_connections:
          for(unsigned int i = 0 ; i < m_graph[next_node * MAX_EDGES]; i++){
            #pragma HLS loop_tripcount min=1 avg=2 max=MAX_EDGES
            if(!processed[m_graph[next_node * MAX_EDGES + 1 + i]] && current_component_size < MAX_COMPONENT_SIZE){
              bool new_node = true;
              compute_check_component:
              for(unsigned int j = 0 ; j < current_component_size; j++){
                #pragma HLS loop_tripcount min=1 avg=4 max=MAX_COMPONENT_SIZE
                  if(component[j] == m_graph[next_node * MAX_EDGES + 1 + i])
                    new_node = false;
              }
              if(new_node){
                component[current_component_size] = m_graph[next_node * MAX_EDGES + 1 + i];
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
  outStream << 0;
}

extern "C" {
  void CCL(unsigned int* in_full_graph, float* in_scores, unsigned int* io_graph, unsigned int* io_lookup, unsigned int* io_lookup_filter, unsigned int* out_components, unsigned int num_edges, unsigned int num_nodes) {
    static hls::stream<unsigned int> outStream_components("output_stream_components");
    
    #pragma HLS INTERFACE m_axi port = in_full_graph    bundle=gmem0 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = in_scores        bundle=gmem1 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = io_graph         bundle=gmem2 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = io_lookup        bundle=gmem3 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = io_lookup_filter bundle=gmem4 max_widen_bitwidth=512
    #pragma HLS INTERFACE m_axi port = out_components   bundle=gmem5 max_widen_bitwidth=512

    static unsigned int graph_size;
    #pragma HLS STREAM variable=graph_size type=pipo

    #pragma HLS dataflow
    filter_memory(in_full_graph, in_scores, num_nodes, io_graph, io_lookup, io_lookup_filter, graph_size);

    compute_core(io_graph, graph_size, outStream_components, io_lookup);

    write_components(out_components, outStream_components, num_nodes);
  }
}




