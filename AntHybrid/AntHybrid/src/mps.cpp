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

bool debug = false;

MpsProblem::MpsProblem(std::vector< Task>* tasks, std::vector<Core>* cores) {
  if (debug)
    std::cout << "MpsProblem::MpsProblem" << std::endl;
  tasks_ = tasks;
  cores_ = cores;
  task_size_ = (int) tasks->size();
  
  cleanup();
  
  // Sanity Checks
  std::vector<Task>::iterator it;
  unsigned int predecessor_level = 0;
  for ( it=tasks_->begin() ; it < tasks_->end(); it++ ) {
    if (predecessor_level > (*it).pred_level_)
      throw "Task list is not sorted by priority";
    
    predecessor_level = (*it).pred_level_;
  }
}

MpsProblem::~MpsProblem(){
  if (debug)
    std::cout << "MpsProblem::~MpsProblem" << std::endl;
}

unsigned int MpsProblem::get_max_tour_size(){
  unsigned int result = (int) (*tasks_).size();
  if (debug)
    std::cout << "MpsProblem::get_max_tour_size: " << result << std::endl;
  return result;
}

unsigned int MpsProblem::number_of_vertices(){
  unsigned int result = (int) ((*tasks_).size() * (*cores_).size());
  if (debug)
    std::cout << "MpsProblem::number_of_vertices: " << result << std::endl;
  
  return result;
}

std::map<unsigned int,double> MpsProblem::get_feasible_start_vertices(){
  if (debug)
    std::cout << "MpsProblem::get_feasible_start_vertices" << std::endl;
  
  // Start on any processor, but you must execute the first task first
  std::map<unsigned int,double> vertices;
  unsigned int start = get_vertex_for(Util::random_number( (int) cores_->size()), 0);
  vertices[start] = 1.0;
  return vertices;
}

std::map<unsigned int,double> MpsProblem::get_feasible_neighbours(unsigned int vertex){
  if (debug)
    std::cout << "MpsProblem::get_feasible_neighbors: " << debug_vertex(vertex) << std::endl;
  
  // TODO update this! We don't have to impose this limit here, it is imposed in the
  // evaluation. We are limited the search of the solution space needlessly.
  // Instead, we should simply say a feasible neighbor any task that has not been scheduled,
  // on any processor

  // TODO this constraining of the solution space may actually be a minor example of
  // Constraint programming e.g. only allow scheduling in precedence ordering as this might
  // be leading to better solutions naturally
  
  // Limit feasible neighbors to tasks of allowed priorities that have
  // not been scheduled
  unsigned int task, core;
  get_task_and_core_from_vertex(vertex, task, core);
  unsigned int pred_level = (*tasks_)[task].pred_level_;
  std::map<unsigned int, double> neighbors;
  for (unsigned int i = 0; i < tasks_->size(); i++) {
    Task cur = (*tasks_)[i];
    
    // Unscheduled tasks at this priority?
    if (cur.pred_level_ == pred_level && !cur.scheduled_)
      for (unsigned int core = 0; core < cores_->size(); core++)
        neighbors[get_vertex_for(core, i)] = 1.0;
    
    if (cur.pred_level_ > pred_level) {
      if (neighbors.size() == 0) {
        pred_level += 1;
        for (unsigned int core = 0; core < cores_->size(); core++)
          neighbors[get_vertex_for(core, i)] = 1.0;
      } else
        break;
    }
  }
  
  if (debug)
  {
    std::cout << "          ::";
    std::map<unsigned int,double>::iterator it;
    for (it = neighbors.begin(); it != neighbors.end(); it++)
      std::cout << debug_vertex(it->first) << "*" << it->second << " ";
    
    std::cout << std::endl;
  }
  
  // Sanity Check
  if (neighbors.size() == 0)
    throw "What happened here!?";
  
  return neighbors;
}

// NOTE: This is called as so:
//   MpsProblem::eval_tour
//   MpsProblem::cleanup
//   MpsProblem::apply_local_search
//   MpsProblem::eval_tour
// So you must evaluate a tour based upon this vector and iteration-independent
// variables only
double MpsProblem::eval_tour(const std::vector<unsigned int> &tour){
  MpSchedule schedule = convert_tour_to_schedule(tour);
  
  double completion_time = schedule.get_completion_time();
  if (debug)
    std::cout << "MpsProblem::eval_tour: " << completion_time << std::endl;
  if (completion_time == 15)
    convert_tour_to_schedule(tour);

  return completion_time;
}

MpSchedule MpsProblem::convert_tour_to_schedule(std::vector<unsigned int> tour) {
  
  // Flatten the scheduling order into a real schedule
  MpSchedule schedule;
  std::vector<unsigned int>::const_iterator it = tour.begin();
  unsigned int task_i, core_i;
  Task* task;

  for (it = tour.begin(); it < tour.end(); it++) {
    get_task_and_core_from_vertex(*it, task_i, core_i);
    task = &(*tasks_)[task_i];
    
    ScheduleItem si;
    si.task_ = task;
    si.start_ = schedule.get_earliest_start_time(task, core_i);;

    // Pretending that all processors are homogeneous
    si.end_ = si.start_ + task->execution_time_;
    schedule.add_task(core_i, si);
  }
    
  return schedule;
}

double MpsProblem::pheromone_update(unsigned int v, double tour_length){
  if (debug)
    std::cout << "MpsProblem::pheromone_update: " << v << ", " << tour_length << std::endl;
  
  return 1.0 / tour_length;
}

void MpsProblem::added_vertex_to_tour(unsigned int vertex){
  if (debug)
    std::cout << "MpsProblem::added_vertex_to_tour: " << debug_vertex(vertex) << std::endl;
  
  unsigned int task, core;
  get_task_and_core_from_vertex(vertex, task, core);
  
  (*tasks_)[task].scheduled_ = true;
}

bool MpsProblem::_verr(const char *fmt, ...) {
  va_list arg;
  va_start(arg, fmt);
  std::vprintf(fmt, arg);
  va_end(arg);
  return false;
}

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

bool MpsProblem::is_tour_complete(const std::vector<unsigned int> &tour){
  bool complete = (tour.size() == tasks_->size());
  if (debug)
    std::cout << "MpsProblem::is_tour_complete: " << (complete ? "Yes" : "No") << std::endl;
  
  return complete;
}

std::vector<unsigned int> MpsProblem::apply_local_search(const std::vector<unsigned int> &tour){
  if (debug)
    std::cout << "MpsProblem::apply_local_search" << std::endl;
  
  return tour;
}

void MpsProblem::cleanup(){
  if (debug)
    std::cout << "MpsProblem::cleanup" << std::endl;
  
  std::vector<Task>::iterator it;
  for (it = tasks_->begin(); it < tasks_->end(); it++) {
    it->scheduled_ = false;
  }
}

void MpsProblem::print_tour(std::vector<unsigned int> tour) {
  unsigned int task, core;
  for(unsigned int i=0;i<tour.size();i++) {
    get_task_and_core_from_vertex(tour[i], task, core);
    std::cout << "(C" << core << ",T" << task << ")" << ((i == (tour.size()-1)) ? "" : ",");
  }
}

std::string MpsProblem::debug_vertex(unsigned int vertex) {
  unsigned int task, core;
  get_task_and_core_from_vertex(vertex, task, core);
  
  std::ostringstream oss;
  oss << "(C" << core << ",T" << task << ")";
  return oss.str();
}

FileNotFoundException::FileNotFoundException(const char *filepath) {
  filepath_ = filepath;
}

const char *FileNotFoundException::what() const throw() {
  return filepath_;
}

bool precedence_sort(Task a, Task b) {
  return a.pred_level_ < b.pred_level_;
}

std::vector<Task>* Parser::parse_stg(const char *filepath) throw(FileNotFoundException) {
  enum section {
    HEADER,
    GRAPH,
    FOOTER
  };
  
  std::vector<Task>* tasks = NULL;
  //DirectedAdjacencyMatrixGraph* graph = NULL;
  
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
        //graph = new DirectedAdjacencyMatrixGraph(task_count);
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
        //graph->add_edge(pred_id, task_id);
        
        task->pred_level_ = std::max(task->pred_level_,
                                     tasks->at(pred_id).pred_level_ + 1);
      }
      
      task->execution_time_ = task_processing_time;
      std::ostringstream oss;
      oss << "t" << task_id;
      task->identifier_ = oss.str();
      
    } else if (s == FOOTER) {
      std::string line;
      file >> line;
    }
  }

  file.close();
  
  std::sort(tasks->begin(), tasks->end(), precedence_sort);
  tasks->pop_back();
  tasks->erase(tasks->begin());

  return tasks;
}


