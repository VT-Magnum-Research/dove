#include <map>
#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <exception>
#include <libaco/ants.h>
#include <libaco/util.h>

class Problem : public OptimizationProblem {
  public:
    Problem();
    ~Problem();
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
