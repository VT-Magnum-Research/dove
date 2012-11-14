#include <string>
#include <cstdlib>
#include <cmath>
#include <acotemplate/template.h>
#include <liblocalsearch/localsearch.h>

Problem::Problem() {
}

Problem::~Problem() {
}

unsigned int Problem::get_max_tour_size() {
}

unsigned int Problem::number_of_vertices() {
}

std::map<unsigned int,double> Problem::get_feasible_start_vertices() {
}

std::map<unsigned int,double> Problem::get_feasible_neighbours(unsigned int vertex) {
}

double Problem::eval_tour(const std::vector<unsigned int> &tour) {
}

double Problem::pheromone_update(unsigned int v, double tour_length) {
}

void Problem::added_vertex_to_tour(unsigned int vertex) {
}

bool Problem::is_tour_complete(const std::vector<unsigned int> &tour) {
}

std::vector<unsigned int> Problem::apply_local_search(const std::vector<unsigned int> &tour) {
}

void Problem::cleanup() {
}
