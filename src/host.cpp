// Standard system includes
#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <map>
#include <stdexcept>
#include <chrono>
#include <thread>

// XRT includes
#include "xrt/xrt_bo.h"
#include <experimental/xrt_xclbin.h>
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"

// External includes
#include "args.hpp"

// Custom includes
#include "kernels.hpp"

//============================================
//
// Global parameters
//
//============================================
const bool ooo = false;
const u_int32_t num_threads = 2;
const std::vector<std::string> csv_names = {"dat/dummy.csv", "dat/dummy.csv"}; // {"dat/u_event005001604.csv", "dat/u_event005001608.csv", "dat/u_event005001614.csv", "dat/u_event005001664.csv", "dat/u_event005001670.csv"}; // {"dat/dummy.csv"}; // {"dat/event005001514.csv"}; // {"dat/dummy.csv"}; // {"dat/event005001514.csv", "dat/event005001514.csv"}; // {"dat/dummy.csv", "dat/dummy.csv"};
const u_int32_t num_events = (const u_int32_t)csv_names.size();

u_int32_t size_full_graph = MAX_TOTAL_NODES * MAX_FULL_GRAPH_EDGES;
u_int32_t size_scores = MAX_TOTAL_NODES * MAX_FULL_GRAPH_EDGES;
u_int32_t size_graph = MAX_TRUE_NODES * MAX_EDGES;
u_int32_t size_lookup = MAX_TRUE_NODES;
u_int32_t size_lookup_filter = MAX_TOTAL_NODES;
u_int32_t size_components = MAX_TRUE_NODES + MAX_COMPONENTS;

size_t size_full_graph_byte = sizeof(unsigned int) * size_full_graph;
size_t size_scores_byte = sizeof(float) * size_scores;
size_t size_graph_byte = sizeof(unsigned int) * size_graph;
size_t size_lookup_byte = sizeof(unsigned int) * size_lookup;
size_t size_lookup_filter_byte = sizeof(unsigned int) * size_lookup_filter;
size_t size_components_byte = sizeof(unsigned int) * size_components;

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
  xrt::bo in_scores;
  xrt::bo inout_graph;
  xrt::bo inout_lookup;
  xrt::bo inout_lookup_filter;
  xrt::bo out_components;
  kernel_buffers(xrt::device &m_device, xrt::kernel &m_kernel, u_int32_t offset, u_int32_t st = 3){
    device = m_device;
    kernel = m_kernel;
    std::cout << "[    ]    " << offset << " 0" << std::endl; in_full_graph = xrt::bo(device, size_full_graph_byte, kernel.group_id(0 * st + offset));
    std::cout << "[    ]    " << offset << " 1" << std::endl; in_scores = xrt::bo(device, size_scores_byte, kernel.group_id(1 * st + offset));
    std::cout << "[    ]    " << offset << " 2" << std::endl; inout_graph = xrt::bo(device, size_graph_byte, kernel.group_id(2 * st + offset));
    std::cout << "[    ]    " << offset << " 3" << std::endl; inout_lookup = xrt::bo(device, size_lookup_byte, kernel.group_id(3 * st + offset));
    std::cout << "[    ]    " << offset << " 4" << std::endl; inout_lookup_filter = xrt::bo(device, size_lookup_filter_byte, kernel.group_id(4 * st + offset));
    std::cout << "[    ]    " << offset << " 5" << std::endl; out_components = xrt::bo(device, size_components_byte, kernel.group_id(5 * st + offset));
    std::cout << "[    ]    " << offset << " 6" << std::endl;
  }
};
struct kernel_maps{
  unsigned int* in_full_graph;
  float* in_scores;
  unsigned int* inout_graph;
  unsigned int* inout_lookup;
  unsigned int* inout_lookup_filter;
  unsigned int* out_components;
  kernel_maps(kernel_buffers &m_bo){
    in_full_graph = m_bo.in_full_graph.map<unsigned int*>();
    in_scores = m_bo.in_scores.map<float*>();
    inout_graph = m_bo.inout_graph.map<unsigned int*>();
    inout_lookup = m_bo.inout_lookup.map<unsigned int*>();
    inout_lookup_filter = m_bo.inout_lookup_filter.map<unsigned int*>();
    out_components = m_bo.out_components.map<unsigned int*>();
  }
};

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
void print_global_prameters(void){
  std::cout << "[INFO] Number of threads:     " << num_threads << std::endl;
  std::cout << "[    ] Number of events:      " << num_events << std::endl;
  std::cout << "[INFO] Size of full_graph:    ";
  if(size_full_graph_byte > 1024 * 1024) std::cout << size_full_graph_byte / (1024 * 1024) << " MB" << std::endl;
  else if(size_full_graph_byte > 1024) std::cout << size_full_graph_byte / 1024 << " KB" << std::endl;
  else std::cout << size_full_graph_byte / 1024 << " Bytes" << std::endl;
  std::cout << "[    ] Size of scores:        ";
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
  std::cout << "[    ] Size of lookup_filter: ";
  if(size_lookup_filter_byte > 1024 * 1024) std::cout << size_lookup_filter_byte / (1024 * 1024) << " MB" << std::endl;
  else if(size_lookup_filter_byte > 1024) std::cout << size_lookup_filter_byte / 1024 << " KB" << std::endl;
  else std::cout << size_lookup_filter_byte / 1024 << " Bytes" << std::endl;
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

// execute kernel
void exe_kernel(u_int32_t tid,
                std::vector<kernel_buffers> &m_bo, 
                kernel_maps &m_map, 
                kernel_timing &timing, 
                u_int32_t m_num_events, 
                std::vector<unsigned int*> m_ev_full_graph, 
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
    std::cout << "[KRNL] [" << tid << "] [" << ev << "] Write Event to Global Memory Buffer" << std::endl;
    for(unsigned int i = 0; i < size_scores ; i++){
      m_map.in_full_graph[i] = m_ev_full_graph[ev][i];
      m_map.in_scores[i] = m_ev_score[ev][i];
    }
    std::fill(m_map.inout_graph, m_map.inout_graph + size_graph, 0.0);
    std::fill(m_map.inout_lookup, m_map.inout_lookup + size_lookup, 0);
    std::fill(m_map.inout_lookup_filter, m_map.inout_lookup_filter + size_lookup_filter, 0);
    std::fill(m_map.out_components, m_map.out_components + size_components, 0);
    timing.in_written.push_back(std::chrono::system_clock::now());

    //
    // Synchronize input buffer data to device global memory
    //
    std::cout << "[    ] [" << tid << "] [" << ev << "] Synchronize input buffer data to device global memory" << std::endl;
    m_bo[tid].in_full_graph.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    m_bo[tid].in_scores.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    m_bo[tid].inout_graph.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    m_bo[tid].inout_lookup.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    m_bo[tid].inout_lookup_filter.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    timing.in_synced.push_back(std::chrono::system_clock::now());

    //
    // Execute Kernel
    //
    std::cout << "[    ] [" << tid << "] [" << ev << "] Start Kernel" << std::endl;
    // auto run = m_bo.kernel(m_bo.in_full_graph, m_bo.in_scores, m_bo.inout_graph, m_bo.inout_lookup, m_bo.inout_lookup_filter, m_bo.out_components, 0, m_num_nodes[ev]);
    xrt::run run;
    if(tid == 0){
      run = m_bo[tid].kernel( m_bo[0].in_full_graph, NULL, NULL,
                              m_bo[0].in_scores, NULL, NULL,
                              m_bo[0].inout_graph, NULL, NULL,
                              m_bo[0].inout_lookup, NULL, NULL,
                              m_bo[0].inout_lookup_filter, NULL, NULL,
                              m_bo[0].out_components, NULL, NULL,
                              m_num_nodes[ev], NULL, NULL,
                              tid);
    }
    else if(tid == 1){
      run = m_bo[tid].kernel( NULL, m_bo[1].in_full_graph, NULL,
                              NULL, m_bo[1].in_scores, NULL,
                              NULL, m_bo[1].inout_graph, NULL,
                              NULL, m_bo[1].inout_lookup, NULL,
                              NULL, m_bo[1].inout_lookup_filter, NULL,
                              NULL, m_bo[1].out_components, NULL,
                              NULL, m_num_nodes[ev], NULL,
                              tid);
    }
    else if(tid == 2){
      run = m_bo[tid].kernel( NULL, NULL, m_bo[2].in_full_graph,
                              NULL, NULL, m_bo[2].in_scores,
                              NULL, NULL, m_bo[2].inout_graph,
                              NULL, NULL, m_bo[2].inout_lookup,
                              NULL, NULL, m_bo[2].inout_lookup_filter,
                              NULL, NULL, m_bo[2].out_components,
                              NULL, NULL, m_num_nodes[ev],
                              tid);
    }

  // void CCL( unsigned int* in_full_graph_0, unsigned int* in_full_graph_1, unsigned int* in_full_graph_2,
  //           float* in_scores_0, float* in_scores_1, float* in_scores_2,
  //           unsigned int* io_graph_0, unsigned int* io_graph_1, unsigned int* io_graph_2,
  //           unsigned int* io_lookup_0, unsigned int* io_lookup_1, unsigned int* io_lookup_2,
  //           unsigned int* io_lookup_filter_0, unsigned int* io_lookup_filter_1, unsigned int* io_lookup_filter_2,
  //           unsigned int* out_components_0, unsigned int* out_components_1, unsigned int* out_components_2,
  //           unsigned int num_nodes_0, unsigned int num_nodes_1, unsigned int num_nodes_2,
  //           unsigned int tid);

    timing.krnl_started.push_back(std::chrono::system_clock::now());
    std::cout << "[    ] [" << tid << "] [" << ev << "] Wait for Kernel to finish" << std::endl;
    run.wait();
    timing.krnl_ended.push_back(std::chrono::system_clock::now());

    //
    // Synchronize device global memory to output buffer
    //
    std::cout << "[    ] [" << tid << "] [" << ev << "] Read back data from Kernel" << std::endl;
    m_bo[tid].out_components.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    timing.out_synced.push_back(std::chrono::system_clock::now());

    //
    // Read event buffer from global memory buffer
    //
    std::cout << "[    ] [" << tid << "] [" << ev << "] Write results from global memory back to event buffer" << std::endl;
    for(unsigned int i = 0; i < size_components ; i++)
      m_ev_components[ev][i] = m_map.out_components[i];
    timing.out_written.push_back(std::chrono::system_clock::now());
    
  }

  timing.final_end = std::chrono::system_clock::now();
}

// execute kernel
void exe_kernel_ooo(u_int32_t tid,
                kernel_buffers &m_bo, 
                kernel_maps &m_map, 
                kernel_timing &timing, 
                u_int32_t m_num_events, 
                std::vector<unsigned int*> m_ev_full_graph, 
                std::vector<float*> m_ev_score, 
                std::vector<unsigned int*> m_ev_components, 
                std::vector<unsigned int> m_num_nodes){

  timing.iterations = m_num_events;
  timing.init_start = std::chrono::system_clock::now();

  //
  // [0] Write event buffer to global memory buffer
  //
  std::cout << "[    ] [" << tid << "] [" << 0 << "] wr" << std::endl;
  for(unsigned int i = 0; i < size_scores ; i++){
    m_map.in_full_graph[i] = m_ev_full_graph[0][i];
    m_map.in_scores[i] = m_ev_score[0][i];
  }
  std::fill(m_map.inout_graph, m_map.inout_graph + size_graph, 0.0);
  std::fill(m_map.inout_lookup, m_map.inout_lookup + size_lookup, 0);
  std::fill(m_map.inout_lookup_filter, m_map.inout_lookup_filter + size_lookup_filter, 0);
  std::fill(m_map.out_components, m_map.out_components + size_components, 0);
  timing.in_written.push_back(std::chrono::system_clock::now());

  //
  // Event loop
  //
  for(unsigned int ev = 0; ev < m_num_events ; ev++){
    //
    // Synchronize input buffer data to device global memory
    //
    std::cout << "[    ] [" << tid << "] [" << ev << "] sy-w" << std::endl;
    m_bo.in_full_graph.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    m_bo.in_scores.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    m_bo.inout_graph.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    m_bo.inout_lookup.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    m_bo.inout_lookup_filter.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    timing.in_synced.push_back(std::chrono::system_clock::now());

    //
    // Start Kernel
    //
    std::cout << "[    ] [" << tid << "] [" << ev << "] st" << std::endl;
    auto run = m_bo.kernel(m_bo.in_full_graph, m_bo.in_scores, m_bo.inout_graph, m_bo.inout_lookup, m_bo.inout_lookup_filter, m_bo.out_components, 0, m_num_nodes[ev]);
    timing.krnl_started.push_back(std::chrono::system_clock::now());

    //
    // [i + 1] Write event buffer to global memory buffer
    //
    if(ev < m_num_events - 1){
      std::cout << "[    ] [" << tid << "] [" << ev + 1 << "] wr" << std::endl;
      for(unsigned int i = 0; i < size_scores ; i++){
        m_map.in_full_graph[i] = m_ev_full_graph[ev + 1][i];
        m_map.in_scores[i] = m_ev_score[ev + 1][i];
      }
      timing.in_written.push_back(std::chrono::system_clock::now());
    }

    //
    // [i - 1] Read event buffer from global memory buffer
    //
    if(ev > 0){
      std::cout << "[    ] [" << tid << "] [" << ev - 1 << "] re" << std::endl;
      for(unsigned int i = 0; i < size_components ; i++)
        m_ev_components[ev - 1][i] = m_map.out_components[i];
      timing.out_written.push_back(std::chrono::system_clock::now());
    }

    //
    // Wait for Kernel
    //
    std::cout << "[    ] [" << tid << "] [" << ev << "] wa" << std::endl;
    run.wait();
    timing.krnl_ended.push_back(std::chrono::system_clock::now());

    //
    // Synchronize device global memory to output buffer
    //
    std::cout << "[    ] [" << tid << "] [" << ev << "] sy-r" << std::endl;
    m_bo.out_components.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    timing.out_synced.push_back(std::chrono::system_clock::now());

  }
  //
  // [max] Read event buffer from global memory buffer
  //
  std::cout << "[    ] [" << tid << "] [max] re" << std::endl;
  for(unsigned int i = 0; i < size_components ; i++)
    m_ev_components[m_num_events - 1][i] = m_map.out_components[i];
  timing.out_written.push_back(std::chrono::system_clock::now());
    

  timing.final_end = std::chrono::system_clock::now();
}

// execute threads
void exe_threads(xrt::device &m_device, 
                xrt::kernel &m_kernel, 
                thread_timing &m_thread_time, 
                u_int32_t m_num_events, 
                u_int32_t m_num_threads, 
                std::vector<unsigned int*> m_ev_full_graph, 
                std::vector<float*> m_ev_score, 
                std::vector<unsigned int*> m_ev_components, 
                std::vector<unsigned int> m_ev_nodes){

  // std::cout << "[THRD] Allocate kernel parent buffers" << std::endl;
  // std::cout << "[    ] 0" << std::endl; xrt::bo parent_full_graph = xrt::bo(m_device, size_full_graph_byte * m_num_threads, m_kernel.group_id(0));
  // std::cout << "[    ] 1" << std::endl; xrt::bo parent_scores = xrt::bo(m_device, size_scores_byte * m_num_threads, m_kernel.group_id(5)); // 5 + offset));
  // std::cout << "[    ] 2" << std::endl; xrt::bo parent_graph = xrt::bo(m_device, size_graph_byte * m_num_threads, m_kernel.group_id(10)); // 10 + offset));
  // std::cout << "[    ] 3" << std::endl; xrt::bo parent_lookup = xrt::bo(m_device, size_lookup_byte * m_num_threads, m_kernel.group_id(15)); // 15 + offset));
  // std::cout << "[    ] 4" << std::endl; xrt::bo parent_lookup_filter = xrt::bo(m_device, size_lookup_filter_byte * m_num_threads, m_kernel.group_id(20)); // 20 + offset));
  // std::cout << "[    ] 5" << std::endl; xrt::bo parent_components = xrt::bo(m_device, size_components_byte * m_num_threads, m_kernel.group_id(25)); // 25 + offset));

  std::cout << "[THRD] Allocate kernel buffers" << std::endl;
  std::vector<u_int32_t> ev_numbers;
  std::vector<kernel_buffers> bo;
  std::vector<kernel_maps> maps;
  for(u_int32_t i = 0; i < m_num_threads ; i++){
    std::cout << "[    ] " << i << "0" << std::endl; ev_numbers.push_back(0);
    std::cout << "[    ] " << i << "1" << std::endl; bo.push_back(kernel_buffers(m_device, m_kernel, i));
    std::cout << "[    ] " << i << "2" << std::endl; maps.push_back(kernel_maps(bo[i]));
    std::cout << "[    ] " << i << "3" << std::endl; m_thread_time.krnls.push_back(kernel_timing());
    std::cout << "[    ] " << i << "4" << std::endl;
  }
  
  std::cout << "[THRD] Create event queues for each kernel" << std::endl;
  std::vector<std::vector<unsigned int*>> thread_ev_full_graph(m_num_threads);
  std::vector<std::vector<float*>> thread_ev_scores(m_num_threads);
  std::vector<std::vector<unsigned int*>> thread_ev_components(m_num_threads);
  std::vector<std::vector<unsigned int>> thread_num_nodes(m_num_threads);
  u_int32_t residue = m_num_events % m_num_threads;
  u_int32_t ev = 0;
  for(u_int32_t k = 0; k < m_num_events / m_num_threads ; k++)
    for(u_int32_t i = 0; i < m_num_threads ; i++){
      thread_ev_full_graph[i].push_back(m_ev_full_graph[ev]);
      thread_ev_scores[i].push_back(m_ev_score[ev]);
      thread_ev_components[i].push_back(m_ev_components[ev]);
      thread_num_nodes[i].push_back(m_ev_nodes[ev]);
      ev_numbers[i]++;
      ev++;
    }
  for(u_int32_t i = 0; i < residue ; i++){
    thread_ev_full_graph[i].push_back(m_ev_full_graph[ev]);
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
  if(ooo)
    for(u_int32_t i = 0; i < m_num_threads ; i++)
      t[i] = std::thread(exe_kernel_ooo, i, std::ref(bo[i]), std::ref(maps[i]), std::ref(m_thread_time.krnls[i]), ev_numbers[i], thread_ev_full_graph[i], thread_ev_scores[i], thread_ev_components[i], thread_num_nodes[i]);
  else
    for(u_int32_t i = 0; i < m_num_threads ; i++)
      t[i] = std::thread(exe_kernel, i, std::ref(bo), std::ref(maps[i]), std::ref(m_thread_time.krnls[i]), ev_numbers[i], thread_ev_full_graph[i], thread_ev_scores[i], thread_ev_components[i], thread_num_nodes[i]);

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
  auto krnl = xrt::kernel(targetDevice, ConfigID, "CCL", xrt::kernel::cu_access_mode::exclusive);

  //============================================
  //
  // Define Kernel Function to be executed on Device
  //
  //============================================
  print_global_prameters();

  //============================================
  //
  // Allocate Host Memory for each event
  //
  //============================================
  std::vector<unsigned int*> ev_in_full_graph;
  std::vector<float*> ev_in_scores;
  std::vector<unsigned int*> ev_out_components;
  std::vector<unsigned int> ev_num_edges;
  std::vector<unsigned int> ev_num_nodes;
  std::vector<std::vector<unsigned int>> ev_ref_labels;

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
    std::vector<unsigned int> edge_from;
    std::vector<unsigned int> edge_to;
    std::vector<unsigned int> ref_labels;
    std::vector<float> scores;
    // create an input file stream from the csv-file
    std::ifstream csv_file(csv_file_name);
    if(not csv_file.is_open()) throw std::runtime_error("Could not open CSV-file");

    // read and control the headers
    if(csv_file.good()){
      std::cout << "[    ] [" << ev << "] CSV-file opened ";
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
          ref_labels.push_back(std::stoi(word));
        }
      }
    }  
    csv_file.close();
  
    //
    // Add structures for this event
    //
    ev_ref_labels.push_back(ref_labels);
    ev_in_full_graph.push_back(new unsigned int[size_full_graph]);
    std::fill(ev_in_full_graph[ev], ev_in_full_graph[ev] + size_full_graph, 0);
    ev_in_scores.push_back(new float[size_scores]);
    std::fill(ev_in_scores[ev], ev_in_scores[ev] + size_scores, 0.0);
    ev_out_components.push_back(new unsigned int[size_components]);
    std::fill(ev_out_components[ev], ev_out_components[ev] + size_components, 0);

    //
    // Fill event buffers with data from CSV
    //
    std::cout << "; CSV-data -> event buffers";
    unsigned int score_missmatch = 0;
    for(unsigned int i = 0; i < ev_num_edges[ev] ; i++){
      // if size of table is exceeded write a warning
      if(ev_in_full_graph[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES] < MAX_FULL_GRAPH_EDGES - 1){
        // check if node is already present in row, to filter out duplicate edges
        bool new_node = true;
        for(unsigned int j = 0 ; j < ev_in_full_graph[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES] ; j++)
          if(ev_in_full_graph[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES + 1 + j] == edge_to[i]){
            new_node = false;
            if(missmatch_found(ev_in_scores[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES + j], scores[i], 0.5)){
              score_missmatch++;
              ev_in_scores[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES + j] = 1.0;
            }
          }
        if(new_node){
          // fill tables with connections and scores of the edges and keep the number of connections for each node up to date
          ev_in_full_graph[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES + ev_in_full_graph[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES] + 1] = edge_to[i];
          ev_in_scores[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES + ev_in_full_graph[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES]] = scores[i];
          ev_in_full_graph[ev][edge_from[i] * MAX_FULL_GRAPH_EDGES]++;
        }
      }
      else
          std::cout << "[WARNING] Full graph data structure is exceeded!\n[       ] Row " << edge_from[i] << " is already full and can not take in node " << edge_to[i] << " anymore" << std::endl;

      // if size of table is exceeded write a warning
      if(ev_in_full_graph[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES] < MAX_FULL_GRAPH_EDGES - 1){
        // check if node is already present in row, to filter out duplicate edges
        bool new_node = true;
        for(unsigned int j = 0 ; j < ev_in_full_graph[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES] ; j++)
          if(ev_in_full_graph[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES + 1 + j] == edge_from[i]){
            new_node = false;
            if(missmatch_found(ev_in_scores[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES + j], scores[i], 0.5)){
              score_missmatch++;
              ev_in_scores[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES + j] = 1.0;
            }
          }
        if(new_node){
          // fill tables with connections and scores of the edges and keep the number of connections for each node up to date
          ev_in_full_graph[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES + ev_in_full_graph[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES] + 1] = edge_from[i];
          ev_in_scores[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES + ev_in_full_graph[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES]] = scores[i];
          ev_in_full_graph[ev][edge_to[i] * MAX_FULL_GRAPH_EDGES]++;
        }
      }
      else
          std::cout << "[WARNING] Full graph data structure is exceeded!\n[       ] Row " << edge_to[i] << " is already full and can not take in node " << edge_from[i] << " anymore" << std::endl;
    }
    std::cout << "; score missmatches: " << score_missmatch << "; done" << std::endl;
  }

  //============================================
  //
  // Run threads which run kernels
  //
  //============================================
  thread_timing thread_time;
  std::cout << "[INFO] Start exe_thread()" << std::endl;
  exe_threads(targetDevice, krnl, thread_time, num_events, num_threads, ev_in_full_graph, ev_in_scores, ev_out_components, ev_num_nodes);

  //============================================
  //
  // Write time stemps / durations
  //
  //============================================
  print_thread_time(thread_time);

  //============================================
  //
  // Validate results
  //
  //============================================
  {
    std::cout << "[INFO] Validate results" << std::endl;
    bool correct = true;
    std::vector<bool> k_correct;
    unsigned int num_errors = 0;
    std::vector<unsigned int> k_errors;

    for(unsigned int ev = 0; ev < num_events ; ev++){
      k_correct.push_back(true);
      k_errors.push_back(0);

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
              correct = false;
              k_errors[ev]++;
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
              correct = false;
              k_errors[ev]++;
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
            correct = false;
            k_errors[ev]++;
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
        std::cout << "[    ] [" << ev << "] " << k_errors[ev] << " found mismatches" << std::endl;
      }
    }

    if(correct){
      std::cout << "[    ]\n[INFO] TEST PASSED\n[    ]" << std::endl;
      std::cout << "[INFO] All results match the expected results" << std::endl;
    } else {
      std::cout << "[       ]\n[WARNING] TEST FAILED\n[       ]" << std::endl;
      std::cout << "[WARNING] " << num_errors << " total mismatches\n[       ]" << std::endl;
    }
  }

  //============================================
  //
  // Free allocated memory
  //
  //============================================
  for(unsigned int ev = 0; ev < num_events ; ev++){
    delete ev_in_full_graph[ev];
    delete ev_in_scores[ev];
    delete ev_out_components[ev];
  }

  return 0;
}

