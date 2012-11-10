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
#include "util.h"

#include "ants.h"

struct Task {
    // 1 is 'highest' priority and will be executed before 2,
    // which is before 3 ... etc
    unsigned int priority_;
    
    bool scheduled_;
    
    // TODO temporary, I need to implement the time matrix
    unsigned int execution_time_;
};

struct Core {
};


/// Multiprocessor scheduling using the libaco implemenation.
//   At the basic level, this does nothing more than allocate
//   tasks to cores. If you pass all precedence numebrs as 1, it
//   will schedule tasks with no precedence relationships.
//   Other details can be removed via similar means
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
    
    // Index to the next task that should be executed. Note that when
    // next_ready_task_ == tasks_.size() you have completed a tour
    //unsigned int next_ready_task_;
    
    // Initialized with the number of tasks in this problem
    // TODO: Eventually replace this, it's hacky
    static unsigned int task_size_;
    
    // Returns all tasks with the same priority as next_ready_task_
    //std::list<Task> get_ready_tasks();
    
    std::string debug_vertex(unsigned int vertex);
    
    static inline unsigned int get_vertex_for(unsigned int core, unsigned int task) {
        return core * task_size_ + task;
    }

public:

    static inline void get_task_and_core_from_vertex(unsigned int vertex_id, unsigned int & task, unsigned int & core) {
        task = vertex_id % task_size_;
        core = vertex_id / task_size_;
    }

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
