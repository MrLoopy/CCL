#ifndef DDR_KERNELS_H
#define DDR_KERNELS_H


#define MAX_TOTAL_NODES 524288 //8192
#define MAX_EDGES 16 //8
#define MAX_FULL_GRAPH_EDGES 128 //256
#define MAX_FULL_GRAPH_BLOCKS MAX_FULL_GRAPH_EDGES / 16
#define MAX_COMPONENT_SIZE 128 //16
#define MAX_TRUE_NODES 65536 //512
#define MAX_COMPONENTS 8192 //64

extern "C" {
  void CCL(
            hls::vector<uint32_t, 16>* in_full_graph_0, ap_uint<512>* in_full_graph_cons_0, hls::vector<float, 16>* in_scores_0,
            hls::vector<uint32_t, 16>* in_full_graph_1, ap_uint<512>* in_full_graph_cons_1, hls::vector<float, 16>* in_scores_1,
            hls::vector<uint32_t, 16>* in_full_graph_2, ap_uint<512>* in_full_graph_cons_2, hls::vector<float, 16>* in_scores_2,
            hls::vector<uint32_t, 16>* in_full_graph_3, ap_uint<512>* in_full_graph_cons_3, hls::vector<float, 16>* in_scores_3,
            // hls::vector<uint32_t, 16>* in_full_graph_4, ap_uint<512>* in_full_graph_cons_4, hls::vector<float, 16>* in_scores_4,
            // hls::vector<uint32_t, 16>* in_full_graph_5, ap_uint<512>* in_full_graph_cons_5, hls::vector<float, 16>* in_scores_5,
            // hls::vector<uint32_t, 16>* in_full_graph_6, ap_uint<512>* in_full_graph_cons_6, hls::vector<float, 16>* in_scores_6,
            // hls::vector<uint32_t, 16>* in_full_graph_7, ap_uint<512>* in_full_graph_cons_7, hls::vector<float, 16>* in_scores_7,
            unsigned int* out_components, unsigned int num_nodes, float cutoff);
}
#endif // DDR_KERNELS_H
