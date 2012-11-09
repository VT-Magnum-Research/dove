//
//  hybrid.cpp
//  AntHybrid
//
//  Created by Hamilton Turner on 11/9/12.
//  Copyright (c) 2012 Virginia Tech. All rights reserved.
//

#include "mps.h"


MpsProblem::MpsProblem(Matrix<unsigned int> *distances) {
    
}

MpsProblem::~MpsProblem(){}

unsigned int MpsProblem::get_max_tour_size(){
    return 0;
}

unsigned int MpsProblem::number_of_vertices(){
    return 0;
}

std::map<unsigned int,double> MpsProblem::get_feasible_start_vertices(){
    std::map<unsigned int, double> foo;
    return foo;
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
    std::vector<unsigned int> foo;
    return foo;
}

void MpsProblem::cleanup(){}
