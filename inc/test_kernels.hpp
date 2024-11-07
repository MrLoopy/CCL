#ifndef TEST_KERNELS_H
#define TEST_KERNELS_H


#define MAX_TOTAL_NODES 8192 // 262144 // 8192
#define MAX_EDGES 8 // 16 // 8
#define MAX_FULL_GRAPH_EDGES 256 // 256 // 256
#define MAX_COMPONENT_SIZE 16 // 128 // 16
#define MAX_TRUE_NODES 512 // 65536 // 512
#define MAX_COMPONENTS 64 // 8192 // 64

#define CSV_FILE "dat/event005001514.csv"

extern "C" {
  void CCL( unsigned int* in_full_graph, float* in_scores, unsigned int* out_components, unsigned int num_nodes, float cutoff);
}
#endif // TEST_KERNELS_H
