// Standard system includes
#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <map>
#include <stdexcept>

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
  Args args(argc, argv, "FPGA studies");

  //============================================
  //
  // Read CSV-file
  //
  //============================================

  std::string csv_file_name = CSV_FILE;
  std::vector<int> edge_from;
  std::vector<int> edge_to;
  std::vector<int> ref_labels;
  std::vector<float> scores;
  int num_edges, num_nodes;
  // float cutoff = 0.5;

  // create an input file stream from the csv-file
  std::ifstream csv_file(csv_file_name);
  if(not csv_file.is_open()) throw std::runtime_error("Could not open CSV-file");

  // read an control the headers
  if(csv_file.good()){
    std::string line, field_0, field_1, field_2, field_3, word;
    int num_0 = 0;
    int num_1 = 0;
    int num_2 = 0;
    int num_3 = 0;

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
  size_t size_edges_byte = sizeof(int) * num_edges;     // size of edge_from and edge_to -> int
  size_t size_scores_byte = sizeof(float) * num_edges;  // size of scores -> float
  size_t size_labels_byte = sizeof(int) * num_nodes;    // size of labels -> int
  std::cout << "[INFO] Size of egdes:  " << size_edges_byte << " Bytes" << std::endl;
  std::cout << "[    ] Size of scores: " << size_scores_byte << " Bytes" << std::endl;
  std::cout << "[    ] Size of labels: " << size_labels_byte << " Bytes" << std::endl;

  //
  // Define Kernel Function to be executed on Device
  //
  auto krnl = xrt::kernel(targetDevice, ConfigID, "CCL", xrt::kernel::cu_access_mode::exclusive);

  //
  // Allocate Buffer in Global Memory
  //
  std::cout << "[INFO] Allocate Buffer in Global Memory" << std::endl;
  auto buffer_in_edge_from  = xrt::bo(targetDevice, size_edges_byte, krnl.group_id(0));
  auto buffer_in_edge_to    = xrt::bo(targetDevice, size_edges_byte, krnl.group_id(1));
  auto buffer_in_scores     = xrt::bo(targetDevice, size_scores_byte, krnl.group_id(2));
  auto buffer_out_labels    = xrt::bo(targetDevice, size_labels_byte, krnl.group_id(3));

  auto map_in_edge_from   = buffer_in_edge_from.map<int*>();
  auto map_in_edge_to     = buffer_in_edge_to.map<int*>();
  auto map_in_scores      = buffer_in_scores.map<float*>();
  auto map_out_labels     = buffer_out_labels.map<int*>();

  //set to const 0 for output only -> necessary?
  std::fill(map_out_labels, map_out_labels + num_nodes, -1);

  //
  // Fill input buffers with test data from CSV
  //
  std::cout << "[INFO] Fill input Buffers with data from CSV-file" << std::endl;
  for(int i = 0; i < num_edges ; i++){
    map_in_edge_from[i] = edge_from[i];
    map_in_edge_to[i] = edge_to[i];
    map_in_scores[i] = scores[i];    
  }

  //
  // Synchronize input buffer data to device global memory
  //
  std::cout << "[INFO] Synchronize input buffer data to device global memory" << std::endl;
  buffer_in_edge_from.sync(XCL_BO_SYNC_BO_TO_DEVICE);
  buffer_in_edge_to.sync(XCL_BO_SYNC_BO_TO_DEVICE);
  buffer_in_scores.sync(XCL_BO_SYNC_BO_TO_DEVICE);

  //
  // Execute Kernel
  //
  std::cout << "[INFO] Execute Kernel" << std::endl;
  auto run = krnl(buffer_in_edge_from, buffer_in_edge_to, buffer_in_scores, buffer_out_labels, num_edges, num_nodes);
  run.wait();

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
  const int max_num_labels = 50;
  int lookup[max_num_labels];
  for(int i = 0; i < max_num_labels ; i++)
    lookup[i] = -1;
  for(int i = 0; i < num_nodes ; i++){
    if(lookup[map_out_labels[i]] < 0)
      lookup[map_out_labels[i]] = ref_labels[i];
    else if(lookup[map_out_labels[i]] != ref_labels[i]){
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

