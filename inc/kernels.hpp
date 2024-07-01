#ifndef KERNELS_H
#define KERNELS_H


extern "C" {
  void CCL(unsigned int* in_edge_from, unsigned int* in_edge_to, float* in_scores, unsigned int* out_labels, int num_edges, int num_nodes);
}
#endif // KERNELS_H
