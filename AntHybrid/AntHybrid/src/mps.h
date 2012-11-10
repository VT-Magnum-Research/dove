//
//  hybrid.h
//  AntHybrid
//
//  Created by Hamilton Turner on 11/9/12.
//  Copyright (c) 2012 Virginia Tech. All rights reserved.
//

#ifndef __AntHybrid__hybrid__
#define __AntHybrid__hybrid__

#include <iostream>
#include <cstdlib>
#include <map>
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include "util.h"

#include "ants.h"
#include "data.h"
#include "MpSchedule.h"


/// Multiprocessor scheduling using the libaco implemenation.
//   At the basic level, this does nothing more than allocate
//   tasks to cores. If you pass all precedence numebrs as 1, it
//   will schedule tasks with no precedence relationships.
//   Other details can be removed via similar means
// Inherent assumptions:
//  - Every task must be scheduled once
//  - A core can only run one task simultaneously
//  - No preemption
//  - No task duplication (currently)
class MpsProblem : public OptimizationProblem {
private:
  
  /// ============= Core Abstraction =============
  
  // Each task must be scheduled once, and tasks should be in
  // ascending priority order before the aco begins execution
  std::vector<Task>* tasks_;
  
  // libaco needs a fixed ordering
  std::vector<Core>* cores_;
  
  /// ============= Removing Assumptions =========
  
  // TODO ignoring this for now...
  // Execution times for running a task on a processor
  //
  // (*execution_times_)[v][w] gives you the time to run task v on processor w
  // Matrix<unsigned int> * execution_times_;
  
  // TODO ignoring this for now...
  // Routing times for passing messages between processors
  //
  // (*routing_times_)[v][w] gives you the time to pass a message from v to w
  // Matrix<unsigned int> * routing_times_;
  
  /// ============= Implementation Variables =====
  
  // Helper method for the verification
  // Note: The attribute will likely only work on Mac systems. Feel free
  // to remove it if it's causing trouble
  bool _verr(const char *fmt, ...)
    __attribute__((format (printf, 2, 3)));
  
  #define verr(fmt,...) _verr(fmt"\n", ##__VA_ARGS__)
  
  // Initialized with the number of tasks in this problem
  unsigned int task_size_;
  
  std::string debug_vertex(unsigned int vertex);
  
  inline unsigned int get_vertex_for(unsigned int core, unsigned int task) {
    return core * task_size_ + task;
  }
  
  inline void get_task_and_core_from_vertex(unsigned int vertex_id, unsigned int & task, unsigned int & core) {
    task = vertex_id % task_size_;
    core = vertex_id / task_size_;
  }
  
public:
  void print_schedule(MpSchedule schedule);
  void print_tour(std::vector<unsigned int> tour);
  bool verify_schedule_passes_constraints(MpSchedule schedule);

  MpSchedule convert_tour_to_schedule(std::vector<unsigned int> tour);
  
  MpsProblem(std::vector<Task>* tasks, std::vector<Core>* cores);
  ~MpsProblem();
  unsigned int get_max_tour_size();
  unsigned int number_of_vertices();
  
  // Vertex ids must be 0...number_of_vertices() - 1
  std::map<unsigned int,double> get_feasible_start_vertices();
  
  // Vertex ids must be 0...number_of_vertices() - 1
  std::map<unsigned int,double> get_feasible_neighbours(unsigned int vertex);
  double eval_tour(const std::vector<unsigned int> &tour);
  double pheromone_update(unsigned int v, double tour_length);
  void added_vertex_to_tour(unsigned int vertex);
  bool is_tour_complete(const std::vector<unsigned int> &tour);
  std::vector<unsigned int> apply_local_search(const std::vector<unsigned int> &tour);
  void cleanup();
};

#endif /* defined(__AntHybrid__hybrid__) */
