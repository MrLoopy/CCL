#ifndef PAR_KERNELS_H
#define PAR_KERNELS_H


#define MAX_TOTAL_NODES 524288 //8192
#define MAX_EDGES 16 //8
#define MAX_FULL_GRAPH_EDGES 128 //256
#define MAX_FULL_GRAPH_BLOCKS MAX_FULL_GRAPH_EDGES / 16
#define MAX_COMPONENT_SIZE 128 //16
#define MAX_TRUE_NODES 65536 //512
#define MAX_COMPONENTS 8192 //64

#define DIRECT_SPLIT 2

#define CSV_FILE "dat/event005001514.csv"

extern "C" {
  void CCL(
            hls::vector<uint32_t, 16>* in_full_graph_0, ap_uint<512>* in_full_graph_cons_0, hls::vector<float, 16>* in_scores_0,
            hls::vector<uint32_t, 16>* in_full_graph_1, ap_uint<512>* in_full_graph_cons_1, hls::vector<float, 16>* in_scores_1,
            hls::vector<uint32_t, 16>* in_full_graph_2, ap_uint<512>* in_full_graph_cons_2, hls::vector<float, 16>* in_scores_2,
            hls::vector<uint32_t, 16>* in_full_graph_3, ap_uint<512>* in_full_graph_cons_3, hls::vector<float, 16>* in_scores_3,
            unsigned int* out_components, unsigned int num_nodes, float cutoff);
}
#endif // PAR_KERNELS_H
