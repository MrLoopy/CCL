#ifndef KERNELS_H
#define KERNELS_H


#define MAX_TOTAL_NODES 524288 //8192
#define MAX_EDGES 16 //8
#define MAX_FULL_GRAPH_EDGES 128 //256
#define MAX_FULL_GRAPH_BLOCKS MAX_FULL_GRAPH_EDGES / 16
#define MAX_COMPONENT_SIZE 128 //16
#define MAX_TRUE_NODES 65536 //512
#define MAX_COMPONENTS 8192 //64

#define FILTER_SPLIT 4

// #define CSV_FILE "dat/dummy.csv"
// #define CSV_FILE "dat/dummy_long.csv"
#define CSV_FILE "dat/event005001514.csv"

extern "C" {
  void CCL( unsigned int* in_num_nodes,
            unsigned int* in_full_graph_sub_0, unsigned int* in_full_graph_sub_1, unsigned int* in_full_graph_sub_2, unsigned int* in_full_graph_sub_3,
            float* in_scores_sub_0, float* in_scores_sub_1, float* in_scores_sub_2, float* in_scores_sub_3,
            unsigned int* io_graph_sub_0, unsigned int* io_graph_sub_1, unsigned int* io_graph_sub_2, unsigned int* io_graph_sub_3,
            unsigned int* io_graph_main, unsigned int* io_lookup, unsigned int* io_lookup_filter, unsigned int* out_components);
}
// extern "C" {
//   void CCL(
//             hls::vector<uint32_t, 16>* in_full_graph, ap_uint<512>* in_full_graph_cons,  hls::vector<float, 16>* in_scores, unsigned int* io_graph,
//             ap_uint<512>* io_graph_cons, unsigned int* out_components, unsigned int num_nodes, float cutoff);
// }
#endif // KERNELS_H
