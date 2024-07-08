#ifndef KERNELS_H
#define KERNELS_H


#define MAX_NODES 110
#define MAX_CONNECTIONS 16
#define MAX_COMPONENT_SIZE 20
#define MAX_COMPONENT_NODES 100
#define CSV_FILE "dat/dummy_long.csv"

struct node_information{
  bool processed = false;
  int connections = 0;
};

extern "C" {
  void CCL(unsigned int* in_edge_from, unsigned int* in_edge_to, float* in_scores, unsigned int* out_labels, int num_edges, int num_nodes);
  // void CCL(unsigned int* in_edge_from, unsigned int* in_edge_to, float* in_scores, node_information* out_node_info, int* out_graph, int num_edges, int num_nodes);
}
#endif // KERNELS_H
