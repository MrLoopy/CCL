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

bool missmatch_found(float a, float b, float cutoff = 0.5){
  if(a < cutoff && b > cutoff)
    return true;
  if(a > cutoff && b < cutoff)
    return true;
  return false;
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

  std::string csv_file_name = "dat/event005001514.csv"; // "dat/dummy.csv"; //
  // std::string csv_file_name = CSV_FILE;
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
  size_t size_full_graph_byte = sizeof(unsigned int) * MAX_TOTAL_NODES * MAX_FULL_GRAPH_EDGES;
  size_t size_scores_byte = sizeof(float) * MAX_TOTAL_NODES * MAX_FULL_GRAPH_EDGES;
  size_t size_graph_byte = sizeof(unsigned int) * MAX_TRUE_NODES * MAX_EDGES;
  size_t size_lookup_byte = sizeof(unsigned int) * MAX_TRUE_NODES;
  size_t size_lookup_filter_byte = sizeof(unsigned int) * MAX_TOTAL_NODES;
  size_t size_components_byte = sizeof(unsigned int) * (MAX_TRUE_NODES + MAX_COMPONENTS);
  std::cout << "[INFO] Size of full_graph:    " << size_full_graph_byte << " Bytes" << std::endl;
  std::cout << "[    ] Size of scores:        " << size_scores_byte << " Bytes" << std::endl;
  std::cout << "[    ] Size of graph:         " << size_graph_byte << " Bytes" << std::endl;
  std::cout << "[    ] Size of lookup:        " << size_lookup_byte << " Bytes" << std::endl;
  std::cout << "[    ] Size of lookup_filter: " << size_lookup_filter_byte << " Bytes" << std::endl;
  std::cout << "[    ] Size of components:    " << size_components_byte << " Bytes" << std::endl;
  
  //
  // Define Kernel Function to be executed on Device
  //
  auto krnl = xrt::kernel(targetDevice, ConfigID, "CCL", xrt::kernel::cu_access_mode::exclusive);

  //
  // Allocate Buffer in Global Memory
  //
  std::cout << "[INFO] Allocate Buffer in Global Memory" << std::endl;
  auto buffer_in_full_graph = xrt::bo(targetDevice, size_full_graph_byte, krnl.group_id(0));    // gourp_id must fit to HBM-bank in u280.cfg
  auto buffer_in_scores = xrt::bo(targetDevice, size_scores_byte, krnl.group_id(1));
  auto buffer_inout_graph = xrt::bo(targetDevice, size_graph_byte, krnl.group_id(2));
  auto buffer_inout_lookup = xrt::bo(targetDevice, size_lookup_byte, krnl.group_id(3));
  auto buffer_inout_lookup_filter = xrt::bo(targetDevice, size_lookup_filter_byte, krnl.group_id(4));
  auto buffer_out_components = xrt::bo(targetDevice, size_components_byte, krnl.group_id(5));

  auto map_in_full_graph = buffer_in_full_graph.map<unsigned int*>();
  auto map_in_scores = buffer_in_scores.map<float*>();
  auto map_inout_graph = buffer_inout_graph.map<unsigned int*>();
  auto map_inout_lookup = buffer_inout_lookup.map<unsigned int*>();
  auto map_inout_lookup_filter = buffer_inout_lookup_filter.map<unsigned int*>();
  auto map_out_components = buffer_out_components.map<unsigned int*>();

  //set to const 0 for output only -> necessary?
  std::fill(map_in_full_graph, map_in_full_graph + MAX_TOTAL_NODES * MAX_FULL_GRAPH_EDGES, 0);
  std::fill(map_in_scores, map_in_scores + MAX_TOTAL_NODES * MAX_FULL_GRAPH_EDGES, 0.0);
  std::fill(map_inout_graph, map_inout_graph + MAX_TRUE_NODES * MAX_EDGES, 0);
  std::fill(map_inout_lookup, map_inout_lookup + MAX_TRUE_NODES, 0);
  std::fill(map_inout_lookup_filter, map_inout_lookup_filter + MAX_TOTAL_NODES, 0);
  std::fill(map_out_components, map_out_components + MAX_TRUE_NODES + MAX_COMPONENTS, 0);

  //
  // Fill input buffers with test data from CSV
  //
  std::cout << "[INFO] Fill full graph data structure with data from CSV-file" << std::endl;

  unsigned int score_missmatch = 0;
  for(unsigned int i = 0; i < num_edges ; i++){
    // if size of table is exceeded write a warning
    if(map_in_full_graph[edge_from[i] * MAX_FULL_GRAPH_EDGES] < MAX_FULL_GRAPH_EDGES - 1){
      // check if node is already present in row, to filter out duplicate edges
      bool new_node = true;
      for(unsigned int j = 0 ; j < map_in_full_graph[edge_from[i] * MAX_FULL_GRAPH_EDGES] ; j++)
        if(map_in_full_graph[edge_from[i] * MAX_FULL_GRAPH_EDGES + 1 + j] == edge_to[i]){
          new_node = false;
          if(missmatch_found(map_in_scores[edge_from[i] * MAX_FULL_GRAPH_EDGES + j], scores[i], 0.5)){
            score_missmatch++;
            map_in_scores[edge_from[i] * MAX_FULL_GRAPH_EDGES + j] = 1.0;
          }
        }
      if(new_node){
        // fill tables with connections and scores of the edges and keep the number of connections for each node up to date
        map_in_full_graph[edge_from[i] * MAX_FULL_GRAPH_EDGES + map_in_full_graph[edge_from[i] * MAX_FULL_GRAPH_EDGES] + 1] = edge_to[i];
        map_in_scores[edge_from[i] * MAX_FULL_GRAPH_EDGES + map_in_full_graph[edge_from[i] * MAX_FULL_GRAPH_EDGES]] = scores[i];
        map_in_full_graph[edge_from[i] * MAX_FULL_GRAPH_EDGES]++;
      }
    }
    else
        std::cout << "[WARNING] Full graph data structure is exceeded!\n[       ] Row " << edge_from[i] << " is already full and can not take in node " << edge_to[i] << " anymore" << std::endl;

    // if size of table is exceeded write a warning
    if(map_in_full_graph[edge_to[i] * MAX_FULL_GRAPH_EDGES] < MAX_FULL_GRAPH_EDGES - 1){
      // check if node is already present in row, to filter out duplicate edges
      bool new_node = true;
      for(unsigned int j = 0 ; j < map_in_full_graph[edge_to[i] * MAX_FULL_GRAPH_EDGES] ; j++)
        if(map_in_full_graph[edge_to[i] * MAX_FULL_GRAPH_EDGES + 1 + j] == edge_from[i]){
          new_node = false;
          if(missmatch_found(map_in_scores[edge_to[i] * MAX_FULL_GRAPH_EDGES + j], scores[i], 0.5)){
            score_missmatch++;
            map_in_scores[edge_to[i] * MAX_FULL_GRAPH_EDGES + j] = 1.0;
          }
        }
      if(new_node){
        // fill tables with connections and scores of the edges and keep the number of connections for each node up to date
        map_in_full_graph[edge_to[i] * MAX_FULL_GRAPH_EDGES + map_in_full_graph[edge_to[i] * MAX_FULL_GRAPH_EDGES] + 1] = edge_from[i];
        map_in_scores[edge_to[i] * MAX_FULL_GRAPH_EDGES + map_in_full_graph[edge_to[i] * MAX_FULL_GRAPH_EDGES]] = scores[i];
        map_in_full_graph[edge_to[i] * MAX_FULL_GRAPH_EDGES]++;
      }
    }
    else
        std::cout << "[WARNING] Full graph data structure is exceeded!\n[       ] Row " << edge_to[i] << " is already full and can not take in node " << edge_from[i] << " anymore" << std::endl;
  }
  std::cout << "[INFO] Missmatched scores of doubled edges detected and have their scores set to 1.0: " << score_missmatch << std::endl;
  if(score_missmatch % 2 == 0)
    std::cout << "[    ] Affected edges: " << score_missmatch / 2  << std::endl;

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
  auto run = krnl(buffer_in_full_graph, buffer_in_scores, buffer_inout_graph, buffer_inout_lookup, buffer_inout_lookup_filter, buffer_out_components, num_edges, num_nodes);
  run.wait();
  auto end = std::chrono::system_clock::now();

  std::chrono::duration<double> elapsed = end - start;
  std::cout << "[    ] Elapsed time:   " << elapsed.count() << " s" << std::endl;

  //
  // Read back data from Kernel
  //
  std::cout << "[INFO] Read back data from Kernel" << std::endl;
  buffer_out_components.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

  //
  // Validate results
  //
  std::cout << "[INFO] Validate results" << std::endl;
  // ##########################################################################################
  // for(unsigned int i = 0; i < map_out_components[0] ; i++)
  //   std::cout << "[    ] " << i << " - " << map_out_components[i] << std::endl;
  // ##########################################################################################

  bool correct = true;
  bool end_reached = false;
  bool first_free_node = true;
  bool processed[num_nodes];
  for(unsigned int i = 0; i < num_nodes ; i++)
    processed[i] = false;
  unsigned int num_errors = 0;
  unsigned int component_size = 0;
  unsigned int node = 0;
  unsigned int first_node = 0;
  unsigned int label = 0;
  unsigned int idx = 1;
  unsigned int output_size = map_out_components[0];
  std::cout << "[INFO] output_size = " << output_size << std::endl;
  std::cout << "[    ] max size of output = " << MAX_TRUE_NODES + MAX_COMPONENTS << std::endl;

  // count #components, #comp_nodes, comp_sizes
  unsigned int num_components = 0;
  unsigned int num_component_nodes = 0;
  unsigned int size_of_components[MAX_COMPONENT_SIZE];
  for(unsigned int i = 0; i < MAX_COMPONENT_SIZE ; i++)
    size_of_components[i] = 0;


  while(!end_reached){
    // if the index reaches the size of the output, all found components have been processed
    if(idx >= output_size){
      end_reached = true;
      break;
    }
    else{
      // read the size of the next component and get the label of its first snode
      component_size = map_out_components[idx];
      idx++;
      size_of_components[component_size]++;
      num_components++;
      first_node = map_out_components[idx];
      idx++;
      num_component_nodes++;
      label = ref_labels[first_node];
      processed[first_node] = true;
      // iterate through the rest of the component and compare each label to the one of the first node
      // this makes sure, that each found component is also a component in the reference
      for(unsigned int i = 1; i < component_size ; i++){
        node = map_out_components[idx];
        idx++;
        num_component_nodes++;
        // if a missmatch has been found, there is an error in the solution
        if(label != ref_labels[node]){
          num_errors++;
          correct = false;
          std::cout << "[WARNING] Wrong result. The two following nodes should not be part of the same component" << std::endl;
          std::cout << "[       ] node " << first_node << " has label " << label << " ; node " << node << " has label " << ref_labels[node] << std::endl;
        }
        processed[node] = true;
      }
      // iterate through all reference-labels and make sure that no other nodes were supposed to be part of that component
      for(unsigned int i = 0; i < num_nodes ; i++){
        if(!processed[i] && ref_labels[i] == label){
          num_errors++;
          correct = false;
          processed[i] = true;
          std::cout << "[WARNING] Wrong result. The following node should also be part of the last component" << std::endl;
          std::cout << "[       ] component with label " << label << " did not include node " << i << " with the label" << ref_labels[i] << std::endl;
        }
      }
    }
  }
  // make sure no component has been forgotten
  for(unsigned int i = 0; i < num_nodes ; i++){
    if(!processed[i]){
      if(first_free_node){
        first_free_node = false;
        label = ref_labels[i];
      }
      else if(label != ref_labels[i]){
        num_errors++;
        correct = false;
        std::cout << "[WARNING] Wrong result. The following node should be part of a component, but was not" << std::endl;
        std::cout << "[       ] node " << i << " has label " << ref_labels[i] << " while the label for free nodes should be " << label << std::endl;
      }
    }
  }

  if(correct){
    std::cout << "[    ]\n[INFO] TEST PASSED\n[    ]" << std::endl;
    std::cout << "[INFO] All results of the kernel match the expected results" << std::endl;
  } else {
    std::cout << "[       ]\n[WARNING] TEST FAILED\n[       ]" << std::endl;
    std::cout << "[WARNING] " << num_errors << "  mismatches in the results have been found\n[       ]" << std::endl;
  }

  std::cout << "[    ]\n[INFO] Number of different components: " << num_components << std::endl;
  std::cout << "[    ] Number of nodes that are part of components: " << num_component_nodes << std::endl;
  std::cout << "[    ] Number of components with each size (number nodes per component, starting at size 0)\n[    ] [" << size_of_components[0];
  for(unsigned int i = 1; i < MAX_COMPONENT_SIZE ; i++)
    std::cout << "," << size_of_components[i];
  std::cout << "]\n[    ]" << std::endl;

  return 0;
}

