// HLS-related includes
#include "ap_int.h"
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
#include <iostream>
#include "ddr_kernels.hpp"

static void filter_memory(float m_cutoff, unsigned int* full_graph, ap_uint<512> full_graph_cons, float* m_scores, unsigned int m_num_nodes,
                          unsigned int* m_graph, ap_uint<512> graph_cons, unsigned int* m_lookup, unsigned int* m_lookup_filter, unsigned int& m_graph_size) {

  unsigned int connections = 0;
  unsigned int new_from, new_to;
  m_graph_size = 1; // has to start at 1, cause 0 indicates that no index has been given yet
  unsigned int row = 0;
  
  graph_cons[0] = full_graph_cons[0];

  ap_uint<512> cons = 0;
  ap_uint<8>* ptr_cons = (ap_uint<8>*) &cons;
  unsigned int con_iterations = m_num_nodes / 64; // number of 512-bit-values that need to be read
  unsigned int con_residue = m_num_nodes % 64; // number of 8-bit-values that are missing after the last complete 512-bit-value
  std::cout << "[KRNL] con_iterations: " << con_iterations << " con_residue: " << con_residue << std::endl;

  cons = 0;
  std::cout << "[    ]  " << cons << std::endl;
  cons = full_graph_cons[0];
  std::cout << "[    ] ";
  for(unsigned int j = 0; j < 16 ; j++)
    std::cout << ptr_cons[j] << " ";
  std::cout << std::endl;

  if(con_iterations > 0)
    for (unsigned int c = 0; c < con_iterations ; c++){
      cons = full_graph_cons[c];
      for (unsigned int k = 0; k < 64 ; k++) // iterate through 64 8-bit values inside the 512-bit variable
        if(ptr_cons[k] > 0){
          connections = 0;
          new_from = 0;
          row = c * 64 + k;
          std::cout << "[    ] row: " << row << std::endl;
          for (unsigned int i = 0; i < ptr_cons[k]; i++)
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
          if(new_from != 0)
            m_graph[new_from * MAX_EDGES] = connections;
        }
    }
  if(con_residue > 0){
    cons = full_graph_cons[con_iterations];
    for (unsigned int k = 0; k < con_residue ; k++) // iterate through 64 8-bit values inside the 512-bit variable
      if(ptr_cons[k] > 0){
        connections = 0;
        new_from = 0;
        row = con_iterations * 64 + k;
        std::cout << "[    ] row: " << row << " len(row): " << ptr_cons[k] << " len(full_graph(row)): " << full_graph[row * MAX_FULL_GRAPH_EDGES] << std::endl;
        for (unsigned int i = 0; i < ptr_cons[k]; i++)
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
        if(new_from != 0)
          m_graph[new_from * MAX_EDGES] = connections;
      }
  }

/* 
  // for each node in the full graph / for each row in the graph-table
  filter_rows:
  for (row = 0; row < m_num_nodes; row++){
    #pragma HLS loop_tripcount min=1000 avg=4000 max=MAX_TOTAL_NODES
    connections = 0;
    new_from = 0;
    // for each connection of that node
    if(full_graph[row * MAX_FULL_GRAPH_EDGES] > 0){ // avoid for-loop 0 times
      filter_nodes:
      for (unsigned int i = 0; i < full_graph[row * MAX_FULL_GRAPH_EDGES]; i++){
        #pragma HLS loop_tripcount min=1 avg=5 max=MAX_FULL_GRAPH_EDGES
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
 */
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
    #pragma HLS loop_tripcount min=150 avg=200 max=MAX_TRUE_NODES
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

  void CCL( unsigned int* in_full_graph, ap_int<512>* in_full_graph_cons, float* in_scores, unsigned int* io_graph, ap_int<512>* io_graph_cons, unsigned int* io_lookup, unsigned int* io_lookup_filter, unsigned int* out_components, unsigned int num_nodes, float cutoff) {
    
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
    compute_core(io_graph, graph_size, outStream_components, io_lookup);
    write_components(out_components, outStream_components, num_nodes);

  }
}




