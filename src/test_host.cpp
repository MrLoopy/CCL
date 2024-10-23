// Standard system includes
#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <map>
#include <stdexcept>
#include <chrono>
#include <iomanip>

// XRT includes
#include "xrt/xrt_bo.h"
#include <experimental/xrt_xclbin.h>
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"

// External includes
#include "args.hpp"

// Custom includes
#include "test_kernels.hpp"

//============================================
//
// Global parameters
//
//============================================
const u_int32_t num_threads = 1;
const std::vector<std::string> csv_names = {"dat/dummy.csv", "dat/dummy.csv"}; // {"dat/dummy.csv"}; // {"dat/event005001514.csv"}; // {"dat/dummy.csv"}; // {"dat/event005001514.csv", "dat/event005001514.csv"}; // {"dat/dummy.csv", "dat/dummy.csv"};
const u_int32_t num_events = (const u_int32_t)csv_names.size();

const u_int32_t size_scores = MAX_TOTAL_NODES * MAX_FULL_GRAPH_EDGES;
const u_int32_t size_graph = MAX_TRUE_NODES * MAX_FULL_GRAPH_EDGES;
const u_int32_t size_lookup = MAX_TRUE_NODES;
const u_int32_t size_components = MAX_TOTAL_NODES;

const size_t size_scores_byte = sizeof(float) * size_scores;
const size_t size_graph_byte = sizeof(float) * size_graph;
const size_t size_lookup_byte = sizeof(unsigned int) * size_lookup;
const size_t size_components_byte = sizeof(unsigned int) * (size_components);

template <typename S>
std::ostream& operator<<(std::ostream& os, const std::vector<S>& vector){
    // Printing all the elements using <<
    for (auto element : vector) {
        os << element << " ";
    }
    return os;
}

struct kernel_timing{
  std::chrono::system_clock::time_point init_start;
  std::chrono::system_clock::time_point alloc_done;
  std::chrono::system_clock::time_point final_end;
  std::vector<std::chrono::system_clock::time_point> in_written;
  std::vector<std::chrono::system_clock::time_point> in_synced;
  std::vector<std::chrono::system_clock::time_point> krnl_started;
  std::vector<std::chrono::system_clock::time_point> krnl_ended;
  std::vector<std::chrono::system_clock::time_point> out_synced;
  std::vector<std::chrono::system_clock::time_point> out_written;
};

bool missmatch_found(float a, float b, float cutoff = 0.5){
  if(a < cutoff && b > cutoff)
    return true;
  if(a > cutoff && b < cutoff)
    return true;
  return false;
}

void print_global_prameters(void){
  std::cout << "[INFO] Number of threads:     " << num_threads << std::endl;
  std::cout << "[    ] Number of events:      " << num_events << std::endl;
  std::cout << "[INFO] Size of scores:        ";
  if(size_scores_byte > 1024 * 1024) std::cout << size_scores_byte / (1024 * 1024) << " MB" << std::endl;
  else if(size_scores_byte > 1024) std::cout << size_scores_byte / 1024 << " KB" << std::endl;
  else std::cout << size_scores_byte / 1024 << " Bytes" << std::endl;
  std::cout << "[    ] Size of graph:         ";
  if(size_graph_byte > 1024 * 1024) std::cout << size_graph_byte / (1024 * 1024) << " MB" << std::endl;
  else if(size_graph_byte > 1024) std::cout << size_graph_byte / 1024 << " KB" << std::endl;
  else std::cout << size_graph_byte / 1024 << " Bytes" << std::endl;
  std::cout << "[    ] Size of lookup:        ";
  if(size_lookup_byte > 1024 * 1024) std::cout << size_lookup_byte / (1024 * 1024) << " MB" << std::endl;
  else if(size_lookup_byte > 1024) std::cout << size_lookup_byte / 1024 << " KB" << std::endl;
  else std::cout << size_lookup_byte / 1024 << " Bytes" << std::endl;
  std::cout << "[    ] Size of components:    ";
  if(size_components_byte > 1024 * 1024) std::cout << size_components_byte / (1024 * 1024) << " MB" << std::endl;
  else if(size_components_byte > 1024) std::cout << size_components_byte / 1024 << " KB" << std::endl;
  else std::cout << size_components_byte / 1024 << " Bytes" << std::endl;
}
void print_time_stemps(kernel_timing &timing){
  std::chrono::duration<double> duration;
  uint32_t iterations = timing.in_written.size();

  // Format numbers
  std::cout << std::fixed << std::setprecision(3);
  
  std::cout << "[    ]\n[INFO] Write time stemps" << std::endl;
  std::cout << "[    ] Start:              0.000 ms" << std::endl;
  duration = timing.alloc_done - timing.init_start;
  std::cout << "[    ] Initiation:      " << std::setw(9) << duration.count() * 1000 << " ms" << std::endl;
  duration = timing.final_end - timing.init_start;
  std::cout << "[    ] Total execution: " << std::setw(9) << duration.count() * 1000 << " ms\n[    ]" << std::endl;

  for(unsigned int i = 0; i < iterations ; i++){
    std::cout << "[    ] All time stemps" << std::endl;
    duration = timing.in_written[i] - timing.init_start;
    std::cout << "[    ][" << i << "] Write inputs: " << std::setw(9) << duration.count() * 1000 << " ms" << std::endl;
    duration = timing.in_synced[i] - timing.init_start;
    std::cout << "[    ][" << i << "] Sync inputs:  " << std::setw(9) << duration.count() * 1000 << " ms" << std::endl;
    duration = timing.krnl_started[i] - timing.init_start;
    std::cout << "[    ][" << i << "] Start kernel: " << std::setw(9) << duration.count() * 1000 << " ms" << std::endl;
    duration = timing.krnl_ended[i] - timing.init_start;
    std::cout << "[    ][" << i << "] End kernel:   " << std::setw(9) << duration.count() * 1000 << " ms" << std::endl;
    duration = timing.out_synced[i] - timing.init_start;
    std::cout << "[    ][" << i << "] Sync output:  " << std::setw(9) << duration.count() * 1000 << " ms" << std::endl;
    duration = timing.out_written[i] - timing.init_start;
    std::cout << "[    ][" << i << "] Write output: " << std::setw(9) << duration.count() * 1000 << " ms" << std::endl;
    duration = timing.krnl_ended[i] - timing.krnl_started[i];
    std::cout << "[    ][" << i << "] Kernel time:  " << std::setw(9) << duration.count() * 1000 << " ms\n[    ]" << std::endl;
  }
  if(iterations > 1){
    std::cout << "[    ] Total and Average " << iterations << " iterations" << std::endl;
    duration = timing.in_written[0] - timing.alloc_done;
    for(unsigned int i = 1; i < iterations ; i++) duration += timing.in_written[i] - timing.out_written[i - 1];
    std::cout << "[    ][x] Write inputs: " << std::setw(9) << duration.count() * 1000 << " ms\t" << std::setw(9) << duration.count() * 1000 / iterations << " ms" << std::endl;
    duration = timing.init_start - timing.init_start;
    for(unsigned int i = 0; i < iterations ; i++) duration += timing.in_synced[i] - timing.in_written[i];
    std::cout << "[    ][x] Sync inputs:  " << std::setw(9) << duration.count() * 1000 << " ms\t" << std::setw(9) << duration.count() * 1000 / iterations << " ms" << std::endl;
    duration = timing.init_start - timing.init_start;
    for(unsigned int i = 0; i < iterations ; i++) duration += timing.krnl_started[i] - timing.in_synced[i];
    std::cout << "[    ][x] Start kernel: " << std::setw(9) << duration.count() * 1000 << " ms\t" << std::setw(9) << duration.count() * 1000 / iterations << " ms" << std::endl;
    duration = timing.init_start - timing.init_start;
    for(unsigned int i = 0; i < iterations ; i++) duration += timing.krnl_ended[i] - timing.krnl_started[i];
    std::cout << "[    ][x] Kernel time:  " << std::setw(9) << duration.count() * 1000 << " ms\t" << std::setw(9) << duration.count() * 1000 / iterations << " ms" << std::endl;
    duration = timing.init_start - timing.init_start;
    for(unsigned int i = 0; i < iterations ; i++) duration += timing.out_synced[i] - timing.krnl_ended[i];
    std::cout << "[    ][x] Sync output:  " << std::setw(9) << duration.count() * 1000 << " ms\t" << std::setw(9) << duration.count() * 1000 / iterations << " ms" << std::endl;
    duration = timing.init_start - timing.init_start;
    for(unsigned int i = 0; i < iterations ; i++) duration += timing.out_written[i] - timing.out_synced[i];
    std::cout << "[    ][x] Write output: " << std::setw(9) << duration.count() * 1000 << " ms\t" << std::setw(9) << duration.count() * 1000 / iterations << " ms\n[    ]" << std::endl;
  }
}

// execute kernel
void exe_kernel(xrt::device m_device, 
                xrt::kernel m_kernel, 
                kernel_timing &timing, 
                u_int32_t m_num_events, 
                std::vector<float*> m_ev_score, 
                std::vector<unsigned int*> m_ev_components, 
                std::vector<unsigned int> m_num_nodes){
  //
  // Allocate Buffer in Global Memory
  //
  timing.init_start = std::chrono::system_clock::now();
  std::cout << "[KRNL] Allocate Buffer in Global Memory" << std::endl;
  auto buffer_in_scores = xrt::bo(m_device, size_scores_byte, m_kernel.group_id(0));
  auto buffer_inout_graph = xrt::bo(m_device, size_graph_byte, m_kernel.group_id(1));
  auto buffer_inout_lookup = xrt::bo(m_device, size_lookup_byte, m_kernel.group_id(2));
  auto buffer_out_components = xrt::bo(m_device, size_components_byte, m_kernel.group_id(3));

  auto map_in_scores = buffer_in_scores.map<float*>();
  auto map_inout_graph = buffer_inout_graph.map<float*>();
  auto map_inout_lookup = buffer_inout_lookup.map<unsigned int*>();
  auto map_out_components = buffer_out_components.map<unsigned int*>();

  //set to const 0 for output only -> necessary?
  std::fill(map_in_scores, map_in_scores + size_scores, 0.0);
  std::fill(map_inout_graph, map_inout_graph + size_graph, 0.0);
  std::fill(map_inout_lookup, map_inout_lookup + size_lookup, 0);
  std::fill(map_out_components, map_out_components + size_components, 0);

  timing.alloc_done = std::chrono::system_clock::now();

  // Event loop
  for(unsigned int ev = 0; ev < m_num_events ; ev++){
    //
    // Write event buffer to global memory buffer
    //
    std::cout << "[KRNL][" << ev << "] Write Event to Global Memory Buffer" << std::endl;
    for(unsigned int i = 0; i < size_scores ; i++)
      map_in_scores[i] = m_ev_score[ev][i];
    std::fill(map_inout_graph, map_inout_graph + size_graph, 0.0);
    std::fill(map_inout_lookup, map_inout_lookup + size_lookup, 0);
    std::fill(map_out_components, map_out_components + size_components, 0);
    timing.in_written.push_back(std::chrono::system_clock::now());

    //
    // Synchronize input buffer data to device global memory
    //
    std::cout << "[    ][" << ev << "] Synchronize input buffer data to device global memory" << std::endl;
    buffer_in_scores.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    buffer_inout_graph.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    buffer_inout_lookup.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    timing.in_synced.push_back(std::chrono::system_clock::now());

    //
    // Execute Kernel
    //
    std::cout << "[    ][" << ev << "] Start Kernel" << std::endl;
    auto run = m_kernel(buffer_in_scores, buffer_inout_graph, buffer_inout_lookup, buffer_out_components, m_num_nodes[ev]);
    timing.krnl_started.push_back(std::chrono::system_clock::now());
    std::cout << "[    ][" << ev << "] Wait for Kernel to finish" << std::endl;
    run.wait();
    timing.krnl_ended.push_back(std::chrono::system_clock::now());

    //
    // Synchronize device global memory to output buffer
    //
    std::cout << "[    ][" << ev << "] Read back data from Kernel" << std::endl;
    buffer_out_components.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    timing.out_synced.push_back(std::chrono::system_clock::now());

    //
    // Read event buffer from global memory buffer
    //
    std::cout << "[    ][" << ev << "] Write results from global memory back to event buffer" << std::endl;
    for(unsigned int i = 0; i < size_components ; i++)
      m_ev_components[ev][i] = map_out_components[i];
    timing.out_written.push_back(std::chrono::system_clock::now());
    
  }

  timing.final_end = std::chrono::system_clock::now();
}

// execute threads

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
  // Device Configuration
  //
  //============================================
  // Ensure that an XCL binary file is provided
  if (args.fXCLBinFile.empty()) {
    std::cout << "[WARNING] No XCL binary provided! Software and hardware emulation will not be performed." << std::endl;
    return 1;
  }

  //============================================
  //
  // Read settings
  //
  //============================================
  std::string binaryFile = args.fXCLBinFile;
  int deviceIndex = 0;
  std::cout << "[INFO] Opening the target device: " << deviceIndex << std::endl;
  auto targetDevice = xrt::device(deviceIndex);
  std::cout << "[INFO] Loading XCLbin: " << binaryFile << std::endl;
  auto ConfigID = targetDevice.load_xclbin(binaryFile);

  //============================================
  //
  // Define Kernel Function to be executed on Device
  //
  //============================================
  auto krnl = xrt::kernel(targetDevice, ConfigID, "PIPE_TEST", xrt::kernel::cu_access_mode::exclusive);

  //============================================
  //
  // Print gloabal parameters
  //
  //============================================
  print_global_prameters();

  //============================================
  //
  // Allocate Host Memory for each event
  //
  //============================================
  std::vector<float*> ev_in_score;
  std::vector<unsigned int*> ev_out_components;
  std::vector<unsigned int> ev_num_edges;
  std::vector<unsigned int> ev_num_nodes;
  std::vector<std::vector<unsigned int>> ev_ref_labels;
  // dummy-mem for csv->ev::bo
  unsigned int* graph_cons = new unsigned int[size_scores];

  //============================================
  //
  // Prepare Event data
  //
  //============================================
  for(unsigned int ev = 0; ev < num_events ; ev++){
    //
    // Read CSV-file
    //
    std::string csv_file_name = csv_names[ev]; // "dat/event005001514.csv"; // "dat/dummy.csv"; //
    // std::string csv_file_name = CSV_FILE;
    std::vector<unsigned int> edge_from;
    std::vector<unsigned int> edge_to;
    std::vector<unsigned int> ref_labels;
    std::vector<float> scores;
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
        std::cout << "[INFO][" << ev << "] CSV-file opened and correct format detected" << std::endl;
      }
      else{
        std::cout << "[WARNING][" << ev << "] CSV-file has wrong format" << std::endl;
        std::cout << "[       ][" << ev << "] edge_from: " << field_0 << " ; edge_to: " << field_1 << " ; scores: " << field_2 << " ; labels: " << field_3 << std::endl;
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
        ev_num_edges.push_back(num_0);
        ev_num_nodes.push_back(num_3);
        std::cout << "[    ][" << ev << "] Number of edges: " << ev_num_edges[ev] << " number of nodes: " << ev_num_nodes[ev] << std::endl;
      }
      else{
        std::cout << "[WARNING][" << ev << "] number of edges is not consistant in CSV-file" << std::endl;
        std::cout << "[       ][" << ev << "] length edge_from: " << num_0 << " length edge_to: " << num_1 << " length scores: " << num_2 << std::endl;
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

    //
    // Add structures for this event
    //
    ev_ref_labels.push_back(ref_labels);
    ev_in_score.push_back(new float[size_scores]);
    std::fill(ev_in_score[ev], ev_in_score[ev] + size_scores, 0.0);
    ev_out_components.push_back(new unsigned int[size_components]);
    std::fill(ev_out_components[ev], ev_out_components[ev] + size_components, 0);
  
    //
    // Fill event buffers with data from CSV
    //
    std::cout << "[    ][" << ev << "] Fill full graph data structure with data from CSV-file" << std::endl;
    for(unsigned int i = 0; i < size_scores ; i++)
      graph_cons[i] = 0;

    unsigned int score_missmatch = 0;
    for(unsigned int i = 0; i < ev_num_edges[ev] ; i++){
      // if size of table is exceeded write a warning
      if(graph_cons[edge_from[i] * MAX_FULL_GRAPH_EDGES] < MAX_FULL_GRAPH_EDGES - 1){
        // check if node is already present in row, to filter out duplicate edges
        bool new_node = true;
        for(unsigned int j = 0 ; j < graph_cons[edge_from[i] * MAX_FULL_GRAPH_EDGES] ; j++)
          if(graph_cons[edge_from[i] * MAX_FULL_GRAPH_EDGES + 1 + j] == edge_to[i]){
            new_node = false;
            if(missmatch_found(ev_in_score[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES + j], scores[i], 0.5)){
              score_missmatch++;
              ev_in_score[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES + j] = 1.0;
            }
          }
        if(new_node){
          // fill tables with connections and scores of the edges and keep the number of connections for each node up to date
          graph_cons[edge_from[i] * MAX_FULL_GRAPH_EDGES + graph_cons[edge_from[i] * MAX_FULL_GRAPH_EDGES] + 1] = edge_to[i];
          ev_in_score[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES + graph_cons[edge_from[i] * MAX_FULL_GRAPH_EDGES]] = scores[i];
          graph_cons[edge_from[i] * MAX_FULL_GRAPH_EDGES]++;
        }
      }
      else
          std::cout << "[WARNING] Full graph data structure is exceeded!\n[       ] Row " << edge_from[i] << " is already full and can not take in node " << edge_to[i] << " anymore" << std::endl;

      // if size of table is exceeded write a warning
      if(graph_cons[edge_to[i] * MAX_FULL_GRAPH_EDGES] < MAX_FULL_GRAPH_EDGES - 1){
        // check if node is already present in row, to filter out duplicate edges
        bool new_node = true;
        for(unsigned int j = 0 ; j < graph_cons[edge_to[i] * MAX_FULL_GRAPH_EDGES] ; j++)
          if(graph_cons[edge_to[i] * MAX_FULL_GRAPH_EDGES + 1 + j] == edge_from[i]){
            new_node = false;
            if(missmatch_found(ev_in_score[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES + j], scores[i], 0.5)){
              score_missmatch++;
              ev_in_score[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES + j] = 1.0;
            }
          }
        if(new_node){
          // fill tables with connections and scores of the edges and keep the number of connections for each node up to date
          graph_cons[edge_to[i] * MAX_FULL_GRAPH_EDGES + graph_cons[edge_to[i] * MAX_FULL_GRAPH_EDGES] + 1] = edge_from[i];
          ev_in_score[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES + graph_cons[edge_to[i] * MAX_FULL_GRAPH_EDGES]] = scores[i];
          graph_cons[edge_to[i] * MAX_FULL_GRAPH_EDGES]++;
        }
      }
      else
          std::cout << "[WARNING] Full graph data structure is exceeded!\n[       ] Row " << edge_to[i] << " is already full and can not take in node " << edge_from[i] << " anymore" << std::endl;
    }
    if(score_missmatch > 0){
      std::cout << "[INFO][" << ev << "] Missmatched scores of doubled edges detected and have their scores set to 1.0: " << score_missmatch << std::endl;
      if(score_missmatch % 2 == 0)
        std::cout << "[    ][" << ev << "] Affected edges: " << score_missmatch / 2  << std::endl;
    }
  }

  //============================================
  //
  // Run threads which run kernels
  //
  //============================================
  kernel_timing k_timing;
  exe_kernel(targetDevice, krnl, k_timing, num_events, ev_in_score, ev_out_components, ev_num_nodes);

  //============================================
  //
  // Write time stemps / durations
  //
  //============================================
  print_time_stemps(k_timing);

  //============================================
  //
  // Validate results
  //
  //============================================
  std::cout << "[INFO] Validate results" << std::endl;    // parameter for each event
  {
    const float cutoff = 0.5;
    bool correct = true;
    std::vector<bool> k_correct;
    unsigned int num_errors = 0;
    std::vector<unsigned int> k_errors;
    unsigned int num_connections = 0;

    for(unsigned int ev = 0; ev < num_events ; ev++){
      k_correct.push_back(true);
      k_errors.push_back(0);
      for (unsigned int row = 0; row < ev_num_nodes[ev]; row++){
        num_connections = 0;
        for (unsigned int i = 0; i < MAX_FULL_GRAPH_EDGES; i++)
          if(ev_in_score[ev][row * MAX_FULL_GRAPH_EDGES + i] > cutoff)
            num_connections++;
        if(ev_out_components[ev][row] != num_connections){
          k_correct[ev] = false;
          correct = false;
          k_errors[ev]++;
          num_errors++;
          std::cout << "[    ][" << ev << "] " << row << " - expected: " << num_connections << " got: " << ev_out_components[ev][row] << std::endl;
        }
      }
    }

    if(correct){
      std::cout << "[    ]\n[INFO] TEST PASSED\n[    ]" << std::endl;
      std::cout << "[INFO] All results of the kernel match the expected results" << std::endl;
    } else {
      std::cout << "[       ]\n[WARNING] TEST FAILED\n[       ]" << std::endl;
      std::cout << "[WARNING] " << num_errors << " mismatches in the results have been found" << std::endl;
      for(unsigned int ev = 0; ev < num_events ; ev++)
        if(!k_correct[ev]){
          std::cout << "[       ][" << ev << "] " << k_errors[ev] << " mismatches" << std::endl;
          for (unsigned int row = 0; row < ev_num_nodes[ev]; row++){
            num_connections = 0;
            for (unsigned int i = 0; i < MAX_FULL_GRAPH_EDGES; i++)
              if(ev_in_score[ev][row * MAX_FULL_GRAPH_EDGES + i] > cutoff)
                num_connections++;
            std::cout << "[       ][ ] " << row << " - expected: " << num_connections << " got: " << ev_out_components[ev][row];
            if(ev_out_components[ev][row] != num_connections) std::cout << " E" << std::endl;
            else std::cout << std::endl;
          }
        std::cout << "[       ]" << std::endl;
        }
    }
  }

  //============================================
  //
  // Free allocated memory
  //
  //============================================
  delete graph_cons;
  for(unsigned int ev = 0; ev < num_events ; ev++){
    delete ev_in_score[ev];
    delete ev_out_components[ev];
  }

  return 0;
}

