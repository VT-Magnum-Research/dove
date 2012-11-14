#include <map>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <exception>
#include <map>
#include <list>
#include <cstring>
#include <csignal>
#include <libaco/ants.h>
#include <liblocalsearch/localsearch.h>

class EliminationGraph {
  private:
    unsigned int **a__;
    unsigned int *degrees__;
    unsigned int *initial_degrees__;
    bool **t__;
    bool *eliminated__;
    unsigned int nr_eliminations__;
    unsigned int size__;
  public:
    EliminationGraph(const Graph &graph);
    ~EliminationGraph();
    unsigned int eliminate(unsigned int vertex);
    unsigned int eliminate_efficient(unsigned int vertex, unsigned int *elim_positions, unsigned int *vertex_neighbours);
    unsigned int eval_ordering(const std::vector<unsigned int> &ordering);
    std::vector<unsigned int> get_max_clique_positions(const std::vector<unsigned int> &solution);
    unsigned int get_degree(unsigned int vertex) const;
    unsigned int min_fill(unsigned int vertex) const;
    std::vector<unsigned int> get_neighbours(unsigned int vertex) const;
    unsigned int number_of_vertices();
    void rollback();
};

typedef double (*heuristicf)(const EliminationGraph &graph, unsigned int vertex);

namespace Heuristic {
  double min_degree(const EliminationGraph &graph, unsigned int vertex);
  double min_fill(const EliminationGraph &graph, unsigned int vertex);
}

enum LocalSearchType { NO_LS, HILL_CLIMBING, ITERATED_LS };

class MaxCliqueRandomNeighbour : public Neighbourhood {
  private:
    EliminationGraph *elim_graph_;
    std::vector<unsigned int> solution_;
    std::vector<unsigned int> neighbour_;
    bool has_next_neighbour_;
  public:
    MaxCliqueRandomNeighbour(EliminationGraph &graph, std::vector<unsigned int> solution);
    ~MaxCliqueRandomNeighbour();
    void set_solution(std::vector<unsigned int> solution);
    std::vector<unsigned int> get_solution();
    bool has_next_neighbour_solution();
    std::vector<unsigned int> next_neighbour_solution();
};

class MaxCliqueNeighbourhood : public Neighbourhood {
  private:
    EliminationGraph *elim_graph_;
    std::vector<unsigned int> solution_;
    unsigned int max_clique_pos_;
    unsigned int swap_pos_;
  public:
    MaxCliqueNeighbourhood(EliminationGraph &graph, std::vector<unsigned int> solution);
    ~MaxCliqueNeighbourhood();
    void set_solution(std::vector<unsigned int> solution);
    std::vector<unsigned int> get_solution();
    bool has_next_neighbour_solution();
    std::vector<unsigned int> next_neighbour_solution();
};

class DecompLocalSearch : public LocalSearch {
  private:
    void search_neighbourhood();
  public:
    DecompLocalSearch(std::vector<unsigned int> initial_solution, EvaluationFunction &eval_func, Neighbourhood &neighbourhood);
};

template <class T> class DecompProblem : public OptimizationProblem, public EvaluationFunction, public IterativeLocalSearchClient {
  protected:
    T *graph_;
    EliminationGraph *elim_graph_;
    std::vector<double> vertex_weight_;
    heuristicf heuristic_; 
    unsigned int vertices_eliminated_;
    bool pheromone_update_es_;
    LocalSearchType ls_type_;
    unsigned int iterations_without_improve_;
    unsigned int ls_iterations_without_improve_;
    bool use_heuristic_;
    std::map<unsigned int,double> vertices_;
    std::map<unsigned int,double> feasible_vertices_;
  public:
    DecompProblem(T *graph, heuristicf heuristic=Heuristic::min_degree, bool use_heuristic=true, bool pheromone_update_es=false, LocalSearchType ls_type=HILL_CLIMBING) {
      graph_ = graph;
      elim_graph_ = new EliminationGraph(*graph);
      vertex_weight_.reserve(graph->number_of_vertices());
      for(unsigned int i=0;i<graph_->number_of_vertices();i++) {
        vertices_[i] = 1;
      }
      feasible_vertices_ = vertices_;
      heuristic_ = heuristic;
      vertices_eliminated_ = 0;
      pheromone_update_es_ = pheromone_update_es;
      ls_type_ = ls_type;
      use_heuristic_ = use_heuristic;
      iterations_without_improve_ = 10;
      ls_iterations_without_improve_ = 10;
    }

    ~DecompProblem() {
      delete graph_;
      delete elim_graph_;
    }

    unsigned int get_max_tour_size() {
      return graph_->number_of_vertices();
    }

    unsigned int number_of_vertices() {
      return graph_->number_of_vertices();
    }

    virtual std::map<unsigned int,double> get_feasible_start_vertices() {
      std::map<unsigned int,double> vertices;
      for(unsigned int i=0;i<graph_->number_of_vertices();i++) {
        vertices[i] = use_heuristic_ ? heuristic_(*elim_graph_, i) : 1;
      }
      return vertices;
    }

    virtual std::map<unsigned int,double> get_feasible_neighbours(unsigned int vertex) {
      if(use_heuristic_) {
        std::map<unsigned int,double>::iterator it;
        for (it=feasible_vertices_.begin();it!=feasible_vertices_.end();it++) {
          (*it).second = heuristic_(*elim_graph_, (*it).first);
        }
      }
      return feasible_vertices_;
    }

    double eval_tour(const std::vector<unsigned int> &tour) {
      double width = compute_width(tour);
      elim_graph_->rollback();
      return width;
    }

    double pheromone_update(unsigned int v, double tour_length) {
      if (pheromone_update_es_) {
        return 1.0 / (vertex_weight_[v] + 1.0) * (1.0 / tour_length);
      } else {
        return 1.0 / tour_length;
      }
    }

    void added_vertex_to_tour(unsigned int vertex) {
      if (use_heuristic_) {
        vertex_weight_[vertex] = elim_graph_->get_degree(vertex) * (1.0 / elim_graph_->number_of_vertices());
        elim_graph_->eliminate(vertex);
        vertices_eliminated_++;
      }
      feasible_vertices_.erase(vertex);
    }

    bool is_tour_complete(const std::vector<unsigned int> &tour) {
      bool complete = (tour.size() == graph_->number_of_vertices());
      return complete;
    }

    double eval_solution(const std::vector<unsigned int> &solution) {
      return 1.0 / this->eval_tour(solution);
    }

    std::vector<unsigned int> perturbate(const std::vector<unsigned int> &solution) {
      std::vector<unsigned int> new_solution = solution;

      unsigned int max_clique_perturbation = random_number(2);
      if(max_clique_perturbation) {
        std::vector<unsigned int> max_cliques = elim_graph_->get_max_clique_positions(solution);
        for(unsigned int i=0;i<max_cliques.size();i++) {
          /*unsigned int swap_pos = random_number(new_solution.size());
          unsigned int tmp = new_solution[max_cliques[i]];
          new_solution[max_cliques[i]] = new_solution[swap_pos];
          new_solution[swap_pos] = tmp;*/
          unsigned int tmp = new_solution[max_cliques[i]];
          new_solution.erase(new_solution.begin()+max_cliques[i]);
          unsigned int ins_pos = random_number(new_solution.size());
          new_solution.insert(new_solution.begin()+ins_pos, tmp);
        }
      } else {
        for(unsigned int i=0;i<2;i++) {
          unsigned int v1 = random_number(new_solution.size());
          unsigned int tmp = new_solution[v1];
          new_solution.erase(new_solution.begin()+v1);
          unsigned int v2 = random_number(new_solution.size());
          new_solution.insert(new_solution.begin()+v2, tmp);
        }
      }
      return new_solution;
    }

    bool is_solution_accepted(double new_quality, double best_quality) {
      return ((1.0 / new_quality) < ((1.0 / best_quality)+3));
    }

    void set_iteratedls_parameters(unsigned int iterations_without_improve, unsigned int ls_iterations_without_improve) {
      iterations_without_improve_ = iterations_without_improve;
      ls_iterations_without_improve_ = ls_iterations_without_improve;
    }

    std::vector<unsigned int> apply_local_search(const std::vector<unsigned int> &tour) {
      if (ls_type_ == ITERATED_LS) {
        MaxCliqueRandomNeighbour neighbourhood(*elim_graph_, tour);
        DecompLocalSearch local_search(tour, *this, neighbourhood);
        IterativeLocalSearch search(&local_search, this);
        search.run(iterations_without_improve_, ls_iterations_without_improve_);
        return search.get_best_solution();
      } else if(ls_type_ == HILL_CLIMBING) {
        MaxCliqueNeighbourhood neighbourhood(*elim_graph_, tour);
        HillClimbing local_search(tour, *this, neighbourhood);
        local_search.search_iterations_without_improve(1);
        return local_search.get_best_so_far_solution();
      }
      return tour;
    }

    void cleanup() {
      if (use_heuristic_) {
        vertices_eliminated_ = 0;
        elim_graph_->rollback();
      }
      feasible_vertices_ = vertices_;
    }

    virtual unsigned int compute_width(const std::vector<unsigned int> &tour) = 0;
};

template <class T> class TreeDecompProblem : public DecompProblem<T> {
  public:
    TreeDecompProblem(T *graph, heuristicf heuristic=Heuristic::min_degree, bool use_heuristic=false, bool pheromone_update_es=false, LocalSearchType ls_type=HILL_CLIMBING) : DecompProblem<T>(graph, heuristic, use_heuristic, pheromone_update_es, ls_type) {
    }

    unsigned int compute_width(const std::vector<unsigned int> &tour) {
      DecompProblem<T>::elim_graph_->rollback();
      return DecompProblem<T>::elim_graph_->eval_ordering(tour);
    }
};

template <class T> class HyperTreeDecompProblem : public DecompProblem<T> {
  private:
    HyperGraph *hypergraph_;
  public:
    HyperTreeDecompProblem(HyperGraph *hypergraph, heuristicf heuristic=Heuristic::min_degree, bool use_heuristic=false, bool pheromone_update_es=false, LocalSearchType ls_type=HILL_CLIMBING) : DecompProblem<T>(&hypergraph->get_primal_graph<T>(), heuristic, use_heuristic, pheromone_update_es, ls_type) {
      hypergraph_ = hypergraph;
    }

    ~HyperTreeDecompProblem() {
      delete hypergraph_;
    }

    unsigned int compute_width(const std::vector<unsigned int> &tour) {
      unsigned int width = 0;
      unsigned int *vertex_neighbours = new unsigned int[tour.size()];
      unsigned int *elim_positions = new unsigned int[tour.size()];
      for(unsigned int j=0;j<tour.size();j++) {
        elim_positions[tour[j]] = j;
      }
      DecompProblem<T>::elim_graph_->rollback();
      for(unsigned int i=0;i<tour.size();i++) {
        std::vector<unsigned int> vertices = DecompProblem<T>::elim_graph_->get_neighbours(tour[i]);
        vertices.push_back(tour[i]);
        // set covering
        unsigned int w = compute_greedy_hyperedge_covering(vertices).size();
        DecompProblem<T>::elim_graph_->eliminate_efficient(tour[i], elim_positions, vertex_neighbours);
        if(w > width) {
          width = w;
        }
      }
      delete[] elim_positions;
      delete[] vertex_neighbours;
      return width;
    }

    std::vector<unsigned int> compute_greedy_hyperedge_covering(std::vector<unsigned int> vertices) {
      std::vector<unsigned int> edges;
      std::map< unsigned int, std::vector<unsigned int> > edges_vertices;
      while(!vertices.empty()) {
        for(unsigned int i=0;i<vertices.size();i++) {
          unsigned int vertex = vertices[i];
          std::vector<unsigned int> vertex_edges = hypergraph_->get_edges_for_vertex(vertex);
          for(unsigned int j=0;j<vertex_edges.size();j++) {
            edges_vertices[vertex_edges[j]].push_back(vertex);
          }
        }

        //get edge that covers a maximum of vertices
        unsigned int maximum_edge;
        std::vector<unsigned int> vertices_covered;
        for(std::map< unsigned int, std::vector<unsigned int> >::iterator it=edges_vertices.begin();it!=edges_vertices.end();it++) {
          if((*it).second.size() > vertices_covered.size()) {
            maximum_edge = (*it).first;
            vertices_covered = (*it).second;
          }
        }

        //remove vertices covered by edge from vertices
        for(unsigned int k=0;k<vertices_covered.size();k++) {
          for(std::vector<unsigned int>::iterator it2=vertices.begin();it2!=vertices.end();it2++) {
            if(*it2 == vertices_covered[k]) {
              vertices.erase(it2);
              break;
            }
          }
        }

        edges.push_back(maximum_edge);
      }
      return edges;
    }
};

class FileNotFoundException : public std::exception {
  private:
    const char *filepath_;
  public: 
    FileNotFoundException(const char *filepath);
    const char *what() const throw();
};

namespace Parser {
  template <class T> Graph &parse_dimacs(const char *filepath) throw(FileNotFoundException) {
    Graph *graph = NULL;
    int number_of_vertices;
    int vertex_a, vertex_b;
    char problem[5];
    char buf[1024];
    char flag;
    std::ifstream file(filepath);

    if(!file) {
      throw FileNotFoundException(filepath);
    }

    while(file.good()) {
      file >> flag;
      switch(flag) {
        case 'p':
          file >> problem;
          if (!strcmp(problem, "edge")) {
            file >> number_of_vertices;
            graph = new T(number_of_vertices);
          }
          break;
        case 'e':
          file >> vertex_a;
          file >> vertex_b;
          graph->add_edge(vertex_a-1, vertex_b-1);
          break;
        default:
          file.getline(buf, 1024);
          break;
      }
    }
    file.close();
    return *graph;
  }

  HyperGraph &parse_hypertreelib(const char *filepath) throw(FileNotFoundException);
}
