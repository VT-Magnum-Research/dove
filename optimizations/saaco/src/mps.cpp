//
//  hybrid.cpp
//  AntHybrid
//
//  Created by Hamilton Turner on 11/9/12.
//  Copyright (c) 2012 Virginia Tech. All rights reserved.
//

#include "mps.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <vector>
#include <fstream>




MpsProblem::MpsProblem(SymmetricMatrix<unsigned int>* routing_costs,
                       Matrix<unsigned int>*          run_times,
                       DirectedAcyclicGraph*          task_precedence,
                       std::vector<unsigned int>*     task_scheduling_order) :
                      current_tour_length_(0),
                      local_search_occurred_(false),
                      running_times_(run_times),
                      routing_costs_(routing_costs),
                      precedence_graph_(task_precedence),
                      task_scheduling_order_(task_scheduling_order) {
  debug("MpsProblem::MpsProblem");

  core_size_ = routing_costs->rows();
  task_size_ = (int) task_scheduling_order->size();
  
  std::vector<unsigned int> replace_mapping(task_size_, 0);
  mapping_ = replace_mapping;
}

MpsProblem::~MpsProblem(){
  debug("MpsProblem::~MpsProblem");
}

void MpsProblem::set_ant_colony(AntColony<Ant>* colony) {
  colony_ = colony;
}

unsigned int MpsProblem::get_max_tour_size(){
  debug("MpsProblem::get_max_tour_size: %u", task_size_);
  return task_size_;
}

unsigned int MpsProblem::number_of_vertices(){
  unsigned int result = task_size_ * core_size_;
  debug("MpsProblem::number_of_vertices: %u", result);
  return result;
}

std::map<unsigned int,double> MpsProblem::get_feasible_start_vertices(){
  debug("MpsProblem::get_feasible_start_vertices");

  unsigned int task = task_scheduling_order_->front();

  // Prefer more centralized cores
  std::map<unsigned int,double> vertices;
  for ( int i = 0; i < core_size_; i++) {
    int total_routing_time = 1;
    for (int j = 0; j < core_size_; j++)
      total_routing_time += routing_costs_->operator[](i)[j];
    
    vertices[get_vertex_for(i, task)] = 1.0 / total_routing_time;
    debug("\tAssigning start %s heuristic %f. Routing time is %f. ",
          debug_vertex(get_vertex_for(i, task)).c_str(), 1.0 / total_routing_time, 1.0 * total_routing_time);
  }
  
  return vertices;
}

std::map<unsigned int,double> MpsProblem::get_feasible_neighbours(unsigned int vertex){
  debug("MpsProblem::get_feasible_neighbors: %s", debug_vertex(vertex).c_str());
  
  // Are we scheduling the next to last task? If so, force all ants onto the
  // same core and don't bother with a full calculation
  if (current_tour_length_ == (task_size_ - 1)) {
    std::map<unsigned int,double> neighbors;
    neighbors[get_vertex_for(0, task_size_ - 1)] = 1;
    return neighbors;
  }
  
  // We know what task is being scheduled currently, so let's get it
  unsigned int task = task_scheduling_order_->at(current_tour_length_);
  
  // Find the cores that all predecessors have been placed on
  std::vector<unsigned int> predecessors = precedence_graph_->get_predecessors(task);
  for (int cur = 0; cur < predecessors.size(); cur++)
    predecessors[cur] = mapping_[predecessors[cur]];
  
  // Avoid cores where the total predecessor routing time is higher
  // TODO potentially favor cores where completion time is lower
  std::map<unsigned int,double> neighbors;
  std::vector<unsigned int>::iterator predecessor_iter;
  
  for (int core = 0; core < core_size_; core++) {
    // double running_time = (double) (*running_times_)[task][core];
    double total_routing_time = 1;
    Row<unsigned int> core_routing = (*routing_costs_)[core];
    for (predecessor_iter = predecessors.begin();
         predecessor_iter != predecessors.end();
         predecessor_iter++)
      total_routing_time += (double) core_routing[*predecessor_iter];
    
    unsigned int vertex_id = get_vertex_for(core, task);
    //neighbors[vertex_id] = total_routing_time / running_time;
    neighbors[vertex_id] = 1.0 / total_routing_time;
    debug("\tAssigning %s heuristic %f. Routing time is %f. ",
          debug_vertex(vertex_id).c_str(), neighbors[vertex_id], total_routing_time);
  }
  
  if (neighbors.size() == 0) {
    debug("There were no feasible neighbors");
    log("There were no feasible neighbors");
    throw "There were no feasible neighbors!";
  }
  
  return neighbors;
}

// NOTE: This is called as so:
//   MpsProblem::eval_tour
//   MpsProblem::cleanup
//   MpsProblem::apply_local_search
//   MpsProblem::eval_tour
// So you must evaluate a tour based upon this vector and iteration-independent
// variables only. I have avoided this problem slightly by doing nothing in
// cleanup, and instead doing all of my cleaning when libaco first calls
// get_feasible_start_vertices()
double MpsProblem::eval_tour(const std::vector<unsigned int> &tour){
  
  debug("MpsProblem::eval_tour");
  
  std::vector<unsigned int> current_core_completion_time(core_size_, 0);
  std::vector<unsigned int> current_min_task_starting_time(task_size_, 0);
  
  // Start at one to avoid first dummy task, and end at size-1 for the end dummy
  // node
  for (int cur = 1; cur < (tour.size() - 1); cur++) {
    unsigned int task = task_scheduling_order_->at(cur);
    unsigned int core = mapping_[task];
    get_task_and_core_from_vertex(tour[cur], task, core);
    
    // Have any predecessors delayed me?
    unsigned int min_start_time = current_min_task_starting_time[task];

    // Is my core already busy?
    min_start_time = std::max(min_start_time, current_core_completion_time[core]);
    
    // What's our running time?
    unsigned int run_time = (*running_times_)[task][core];

    unsigned int finish_time = min_start_time + run_time;
    current_core_completion_time[core] = finish_time;
  
    // Delay all of our successors, except for the final dummy node
    // Node 0 will never be a successor either
    Row<unsigned int> successors = precedence_graph_->get_successor_row(task);
    for (int cur=1; cur < successors.size() - 1; cur++)
    {
      // Are they actually a successor?
      if (successors[cur] == 0 )
        continue;
      
      // What is the time to route from my core to theirs?
      unsigned int their_core = mapping_[cur];
      unsigned int routing_time = (*routing_costs_)[core][their_core];
      unsigned int earliest_start_time = finish_time + routing_time;
      
      current_min_task_starting_time[cur] = std::max(current_min_task_starting_time[cur],
                                                   earliest_start_time);
    }
    
  }

  // The maximum should be identical whether you consider tasks or
  // processors, so let's use the smaller
  unsigned int completion_time = 0;
  for (int cur = 0; cur < current_core_completion_time.size(); cur++)
    completion_time = std::max(current_core_completion_time[cur], completion_time);
  
  debug("MpsProblem::eval_tour: %u", completion_time);
  return completion_time;
}

// This is the destination vertex. This method allows us to add a lot of pheremone to
// a vertex even if it has heuristically undesirable properties in order to train the
// ants that there is something better over the hill
double MpsProblem::pheromone_update(unsigned int vertex, double tour_length){
  
  /*unsigned int task, core;
  get_task_and_core_from_vertex(vertex, task, core);
  
  // Locate the running time our heuristic (earliest completion time) considers
  // best for this task
  unsigned int best_running_time = -1;
  for (int cur_core = 0; cur_core < core_size_; cur_core++)
    best_running_time = std::min((*running_times_)[task][cur_core], best_running_time);
  
  // Can occur if the task running time is set to zero on all cores
  if (best_running_time == 0)
    return 1.0 / tour_length;
  
  unsigned int my_running_time = (*running_times_)[task][core];

  // Never less than one
  double percent_of_best_guess_of_heuristic = (double) my_running_time / (double) best_running_time;
  
  // Bounded reward, we don't want to get too carried away and have ants selecting awful nodes
  // because the pheremone trail was really heavy.
  // In the future this should potentially include the processor/task heterogeneity in determining
  // the bound
  double pheromone = std::min(percent_of_best_guess_of_heuristic, 2.0) / tour_length;

  return pheromone;
   */

  // Pheremone deposit is always between zero and one, and is linearly scaled depending upon
  // how close this solution was to the best solution
  //
  /*double best = colony_->get_best_tour_length();
  best = std::min(best, colony_->get_best_tour_length_in_iteration());
  
  double percent_improvement = (best - tour_length) / best;
  if (percent_improvement <= 0)
    return 0;
  
  return std::pow(percent_improvement * 100, 1.5);
   */
  debug("MpsProblem::pheromone_update: %s, %f", debug_vertex(vertex).c_str(), 3.5);
  return 3.5;
}

void MpsProblem::added_vertex_to_tour(unsigned int vertex){
  debug("MpsProblem::added_vertex_to_tour: %s", debug_vertex(vertex).c_str());
  
  unsigned int task, core;
  get_task_and_core_from_vertex(vertex, task, core);
  mapping_[task] = core;
  
  current_tour_length_++;
}

bool MpsProblem::is_tour_complete(const std::vector<unsigned int> &tour){
  bool complete = (task_size_ == current_tour_length_);
  debug("MpsProblem::is_tour_complete: %s", (complete ? "Yes" : "No"));
  return complete;
}

std::vector<unsigned int> MpsProblem::apply_local_search(const std::vector<unsigned int> &old_tour){
  debug("MpsProblem::apply_local_search");
  
  local_search_occurred_ = true;

  std::vector<unsigned int> tour(old_tour);
  double time = eval_tour(tour);
  unsigned int swaps = 10;
  while (swaps-- != 0) {
    // Randomly choose a vertex to modify
    unsigned int vertex_pos = (int) rand() % tour.size();
    unsigned int vertex = tour[vertex_pos];
    unsigned int task,core;
    get_task_and_core_from_vertex(vertex, task, core);

    // Randomly choose a new core for the task
    unsigned int new_core = core;
    while (new_core == core)
      new_core = rand() % core_size_;
    
    // Build the new vertex
    unsigned int new_vertex = get_vertex_for(new_core, task);
    tour[vertex_pos] = new_vertex;
    mapping_[task] = new_core;
    
    // Evaluate the new tour, undo if we made it worse
    double new_time = eval_tour(tour);
    //std::cout << "Time is " << time << " and new time is " << new_time << std::endl;
    if (new_time > time) {
      tour[vertex_pos] = vertex;
      mapping_[task] = core;
    } else
      time = new_time;
  }
  
  return tour;
}

void MpsProblem::cleanup(){
  debug("MpsProblem::cleanup");
  
  current_tour_length_ = 0;
  
  if (local_search_occurred_) {
    local_search_occurred_ = false;
    mapping_.clear();
  }
}

void MpsProblem::print_mapping(std::vector<unsigned int> tour, std::ostream &out) {
  out << "# " << std::endl;
  out << "# Full Mapping" << std::endl;
  out << "# Format is (C<<core>>,T<<task>>)|(C<<core>>,T<<task>>)" << std::endl;
  unsigned int task, core;
  for(unsigned int i=0;i<tour.size();i++) {
    get_task_and_core_from_vertex(tour[i], task, core);
    out << "(C" << core << ",T" << task << ")" << ((i == (tour.size()-1)) ? "" : "|");
  }
  out << std::endl;
}



std::string MpsProblem::debug_vertex(unsigned int vertex) {
  unsigned int task, core;
  get_task_and_core_from_vertex(vertex, task, core);
  
  std::ostringstream oss;
  oss << "(C" << core << ",T" << task << "):" << vertex;
  return oss.str();
}

FileNotFoundException::FileNotFoundException(const char *filepath) {
  filepath_ = filepath;
}

const char *FileNotFoundException::what() const throw() {
  return filepath_;
}

std::vector<Task>* Parser::parse_stg(const char *filepath, DirectedAcyclicGraph* &graph)
                    throw(FileNotFoundException) {
  enum section {
    HEADER,
    GRAPH,
    FOOTER
  };
  
  std::vector<Task>* tasks = NULL;
  
  section s = HEADER;
  std::ifstream file(filepath);
  
  if(!file)
    throw FileNotFoundException(filepath);

  unsigned int task_count = 0;
  while(file.good()) {
    if(s == HEADER) {
      file >> task_count;
      if (task_count !=0) {
        // STG format always includes two dummy nodes
        task_count = task_count + 2;
        graph = new DirectedAcyclicGraph(task_count);
        tasks = new std::vector<Task>(task_count);
        s = GRAPH;
      }
    } else if(s == GRAPH) {
      if (task_count-- == 0) {
        s = FOOTER;
        continue;
      }
      unsigned int task_id, task_processing_time, predecessor_count;
      file >> task_id;
      file >> task_processing_time;
      file >> predecessor_count;
      
      Task* task = &(tasks->at(task_id));
      if (task_id == 0)
        task->pred_level_ = 1;
      
      for (int i = 0; i < predecessor_count; i++) {
        unsigned int pred_id;
        file >> pred_id;
        graph->add_edge(pred_id, task_id);
        
        task->pred_level_ = std::max(task->pred_level_,
                                     tasks->at(pred_id).pred_level_ + 1);
      }
      
      task->execution_time_ = task_processing_time;
      std::ostringstream oss;
      oss << "t" << task_id;
      task->identifier_ = oss.str();
      task->int_identifier_ = task_id;
      
    } else if (s == FOOTER) {
      std::string line;
      file >> line;
    }
  }

  file.close();
  
  // I want to keep my start and end tasks, so that I know the DAG begins at
  // (*graph)[0][0] and I can request successors from there
  //std::sort(tasks->begin(), tasks->end(), precedence_sort);
  //tasks->pop_back();
  //tasks->erase(tasks->begin());

  return tasks;
}

void MpsProblem::_log(const char *fmt, ...) {
  va_list arg;
  va_start(arg, fmt);
  std::vfprintf(stderr, fmt, arg);
  va_end(arg);
}

// ============== Older (unused) functions

/*
// For my challenge, this is largely:
// 1 - verify precedence is maintained on each core
// 2 - verify precedence is maintained throughout the system
// 3 - verify communication lags are correct
bool MpsProblem::verify_schedule_passes_constraints(MpSchedule schedule) {
  
  // TODO-->Verify 1-1-1 mapping of processor, task, time slot e.g. no dual-processing of tasks
  
  // Next steps: make cores and tasks have IDs when I load them in
  //             train MpsProblem to print the schedule (or the schedule to print itself?)
  
  // Y - Verify A.end >= A.start for all tasks
  // Y - Verify A before B in vector implies A.end <= B.start
  // Y - Verify the precedence of tasks is maintained on each processor independently
  for (int i = 0; i < cores_->size(); i++)
  {
    std::vector<ScheduleItem> core_sched = *schedule.get_schedule_for_core(i);
    std::vector<ScheduleItem>::iterator it;
    ScheduleItem* prior = NULL;
    for (it = core_sched.begin(); it != core_sched.end(); it++)
    {
      ScheduleItem* cur = &(*it);
      if (cur->end_ < cur->start_)
        return verr("End time before start time: %s", cur->get_cstr());
      
      if (prior == NULL) {
        prior = cur;
        continue;
      }
      
      if (cur->start_ < prior->end_)
        return verr("Two tasks overlapping: %s & %s",
                    prior->get_cstr(), cur->get_cstr());
      
      if (cur->pred_level() < prior->pred_level())
        return verr("Tasks not following priority precedence: %s & %s",
                    prior->get_cstr(), cur->get_cstr());
    }
  }
  
  //
  //
  //  // Check priority precedence
  //  unsigned int priority_current = 0;
  //  unsigned int priority_finish_time = 0;
  //  unsigned int task, core;
  //  for(unsigned int i=0;i<tour.size();i++) {
  //    get_task_and_core_from_vertex(tour[i], task, core);
  //    Task t = (*tasks_)[task];
  //    //t.priority_
  //  }
  
  return true;
}
 */


