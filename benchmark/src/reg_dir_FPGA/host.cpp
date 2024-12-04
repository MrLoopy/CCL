// Standard system includes
#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <map>
#include <stdexcept>
#include <chrono>
#include <iomanip>
#include <thread>

// XRT includes
#include "xrt/xrt_bo.h"
#include <experimental/xrt_xclbin.h>
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"
#include "ap_int.h"
#include "hls_vector.h"

// External includes
#include "args.hpp"

// Custom includes
#include "kernels.hpp"

//============================================
//
// Global parameters
//
//============================================
// {"dat/dummy.csv"}; //
const u_int32_t num_threads = 1;
const std::vector<std::string> csv_names = {"dat/reg/r_event005008301.csv", "dat/reg/r_event005008302.csv", "dat/reg/r_event005008303.csv", "dat/reg/r_event005008304.csv", "dat/reg/r_event005008306.csv", "dat/reg/r_event005008308.csv", "dat/reg/r_event005008310.csv", "dat/reg/r_event005008312.csv"}; // {"dat/event005001514.csv"}; //, "dat/u_event005001604.csv", "dat/u_event005001608.csv", "dat/u_event005001614.csv", "dat/u_event005001664.csv", "dat/u_event005001670.csv"}; // {"dat/reg/r_event005008301.csv", "dat/reg/r_event005008302.csv", "dat/reg/r_event005008303.csv", "dat/reg/r_event005008304.csv", "dat/reg/r_event005008306.csv", "dat/reg/r_event005008308.csv", "dat/reg/r_event005008310.csv", "dat/reg/r_event005008312.csv"}; // {"dat/event005001514.csv", "dat/u_event005001604.csv", "dat/u_event005001608.csv", "dat/u_event005001614.csv", "dat/u_event005001664.csv", "dat/u_event005001670.csv"}; // {"dat/dummy.csv"}; // {"dat/event005001514.csv"}; // {"dat/dummy.csv"}; // {"dat/event005001514.csv", "dat/event005001514.csv"}; // {"dat/dummy.csv", "dat/dummy.csv"};
const u_int32_t num_events = (const u_int32_t)csv_names.size();
const float cutoff = 0.8;

u_int32_t size_full_graph = MAX_TOTAL_NODES * MAX_FULL_GRAPH_EDGES;
u_int32_t size_full_graph_cons = MAX_TOTAL_NODES;
u_int32_t size_scores = MAX_TOTAL_NODES * MAX_FULL_GRAPH_EDGES;
u_int32_t size_components = MAX_TRUE_NODES + MAX_COMPONENTS;

size_t size_full_graph_byte = sizeof(uint16_t) * size_full_graph;
size_t size_full_graph_cons_byte = sizeof(uint16_t) * size_full_graph_cons;
size_t size_scores_byte = sizeof(bool) * size_scores;
size_t size_components_byte = sizeof(uint16_t) * size_components;

template <typename S>
std::ostream& operator<<(std::ostream& os, const std::vector<S>& vector){
    // Printing all the elements using <<
    for (auto element : vector) {
        os << element << " ";
    }
    return os;
}

struct kernel_timing{
  u_int32_t iterations = 0;
  std::chrono::system_clock::time_point init_start;
  std::chrono::system_clock::time_point final_end;
  std::vector<std::chrono::system_clock::time_point> in_written;
  std::vector<std::chrono::system_clock::time_point> in_synced;
  std::vector<std::chrono::system_clock::time_point> krnl_started;
  std::vector<std::chrono::system_clock::time_point> krnl_ended;
  std::vector<std::chrono::system_clock::time_point> out_synced;
  std::vector<std::chrono::system_clock::time_point> out_written;
};
struct thread_timing{
  u_int32_t max_iterations = 0;
  std::chrono::system_clock::time_point start_threads;
  std::chrono::system_clock::time_point threads_running;
  std::chrono::system_clock::time_point threads_done;
  std::vector<kernel_timing> krnls;
};
struct kernel_buffers{
  xrt::device device;
  xrt::kernel kernel;
  xrt::bo in_full_graph;
  xrt::bo in_full_graph_cons;
  xrt::bo in_scores;
  xrt::bo out_components;
  kernel_buffers(xrt::device &m_device, xrt::kernel &m_kernel){
    device = m_device;
    kernel = m_kernel;
    in_full_graph = xrt::bo(device, size_full_graph_byte, kernel.group_id(0));
    in_full_graph_cons = xrt::bo(device, size_full_graph_cons_byte, kernel.group_id(1));
    in_scores = xrt::bo(device, size_scores_byte, kernel.group_id(2));
    out_components = xrt::bo(device, size_components_byte, kernel.group_id(3));
  }
};
struct kernel_maps{
  uint16_t* in_full_graph;
  uint16_t* in_full_graph_cons;
  bool* in_scores;
  uint16_t* out_components;
  kernel_maps(kernel_buffers &m_bo){
    in_full_graph = m_bo.in_full_graph.map<uint16_t*>();
    in_full_graph_cons = m_bo.in_full_graph_cons.map<uint16_t*>();
    in_scores = m_bo.in_scores.map<bool*>();
    out_components = m_bo.out_components.map<uint16_t*>();
  }
};

bool missmatch_found(float a, float b, float cutoff = 0.5){
  if(a < cutoff && b > cutoff)
    return true;
  if(a > cutoff && b < cutoff)
    return true;
  return false;
}
void print_global_prameters(void){
  std::cout << "[INFO] Number of events:      " << num_events << std::endl;
  std::cout << "[    ] Score-cut at:          " << cutoff << std::endl;
  std::cout << "[INFO] Size of full_graph:    ";
  if(size_full_graph_byte > 1024 * 1024) std::cout << size_full_graph_byte / (1024 * 1024) << " MB" << std::endl;
  else if(size_full_graph_byte > 1024) std::cout << size_full_graph_byte / 1024 << " KB" << std::endl;
  else std::cout << size_full_graph_byte / 1024 << " Bytes" << std::endl;
  std::cout << "[    ] Size of scores:        ";
  if(size_scores_byte > 1024 * 1024) std::cout << size_scores_byte / (1024 * 1024) << " MB" << std::endl;
  else if(size_scores_byte > 1024) std::cout << size_scores_byte / 1024 << " KB" << std::endl;
  else std::cout << size_scores_byte / 1024 << " Bytes" << std::endl;
  std::cout << "[    ] Size of components:    ";
  if(size_components_byte > 1024 * 1024) std::cout << size_components_byte / (1024 * 1024) << " MB" << std::endl;
  else if(size_components_byte > 1024) std::cout << size_components_byte / 1024 << " KB" << std::endl;
  else std::cout << size_components_byte / 1024 << " Bytes" << std::endl;
}
void print_thread_time(thread_timing &timing){
  std::chrono::duration<double> duration;
  std::vector<std::chrono::duration<double>> totals;
  for(u_int32_t i = 0; i < 8 ; i++) totals.push_back(timing.start_threads - timing.start_threads);
  int w = 9;
  std::cout << std::fixed << std::setprecision(3);

  std::cout << "[    ]\n[INFO] Write time stemps" << std::endl;
  std::cout << "[    ] Start at:                0.000 ms" << std::endl;
  duration = timing.threads_running - timing.start_threads;
  std::cout << "[    ] All threads running: " << std::setw(w) << duration.count() * 1000 << " ms" << std::endl;
  duration = timing.threads_done - timing.start_threads;
  std::cout << "[    ] All threads done:    " << std::setw(w) << duration.count() * 1000 << " ms" << std::endl;

  std::cout << "[    ]\n[    ] Event time stemps [ms]" << std::endl;
  uint32_t ev = 0;
  for(u_int32_t i = 0; i < timing.max_iterations ; i++){
    for(u_int32_t line = 0; line < 8 ; line++){
      std::cout << "[    ] ";
      switch(line){
        case 0: std::cout << "Event:          "; break;
        case 1: std::cout << "Write inputs: "; break;
        case 2: std::cout << "Sync inputs:  "; break;
        case 3: std::cout << "Start kernel: "; break;
        case 4: std::cout << "End kernel:   "; break;
        case 5: std::cout << "Sync output:  "; break;
        case 6: std::cout << "Write output: "; break;
        case 7: std::cout << "Kernel delta: "; break;
      }
      for(u_int32_t t = 0; t < num_threads ; t++){
        if(i == 0 && line == 0) totals[7] += timing.krnls[t].final_end - timing.krnls[t].init_start;
        if(i < timing.krnls[t].iterations){
          switch(line){
            case 0: ev = i * num_threads + t;
              std::cout << " " << ev << "         "; break;
            case 1:
              duration = timing.krnls[t].in_written[i] - timing.start_threads;
              if(i == 0) totals[line] += timing.krnls[t].in_written[0] - timing.krnls[t].init_start;
              else totals[line] += timing.krnls[t].in_written[i] - timing.krnls[t].out_written[i - 1];
              std::cout << " " << std::setw(w) << duration.count() * 1000 << " "; break;
            case 2: totals[line] += timing.krnls[t].in_synced[i] - timing.krnls[t].in_written[i];
              duration = timing.krnls[t].in_synced[i] - timing.start_threads;
              std::cout << " " << std::setw(w) << duration.count() * 1000 << " "; break;
            case 3: totals[line] += timing.krnls[t].krnl_started[i] - timing.krnls[t].in_synced[i];
              duration = timing.krnls[t].krnl_started[i] - timing.start_threads;
              std::cout << " " << std::setw(w) << duration.count() * 1000 << " "; break;
            case 4: totals[line] += timing.krnls[t].krnl_ended[i] - timing.krnls[t].krnl_started[i];
              duration = timing.krnls[t].krnl_ended[i] - timing.start_threads;
              std::cout << " " << std::setw(w) << duration.count() * 1000 << " "; break;
            case 5: totals[line] += timing.krnls[t].out_synced[i] - timing.krnls[t].krnl_ended[i];
              duration = timing.krnls[t].out_synced[i] - timing.start_threads;
              std::cout << " " << std::setw(w) << duration.count() * 1000 << " "; break;
            case 6: totals[line] += timing.krnls[t].out_written[i] - timing.krnls[t].out_synced[i];
              duration = timing.krnls[t].out_written[i] - timing.start_threads;
              std::cout << " " << std::setw(w) << duration.count() * 1000 << " "; break;
            case 7:
              duration = timing.krnls[t].krnl_ended[i] - timing.krnls[t].krnl_started[i];
              std::cout << " " << std::setw(w) << duration.count() * 1000 << " "; break;
          }
        }
      }
      std::cout << std::endl;
    }
    std::cout << "[    ]" << std::endl;
  }

  ev++;
  std::cout << "[    ] Total and average duration [ms] for " << ev << " events" << std::endl;
  std::cout << "[    ] Write inputs:    " << std::setw(9) << totals[1].count() * 1000 << "  " << std::setw(9) << totals[1].count() * 1000 / ev << std::endl;
  std::cout << "[    ] Sync inputs:     " << std::setw(9) << totals[2].count() * 1000 << "  " << std::setw(9) << totals[2].count() * 1000 / ev << std::endl;
  std::cout << "[    ] Start kernel:    " << std::setw(9) << totals[3].count() * 1000 << "  " << std::setw(9) << totals[3].count() * 1000 / ev << std::endl;
  std::cout << "[    ] Kernel time:     " << std::setw(9) << totals[4].count() * 1000 << "  " << std::setw(9) << totals[4].count() * 1000 / ev << std::endl;
  std::cout << "[    ] Sync output:     " << std::setw(9) << totals[5].count() * 1000 << "  " << std::setw(9) << totals[5].count() * 1000 / ev << std::endl;
  std::cout << "[    ] Write output:    " << std::setw(9) << totals[6].count() * 1000 << "  " << std::setw(9) << totals[6].count() * 1000 / ev << std::endl;
  std::cout << "[    ] Thread time:     " << std::setw(9) << totals[7].count() * 1000 << "  " << std::setw(9) << totals[7].count() * 1000 / ev << "\n[    ]" << std::endl;
  duration = timing.threads_done - timing.start_threads;
  std::cout << "[    ] Wall clock time: " << std::setw(9) << duration.count() * 1000 << "  " << std::setw(9) << duration.count() * 1000 / ev << "\n[    ]" << std::endl;

}
void print_kernel_time(kernel_timing &timing){
  std::chrono::duration<double> duration;
  std::vector<std::chrono::duration<double>> totals;
  for(u_int32_t i = 0; i < 8 ; i++) totals.push_back(timing.init_start - timing.init_start);
  int w = 9;
  std::cout << std::fixed << std::setprecision(3);

  std::cout << "[    ]\n[INFO] Write time stemps" << std::endl;
  std::cout << "[    ] Start at:                0.000 ms" << std::endl;
  duration = timing.final_end - timing.init_start;
  std::cout << "[    ] Total duration:      " << std::setw(w) << duration.count() * 1000 << " ms" << std::endl;

  std::cout << "[    ]\n[    ] Event time stemps [ms]" << std::endl;
  for(u_int32_t ev = 0; ev < timing.iterations ; ev++){
    std::cout << "[    ] Event:           " << ev << std::endl;
    duration = timing.in_written[ev] - timing.init_start;
    if(ev == 0) totals[1] += timing.in_written[0] - timing.init_start;
    else totals[1] += timing.in_written[ev] - timing.out_written[ev - 1];
    std::cout << "[    ] Input written:   " << std::setw(w) << duration.count() * 1000 << " " << std::endl;
    duration = timing.in_synced[ev] - timing.init_start;
    totals[2] += timing.in_synced[ev] - timing.in_written[ev];
    std::cout << "[    ] Input synced:    " << std::setw(w) << duration.count() * 1000 << " " << std::endl;
    duration = timing.krnl_started[ev] - timing.init_start;
    totals[3] += timing.krnl_started[ev] - timing.in_synced[ev];
    std::cout << "[    ] Kernel started:  " << std::setw(w) << duration.count() * 1000 << " " << std::endl;
    duration = timing.krnl_ended[ev] - timing.init_start;
    totals[4] += timing.krnl_ended[ev] - timing.krnl_started[ev];
    std::cout << "[    ] Kernel ended:    " << std::setw(w) << duration.count() * 1000 << " " << std::endl;
    duration = timing.out_synced[ev] - timing.init_start;
    totals[5] += timing.out_synced[ev] - timing.krnl_ended[ev];
    std::cout << "[    ] Output synced:   " << std::setw(w) << duration.count() * 1000 << " " << std::endl;
    duration = timing.out_written[ev] - timing.init_start;
    totals[6] += timing.out_written[ev] - timing.out_synced[ev];
    std::cout << "[    ] Output written:  " << std::setw(w) << duration.count() * 1000 << " " << std::endl;
    duration = timing.krnl_ended[ev] - timing.in_synced[ev];
    std::cout << "[    ] Kernel delta:    " << std::setw(w) << duration.count() * 1000 << " " << std::endl;
    std::cout << "[    ]" << std::endl;
  }

  u_int32_t ev = timing.iterations;
  std::cout << "[    ] Total and average duration [ms] for " << ev << " events" << std::endl;
  std::cout << "[    ] Write inputs:    " << std::setw(9) << totals[1].count() * 1000 << "  " << std::setw(9) << totals[1].count() * 1000 / ev << std::endl;
  std::cout << "[    ] Sync inputs:     " << std::setw(9) << totals[2].count() * 1000 << "  " << std::setw(9) << totals[2].count() * 1000 / ev << std::endl;
  std::cout << "[    ] Start kernel:    " << std::setw(9) << totals[3].count() * 1000 << "  " << std::setw(9) << totals[3].count() * 1000 / ev << std::endl;
  std::cout << "[    ] Kernel time:     " << std::setw(9) << (totals[3].count() + totals[4].count()) * 1000 << "  " << std::setw(9) << (totals[3].count() + totals[4].count()) * 1000 / ev << std::endl;
  std::cout << "[    ] Sync output:     " << std::setw(9) << totals[5].count() * 1000 << "  " << std::setw(9) << totals[5].count() * 1000 / ev << std::endl;
  std::cout << "[    ] Write output:    " << std::setw(9) << totals[6].count() * 1000 << "  " << std::setw(9) << totals[6].count() * 1000 / ev << "\n[    ]" << std::endl;
  duration = timing.final_end - timing.init_start;
  std::cout << "[    ] Wall clock time: " << std::setw(9) << duration.count() * 1000 << "  " << std::setw(9) << duration.count() * 1000 / ev << "\n[    ]" << std::endl;

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
  auto krnl = xrt::kernel(targetDevice, ConfigID, "CCL", xrt::kernel::cu_access_mode::exclusive);

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
  std::vector<uint16_t*> ev_in_full_graph;
  std::vector<uint16_t*> ev_in_full_graph_cons;
  std::vector<float*> ev_in_scores;
  std::vector<uint16_t*> ev_out_components;
  std::vector<uint16_t> ev_num_edges;
  std::vector<uint16_t> ev_real_edges;
  std::vector<uint16_t> ev_true_edges;
  std::vector<uint16_t> ev_num_nodes;
  std::vector<std::vector<uint16_t>> ev_ref_labels;

  //============================================
  //
  // Prepare Event data
  //
  //============================================
  std::cout << "[INFO] Prepare Event Data with Data from CSV files" << std::endl;
  for(unsigned int ev = 0; ev < num_events ; ev++){
    //
    // Read CSV-file
    //
    std::string csv_file_name = csv_names[ev];
    std::vector<uint16_t> edge_from;
    std::vector<uint16_t> edge_to;
    std::vector<uint16_t> ref_labels;
    std::vector<float> scores;
    // create an input file stream from the csv-file
    std::ifstream csv_file(csv_file_name);
    if(not csv_file.is_open()) throw std::runtime_error("Could not open CSV-file");

    // read and control the headers
    if(csv_file.good()){
      std::cout << "[    ] [" << ev << "] CSV-file opened ";
      std::string line, field_0, field_1, field_2, field_3, word;
      uint16_t num_0 = 0;
      uint16_t num_1 = 0;
      uint16_t num_2 = 0;
      uint16_t num_3 = 0;
      // read first line
      std::getline(csv_file, line);
      std::stringstream linestream_first(line);
      // read column-names
      std::getline(linestream_first, field_0, ',');
      std::getline(linestream_first, field_1, ',');
      std::getline(linestream_first, field_2, ',');
      std::getline(linestream_first, field_3, ',');
      if(field_0 == "edge_from" && field_1 == "edge_to" && field_2 == "scores" && field_3 == "labels\r"){
        std::cout << "and correct format detected; ";
      }
      else{
        std::cout << "\n[WARNING] [" << ev << "] CSV-file has wrong format" << std::endl;
        std::cout << "[       ] [" << ev << "] edge_from: " << field_0 << " ; edge_to: " << field_1 << " ; scores: " << field_2 << " ; labels: " << field_3 << std::endl;
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
        std::cout << "#edges: " << ev_num_edges[ev] << " #nodes: " << ev_num_nodes[ev];
      }
      else{
        std::cout << "\n[WARNING] number of edges is not consistant in CSV-file" << std::endl;
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
          if(std::stoi(word) < 0)
            ref_labels.push_back(65535); // 4294967295 // if -1 -> set to max_value
          else
            ref_labels.push_back(std::stoi(word));
        }
      }
    }  
    csv_file.close();
  
    //
    // Add structures for this event
    //
    ev_real_edges.push_back(0);
    ev_true_edges.push_back(0);
    ev_ref_labels.push_back(ref_labels);
    ev_in_full_graph.push_back(new uint16_t[size_full_graph]);
    std::fill(ev_in_full_graph[ev], ev_in_full_graph[ev] + size_full_graph, 0);
    ev_in_full_graph_cons.push_back(new uint16_t[size_full_graph_cons]);
    std::fill(ev_in_full_graph_cons[ev], ev_in_full_graph_cons[ev] + size_full_graph_cons, 0);
    ev_in_scores.push_back(new float[size_scores]);
    std::fill(ev_in_scores[ev], ev_in_scores[ev] + size_scores, 0.0);
    ev_out_components.push_back(new uint16_t[size_components]);
    std::fill(ev_out_components[ev], ev_out_components[ev] + size_components, 0);

    std::vector<uint16_t> exceeded_by;
    std::vector<uint16_t> ex_row;
    //
    // Fill event buffers with data from CSV
    //
    std::cout << "; CSV-data -> event buffers";
    uint16_t score_missmatch = 0;
    bool new_node = true;
    for(unsigned int i = 0; i < ev_num_edges[ev] ; i++){
      // if size of table is exceeded write a warning
      if(ev_in_full_graph_cons[ev][edge_from[i]] < MAX_FULL_GRAPH_EDGES - 1){
        // check if node is already present in row, to filter out duplicate edges
        bool new_node = true;
        for(unsigned int j = 0 ; j < ev_in_full_graph_cons[ev][edge_from[i]] ; j++)
          if(ev_in_full_graph[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES + j] == edge_to[i]){
            new_node = false;
            if(missmatch_found(ev_in_scores[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES + j], scores[i], 0.5)){
              score_missmatch++;
              if(ev_in_scores[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES + j] < 0.5)
                ev_true_edges[ev]++;
              ev_in_scores[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES + j] = 1.0;
            }
          }
        if(new_node){
          // fill tables with connections and scores of the edges and keep the number of connections for each node up to date
          ev_in_full_graph[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES + ev_in_full_graph_cons[ev][edge_from[i]]] = edge_to[i];
          ev_in_scores[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES + ev_in_full_graph_cons[ev][edge_from[i]]] = scores[i];
          ev_in_full_graph_cons[ev][edge_from[i]]++;
          ev_real_edges[ev]++;
          if(scores[i] > 0.5)
            ev_true_edges[ev]++;
        }
      }
      else{
        bool new_row = true;
        for(uint16_t k = 0; k < ex_row.size(); k++)
          if(ex_row[k] == edge_from[i]){
            exceeded_by[k]++;
            new_row = false;
          }
        if(new_row){
          ex_row.push_back(edge_from[i]);
          exceeded_by.push_back(1);
        }
      }

      // if size of table is exceeded write a warning
      if(ev_in_full_graph_cons[ev][edge_to[i]] < MAX_FULL_GRAPH_EDGES - 1){
        // check if node is already present in row, to filter out duplicate edges
        bool new_node = true;
        for(unsigned int j = 0 ; j < ev_in_full_graph_cons[ev][edge_to[i]] ; j++)
          if(ev_in_full_graph[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES + j] == edge_from[i]){
            new_node = false;
            if(missmatch_found(ev_in_scores[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES + j], scores[i], 0.5)){
              score_missmatch++;
              if(ev_in_scores[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES + j] < 0.5)
                ev_true_edges[ev]++;
              ev_in_scores[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES + j] = 1.0;
            }
          }
        if(new_node){
          // fill tables with connections and scores of the edges and keep the number of connections for each node up to date
          ev_in_full_graph[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES + ev_in_full_graph_cons[ev][edge_to[i]]] = edge_from[i];
          ev_in_scores[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES + ev_in_full_graph_cons[ev][edge_to[i]]] = scores[i];
          ev_in_full_graph_cons[ev][edge_to[i]]++;
          ev_real_edges[ev]++;
          if(scores[i] > 0.5)
            ev_true_edges[ev]++;
        }
      }
      else{
        bool new_row = true;
        for(uint16_t k = 0; k < ex_row.size(); k++)
          if(ex_row[k] == edge_to[i]){
            exceeded_by[k]++;
            new_row = false;
          }
        if(new_row){
          ex_row.push_back(edge_to[i]);
          exceeded_by.push_back(1);
        }
      }
    }
    std::cout << "; real #edges: " << ev_real_edges[ev] / 2 << "; #true-edges: " << ev_true_edges[ev] / 2 << "; score missmatches: " << score_missmatch;
    std::cout << "; " << ex_row.size() << " exceeded rows by ";
    // for(uint16_t i = 0; i < ex_row.size() ; i++)
    //   std::cout << exceeded_by[i] << " ";
    std::cout << "; done" << std::endl;
  }

  //============================================
  //
  // Allocate kernel buffers
  //
  //============================================
  std::cout << "[INFO] Allocate kernel buffers" << std::endl;
  kernel_buffers bo(targetDevice, krnl);
  kernel_maps maps(bo);

  //============================================
  //
  // Start computing events
  //
  //============================================
  std::cout << "[INFO] Start computing events" << std::endl;
  kernel_timing timing;
  timing.iterations = num_events;
  timing.init_start = std::chrono::system_clock::now();

  // Event loop
  for(unsigned int ev = 0; ev < num_events ; ev++){
    //
    // Write event buffer to global memory buffer
    //
    std::cout << "[    ] [" << ev << "] Write Event to Global Memory Buffer" << std::endl;
    for(unsigned int i = 0; i < size_full_graph_cons; i++)
      maps.in_full_graph_cons[i] = ev_in_full_graph_cons[ev][i];

    for(unsigned int i = 0; i < size_scores ; i++){
      maps.in_full_graph[i] = ev_in_full_graph[ev][i];
      if(ev_in_scores[ev][i] > cutoff)
        maps.in_scores[i] = true;
      else
        maps.in_scores[i] = false;
    }

    std::fill(maps.out_components, maps.out_components + size_components, 0);
    timing.in_written.push_back(std::chrono::system_clock::now());

    //
    // Synchronize input buffer data to device global memory
    //
    std::cout << "[    ] [" << ev << "] Synchronize input buffer data to device global memory" << std::endl;
    bo.in_full_graph.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo.in_full_graph_cons.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo.in_scores.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo.out_components.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    timing.in_synced.push_back(std::chrono::system_clock::now());

    //
    // Execute Kernel
    //
    std::cout << "[    ] [" << ev << "] Start Kernel" << std::endl;
    auto run = bo.kernel(
                          bo.in_full_graph, bo.in_full_graph_cons, bo.in_scores,
                          bo.out_components, ev_num_nodes[ev], cutoff);
    timing.krnl_started.push_back(std::chrono::system_clock::now());
    std::cout << "[    ] [" << ev << "] Wait for Kernel to finish" << std::endl;
    run.wait();
    timing.krnl_ended.push_back(std::chrono::system_clock::now());

    //
    // Synchronize device global memory to output buffer
    //
    std::cout << "[    ] [" << ev << "] Read back data from Kernel" << std::endl;
    bo.out_components.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    timing.out_synced.push_back(std::chrono::system_clock::now());

    //
    // Read event buffer from global memory buffer
    //
    std::cout << "[    ] [" << ev << "] Write results from global memory back to event buffer" << std::endl;
    for(unsigned int i = 0; i < size_components ; i++)
      ev_out_components[ev][i] = maps.out_components[i];
    timing.out_written.push_back(std::chrono::system_clock::now());
  }

  timing.final_end = std::chrono::system_clock::now();

  //============================================
  //
  // Write time stemps / durations
  //
  //============================================
  print_kernel_time(timing);

  //============================================
  //
  // Validate results
  //
  //============================================
  std::cout << "[INFO] Validate results" << std::endl;
  {
    bool correct = true;
    std::vector<bool> k_correct;
    unsigned int num_errors = 0;
    unsigned int num_errors_0 = 0;
    unsigned int num_errors_1 = 0;
    unsigned int num_errors_2 = 0;
    std::vector<unsigned int> k_errors;
    std::vector<unsigned int> k_errors_0;
    std::vector<unsigned int> k_errors_1;
    std::vector<unsigned int> k_errors_2;

    for(unsigned int ev = 0; ev < num_events ; ev++){
      k_correct.push_back(true);
      k_errors.push_back(0);
      k_errors_0.push_back(0);
      k_errors_1.push_back(0);
      k_errors_2.push_back(0);

      bool end_reached = false;
      bool first_free_node = true;
      bool processed[ev_num_nodes[ev]];
      for(unsigned int i = 0; i < ev_num_nodes[ev] ; i++)
        processed[i] = false;
      unsigned int component_size = 0;
      unsigned int node = 0;
      unsigned int first_node = 0;
      unsigned int label = 0;
      unsigned int idx = 1;
      unsigned int output_size = ev_out_components[ev][0];

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
          component_size = ev_out_components[ev][idx];
          idx++;
          size_of_components[component_size]++;
          num_components++;
          first_node = ev_out_components[ev][idx];
          idx++;
          num_component_nodes++;
          label = ev_ref_labels[ev][first_node];
          processed[first_node] = true;
          // iterate through the rest of the component and compare each label to the one of the first node
          // this makes sure, that each found component is also a component in the reference
          for(unsigned int i = 1; i < component_size ; i++){
            node = ev_out_components[ev][idx];
            idx++;
            num_component_nodes++;
            // if a missmatch has been found, there is an error in the solution
            if(label != ev_ref_labels[ev][node]){
              num_errors++;
              num_errors_0++;
              correct = false;
              k_errors[ev]++;
              k_errors_0[ev]++;
              k_correct[ev] = false;
              // std::cout << "[WARNING] Wrong result. The two following nodes should not be part of the same component" << std::endl;
              // std::cout << "[       ] node " << first_node << " has label " << label << " ; node " << node << " has label " << ev_ref_labels[ev][node] << std::endl;
            }
            processed[node] = true;
          }
          // iterate through all reference-labels and make sure that no other nodes were supposed to be part of that component
          for(unsigned int i = 0; i < ev_num_nodes[ev] ; i++){
            if(!processed[i] && ev_ref_labels[ev][i] == label){
              num_errors++;
              num_errors_1++;
              correct = false;
              k_errors[ev]++;
              k_errors_1[ev]++;
              k_correct[ev] = false;
              processed[i] = true;
              // std::cout << "[WARNING] Wrong result. The following node should also be part of the last component" << std::endl;
              // std::cout << "[       ] component with label " << label << " did not include node " << i << " with the label" << ev_ref_labels[ev][i] << std::endl;
            }
          }
        }
      }
      // make sure no component has been forgotten
      for(unsigned int i = 0; i < ev_num_nodes[ev] ; i++){
        if(!processed[i]){
          if(first_free_node){
            first_free_node = false;
            label = ev_ref_labels[ev][i];
          }
          else if(label != ev_ref_labels[ev][i]){
            num_errors++;
            num_errors_2++;
            correct = false;
            k_errors[ev]++;
            k_errors_2[ev]++;
            k_correct[ev] = false;
            // std::cout << "[WARNING] Wrong result. The following node should be part of a component, but was not" << std::endl;
            // std::cout << "[       ] node " << i << " has label " << ev_ref_labels[ev][i] << " while the label for free nodes should be " << label << std::endl;
          }
        }
      }

      if(k_correct[ev]){
        std::cout << "[    ]\n[    ] [" << ev << "] TEST PASSED" << std::endl;
        std::cout << "[    ] [" << ev << "] output-size: " << output_size << " #components: " << num_components << " #component-nodes: " << num_component_nodes << std::endl;
        std::cout << "[    ] [" << ev << "] #component / comp-size (starting at size 0): [" << size_of_components[0];
        for(unsigned int i = 1; i < MAX_COMPONENT_SIZE ; i++)
          std::cout << "," << size_of_components[i];
        std::cout << "]" << std::endl;
      } else {
        std::cout << "[    ]\n[    ] [" << ev << "] TEST FAILED" << std::endl;
        std::cout << "[    ] [" << ev << "] " << k_errors[ev] << " found mismatches ( " << k_errors_0[ev] << " / " << k_errors_1[ev] << " / " << k_errors_2[ev] << " )" << std::endl;
      }
    }

    if(correct){
      std::cout << "[    ]\n[INFO] TEST PASSED\n[    ]" << std::endl;
      std::cout << "[INFO] All results match the expected results" << std::endl;
    } else {
      std::cout << "[       ]\n[WARNING] TEST FAILED\n[       ]" << std::endl;
      std::cout << "[WARNING] " << num_errors << " total mismatches ( " << num_errors_0 << " / " << num_errors_1 << " / " << num_errors_2 << " )\n[       ]" << std::endl;
    }
  }

  //============================================
  //
  // Free allocated memory
  //
  //============================================
  for(unsigned int ev = 0; ev < num_events ; ev++){
    delete ev_in_full_graph[ev];
    delete ev_in_full_graph_cons[ev];
    delete ev_in_scores[ev];
    delete ev_out_components[ev];
  }


  return 0;
}

