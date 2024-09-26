#ifndef KERNELS_H
#define KERNELS_H


#define MAX_TOTAL_NODES 524288
#define MAX_EDGES 16
#define MAX_FULL_GRAPH_EDGES 256
#define MAX_COMPONENT_SIZE 128
#define MAX_TRUE_NODES 32768
#define CSV_FILE "dat/dummy.csv"
// #define CSV_FILE "dat/dummy_long.csv"
// #define CSV_FILE "dat/event005001514.csv"
// #define MAX_NODES 12
// #define MAX_CONNECTIONS 4
// #define MAX_COMPONENT_SIZE 8
// #define MAX_COMPONENT_NODES 10

struct node_information{
  bool processed = false;
  unsigned int connections = 0;
};

extern "C" {
  void CCL(unsigned int* in_full_graph, float* in_scores, unsigned int* io_graph, unsigned int* io_lookup, unsigned int* io_lookup_filter, unsigned int* out_labels, unsigned int num_edges, unsigned int num_nodes);
  // void CCL(unsigned int* in_edge_from, unsigned int* in_edge_to, float* in_scores, unsigned int* out_labels, unsigned int num_edges, unsigned int num_nodes);
  // void CCL(unsigned int* in_edge_from, unsigned int* in_edge_to, float* in_scores, node_information* out_node_info, int* out_graph, int num_edges, int num_nodes);
}
#endif // KERNELS_H
