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

#include "ants.h"


/// Multiprocessor scheduling using the libaco implemenation.
class MpsProblem : public OptimizationProblem {
    public:
    MpsProblem(Matrix<unsigned int> *distances);
    ~MpsProblem();
    unsigned int get_max_tour_size();
    unsigned int number_of_vertices();
    std::map<unsigned int,double> get_feasible_start_vertices();
    std::map<unsigned int,double> get_feasible_neighbours(unsigned int vertex);
    double eval_tour(const std::vector<unsigned int> &tour);
    double pheromone_update(unsigned int v, double tour_length);
    void added_vertex_to_tour(unsigned int vertex);
    bool is_tour_complete(const std::vector<unsigned int> &tour);
    std::vector<unsigned int> apply_local_search(const std::vector<unsigned int> &tour);
    void cleanup();
};

#endif /* defined(__AntHybrid__hybrid__) */
