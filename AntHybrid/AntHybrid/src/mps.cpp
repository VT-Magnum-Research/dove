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

bool debug = false;

MpsProblem::MpsProblem(std::vector<Task>* tasks, std::vector<Core>* cores) {
  if (debug)
    std::cout << "MpsProblem::MpsProblem" << std::endl;
  tasks_ = tasks;
  cores_ = cores;
  task_size_ = (int) tasks->size();
  
  cleanup();
  
  // Sanity Checks
  std::vector<Task>::iterator it;
  unsigned int priority = 0;
  for ( it=tasks_->begin() ; it < tasks_->end(); it++ ) {
    if (priority > (*it).priority_)
      throw "Task list is not sorted by priority";
    
    priority = (*it).priority_;
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
  
  // Limit feasible neighbors to tasks of allowed priorities that have
  // not been scheduled
  unsigned int task, core;
  get_task_and_core_from_vertex(vertex, task, core);
  unsigned int priority = (*tasks_)[task].priority_;
  std::map<unsigned int, double> neighbors;
  for (unsigned int i = 0; i < tasks_->size(); i++) {
    Task cur = (*tasks_)[i];
    
    // Unscheduled tasks at this priority?
    if (cur.priority_ == priority && !cur.scheduled_)
      for (unsigned int core = 0; core < cores_->size(); core++)
        neighbors[get_vertex_for(core, i)] = 1.0;
    
    if (cur.priority_ > priority) {
      if (neighbors.size() == 0) {
        priority += 1;
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
  
  // Ensure cores are clean
  for (int i = 0; i < cores_->size(); i++) {
    cores_->at(i).current_task_priority = 0;
    cores_->at(i).current_task_completion_time = 0;
  }
  
  // Flatten the scheduling order into a real schedule
  std::vector<unsigned int>::const_iterator it = tour.begin();
  unsigned int task_i, core_i;
  Task task;
  Core* core;
  for (it = tour.begin(); it < tour.end(); it++) {
    get_task_and_core_from_vertex(*it, task_i, core_i);
    task = (*tasks_)[task_i];
    core = &(cores_->at(core_i));
    
    // There are no predecessors, but there may have been some prior level 1
    // stuff run on this core if we had a small number of cores relative to
    // the number of priority 1 tasks
    if (task.priority_ == 1) {
      core->current_task_priority = task.priority_;
      core->current_task_completion_time += task.execution_time_;
    } else {
      
      // Minimum starting time on this processor
      unsigned int min_starting_time = core->current_task_completion_time;
      
      // Do we need to idle to maintain precedence?
      unsigned int previous_priority_level = task.priority_ - 1;
      for (int i = 0; i < cores_->size(); i++)
        if (cores_->at(i).current_task_priority == previous_priority_level)
          min_starting_time = std::max(min_starting_time, cores_->at(i).current_task_completion_time);
      
      core->current_task_completion_time = min_starting_time + task.execution_time_;
      core->current_task_priority = task.priority_;
    }
    
  }
  
  unsigned int maximum_completion_time = 0;
  for (int i = 0; i < cores_->size(); i++)
    maximum_completion_time = std::max(maximum_completion_time, cores_->at(i).current_task_completion_time);
  
  if (debug)
    std::cout << "MpsProblem::eval_tour: " << maximum_completion_time << std::endl;
  
  return maximum_completion_time;
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

