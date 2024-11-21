#ifndef DDR_KERNELS_H
#define DDR_KERNELS_H


#define MAX_TOTAL_NODES 524288 //8192
#define MAX_EDGES 16 //8
#define MAX_FULL_GRAPH_EDGES 128 //256
#define MAX_COMPONENT_SIZE 128 //16
#define MAX_TRUE_NODES 65536 //512
#define MAX_COMPONENTS 8192 //64

extern "C" {
  void CCL( unsigned int* in_full_graph, ap_int<512>* in_full_graph_cons, float* in_scores, unsigned int* io_graph, ap_int<512>* io_graph_cons, unsigned int* io_lookup, unsigned int* io_lookup_filter, unsigned int* out_components, unsigned int num_nodes, float cutoff);
}
#endif // DDR_KERNELS_H
