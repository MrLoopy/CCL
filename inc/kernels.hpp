#ifndef KERNELS_H
#define KERNELS_H


#define MAX_TOTAL_NODES 524288          // maximum number of total nodes in the full graph [325381]
#define MAX_EDGES 16                    // maximum number of high-score connections a node can have [14]
#define MAX_FULL_GRAPH_EDGES 128        // maximum number of all connections a node can have [136]
  // 256 MB per bank
  // -> 4 B * 524288 = 2.097.152
  // 256 MB / 2.097.152 = 128
  // -> MAX_FULL_GRAPH_EDGES = 128, to fill one complete bank
#define MAX_COMPONENT_SIZE 128          // maximum number of nodes, that can be part of the same component [81]
#define MAX_TRUE_NODES 32768            // maximum number of nodes, that have high-score connections and are therefor part of components [31991]
#define MAX_COMPONENTS 8192             // maximum number of different components [4200]
// #define CSV_FILE "dat/dummy.csv"
// #define CSV_FILE "dat/dummy_long.csv"
#define CSV_FILE "dat/event005001514.csv"

struct node_information{
  bool processed = false;
  unsigned int connections = 0;
};

extern "C" {
  void CCL(unsigned int* in_full_graph, float* in_scores, unsigned int* io_graph, unsigned int* io_lookup, unsigned int* io_lookup_filter, unsigned int* out_components, unsigned int num_edges, unsigned int num_nodes);
}
#endif // KERNELS_H
