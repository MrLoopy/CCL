#ifndef DDR_KERNELS_H
#define DDR_KERNELS_H


#define MAX_TOTAL_NODES 8192 // 524288 // 8192
#define MAX_EDGES 8 // 16 // 8
#define MAX_FULL_GRAPH_EDGES 256 // 128 // 256
#define MAX_COMPONENT_SIZE 16 // 128 // 16
#define MAX_TRUE_NODES 512 // 65536 // 512
#define MAX_COMPONENTS 64 // 8192 // 64
#define MAX_FULL_GRAPH_BLOCKS MAX_FULL_GRAPH_EDGES / 16

extern "C" {
  void CCL(
            hls::vector<uint32_t, 16>* in_full_graph, ap_uint<512>* in_full_graph_cons,  hls::vector<float, 16>* in_scores, unsigned int* io_graph,
            ap_uint<512>* io_graph_cons, unsigned int* out_components, unsigned int num_nodes, float cutoff);
}
#endif // DDR_KERNELS_H
