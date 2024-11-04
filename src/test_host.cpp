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

// External includes
#include "args.hpp"

// Custom includes
#include "test_kernels.hpp"

//============================================
//
// Global parameters
//
//============================================
const u_int32_t num_threads = 2;
const std::vector<std::string> csv_names = {"dat/event005001514.csv", "dat/event005001514.csv", "dat/event005001514.csv", "dat/event005001514.csv", "dat/event005001514.csv"}; // {"dat/dummy.csv"}; // {"dat/event005001514.csv"}; // {"dat/dummy.csv"}; // {"dat/event005001514.csv", "dat/event005001514.csv"}; // {"dat/dummy.csv", "dat/dummy.csv"};
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
  xrt::bo in_scores;
  xrt::bo inout_graph;
  xrt::bo inout_lookup;
  xrt::bo out_components;
  kernel_buffers(xrt::device &m_device, xrt::kernel &m_kernel){
    device = m_device;
    kernel = m_kernel;
    in_scores = xrt::bo(device, size_scores_byte, m_kernel.group_id(0));
    inout_graph = xrt::bo(device, size_graph_byte, m_kernel.group_id(1));
    inout_lookup = xrt::bo(device, size_lookup_byte, m_kernel.group_id(2));
    out_components = xrt::bo(device, size_components_byte, m_kernel.group_id(3));
  }
};
struct kernel_maps{
  float* in_scores;
  float* inout_graph;
  unsigned int* inout_lookup;
  unsigned int* out_components;
  kernel_maps(kernel_buffers &m_bo){
    in_scores = m_bo.in_scores.map<float*>();
    inout_graph = m_bo.inout_graph.map<float*>();
    inout_lookup = m_bo.inout_lookup.map<unsigned int*>();
    out_components = m_bo.out_components.map<unsigned int*>();
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
  std::cout << "[    ] Start at:            0.000 ms" << std::endl;
  for(unsigned int i = 0; i < iterations ; i++){
    std::cout << "[    ] Kernel " << i << std::endl;
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
    std::cout << "[    ][" << i << "] Kernel delta: " << std::setw(9) << duration.count() * 1000 << " ms\n[    ]" << std::endl;
  }
  std::cout << "[    ] Total and Average " << iterations << " iterations" << std::endl;
  duration = timing.in_written[0] - timing.init_start;
  for(unsigned int i = 1; i < iterations ; i++) duration += timing.in_written[i] - timing.out_written[i - 1];
  std::cout << "[    ][x] Write inputs: " << std::setw(9) << duration.count() * 1000 << " ms\t" << std::setw(9) << duration.count() * 1000 / num_events << " ms" << std::endl;
  duration = timing.init_start - timing.init_start;
  for(unsigned int i = 0; i < iterations ; i++) duration += timing.in_synced[i] - timing.in_written[i];
  std::cout << "[    ][x] Sync inputs:  " << std::setw(9) << duration.count() * 1000 << " ms\t" << std::setw(9) << duration.count() * 1000 / num_events << " ms" << std::endl;
  duration = timing.init_start - timing.init_start;
  for(unsigned int i = 0; i < iterations ; i++) duration += timing.krnl_started[i] - timing.in_synced[i];
  std::cout << "[    ][x] Start kernel: " << std::setw(9) << duration.count() * 1000 << " ms\t" << std::setw(9) << duration.count() * 1000 / num_events << " ms" << std::endl;
  duration = timing.init_start - timing.init_start;
  for(unsigned int i = 0; i < iterations ; i++) duration += timing.krnl_ended[i] - timing.krnl_started[i];
  std::cout << "[    ][x] Kernel time:  " << std::setw(9) << duration.count() * 1000 << " ms\t" << std::setw(9) << duration.count() * 1000 / num_events << " ms" << std::endl;
  duration = timing.init_start - timing.init_start;
  for(unsigned int i = 0; i < iterations ; i++) duration += timing.out_synced[i] - timing.krnl_ended[i];
  std::cout << "[    ][x] Sync output:  " << std::setw(9) << duration.count() * 1000 << " ms\t" << std::setw(9) << duration.count() * 1000 / num_events << " ms" << std::endl;
  duration = timing.init_start - timing.init_start;
  for(unsigned int i = 0; i < iterations ; i++) duration += timing.out_written[i] - timing.out_synced[i];
  std::cout << "[    ][x] Write output: " << std::setw(9) << duration.count() * 1000 << " ms\t" << std::setw(9) << duration.count() * 1000 / num_events << " ms\n[    ]" << std::endl;
  duration = timing.final_end - timing.init_start;
  std::cout << "[    ] Total execution: " << std::setw(9) << duration.count() * 1000 << " ms\t" << std::setw(9) << duration.count() * 1000 / num_events << " ms\n[    ]" << std::endl;

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

// execute kernel
void exe_kernel(kernel_buffers &m_bo, 
                kernel_maps &m_map, 
                kernel_timing &timing, 
                u_int32_t m_num_events, 
                std::vector<float*> m_ev_score, 
                std::vector<unsigned int*> m_ev_components, 
                std::vector<unsigned int> m_num_nodes){

  timing.iterations = m_num_events;
  timing.init_start = std::chrono::system_clock::now();

  // Event loop
  for(unsigned int ev = 0; ev < m_num_events ; ev++){
    //
    // Write event buffer to global memory buffer
    //
    std::cout << "[KRNL][" << ev << "] Write Event to Global Memory Buffer" << std::endl;
    for(unsigned int i = 0; i < size_scores ; i++)
      m_map.in_scores[i] = m_ev_score[ev][i];
    std::fill(m_map.inout_graph, m_map.inout_graph + size_graph, 0.0);
    std::fill(m_map.inout_lookup, m_map.inout_lookup + size_lookup, 0);
    std::fill(m_map.out_components, m_map.out_components + size_components, 0);
    timing.in_written.push_back(std::chrono::system_clock::now());

    //
    // Synchronize input buffer data to device global memory
    //
    std::cout << "[    ][" << ev << "] Synchronize input buffer data to device global memory" << std::endl;
    m_bo.in_scores.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    m_bo.inout_graph.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    m_bo.inout_lookup.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    timing.in_synced.push_back(std::chrono::system_clock::now());

    //
    // Execute Kernel
    //
    std::cout << "[    ][" << ev << "] Start Kernel" << std::endl;
    auto run = m_bo.kernel(m_bo.in_scores, m_bo.inout_graph, m_bo.inout_lookup, m_bo.out_components, m_num_nodes[ev]);
    timing.krnl_started.push_back(std::chrono::system_clock::now());
    std::cout << "[    ][" << ev << "] Wait for Kernel to finish" << std::endl;
    run.wait();
    timing.krnl_ended.push_back(std::chrono::system_clock::now());

    //
    // Synchronize device global memory to output buffer
    //
    std::cout << "[    ][" << ev << "] Read back data from Kernel" << std::endl;
    m_bo.out_components.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    timing.out_synced.push_back(std::chrono::system_clock::now());

    //
    // Read event buffer from global memory buffer
    //
    std::cout << "[    ][" << ev << "] Write results from global memory back to event buffer" << std::endl;
    for(unsigned int i = 0; i < size_components ; i++)
      m_ev_components[ev][i] = m_map.out_components[i];
    timing.out_written.push_back(std::chrono::system_clock::now());
    
  }

  timing.final_end = std::chrono::system_clock::now();
}

// execute threads
void exe_threads(xrt::device &m_device, 
                xrt::kernel &m_kernel, 
                thread_timing &m_thread_time, 
                u_int32_t m_num_events, 
                u_int32_t m_num_threads, 
                std::vector<float*> m_ev_score, 
                std::vector<unsigned int*> m_ev_components, 
                std::vector<unsigned int> m_ev_nodes){

  std::cout << "[THRD] Allocate kernel buffers" << std::endl;
  std::vector<u_int32_t> ev_numbers;
  std::vector<kernel_buffers> bo;
  std::vector<kernel_maps> maps;
  for(u_int32_t i = 0; i < m_num_threads ; i++){
    ev_numbers.push_back(0);
    bo.push_back(kernel_buffers(m_device, m_kernel)); // + i, 5 + i, 10 + i, 15 + i));
    maps.push_back(kernel_maps(bo[i]));
    m_thread_time.krnls.push_back(kernel_timing());
  }
  
  std::cout << "[THRD] Create event queues for each kernel" << std::endl;
  std::vector<std::vector<float*>> thread_ev_scores(m_num_threads);
  std::vector<std::vector<unsigned int*>> thread_ev_components(m_num_threads);
  std::vector<std::vector<unsigned int>> thread_num_nodes(m_num_threads);
  u_int32_t residue = m_num_events % m_num_threads;
  u_int32_t ev = 0;
  for(u_int32_t k = 0; k < m_num_events / m_num_threads ; k++)
    for(u_int32_t i = 0; i < m_num_threads ; i++){
      thread_ev_scores[i].push_back(m_ev_score[ev]);
      thread_ev_components[i].push_back(m_ev_components[ev]);
      thread_num_nodes[i].push_back(m_ev_nodes[ev]);
      ev_numbers[i]++;
      ev++;
    }
  for(u_int32_t i = 0; i < residue ; i++){
    thread_ev_scores[i].push_back(m_ev_score[ev]);
    thread_ev_components[i].push_back(m_ev_components[ev]);
    thread_num_nodes[i].push_back(m_ev_nodes[ev]);
    ev_numbers[i]++;
    ev++;
  }

  std::cout << "[THRD] Start threads" << std::endl;
  std::vector<std::thread> t(m_num_threads);
  m_thread_time.max_iterations = ev_numbers[0];
  m_thread_time.start_threads = std::chrono::system_clock::now();
  for(u_int32_t i = 0; i < m_num_threads ; i++)
    t[i] = std::thread(exe_kernel, std::ref(bo[i]), std::ref(maps[i]), std::ref(m_thread_time.krnls[i]), ev_numbers[i], thread_ev_scores[i], thread_ev_components[i], thread_num_nodes[i]);
  m_thread_time.threads_running = std::chrono::system_clock::now();
  std::cout << "[THRD] Wait for threads to end" << std::endl;
  for(u_int32_t i = 0; i < m_num_threads ; i++)
    t[i].join();
  m_thread_time.threads_done = std::chrono::system_clock::now();

  std::cout << "[THRD] Threads finished" << std::endl;
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
    // read and control the headers
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
  thread_timing thread_time;
  std::vector<kernel_timing> t_timing;
  std::cout << "[INFO] Start exe_thread()" << std::endl;
  exe_threads(targetDevice, krnl, thread_time, num_events, num_threads, ev_in_score, ev_out_components, ev_num_nodes);

  // kernel_timing k_timing;
  // kernel_buffers k_bo = kernel_buffers(targetDevice, krnl);
  // kernel_maps k_map = kernel_maps(k_bo);
  // exe_kernel(k_bo, k_map, k_timing, num_events, ev_in_score, ev_out_components, ev_num_nodes);

  //============================================
  //
  // Write time stemps / durations
  //
  //============================================
  print_thread_time(thread_time);
  // for(u_int32_t i = 0; i < num_threads ; i++)
    // print_time_stemps(t_timing[i]);

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
          // std::cout << "[    ][" << ev << "] " << row << " - expected: " << num_connections << " got: " << ev_out_components[ev][row] << std::endl;
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

