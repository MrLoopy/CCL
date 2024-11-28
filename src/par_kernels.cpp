// HLS-related includes
#include "ap_int.h"
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
// #include <iostream>
#include "par_kernels.hpp"

/*
static void filter_memory(float m_cutoff, unsigned int* full_graph, float* m_scores, unsigned int m_num_nodes,
                          unsigned int* m_graph, unsigned int* m_lookup, unsigned int* m_lookup_filter, unsigned int& m_graph_size) {

  unsigned int connections = 0;
  unsigned int new_from, new_to;
  m_graph_size = 1; // has to start at 1, cause 0 indicates that no index has been given yet
  
  // for each node in the full graph / for each row in the graph-table
  filter_rows:
  for (unsigned int row = 0; row < m_num_nodes; row++){
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

static void compute_direct(float m_cutoff, unsigned int* full_graph, float* scores, unsigned int* processed, hls::stream<unsigned int>& outStream, unsigned int num_nodes){
  // use lookup_filter as processed, has same length = MAX_TOTAL_NODES

  unsigned int component[MAX_COMPONENT_SIZE];
  unsigned int current_component_size = 0;
  unsigned int processed_nodes = 0;
  unsigned int next_node = 0;
  bool found_component = true;
  bool new_row = true;

  direct_rows: for(unsigned int row = 0; row < num_nodes; row++){
    if(full_graph[row * MAX_FULL_GRAPH_EDGES] > 0 && processed[row] == 0){
      if(found_component){
        found_component = false;
        new_row = true;
        current_component_size = 0;
        processed_nodes = 0;
        direct_reset: for(unsigned int i = 0; i < MAX_COMPONENT_SIZE; i++){
          // #pragma HLS unroll
          component[i] = 4294967295;
        }
      }
      direct_nodes: for (unsigned int i = 0; i < full_graph[row * MAX_FULL_GRAPH_EDGES]; i++)
        if(scores[row * MAX_FULL_GRAPH_EDGES + i] > m_cutoff){
          found_component = true;
          bool new_node = true;
          direct_check_row: for(unsigned int j = 0 ; j < current_component_size; j++){
            // #pragma HLS unroll
            if(component[j] == row)
              new_row = false;
            if(component[j] == full_graph[row * MAX_FULL_GRAPH_EDGES + 1 + i])
              new_node = false;
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
          direct_connections: for(unsigned int i = 0 ; i < full_graph[next_node * MAX_FULL_GRAPH_EDGES]; i++){
            if(scores[next_node * MAX_FULL_GRAPH_EDGES + i] > m_cutoff 
            && processed[full_graph[next_node * MAX_FULL_GRAPH_EDGES + 1 + i]] == 0 
            && current_component_size < MAX_COMPONENT_SIZE){
              bool new_node = true;
              direct_check_component: for(unsigned int j = 0 ; j < current_component_size; j++){
                // #pragma HLS unroll
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
        outStream << current_component_size;
        direct_write_output: for (unsigned int i = 0; i < current_component_size; i++)
          outStream << component[i];
      }
    }
  }
  outStream << 0;
}
*/

/* static void compute_direct( float m_cutoff, hls::vector<uint32_t, 16>* full_graph, ap_uint<512>* full_graph_cons, hls::vector<float, 16>* m_scores,
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

  bool duplicate = false;
  unsigned int row = 0;
  ap_uint<512> multi_full_con = 0;
  ap_uint<8> single_full_con = 0;
  ap_uint<8> next_node_cons = 0;
  hls::vector<float, 16> multi_scores;
  multi_scores = hls::vector<float, 16>(0.0);
  hls::vector<uint32_t, 16> multi_edges;
  multi_edges = hls::vector<uint32_t, 16>(0);
  unsigned int pos_multi = 0;
  unsigned int pos_single = 0;
  unsigned int con_iterations = num_nodes / 64;
  unsigned int con_residue = num_nodes % 64;
  unsigned int scr_iterations = 0;
  unsigned int scr_residue = 0;

  for (unsigned int c = 0; c < con_iterations + 1 ; c++){
    multi_full_con = full_graph_cons[c];
    for (unsigned int k = 0; k < 64 ; k++){
      if(c == con_iterations && k == con_residue){
        break;
      }
      single_full_con = multi_full_con.range((k + 1) * 8 - 1, k * 8);
      row = c * 64 + k;
      if(single_full_con > 0 && !processed[row]){
        if(found_component){
          found_component = false;
          new_row = true;
          current_component_size = 0;
          processed_nodes = 0;
        }
        scr_iterations = single_full_con / 16;
        scr_residue = single_full_con % 16;
        for (unsigned int s = 0; s < scr_iterations + 1 ; s++){
          multi_scores = m_scores[row * MAX_FULL_GRAPH_BLOCKS + s];
          multi_edges = full_graph[row * MAX_FULL_GRAPH_BLOCKS + s];
          for (unsigned int i = 0; i < 16 ; i++){
            if(s == scr_iterations && i == scr_residue){
              break;
            }
            if(multi_scores[i] > m_cutoff){
              found_component = true;
              if(new_row){
                new_row = false;
                component[current_component_size] = row;
                current_component_size++;
              }
              component[current_component_size] = multi_edges[i];
              current_component_size++;
            }
          }
        }
        if(found_component){
          processed[row] = true;
          processed_nodes++;
          while(current_component_size != processed_nodes){
            next_node = component[processed_nodes];

            pos_multi = next_node / 64;
            pos_single = next_node % 64;
            next_node_cons = full_graph_cons[pos_multi].range((pos_single + 1) * 8 - 1, pos_single * 8);
            if(next_node_cons > 0){
              scr_iterations = next_node_cons / 16;
              scr_residue = next_node_cons % 16;
              for (unsigned int s = 0; s < scr_iterations + 1 ; s++){
                multi_scores = m_scores[next_node * MAX_FULL_GRAPH_BLOCKS + s];
                multi_edges = full_graph[next_node * MAX_FULL_GRAPH_BLOCKS + s];
                for (unsigned int i = 0; i < 16 ; i++){
                  if(s == scr_iterations && i == scr_residue){
                    break;
                  }
                  if(multi_scores[i] > m_cutoff && !processed[multi_edges[i]] && current_component_size < MAX_COMPONENT_SIZE){
                    bool new_node = true;
                    direct_check_component:
                    for(unsigned int j = 0 ; j < current_component_size; j++){
                      if(component[j] == multi_edges[i])
                        new_node = false;
                    }
                    if(new_node){
                      component[current_component_size] = multi_edges[i];
                      current_component_size++;
                    }
                  }
                }
              }
            }
            processed[next_node] = true;
            processed_nodes++;
          }
          if(duplicate)
            duplicate = false;
          else{
            outStream << current_component_size;
            direct_write_output:
            for (unsigned int i = 0; i < current_component_size; i++)
              outStream << component[i];
          }
        }
      }
    }
  }
  outStream << 0;
} */

static void sub_direct( float m_cutoff, hls::vector<uint32_t, 16>* full_graph, ap_uint<512>* full_graph_cons, hls::vector<float, 16>* m_scores,
                        hls::stream<unsigned int>& outStream, unsigned int start, unsigned int end){

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

  bool duplicate = false;
  unsigned int row = 0;
  ap_uint<512> multi_full_con = 0;
  ap_uint<8> single_full_con = 0;
  ap_uint<8> next_node_cons = 0;
  hls::vector<float, 16> multi_scores;
  multi_scores = hls::vector<float, 16>(0.0);
  hls::vector<uint32_t, 16> multi_edges;
  multi_edges = hls::vector<uint32_t, 16>(0);
  unsigned int pos_multi = 0;
  unsigned int pos_single = 0;
  unsigned int con_iterations = end / 64;
  unsigned int con_residue = end % 64;
  unsigned int start_iterations = start / 64;
  unsigned int scr_iterations = 0;
  unsigned int scr_residue = 0;

  for (unsigned int c = start_iterations; c < con_iterations + 1 ; c++){
    multi_full_con = full_graph_cons[c];
    for (unsigned int k = 0; k < 64 ; k++){
      if(c == con_iterations && k == con_residue){
        break;
      }
      single_full_con = multi_full_con.range((k + 1) * 8 - 1, k * 8);
      row = c * 64 + k;
      if(single_full_con > 0 && !processed[row]){
        if(found_component){
          found_component = false;
          new_row = true;
          current_component_size = 0;
          processed_nodes = 0;
        }
        scr_iterations = single_full_con / 16;
        scr_residue = single_full_con % 16;
        for (unsigned int s = 0; s < scr_iterations + 1 ; s++){
          multi_scores = m_scores[row * MAX_FULL_GRAPH_BLOCKS + s];
          multi_edges = full_graph[row * MAX_FULL_GRAPH_BLOCKS + s];
          for (unsigned int i = 0; i < 16 ; i++){
            if(s == scr_iterations && i == scr_residue){
              break;
            }
            if(multi_scores[i] > m_cutoff){
              found_component = true;
              if(new_row){
                new_row = false;
                component[current_component_size] = row;
                current_component_size++;
              }
              component[current_component_size] = multi_edges[i];
              current_component_size++;
            }
          }
        }
        if(found_component){
          processed[row] = true;
          processed_nodes++;
          while(current_component_size != processed_nodes){
            next_node = component[processed_nodes];
            if(next_node < start)
              duplicate = true;

            pos_multi = next_node / 64;
            pos_single = next_node % 64;
            next_node_cons = full_graph_cons[pos_multi].range((pos_single + 1) * 8 - 1, pos_single * 8);
            if(next_node_cons > 0){
              scr_iterations = next_node_cons / 16;
              scr_residue = next_node_cons % 16;
              for (unsigned int s = 0; s < scr_iterations + 1 ; s++){
                multi_scores = m_scores[next_node * MAX_FULL_GRAPH_BLOCKS + s];
                multi_edges = full_graph[next_node * MAX_FULL_GRAPH_BLOCKS + s];
                for (unsigned int i = 0; i < 16 ; i++){
                  if(s == scr_iterations && i == scr_residue){
                    break;
                  }
                  if(multi_scores[i] > m_cutoff && !processed[multi_edges[i]] && current_component_size < MAX_COMPONENT_SIZE){
                    bool new_node = true;
                    direct_check_component:
                    for(unsigned int j = 0 ; j < current_component_size; j++){
                      if(component[j] == multi_edges[i])
                        new_node = false;
                    }
                    if(new_node){
                      component[current_component_size] = multi_edges[i];
                      current_component_size++;
                    }
                  }
                }
              }
            }
            processed[next_node] = true;
            processed_nodes++;
          }
          if(duplicate)
            duplicate = false;
          else{
            outStream << current_component_size;
            direct_write_output:
            for (unsigned int i = 0; i < current_component_size; i++)
              outStream << component[i];
          }
        }
      }
    }
  }
  outStream << 0;
}

static void merge_streams(hls::stream<unsigned int>& outStream,
                          hls::stream<unsigned int>& stream_0, hls::stream<unsigned int>& stream_1,
                          hls::stream<unsigned int>& stream_2, hls::stream<unsigned int>& stream_3,
                          hls::stream<unsigned int>& stream_4, hls::stream<unsigned int>& stream_5,
                          hls::stream<unsigned int>& stream_6, hls::stream<unsigned int>& stream_7) {

  bool str_0 = true;
  bool str_1 = true;
  bool str_2 = true;
  bool str_3 = true;
  bool str_4 = true;
  bool str_5 = true;
  bool str_6 = true;
  bool str_7 = true;
  bool strms = true;
  unsigned int component_size = 0;

  merge_while:
  while(strms){
    if(stream_0.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_0.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_0 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5 && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_0:
        for (unsigned int i = 0; i < component_size; i++)
          outStream << stream_0.read();
      }
    }
    if(stream_1.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_1.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_1 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5 && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_1:
        for (unsigned int i = 0; i < component_size; i++)
          outStream << stream_1.read();
      }
    }
    if(stream_2.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_2.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_2 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5 && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_2:
        for (unsigned int i = 0; i < component_size; i++)
          outStream << stream_2.read();
      }
    }
    if(stream_3.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_3.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_3 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5 && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_3:
        for (unsigned int i = 0; i < component_size; i++)
          outStream << stream_3.read();
      }
    }
    if(stream_4.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_4.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_4 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5 && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_4:
        for (unsigned int i = 0; i < component_size; i++)
          outStream << stream_4.read();
      }
    }
    if(stream_5.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_5.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_5 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5 && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_5:
        for (unsigned int i = 0; i < component_size; i++)
          outStream << stream_5.read();
      }
    }
    if(stream_6.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_6.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_6 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5 && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_6:
        for (unsigned int i = 0; i < component_size; i++)
          outStream << stream_6.read();
      }
    }
    if(stream_7.size() > 0){
      // every first number of each component is the size of the component;
      component_size = stream_7.read();
      // if the size == 0, the end of the stream is reached, the total size can be written and the writing ended
      if(component_size == 0){
        str_7 = false;
        if(!str_0 && !str_1 && !str_2 && !str_3 && !str_4 && !str_5 && !str_6 && !str_7){
          outStream << 0;
          strms = false;
          break;
        }
      }
      else{
        // if the stream is still running, the size of the component is written, followed by all its node-indices
        outStream << component_size;
        merge_write_nodes_7:
        for (unsigned int i = 0; i < component_size; i++)
          outStream << stream_7.read();
      }
    }
  }
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

/* void graph_reader( hls::vector<uint32_t, 16>* full_graph, hls::stream<bool>& ctrl,
                          hls::stream<uint32_t>& idx_0, hls::stream<hls::vector<uint32_t, 16>>& graph_0,
                          hls::stream<uint32_t>& idx_1, hls::stream<hls::vector<uint32_t, 16>>& graph_1){
  std::cout << "[KRNL] graph_reader 0" << std::endl;
  bool running = true;
  uint32_t address = 0;
  while(running){
    if(ctrl.size() > 0){
      running = ctrl.read();
      running = false;
      break;
    }
    if(idx_0.size() > 0){
      address = idx_0.read();
      graph_0 << full_graph[address];
    }
    if(idx_1.size() > 0){
      address = idx_1.read();
      graph_1 << full_graph[address];
    }
  }
  std::cout << "[KRNL] graph_reader 1" << std::endl;
} */

extern "C" {

  void CCL(
            hls::vector<uint32_t, 16>* in_full_graph_0, ap_uint<512>* in_full_graph_cons_0, hls::vector<float, 16>* in_scores_0,
            hls::vector<uint32_t, 16>* in_full_graph_1, ap_uint<512>* in_full_graph_cons_1, hls::vector<float, 16>* in_scores_1,
            hls::vector<uint32_t, 16>* in_full_graph_2, ap_uint<512>* in_full_graph_cons_2, hls::vector<float, 16>* in_scores_2,
            hls::vector<uint32_t, 16>* in_full_graph_3, ap_uint<512>* in_full_graph_cons_3, hls::vector<float, 16>* in_scores_3,
            hls::vector<uint32_t, 16>* in_full_graph_4, ap_uint<512>* in_full_graph_cons_4, hls::vector<float, 16>* in_scores_4,
            hls::vector<uint32_t, 16>* in_full_graph_5, ap_uint<512>* in_full_graph_cons_5, hls::vector<float, 16>* in_scores_5,
            hls::vector<uint32_t, 16>* in_full_graph_6, ap_uint<512>* in_full_graph_cons_6, hls::vector<float, 16>* in_scores_6,
            hls::vector<uint32_t, 16>* in_full_graph_7, ap_uint<512>* in_full_graph_cons_7, hls::vector<float, 16>* in_scores_7,
            unsigned int* out_components, unsigned int num_nodes, float cutoff) {
    
    #pragma HLS dataflow

    #pragma HLS INTERFACE m_axi port = in_full_graph_0      bundle=gmem0
    #pragma HLS INTERFACE m_axi port = in_full_graph_cons_0 bundle=gmem1
    #pragma HLS INTERFACE m_axi port = in_scores_0          bundle=gmem2
    #pragma HLS INTERFACE m_axi port = in_full_graph_1      bundle=gmem3
    #pragma HLS INTERFACE m_axi port = in_full_graph_cons_1 bundle=gmem4
    #pragma HLS INTERFACE m_axi port = in_scores_1          bundle=gmem5
    #pragma HLS INTERFACE m_axi port = in_full_graph_2      bundle=gmem6
    #pragma HLS INTERFACE m_axi port = in_full_graph_cons_2 bundle=gmem7
    #pragma HLS INTERFACE m_axi port = in_scores_2          bundle=gmem8
    #pragma HLS INTERFACE m_axi port = in_full_graph_3      bundle=gmem9
    #pragma HLS INTERFACE m_axi port = in_full_graph_cons_3 bundle=gmem10
    #pragma HLS INTERFACE m_axi port = in_scores_3          bundle=gmem11

    #pragma HLS INTERFACE m_axi port = in_full_graph_4      bundle=gmem12
    #pragma HLS INTERFACE m_axi port = in_full_graph_cons_4 bundle=gmem13
    #pragma HLS INTERFACE m_axi port = in_scores_4          bundle=gmem14
    #pragma HLS INTERFACE m_axi port = in_full_graph_5      bundle=gmem15
    #pragma HLS INTERFACE m_axi port = in_full_graph_cons_5 bundle=gmem16
    #pragma HLS INTERFACE m_axi port = in_scores_5          bundle=gmem17
    #pragma HLS INTERFACE m_axi port = in_full_graph_6      bundle=gmem18
    #pragma HLS INTERFACE m_axi port = in_full_graph_cons_6 bundle=gmem19
    #pragma HLS INTERFACE m_axi port = in_scores_6          bundle=gmem20
    #pragma HLS INTERFACE m_axi port = in_full_graph_7      bundle=gmem21
    #pragma HLS INTERFACE m_axi port = in_full_graph_cons_7 bundle=gmem22
    #pragma HLS INTERFACE m_axi port = in_scores_7          bundle=gmem23

    #pragma HLS INTERFACE m_axi port = out_components       bundle=gmem24
    
    static hls::stream<unsigned int> outStream_components("output_stream_components");
    static hls::stream<unsigned int> stream_out_0("stream_0");
    static hls::stream<unsigned int> stream_out_1("stream_1");
    static hls::stream<unsigned int> stream_out_2("stream_2");
    static hls::stream<unsigned int> stream_out_3("stream_3");
    static hls::stream<unsigned int> stream_out_4("stream_4");
    static hls::stream<unsigned int> stream_out_5("stream_5");
    static hls::stream<unsigned int> stream_out_6("stream_6");
    static hls::stream<unsigned int> stream_out_7("stream_7");

    static const unsigned int factor = 8;

    sub_direct(cutoff, in_full_graph_0, in_full_graph_cons_0, in_scores_0, stream_out_0, 0, (num_nodes / factor) - ((num_nodes / factor) % 64));
    sub_direct(cutoff, in_full_graph_1, in_full_graph_cons_1, in_scores_1, stream_out_1, (num_nodes * 1 / factor) - ((num_nodes * 1 / factor) % 64), (num_nodes * 2 / factor) - ((num_nodes * 2 / factor) % 64));
    sub_direct(cutoff, in_full_graph_2, in_full_graph_cons_2, in_scores_2, stream_out_2, (num_nodes * 2 / factor) - ((num_nodes * 2 / factor) % 64), (num_nodes * 3 / factor) - ((num_nodes * 3 / factor) % 64));
    sub_direct(cutoff, in_full_graph_3, in_full_graph_cons_3, in_scores_3, stream_out_3, (num_nodes * 3 / factor) - ((num_nodes * 3 / factor) % 64), (num_nodes * 4 / factor) - ((num_nodes * 4 / factor) % 64));
    sub_direct(cutoff, in_full_graph_4, in_full_graph_cons_4, in_scores_4, stream_out_4, (num_nodes * 4 / factor) - ((num_nodes * 4 / factor) % 64), (num_nodes * 5 / factor) - ((num_nodes * 5 / factor) % 64));
    sub_direct(cutoff, in_full_graph_5, in_full_graph_cons_5, in_scores_5, stream_out_5, (num_nodes * 5 / factor) - ((num_nodes * 5 / factor) % 64), (num_nodes * 6 / factor) - ((num_nodes * 6 / factor) % 64));
    sub_direct(cutoff, in_full_graph_6, in_full_graph_cons_6, in_scores_6, stream_out_6, (num_nodes * 6 / factor) - ((num_nodes * 6 / factor) % 64), (num_nodes * 7 / factor) - ((num_nodes * 7 / factor) % 64));
    sub_direct(cutoff, in_full_graph_7, in_full_graph_cons_7, in_scores_7, stream_out_7, (num_nodes * 7 / factor) - ((num_nodes * 7 / factor) % 64), num_nodes);

    // compute_direct(cutoff, in_full_graph_0, in_full_graph_cons_0, in_scores_0, outStream_components, num_nodes);

    merge_streams(outStream_components, stream_out_0, stream_out_1, stream_out_2, stream_out_3, stream_out_4, stream_out_5, stream_out_6, stream_out_7);

    write_components(out_components, outStream_components, num_nodes);

  }
}




