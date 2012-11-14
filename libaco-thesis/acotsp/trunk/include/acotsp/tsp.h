#include <map>
#include <string>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <exception>
#include <libaco/ants.h>
#include <libaco/util.h>

/// Travelling Salesman Problem libaco implemenation.
class TspProblem : public OptimizationProblem {
  private:
    /// Vertices that were already visited by the current ant.
    ///
    /// If a vertex has been already visited by the current ant it's value in
    /// the map is true.
    std::map<unsigned int,bool> visited_vertices_;

    /// Distance matrix of the TSP instance.
    ///
    /// (*distances_)[v][w] gives you the distance between city v and city w.
    Matrix<unsigned int> *distances_;

    /// The first city the current ant visited on it's tour.
    ///
    /// This is where the tour also needs to end.
    unsigned int start_vertex_;
  public:
    TspProblem(Matrix<unsigned int> *distances);
    ~TspProblem();
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

class FileNotFoundException : public std::exception {
  private:
    const char *filepath_;
  public:
    FileNotFoundException(const char *filepath);
    const char *what() const throw();
};

namespace Parser {
  Matrix<unsigned int> *parse_tsplib(const char *filepath) throw(FileNotFoundException);
}

struct city {
  int num;
  double coord_x;
  double coord_y;
};

