#include <string>
#include <vector>
#include <list>
#include <map>
#include <cmath>
#include "graph.h"

/// \mainpage libaco
///
/// \section intro_sec Introduction
///
///libaco is a library which can be used for solving combinatorial optimization problems using the Ant Colony Optimization (ACO) meta-heuristic. The library implements the following variants of ACO algorithms:
///
/// - Simple Ant System
/// - Elitist Ant System
/// - Rank-Based Ant System
/// - Max-Min Ant System
/// - Ant Colony System 
///
///For detailed descriptions of these algorithms take a look at the book "Ant Colony Optimization" by Marco Dorigo and Thomas Stuetzle.
///
///  All this was implemented as part of my Master's Thesis at the Technical University of Vienna I'm currently working on with the prospective title 'Ant Colony Optimization for Tree and Hypertree Decomposition'. 
///
/// \section getting_started_sec Getting Started
///
/// If you don't have any clue how ant algorithms work I recommend you to read something on the topic first.
///
/// The only interface between libaco and your client code is defined by the virtual class OptimizationProblem. You need to inherit from this class and implement all pure virtual methods.
///
/// \code
/// class MyProblem : public OptimizationProblem {
///   public:
///     unsigned int get_max_tour_size() { /* TODO: implement */ }
///     unsigned int number_of_vertices() { /* TODO: implement */ }
///     std::map<unsigned int,double> get_feasible_start_vertices() { /* TODO: implement */ }
///     std::map<unsigned int,double> get_feasible_neighbours(unsigned int vertex) { /* TODO: implement */ }
///     double eval_tour(const std::vector<unsigned int> &tour) { /* TODO: implement */ }
///     double pheromone_update(unsigned int v, double tour_length) { /* TODO: implement */ }
///     void added_vertex_to_tour(unsigned int vertex) { /* TODO: implement */ }
///     bool is_tour_complete(const std::vector<unsigned int> &tour) { /* TODO: implement */ }
///     std::vector<unsigned int> apply_local_search(const std::vector<unsigned int> &tour) { return tour; }
///     void cleanup() { /* TODO: implement */ };
/// }
/// \endcode
///
/// After you have implemented your problem-specific logic all you need to do is to instantiate an ant colony and supply it with a corresponding configuration object and your OptimizationProblem.
///
/// \code
/// AntColonyConfiguration config;
/// SimpleAntColony colony(new MyProblem(), config);
/// colony.run(); // run one iteration
/// std::vector<unsigned int> tour = colony.get_best_tour();
/// double length = colony.get_best_tour_length();
/// \endcode
///
/// For this example we have used a SimpleAntColony but we also could have used one of the following other variants:
///
/// - ElitistAntColony (together with an ElitistAntColonyConfiguration)
/// - RankBasedAntColony (together with an RankBasedAntColonyConfiguration)
/// - MaxMinAntColony (together with an MaxMinAntColonyConfiguration)
/// - ACSAntColony (together with an ACSAntColonyConfiguration)
///
/// For a far more detailed explanation on how to make use of this library take a look at the tutorial at:
///
/// http://code.google.com/p/libaco/wiki/Tutorial
///
/// It shows step-by-step how to implement a program with libaco for finding solutions to arbitrary instances of the Travelling Salesman Problem.

class PheromoneMatrix : protected Matrix<double> {
  protected:
    double evaporation_rate_;
    double initial_pheromone_;
  public:
    PheromoneMatrix(int vertices, double evaporation_rate, double initial_pheromone);
    virtual ~PheromoneMatrix() {}
    double get(unsigned int v, unsigned int w);
    virtual void add(unsigned int v, unsigned int w, double amount);
    virtual void evaporate(unsigned int v, unsigned int w);
    void evaporate_all();
    double get_evaporation_rate();
    unsigned int size();
    double lambda_branching_factor(unsigned int v, double lambda);
    double average_lambda_branching_factor(double lambda);
};

class MaxMinPheromoneMatrix : public PheromoneMatrix {
  private:
    double max_;
    double min_;
  public:
    MaxMinPheromoneMatrix(int vertices, double evaporation_rate, double initial_pheromone);
    void set_min(double min);
    void set_max(double max);
    void add(unsigned int v, unsigned int w, double amount);
    void evaporate(unsigned int v, unsigned int w);
};

class ACSPheromoneMatrix : public PheromoneMatrix {
  private:
    double xi_;
  public:
    ACSPheromoneMatrix(int vertices, double evaporation_rate, double initial_pheromone);
    void set_xi(double xi);
    void local_pheromone_update(unsigned int v, unsigned int w);
};

class Tour {
  private:
    std::vector<unsigned int> *vertices_;
    unsigned int capacity_;
    double length_;
  public:
    Tour(unsigned int vertices);
    ~Tour();
    double get_length();
    void set_length(double length);
    const std::vector<unsigned int> &get_vertices();
    void set_vertices(const std::vector<unsigned int> &vertices);
    Tour &add_vertex(unsigned int vertex);
    unsigned int &operator[](const unsigned int vertex) const;
    unsigned int size() const;
    unsigned int capacity();
    void clear();
    Tour &operator=(const Tour &t);
    bool operator<(const Tour &t);
};

/// Interface a client of libaco needs to implement.
/// 
/// Interface to the problem-specific logic a client must supply.
class OptimizationProblem {
  public:
    virtual ~OptimizationProblem() {}
    /// Returns the maximum number of nodes in a tour.
    ///
    /// \return maximum number of nodes in a tour.
    virtual unsigned int get_max_tour_size() = 0;

    /// Returns the number of vertices in the construction graph.
    ///
    /// \return the number of vertices in the construction graph.
    virtual unsigned int number_of_vertices() = 0;

    /// Returns a map with the ids of all feasible start vertices as keys and some heuristic value.
    ///
    /// \return a map that maps vertex ids to their heuristic value. the greater the value the more likely the vertex is chosen by the ant.
    virtual std::map<unsigned int,double> get_feasible_start_vertices() = 0;

    /// Returns a map with the ids of all feasible neighbour vertices of vertex as keys and some heuristic value.
    ///
    /// \param vertex the ant's current location.
    /// \return a map that maps neighbour ids to their heuristic value. the greater the value the more likely the neighbour is chosen by the ant.
    virtual std::map<unsigned int,double> get_feasible_neighbours(unsigned int vertex) = 0;

    /// Returns the 'length' of a given tour (the lower the better).
    ///
    /// \param tour that is supposed to be evaluated.
    /// \return the 'length' of the tour.
    virtual double eval_tour(const std::vector<unsigned int> &tour) = 0;

    /// Returns the amount of pheromone that shall be deposited onto the edge pointing to v.
    ///
    /// \param vertex the vertex 
    /// \param tour_length the length of the tour.
    /// \return amount of pheromone that shall be laid on the edge to v.
    virtual double pheromone_update(unsigned int vertex, double tour_length) = 0;
    
    /// Callback that notifies the client code that the current ant has added this vertex to it's tour.
    ///
    /// \param vertex that has been added to the tour.
    virtual void added_vertex_to_tour(unsigned int vertex) = 0;

    /// Determines whether the supplied tour is complete.
    ///
    /// \param tour shall be checked for completeness.
    /// \return true if the tour is complete. false otherwise.
    virtual bool is_tour_complete(const std::vector<unsigned int> &tour) = 0;

    /// Gives the client code the oppurtunity to improve the tour after construction by applying some local search.
    ///
    /// \param tour initial tour for the local search.
    /// \return best tour found.
    virtual std::vector<unsigned int> apply_local_search(const std::vector<unsigned int> &tour) { return tour; }

    /// Here go eventually necessary cleanup actions between two tour constructions.
    virtual void cleanup() = 0;
};

class Ant {
  protected:
    struct MultiMapComp {
      bool operator()(const double &p1, const double &p2) const;
    };
    Tour *tour;
    void update_tour_length(OptimizationProblem &op);
    void add_vertex_to_tour(OptimizationProblem &op, unsigned int vertex);
    std::multimap<double,unsigned int,MultiMapComp> get_feasible_vertices(OptimizationProblem &op, PheromoneMatrix &pheromones, double alpha, double beta);
    int current_vertex();
    unsigned int choose_next_vertex_with_likelihood(std::multimap<double,unsigned int,MultiMapComp> probabilities);
  public:
    Ant(unsigned int vertices);
    Ant(const Ant &ant);
    Ant &operator=(const Ant &ant);
    bool operator<(const Ant &ant);
    virtual ~Ant();
    double get_tour_length();
    std::vector<unsigned int> get_vertices();
    void reset();
    void apply_local_search(OptimizationProblem &op);
    virtual void construct_solution(OptimizationProblem &op, PheromoneMatrix &pheromones, double alpha, double beta) {}
    void construct_rational_solution(OptimizationProblem &op, PheromoneMatrix &pheromones, double alpha, double beta);
    void construct_random_proportional_solution(OptimizationProblem &op, PheromoneMatrix &pheromones, double alpha, double beta);
    virtual void offline_pheromone_update(OptimizationProblem &op, PheromoneMatrix &pheromones, double weight=1.0) {}
};

class SimpleAnt : public Ant {
  public:
    SimpleAnt(unsigned int vertices);
    SimpleAnt(const SimpleAnt &ant);
    SimpleAnt &operator=(const SimpleAnt &ant);
    void construct_solution(OptimizationProblem &op, PheromoneMatrix &pheromones, double alpha, double beta);
    void offline_pheromone_update(OptimizationProblem &op, PheromoneMatrix &pheromones, double weight=1.0);
};

class ACSAnt : public Ant {
  private:
    double q0_;
  public:
    ACSAnt(unsigned int vertices);
    ACSAnt(const ACSAnt &ant);
    ACSAnt &operator=(const ACSAnt &ant);
    void construct_solution(OptimizationProblem &op, PheromoneMatrix &pheromones, double alpha, double beta);
    void construct_pseudorandom_proportional_solution(OptimizationProblem &op, PheromoneMatrix &pheromones, double alpha, double beta);
    void offline_pheromone_update(OptimizationProblem &op, PheromoneMatrix &pheromones, double weight=1.0);
    void set_q0(double q0);
};

/// Constructs a solution based on the heuristic information alone and computes the average pheromone update per edge.
///
/// Is useful for finding a initial pheromone value. A good initial pheromone value might be number_of_ants * average_pheromone_update
double compute_average_pheromone_update(OptimizationProblem &op);

/// Base class of all ACO configurations.
///
/// Includes all configuration parameters all ACO variants have in common.
class AntColonyConfiguration {
  public:
    enum LocalSearchType { LS_NONE, LS_ITERATION_BEST, LS_ALL };

    /// Number of ants that construct a tour in every iteration.
    unsigned int number_of_ants;
    /// Weight of pheromone value in tour construction.
    double alpha;
    /// Weight of heuristic information in tour construction.
    double beta;
    /// Defines how fast pheromone shall evaporate.
    double evaporation_rate;
    /// The initial amount of pheromone deposited on the edges.
    double initial_pheromone;
    /// The type of local search to be performed.
    LocalSearchType local_search;

    AntColonyConfiguration();
};

/// Configuration class for ElististAntColony.
class ElitistAntColonyConfiguration : public AntColonyConfiguration {
  public:
    /// Weight for the pheromone update of the best-so-far ant.
    double elitist_weight;
    ElitistAntColonyConfiguration();
};

/// Configuration class for RankBasedAntColony.
class RankBasedAntColonyConfiguration : public AntColonyConfiguration {
  public:
    /// Number of elitist ants.
    ///
    /// For example: if elitist_ants = 5 then the best-so-far ant and the top
    ///              4 iteration-best ants are allowed to deposit pheromone.
    unsigned int elitist_ants;
    RankBasedAntColonyConfiguration();
};

/// Configuration class for MaxMinAntColony.
class MaxMinAntColonyConfiguration : public AntColonyConfiguration {
  public:
    /// Frequency of best-so-far ant pheromone updates.
    ///
    /// For example: if best_so_far_frequency = 3 then the best-so-far ant
    ///              deposits pheromone in every third iteration instead of the
    ///              iteration-best-ant
    unsigned int best_so_far_frequency;
    /// Determines lower bound of pheromone values.
    ///
    /// For example: if a = 2 then the lower bound of the pheromone values is
    ///              half of the initial pheromone value (the pheromone value
    ///              before the first iteration)
    double a;
    MaxMinAntColonyConfiguration();
};

/// Configuration class for ACSAntColony.
class ACSAntColonyConfiguration : public AntColonyConfiguration {
  public:
    /// Probability that the ant chooses a vertex rationally (according to
    /// the heuristic and pheromone information).
    double q0;
    /// Local pheromone update parameter.
    double xi;
    ACSAntColonyConfiguration();
};

/// Virtual base class of all Ant Colony variants.
///
/// Ant Colony variants differ in the way ants construct their tours and how
/// pheromone values are updated. All other logic is handled by this virtual
/// base class.
template<class T=Ant, class P=PheromoneMatrix> class AntColony {
  private:
    void construct_ants_solutions() {
      for(typename std::list<T>::iterator it=ants_->begin();it!=ants_->end();it++) {
        T &ant = (*it);
        ant.construct_solution(*problem_, *pheromones_, alpha_, beta_);
      }
    }

    void apply_local_search() {
      ants_->sort();
      if(local_search_type_ == AntColonyConfiguration::LS_ITERATION_BEST) {
        typename std::list<T>::iterator it_best = ants_->begin();
        (*it_best).apply_local_search(*problem_);
      } else if(local_search_type_ == AntColonyConfiguration::LS_ALL) {
        for(typename std::list<T>::iterator it=ants_->begin();it!=ants_->end();it++) {
          T &ant = (*it);
          ant.apply_local_search(*problem_);
        }
      }
    }

    void reset_ants() {
      for(typename std::list<T>::iterator it=ants_->begin();it!=ants_->end();it++) {
        it->reset();
      }
    }

    void update_best_tours_no_ls() {
      ants_->sort();
      (*best_iteration_no_ls_) = ants_->front();
      if(best_so_far_no_ls_->get_tour_length() > best_iteration_no_ls_->get_tour_length()) {
        (*best_so_far_no_ls_) = (*best_iteration_no_ls_);
      }
    }

    void update_best_tours() {
      ants_->sort();
      (*best_iteration_) = ants_->front();
      if(best_so_far_->get_tour_length() > best_iteration_->get_tour_length()) {
        (*best_so_far_) = (*best_iteration_);
      }
    }

    double compute_variation_coefficient() {
      double average = 0.0;
      double standard_deviation = 0.0;
      for(typename std::list<T>::iterator it=ants_->begin();it!=ants_->end();it++) {
        average += it->get_tour_length();
      }
      average = average / ants_->size();

      for(typename std::list<T>::iterator it=ants_->begin();it!=ants_->end();it++) {
        standard_deviation += pow(it->get_tour_length() - average, 2);
      }
      standard_deviation *= 1.0 / ants_->size();
      standard_deviation = sqrt(standard_deviation);
      return standard_deviation / average;
    }

    double compute_lambda_branching_factor() {
      return pheromones_->average_lambda_branching_factor(0.05);
    }

    virtual void update_pheromones() = 0;
  protected:
    P *pheromones_;
    double alpha_;
    double beta_;
    AntColonyConfiguration::LocalSearchType local_search_type_;
    std::list<T> *ants_;
    OptimizationProblem *problem_;
    T *best_so_far_;
    T *best_iteration_;
    T *best_so_far_no_ls_;
    T *best_iteration_no_ls_;

  public:
    AntColony(OptimizationProblem *problem, const AntColonyConfiguration &config) {
      problem_ = problem;
      ants_ = new std::list<T>(config.number_of_ants, T(problem->get_max_tour_size()));
      pheromones_ = new P(problem->number_of_vertices()+1, config.evaporation_rate, config.initial_pheromone);
      alpha_ = config.alpha;
      beta_ = config.beta;
      local_search_type_ = config.local_search;
      best_so_far_ = new T(problem->get_max_tour_size());
      best_iteration_ = new T(problem->get_max_tour_size());
      best_so_far_no_ls_ = new T(problem->get_max_tour_size());
      best_iteration_no_ls_ = new T(problem->get_max_tour_size());
    }

    virtual ~AntColony() {
      delete problem_;
      delete ants_;
      delete pheromones_;
      delete best_so_far_;
      delete best_iteration_;
      delete best_so_far_no_ls_;
      delete best_iteration_no_ls_;
    };

    void run() {
      reset_ants();
      construct_ants_solutions();
      update_best_tours_no_ls();
      apply_local_search();
      update_best_tours();
      update_pheromones();
    }

    std::vector<unsigned int> get_best_tour() {
      return best_so_far_->get_vertices();
    }

    std::vector<unsigned int> get_best_tour_in_iteration() {
      return best_iteration_->get_vertices();
    }

    double get_best_tour_length() {
      return best_so_far_->get_tour_length();
    }

    double get_best_tour_length_in_iteration() {
      return best_iteration_->get_tour_length();
    }

    std::vector<unsigned int> get_best_tour_no_ls() {
      return best_so_far_no_ls_->get_vertices();
    }

    std::vector<unsigned int> get_best_tour_in_iteration_no_ls() {
      return best_iteration_no_ls_->get_vertices();
    }

    double get_best_tour_length_no_ls() {
      return best_so_far_no_ls_->get_tour_length();
    }

    double get_best_tour_length_in_iteration_no_ls() {
      return best_iteration_no_ls_->get_tour_length();
    }

    double get_variation_coefficient() {
      return compute_variation_coefficient();
    }

    double get_lambda_branching_factor() {
      return compute_lambda_branching_factor();
    }
};

/// Implementation of ACO variant "Simple Ant System".
///
/// In this ACO variant all ants are allowed to deposit pheromone. Tours are
/// constructed random-proportionally.
class SimpleAntColony : public AntColony<SimpleAnt> {
  public:
    SimpleAntColony(OptimizationProblem *problem, const AntColonyConfiguration &config);
  protected:
    void update_pheromones();
};

/// Implementation of ACO variant "Elitist Ant System".
///
/// In this ACO variant all ants are allowed to deposit pheromone. Additionally
/// the best-so-far ant is allowed to deposit pheromone multiplied by some 
/// elitist_weight_ in every iteration. Tours are constructed random-proportionally.
class ElitistAntColony : public AntColony<SimpleAnt> {
  private:
    double elitist_weight_;
  public:
    ElitistAntColony(OptimizationProblem *problem, const ElitistAntColonyConfiguration &config);
  protected:
    void update_pheromones();
};

/// Implementation of ACO variant "Rank-Based Ant System".
///
/// In this ACO variant only a constant number of ants is allowed to deposit
/// pheromone in every iteration. For example if elitist_ants_ = 5 then the
/// best-so-far ant deposits pheromone * 5, the iteration-best ant deposits
/// pheromone * 4, the second iteration-best ant deposits pheromone * 3 and so
/// on. Tours are constructe random-proporationallyly.
class RankBasedAntColony : public AntColony<SimpleAnt> {
  private:
    unsigned int elitist_ants_;
  public:
    RankBasedAntColony(OptimizationProblem *problem, const RankBasedAntColonyConfiguration &config);
  protected:
    void update_pheromones();
};

/// Implementation of ACO variant "Max-Min Ant System".
///
/// In this ACO variant either the best-so-far or the iteration-best ant is
/// allowed to deposit pheromone depending on the best_so_far_frequency. If the
/// best_so_far_frequency equals 3 then the best-so-far ant deposits it's
/// pheromone every third iteration instead of the iteration-best ant.
/// Upper and lower bounds on the pheromone values are introduced. The upper
/// bound is given by the initial pheromone value while the lower bound is
/// defined as: upper bound / a_. Tours are constructed random-proporationally.
class MaxMinAntColony : public AntColony<SimpleAnt, MaxMinPheromoneMatrix> {
  private:
    unsigned int best_so_far_frequency_;
    double a_;
  public:
    MaxMinAntColony(OptimizationProblem *problem, const MaxMinAntColonyConfiguration &config);
  protected:
    void update_pheromones();
};

/// Implementation of ACO variant "Ant Colony System".
///
/// In this ACO variant only the best-so-far ant is allowed to deposit
/// pheromone. Also pheromone evaporation only affects the edges chosen by the
/// best-so-far ant. Tour construction is done pseudo-random proportionally
/// meaning that with probability q0 the vertex with the best value is chosen
/// and with probability 1 - q0 the vertex with the best value is most likely
/// to be chosen by the ant during construction.
class ACSAntColony : public AntColony<ACSAnt, ACSPheromoneMatrix> {
  public:
    ACSAntColony(OptimizationProblem *problem, const ACSAntColonyConfiguration &config);
  protected:
    void update_pheromones();
};
