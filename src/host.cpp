// Standard system includes
#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <map>
#include <stdexcept>
#include <chrono>

// XRT includes
#include "xrt/xrt_bo.h"
#include <experimental/xrt_xclbin.h>
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"

// External includes
#include "args.hpp"

// Custom includes
#include "kernels.hpp"

template <typename S>
std::ostream& operator<<(std::ostream& os, const std::vector<S>& vector){
    // Printing all the elements using <<
    for (auto element : vector) {
        os << element << " ";
    }
    return os;
}

int main (int argc, char ** argv){
  //============================================
  //
  // Command Line Arguments
  //
  //============================================

  // Pass commandline arguments
  Args args(argc, argv, "hls_ccl");

  //============================================
  //
  // Read CSV-file
  //
  //============================================

  std::string csv_file_name = CSV_FILE;
  std::vector<unsigned int> edge_from;
  std::vector<unsigned int> edge_to;
  std::vector<unsigned int> ref_labels;
  std::vector<float> scores;
  unsigned int num_edges, num_nodes;
  // float cutoff = 0.5;

  // create an input file stream from the csv-file
  std::ifstream csv_file(csv_file_name);
  if(not csv_file.is_open()) throw std::runtime_error("Could not open CSV-file");

  // read an control the headers
  if(csv_file.good()){
    std::string line, field_0, field_1, field_2, field_3, word;
    unsigned int num_0 = 0;
    unsigned int num_1 = 0;
    unsigned int num_2 = 0;
    unsigned int num_3 = 0;

    // read first line
    std::getline(csv_file, line);
    std::stringstream linestream_first(line);
    // read column-names
    std::getline(linestream_first, field_0, ',');
    std::getline(linestream_first, field_1, ',');
    std::getline(linestream_first, field_2, ',');
    std::getline(linestream_first, field_3, ',');

    if(field_0 == "edge_from" && field_1 == "edge_to" && field_2 == "scores" && field_3 == "labels\r"){
      std::cout << "[INFO] CSV-file has correct format" << std::endl;
    }
    else{
      std::cout << "[WARNING] CSV-file has wrong format" << std::endl;
      std::cout << "[       ] edge_from: " << field_0 << " ; edge_to: " << field_1 << " ; scores: " << field_2 << " ; labels: " << field_3 << std::endl;
    }

    // read second line
    std::getline(csv_file, line);
    std::stringstream linestream_second(line);
    // read number of edges and nodes
    std::getline(linestream_second, word, ',');
    num_0 = std::stoi(word);
    std::getline(linestream_second, word, ',');
    num_1 = std::stoi(word);
    std::getline(linestream_second, word, ',');
    num_2 = std::stoi(word);
    std::getline(linestream_second, word, ',');
    num_3 = std::stoi(word);

    if(num_0 == num_1 && num_0 == num_2){
      num_edges = num_0;
      num_nodes = num_3;
      std::cout << "[INFO] Number of edges: " << num_edges << " number of nodes: " << num_nodes << std::endl;
    }
    else{
      std::cout << "[WARNING] number of edges is not consistant in CSV-file" << std::endl;
      std::cout << "[       ] length edge_from: " << num_0 << " length edge_to: " << num_1 << " length scores: " << num_2 << std::endl;
    }

    // read rest of the file and store values in vectors
    while(std::getline(csv_file, line)){
      std::stringstream linestream(line);

      std::getline(linestream, word, ',');
      edge_from.push_back(std::stoi(word));
      std::getline(linestream, word, ',');
      edge_to.push_back(std::stoi(word));
      std::getline(linestream, word, ',');
      scores.push_back(std::stof(word));

      if(std::getline(linestream, word, ',')){
        ref_labels.push_back(std::stoi(word));
      }
    }
  }
  // std::cout << "[INFO] print dummy values" << std::endl;
  // std::cout << "[    ] edge_from: \t" << edge_from << std::endl;
  // std::cout << "[    ] edge_to: \t" << edge_to << std::endl;
  // std::cout << "[    ] scores: \t\t" << scores << std::endl;
  // std::cout << "[    ] ref_labels: \t" << ref_labels << std::endl;
  
  csv_file.close();
  
  //============================================
  //
  // Device Configuration
  //
  //============================================

  // Ensure that an XCL binary file is provided
  if (args.fXCLBinFile.empty()) {
    std::cout << "[WARNING] No XCL binary provided! Software and hardware emulation will not be performed." << std::endl;
    return 1;
  }

  //
  // Read settings
  //
  std::string binaryFile = args.fXCLBinFile;
  int deviceIndex = 0;
  std::cout << "[INFO] Opening the target device: " << deviceIndex << std::endl;
  auto targetDevice = xrt::device(deviceIndex);
  std::cout << "[INFO] Loading XCLbin: " << binaryFile << std::endl;
  auto ConfigID = targetDevice.load_xclbin(binaryFile);

  //
  // Calculate size of vector in bytes
  //
  size_t size_full_graph_byte = sizeof(unsigned int) * num_nodes * MAX_FULL_GRAPH_EDGES;
  size_t size_scores_byte = sizeof(float) * num_edges * 2;
  size_t size_graph_byte = sizeof(unsigned int) * MAX_TRUE_NODES * MAX_EDGES;
  size_t size_lookup_byte = sizeof(unsigned int) * MAX_TOTAL_NODES;
  size_t size_labels_byte = sizeof(unsigned int) * num_nodes;
  std::cout << "[INFO] Size of full graph:  " << size_full_graph_byte << " Bytes" << std::endl;
  std::cout << "[    ] Size of scores: " << size_scores_byte << " Bytes" << std::endl;
  std::cout << "[    ] Size of graph: " << size_graph_byte << " Bytes" << std::endl;
  std::cout << "[    ] Size of lookup: " << size_lookup_byte << " Bytes" << std::endl;
  std::cout << "[    ] Size of labels: " << size_labels_byte << " Bytes" << std::endl;
  
  //
  // Define Kernel Function to be executed on Device
  //
  auto krnl = xrt::kernel(targetDevice, ConfigID, "CCL", xrt::kernel::cu_access_mode::exclusive);

  //
  // Allocate Buffer in Global Memory
  //
  std::cout << "[INFO] Allocate Buffer in Global Memory" << std::endl;
  auto buffer_in_full_graph    = xrt::bo(targetDevice, size_full_graph_byte, krnl.group_id(0));    // gourp_id must fit to HBM-bank in u280.cfg
  auto buffer_in_scores     = xrt::bo(targetDevice, size_scores_byte, krnl.group_id(1));
  auto buffer_inout_graph = xrt::bo(targetDevice, size_graph_byte, krnl.group_id(2));
  auto buffer_inout_lookup = xrt::bo(targetDevice, size_lookup_byte, krnl.group_id(3));
  auto buffer_inout_lookup_filter = xrt::bo(targetDevice, size_lookup_byte, krnl.group_id(4));
  auto buffer_out_labels    = xrt::bo(targetDevice, size_labels_byte, krnl.group_id(5));

  auto map_in_full_graph     = buffer_in_full_graph.map<unsigned int*>();
  auto map_in_scores      = buffer_in_scores.map<float*>();
  auto map_inout_graph    = buffer_inout_graph.map<unsigned int*>();
  auto map_inout_lookup    = buffer_inout_lookup.map<unsigned int*>();
  auto map_inout_lookup_filter    = buffer_inout_lookup.map<unsigned int*>();
  auto map_out_labels     = buffer_out_labels.map<unsigned int*>();

  //set to const 0 for output only -> necessary?
  std::fill(map_in_full_graph, map_in_full_graph + num_nodes * MAX_FULL_GRAPH_EDGES, 0);
  std::fill(map_inout_graph, map_inout_graph + MAX_TRUE_NODES * MAX_EDGES, 0);
  std::fill(map_inout_lookup, map_inout_lookup + MAX_TOTAL_NODES, 0);
  std::fill(map_inout_lookup_filter, map_inout_lookup + MAX_TOTAL_NODES, 0);
  std::fill(map_out_labels, map_out_labels + num_nodes, -1);

  //
  // Fill input buffers with test data from CSV
  //
  std::cout << "[INFO] Fill full graph data structure with data from CSV-file" << std::endl;
  float score_table[num_nodes * MAX_FULL_GRAPH_EDGES];
  // set all indicators that give the number of connections of that node to 0
  for(unsigned int i = 0; i < num_nodes ; i++)
    map_in_full_graph[i * MAX_FULL_GRAPH_EDGES] = 0;
  // fill tables with connections and scores of the edges and keep the number of connections for each node up to date
  for(unsigned int i = 0; i < num_edges ; i++){
    map_in_full_graph[edge_from[i] * MAX_FULL_GRAPH_EDGES + map_in_full_graph[edge_from[i] * MAX_FULL_GRAPH_EDGES] + 1] = edge_to[i];
    score_table[edge_from[i] * MAX_FULL_GRAPH_EDGES + map_in_full_graph[edge_from[i] * MAX_FULL_GRAPH_EDGES]] = scores[i];
    map_in_full_graph[edge_from[i] * MAX_FULL_GRAPH_EDGES]++;
    map_in_full_graph[edge_to[i] * MAX_FULL_GRAPH_EDGES + map_in_full_graph[edge_to[i] * MAX_FULL_GRAPH_EDGES] + 1] = edge_from[i];
    score_table[edge_to[i] * MAX_FULL_GRAPH_EDGES + map_in_full_graph[edge_to[i] * MAX_FULL_GRAPH_EDGES]] = scores[i];
    map_in_full_graph[edge_to[i] * MAX_FULL_GRAPH_EDGES]++;
  }
  // create a stream of scores out of the score-table
  unsigned int edge = 0;
  for(unsigned int row = 0; row < num_nodes ; row++){
    for(unsigned int i = 0; i < map_in_full_graph[row * MAX_FULL_GRAPH_EDGES] ; i++){
      map_in_scores[edge] = score_table[row * MAX_FULL_GRAPH_EDGES + i];
      edge++;
    }
  }

  //
  // Synchronize input buffer data to device global memory
  //
  std::cout << "[INFO] Synchronize input buffer data to device global memory" << std::endl;
  buffer_in_full_graph.sync(XCL_BO_SYNC_BO_TO_DEVICE);
  buffer_in_scores.sync(XCL_BO_SYNC_BO_TO_DEVICE);
  buffer_inout_graph.sync(XCL_BO_SYNC_BO_TO_DEVICE);
  buffer_inout_lookup.sync(XCL_BO_SYNC_BO_TO_DEVICE);
  buffer_inout_lookup_filter.sync(XCL_BO_SYNC_BO_TO_DEVICE);

  //
  // Execute Kernel
  //
  std::cout << "[INFO] Execute Kernel" << std::endl;
  auto start = std::chrono::system_clock::now();
  auto run = krnl(buffer_in_full_graph, buffer_in_scores, buffer_inout_graph, buffer_inout_lookup, buffer_inout_lookup_filter, buffer_out_labels, num_edges, num_nodes);
  run.wait();
  auto end = std::chrono::system_clock::now();

  std::chrono::duration<double> elapsed = end - start;
  std::cout << "[    ] Elapsed time:   " << elapsed.count() << " s" << std::endl;

  //
  // Read back data from Kernel
  //
  std::cout << "[INFO] Read back data from Kernel" << std::endl;
  buffer_out_labels.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

  //
  // Validate results
  //
  std::cout << "[INFO] Validate results" << std::endl;
  bool correct = true;
  int num_errors = 0;
  const int max_num_labels = MAX_TRUE_NODES; //50;
  unsigned int lookupU[max_num_labels];
  bool lookupB[max_num_labels];
  for(unsigned int i = 0; i < max_num_labels ; i++)
    lookupB[i] = false;
  for(unsigned int i = 0; i < num_nodes ; i++){
    if(!lookupB[map_out_labels[i]]){
      lookupU[map_out_labels[i]] = ref_labels[i];
      lookupB[map_out_labels[i]] = true;
    }
    else if(lookupU[map_out_labels[i]] != ref_labels[i]){
      correct = false;
      std::cout << "[WARNING] Wrong result, node: " << i << " expected: " << ref_labels[i] << " got: " << map_out_labels[i] << std::endl;
      num_errors++;
    }
  }
  if(correct){
    std::cout << "[    ]\n[INFO] TEST PASSED\n[    ]" << std::endl;
    std::cout << "[INFO] All results of the kernel match the expected results" << std::endl;
  } else {
    std::cout << "[       ]\n[WARNING] TEST FAILED\n[       ]" << std::endl;
    std::cout << "[WARNING] " << num_errors << "  mismatches in the results have been found\n[       ]" << std::endl;
  }

  return 0;
}

