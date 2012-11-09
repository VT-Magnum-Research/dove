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
    std::map<unsigned int, double> foo;
    return foo;
}

double MpsProblem::eval_tour(const std::vector<unsigned int> &tour){
    return 0;
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

unsigned int MpsProblem::get_vertex_for(unsigned int processor, unsigned int task) {
    // unsigned: 0 to 4294967295
    // Assumming that no one will run this with more than 9999 processors, so I'm
    // reserving the 4 most significant bits for processor ID and all remaining
    // LSBs for the taskID. If this goes production it should be changed
    return (processor * 1000000) + task;
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


