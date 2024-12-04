#ifndef KERNELS_H
#define KERNELS_H


#define MAX_TOTAL_NODES 8192 // 524288 //8192
#define MAX_EDGES 8 // 16 //8
#define MAX_FULL_GRAPH_EDGES 128 // 128 //256
#define MAX_COMPONENT_SIZE 16 // 128 //16
#define MAX_TRUE_NODES 512 // 65536 //512
#define MAX_COMPONENTS 64 // 8192 //64
#define MAX_FULL_GRAPH_BLOCKS MAX_FULL_GRAPH_EDGES / 16

extern "C" {
  void CCL( 
            uint16_t* in_full_graph, uint16_t* in_full_graph_cons, bool* in_scores,
            uint16_t* out_components, uint16_t num_nodes, float cutoff);
}
#endif // KERNELS_H
