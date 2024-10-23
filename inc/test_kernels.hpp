#ifndef TEST_KERNELS_H
#define TEST_KERNELS_H


#define MAX_TOTAL_NODES 524288          // 2^19 maximum number of total nodes in the full graph [325381]
#define MAX_FULL_GRAPH_EDGES 128        // 2^7  maximum number of all connections a node can have [136]
#define MAX_TRUE_NODES 65536            // 2^16 maximum number of nodes, that have high-score connections and are therefor part of components [31991] 32768

#define CSV_FILE "dat/dummy.csv"
// #define CSV_FILE "dat/dummy_long.csv"
// #define CSV_FILE "dat/event005001514.csv"

extern "C" {
  void PIPE_TEST(float* in_scores, float* io_graph, unsigned int* io_lookup, unsigned int* out_components, unsigned int num_nodes);
}
#endif // TEST_KERNELS_H
