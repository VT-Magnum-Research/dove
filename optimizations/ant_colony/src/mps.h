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
#include <exception>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "util.h"

#include "ants.h"
#include "data.h"
//#include "MpSchedule.h"

#include "graph.h"

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
  
  // Should we print the stack and other debug messages as
  // the algorithm runs? 
  const static bool should_debug = false;
  
  // The number of tasks in this problem
  unsigned int task_size_;
  
  // The number of cores in this problem
  unsigned int core_size_;
  
  // The communication cost (in time units) for passing a message between cores
  //
  // routing_costs_[i][j] is the cost of routing a message from i to j and will
  // always equal routing_costs_[j][i]
  SymmetricMatrix<unsigned int>* routing_costs_;
  
  // The time needed to run each task on a core
  // running_times_[i][j] is the time needed to run task i on core j, and
  // running_times_[j][i] is the time needed to run task j on core i
  Matrix<unsigned int>* running_times_;
  
  // Contains all task precedence orderings
  DirectedAcyclicGraph* precedence_graph_;
  
  // Returns the exact order in which tasks should be scheduled by an Ant. More precisely,
  // task_scheduling_order_[current_tour_length_] can be used to get the index of the
  // next task that should be scheduled. No value should be > task_size_
  // Note that internal functions assume this never violates the precedence_graph_ at all
  std::vector<unsigned int>* task_scheduling_order_;
  
  // As the tour is built, this is constructed. It contains the core that
  // each task is mapped to. mapping_[i] = j implies that task i has been
  // mapped to core j. 
  std::vector<unsigned int> mapping_;
  
  // Incremented every time a vertex is added to the ant tour. The tour equals a collection
  // of mappings between task&core, so this is the number of tasks that have been mapped
  int current_tour_length_;
  
  // Used to cache the evaluation of our tour. Avoids having to recalculate the tour_evaluation
  // after a local search is requested, because I know there is no local search being performed
  // yet
  bool local_search_requested_;
  
  // Used to cache the result of the tour
  double tour_evaluation_cache;
  
  AntColony<Ant>* colony_;
  
  // Allows logging. Prints all messages to stderr currently so I can capture algorithm
  // output and implementation debugging separately
  void _log(const char *fmt, ...)
  __attribute__((format (printf, 2, 3)));
  #define log(fmt,...) _log(fmt"\n", ##__VA_ARGS__)
  
  std::string debug_vertex(unsigned int vertex);
  
  inline unsigned int get_vertex_for(unsigned int core, unsigned int task) {
    return core * task_size_ + task;
  }
  
  inline void get_task_and_core_from_vertex(unsigned int vertex_id, unsigned int & task, unsigned int & core) {
    task = vertex_id % task_size_;
    core = vertex_id / task_size_;
  }
  
  
  inline void _debug(const char *fmt, ...) __attribute__((format (printf, 2, 3)))
  {
    if (should_debug) {      
      va_list arg;
      va_start(arg, fmt);
      std::vfprintf(stderr, fmt, arg);
      va_end(arg);
    }

  }
  #define debug(fmt,...) _debug(fmt"\n", ##__VA_ARGS__)
  
public:
  
  void set_ant_colony(AntColony<Ant>* colony);
  
  void print_mapping(std::vector<unsigned int> tour);
  
  // Assumes that all task precedence_levels are contiguous e.g. 1,2,3,4 and not 1,15,23,25,26,40
  // Passed DAG must ensure that there is one starting node with zero execution time on any core,
  // and one finishing node with zero execution time on any core, and that their integer_ids
  // are placed at position zero and position back() in the task_scheduling_order
  MpsProblem(SymmetricMatrix<unsigned int>* routing_costs,
             Matrix<unsigned int>*          run_times,
             DirectedAcyclicGraph*          task_precedence,
             std::vector<unsigned int>*     task_scheduling_order);
  ~MpsProblem();
  unsigned int get_max_tour_size();
  
  // Number of vertices in the construction graph
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

class FileNotFoundException : public std::exception {
private:
  const char *filepath_;
public:
  FileNotFoundException(const char *filepath);
  const char *what() const throw();
};

namespace Parser {
  // Assumptions: There should be one start node, which has a path to all other 'nodes'. This avoids
  // situations with independent chains of priority e.g. 1-->2-->3<--2<--1 (a 2 is valid to run after a 1 completes,
  // which does not match our programming assumptions)
  std::vector<Task>* parse_stg(const char *filepath, DirectedAcyclicGraph* &out_graph) throw(FileNotFoundException);
}


#endif /* defined(__AntHybrid__hybrid__) */
