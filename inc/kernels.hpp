#ifndef KERNELS_H
#define KERNELS_H


#define MAX_NODES 12
#define MAX_CONNECTIONS 4
#define MAX_COMPONENT_SIZE 8
#define MAX_COMPONENT_NODES 10
#define CSV_FILE "dat/dummy.csv"

struct node_information{
  bool processed = false;
  int connections = 0;
};

extern "C" {
  void CCL(unsigned int* in_edge_from, unsigned int* in_edge_to, float* in_scores, unsigned int* out_labels, int num_edges, int num_nodes);
  // void CCL(unsigned int* in_edge_from, unsigned int* in_edge_to, float* in_scores, node_information* out_node_info, int* out_graph, int num_edges, int num_nodes);
}
#endif // KERNELS_H
