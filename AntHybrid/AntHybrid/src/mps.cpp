//
//  hybrid.cpp
//  AntHybrid
//
//  Created by Hamilton Turner on 11/9/12.
//  Copyright (c) 2012 Virginia Tech. All rights reserved.
//

#include "mps.h"

#include <stdexcept>


MpsProblem::MpsProblem(Matrix<unsigned int> *distances) {
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

MpsProblem::~MpsProblem(){}

unsigned int MpsProblem::get_max_tour_size(){
    return (int) (*tasks_).size();
}

unsigned int MpsProblem::number_of_vertices(){
    return (int) ((*tasks_).size() * (*cores_).size());
}

std::map<unsigned int,double> MpsProblem::get_feasible_start_vertices(){
    
    // Start on any processor, but you must execute the first task first
    std::map<unsigned int,double> vertices;
    unsigned int start = get_vertex_for(Util::random_number( (int) cores_->size()), 0);
    vertices[start] = 1.0;
    return vertices;
}

std::map<unsigned int,double> MpsProblem::get_feasible_neighbours(unsigned int vertex){

    // Limit feasible neighbors to tasks of allowed priorities that have
    // not been scheduled
    unsigned int task, core;
    get_task_and_core_from_vertex(vertex, task, core);
    unsigned int priority = (*tasks_)[task].priority_;
    std::map<unsigned int, double> neighbors;
    for (unsigned int i = task + 1; i < tasks_->size(); i++) {
        Task cur = (*tasks_)[i];

        // Unscheduled tasks at this priority?
        if (cur.priority_ == priority && !cur.scheduled_)
            for (unsigned int core = 0; core < cores_->size(); core++)
                neighbors[get_vertex_for(core, i)] = 1.0;

        if (cur.priority_ > priority) {
            if (neighbors.size() == 0)
                priority += 1;
            else
                break;
        }
    }
    
    return neighbors;
}

// NOTE: This is called as so:
//   TspProblem::eval_tour
//   TspProblem::cleanup
//   TspProblem::apply_local_search
//   TspProblem::eval_tour
// So you must evaluate a tour based upon this vector and iteration-independent
// variables only
double MpsProblem::eval_tour(const std::vector<unsigned int> &tour){
    
    // A mini-representation of a core
    struct SmallCore {
        unsigned int current_task_priority = 0;
        unsigned int current_task_completion_time = 0;
    };
    
    // Method-local representation of core state
    SmallCore* cores = new SmallCore[cores_->size()];
    
    // For the entire tour, iteratively update the completion time
    // of each core
    std::vector<unsigned int>::const_iterator it = tour.begin();
    unsigned int task_i, core_i;
    Task* task;
    for (it = tour.begin(); it < tour.end(); it++) {
        get_task_and_core_from_vertex(*it, task_i, core_i);
        task = &((*tasks_)[task_i]);
        
        // There are no predecessors, but there may have been some prior level 1
        // stuff run on this core if we had a small number of cores relative to
        // the number of priority 1 tasks
        if (task->priority_ == 1) {
            cores[core_i].current_task_priority = task->priority_;
            cores[core_i].current_task_completion_time += task->execution_time_;
        } else {

            // Determine our minimum starting time on this processor, based
            // upon making sure we don't violate the precedence relation
            int min_starting_time = cores[core_i].current_task_completion_time;
            for (int i = 0; i < cores_->size(); i++)
            {
                if (cores[i].current_task_priority == task->priority_ - 1)
                {
                    // We cannot execute until all of these are done
                    if (min_starting_time < cores[i].current_task_completion_time)
                        min_starting_time = cores[i].current_task_completion_time;
                }
            }
            
            // Add our execution time to our minimum starting time, and update our
            // executing core's completion time
            cores[core_i].current_task_completion_time = min_starting_time + task->execution_time_;
        }
    }
    
    double maximum_completion_time = 0;
    for (int i = 0; i < cores_->size(); i++)
        if (cores[i].current_task_completion_time > maximum_completion_time)
            maximum_completion_time =cores[i].current_task_completion_time;

    return maximum_completion_time;
}

double MpsProblem::pheromone_update(unsigned int v, double tour_length){
    return 0;
}

void MpsProblem::added_vertex_to_tour(unsigned int vertex){
}

bool MpsProblem::is_tour_complete(const std::vector<unsigned int> &tour){
    return true;
}

std::vector<unsigned int> MpsProblem::apply_local_search(const std::vector<unsigned int> &tour){
    return tour;
}

void MpsProblem::cleanup(){
    next_ready_task_ = 0;
    
    std::vector<Task>::iterator it;
    
}

unsigned int MpsProblem::get_vertex_for(unsigned int core, unsigned int task) {
    // unsigned: 0 to 4294967295
    // Assumming that no one will run this with more than 9999 processors, so I'm
    // reserving the 4 most significant bits for processor ID and all remaining
    // LSBs for the taskID. If this goes production it should be changed
    return (core * 1000000) + task;
}

void MpsProblem::get_task_and_core_from_vertex(unsigned int vertex_id, unsigned int & task, unsigned int & core) {
    core = vertex_id / 1000000;
    task = vertex_id - (core * 1000000);
}

std::list<Task> MpsProblem::get_ready_tasks() {
    int priority = (*tasks_)[next_ready_task_].priority_;
    
    std::list<Task> result;
    for (int i = next_ready_task_ + 1; i < (*tasks_).size(); i++) {
        if ((*tasks_)[i].priority_ == priority)
            result.push_front((*tasks_)[i]);
        else
            break;
    }
    
    return result;
}


