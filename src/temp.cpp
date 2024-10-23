
// execute kernel
  // work on 1 instance of every kernel-memory-block (full-graph, io_graph, lookup, components, num_nodes, etc)

  // write inputs [thread, loop]
  // sync inputs
  // start kernel
  // wait for completion
  // sync output
  // write back and store result [thread, loop]
  // reset in / outputs [maybe reset before waiting?]

  // return (or write reference adress (see EEC))
  // std::chrono::_V2::system_clock::time_point[]
    // (array or vector of all time-steps
    // (initial start, write, sync, start, wait, sync, write/store, reset, final end)) (build struct)
  // multi-dimentional array for each iteration of kernel-loop

// execute threads
  // load all event-data in different Host-memory-blocks
  // have 1 instance of each kernel-memory-block per thread
    // (= 1 for sequential)

  // if single
    // execute kernels sequential
  // if chain           // num_threads = 1 == single ???
    // execute N kernel-threads in parallel
  // return all time-stamps of all threads
    // + gloabal start / finish / intermediate thread-times

// calc all durations of returned time-stamps
// no loop in thread-fun -> only loop inside kernel-fun
  // (loop in t-fun == multi-thread)
// loop inside kernel function (sequential exe)
  // [number loops kernel]
// loop inside single-thread ((no) loop inside kernel-fun)
  //[number loops thread / number loops kernel]
// multi-thread without loops in kernel-fun [threads]
// multi-thread with loops in kernel-fun
  // [number threads / number loops kernel]

// + kernel-fun + return time (1 iter) -> 1 kernel-mem / 1 host-mem / 1 time-struct
// + kernel-fun + return time (multi iter) -> 1 kernel-mem / multi host-mem / multi time-struct
// thread-fun (1 thread)
  // -> 1 kernel-mem / multi host-mem / multi time-struct
// thread-fun (1 thread multi iter)
  // -> 1 kernel-mem / multi host-mem / multi time-struct
// thread-fun (multi thread 1 iter)
  // -> multi kernel-mem / multi host-mem / multi time-struct
// thread-fun (multi thread multi iter)
  // -> multi kernel-mem / multi host-mem / multi time-struct

