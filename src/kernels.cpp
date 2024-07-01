// HLS-related includes
#include "hls_math.h"
#include "hls_vector.h"
#include <hls_stream.h>

// Custom includes
#include "kernels.hpp"

#define DATA_SIZE 1
#define NUM_EDGES 24
#define NUM_NODES 11

// TRIPCOUNT identifier
const int c_size = DATA_SIZE;
const int c_size_e = NUM_EDGES;
const int c_size_n = NUM_NODES;

static void read_input_int(unsigned int* in, hls::stream<unsigned int>& inStream, int size) {
 // Auto-pipeline is going to apply pipeline to this loop
    for (int i = 0; i < size; i++) {
  #pragma HLS LOOP_TRIPCOUNT min = c_size_e max = c_size_e
      inStream << in[i];
    }
}
static void read_input_float(float* in, hls::stream<float>& inStream, int size) {
  // Auto-pipeline is going to apply pipeline to this loop
    for (int i = 0; i < size; i++) {
  #pragma HLS LOOP_TRIPCOUNT min = c_size_e max = c_size_e
      inStream << in[i];
    }
}
static void write_result(unsigned int* out, hls::stream<unsigned int>& outStream, int size) {
    for (int i = 0; i < size; i++) {
  #pragma HLS LOOP_TRIPCOUNT min = c_size_n max = c_size_n
      out[i] = outStream.read();
    }
}

static void compute_operation(hls::stream<unsigned int>& edge_from, hls::stream<unsigned int>& edge_to, hls::stream<float>& scores, hls::stream<unsigned int>& labels, int n_edges, int n_nodes){
  // Auto-pipeline is going to apply pipeline to this loop
  const float cutoff = 0.5;
  int from, to;
  float score;
    for (int i = 0; i < n_edges; i++) {
  #pragma HLS LOOP_TRIPCOUNT min = c_size_e max = c_size_e

      from = edge_from.read();
      to = edge_to.read();
      score = scores.read();

      if(score > cutoff){
        labels << from;
      } else {
        labels << to;
      }
    }
}
    
extern "C" {
  void CCL(unsigned int* in_edge_from, unsigned int* in_edge_to, float* in_scores, unsigned int* out_labels, int num_edges, int num_nodes) {
    static hls::stream<unsigned int> inStream_edge_from("input_stream1");
    static hls::stream<unsigned int> inStream_edge_to("input_stream2");
    static hls::stream<float> inStream_score("input_stream3");
    static hls::stream<unsigned int> outStream_labels("output_stream");
    
    #pragma HLS INTERFACE m_axi port = in_edge_from bundle=gmem0
    #pragma HLS INTERFACE m_axi port = in_edge_from   bundle=gmem1
    #pragma HLS INTERFACE m_axi port = in_scores    bundle=gmem2
    #pragma HLS INTERFACE m_axi port = out_labels   bundle=gmem0

    #pragma HLS dataflow
    // dataflow pragma instruct compiler to run following three APIs in parallel
    read_input_int(in_edge_from, inStream_edge_from, num_edges);
    read_input_int(in_edge_to, inStream_edge_to, num_edges);
    read_input_float(in_scores, inStream_score, num_edges);

    compute_operation(inStream_edge_from, inStream_edge_to, inStream_score, outStream_labels, num_edges, num_nodes);
    
    write_result(out_labels, outStream_labels, num_nodes);
  }
}




