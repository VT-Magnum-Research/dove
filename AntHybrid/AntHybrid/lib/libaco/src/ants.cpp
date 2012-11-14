#include <climits>
#include <limits>
#include <iostream>
#include <list>
#include <cmath>
#include <cfloat>
#include <libaco/ants.h>
#include <libaco/util.h>

PheromoneMatrix::PheromoneMatrix(int vertices, double evaporation_rate, double initial_pheromone) : Matrix<double>(vertices, vertices, initial_pheromone) {
  evaporation_rate_ = evaporation_rate;
  initial_pheromone_ = initial_pheromone;
}

double PheromoneMatrix::get(unsigned int v, unsigned int w) {
  return (*matrix_)[v][w];
}

void PheromoneMatrix::add(unsigned int v, unsigned int w, double amount) {
  (*matrix_)[v][w] += amount;
}

void PheromoneMatrix::evaporate(unsigned int v, unsigned int w) {
  (*matrix_)[v][w] *= 1 - evaporation_rate_;
}

void PheromoneMatrix::evaporate_all() {
  for(unsigned int i=0;i<matrix_->size();i++) {
    for(unsigned int j=0;j<matrix_->size();j++) {
      evaporate(i,j);
    }
  }
}

double PheromoneMatrix::get_evaporation_rate() {
  return evaporation_rate_;
}

unsigned int PheromoneMatrix::size() {
  return matrix_->size();
}

double PheromoneMatrix::lambda_branching_factor(unsigned int v, double lambda) {
  double min_pheromone = DBL_MAX;
  double max_pheromone = 0.0;
  for(unsigned int i=0;i<this->size();i++) {
    double pheromone = (*matrix_)[v][i];
    if(min_pheromone > pheromone) {
      min_pheromone = pheromone;
    }

    if(max_pheromone < pheromone) {
      max_pheromone = pheromone;
    }
  }

  double limit = min_pheromone + lambda * (max_pheromone - min_pheromone);
  unsigned int branching_factor = 0;
  for(unsigned int j=0;j<this->size();j++) {
    if((*matrix_)[v][j] >= limit) {
      branching_factor++;
    }
  }
  return branching_factor;
}

double PheromoneMatrix::average_lambda_branching_factor(double lambda) {
  double sum = 0.0;
  for(unsigned int i=0;i<this->size();i++) {
    sum += lambda_branching_factor(i, lambda);
  }
  return sum / this->size();
}

MaxMinPheromoneMatrix::MaxMinPheromoneMatrix(int vertices, double evaporation_rate, double initial_pheromone) : PheromoneMatrix(vertices, evaporation_rate, initial_pheromone) {
}

void MaxMinPheromoneMatrix::set_min(double min) {
  min_ = min;
}

void MaxMinPheromoneMatrix::set_max(double max) {
  max_ = max;
}

void MaxMinPheromoneMatrix::add(unsigned int v, unsigned int w, double amount) {
  if((*matrix_)[v][w] + amount > max_) {
    (*matrix_)[v][w] = max_;
  } else {
    PheromoneMatrix::add(v, w, amount);
  }
}

void MaxMinPheromoneMatrix::evaporate(unsigned int v, unsigned int w) {
  if((*matrix_)[v][w] * (1 - evaporation_rate_) < min_) {
    (*matrix_)[v][w] = min_;
  } else {
    PheromoneMatrix::evaporate(v, w);
  }
}

ACSPheromoneMatrix::ACSPheromoneMatrix(int vertices, double evaporation_rate, double initial_pheromone) : PheromoneMatrix(vertices, evaporation_rate, initial_pheromone) {
}

void ACSPheromoneMatrix::set_xi(double xi) {
  xi_ = xi;
}

void ACSPheromoneMatrix::local_pheromone_update(unsigned int v, unsigned int w) {
  (*matrix_)[v][w] = (1 - xi_) * (*matrix_)[v][w] + xi_ * initial_pheromone_;
}

Tour::Tour(unsigned int vertices) {
  vertices_ = new std::vector<unsigned int>();
  capacity_ = vertices;
  length_ = UINT_MAX;
}

Tour::~Tour() {
  delete vertices_;
}

double Tour::get_length() {
  return length_;
}

const std::vector<unsigned int> &Tour::get_vertices() {
  return *vertices_;
}

void Tour::set_vertices(const std::vector<unsigned int> &vertices) {
  *vertices_ = vertices;
}

void Tour::set_length(double length) {
  length_ = length;
}

Tour &Tour::add_vertex(unsigned int vertex) {
  vertices_->push_back(vertex);
  return *this;
}

unsigned int Tour::size() const {
  return vertices_->size();
}

unsigned int Tour::capacity() {
  return capacity_;
}

unsigned int &Tour::operator[](const unsigned int vertex) const {
  return (*vertices_)[vertex];
}

void Tour::clear() {
  vertices_->clear();
  length_ = std::numeric_limits<double>::max();
}

Tour &Tour::operator=(const Tour &t) {
  this->vertices_->resize(t.size());
  for(unsigned int i=0;i<this->size();i++) {
    (*this)[i] = t[i];
  }
  this->length_ = t.length_;
  this->capacity_ = t.capacity_;
  return *this;
}

bool Tour::operator<(const Tour &t) {
  return this->length_ < t.length_;
}

Ant::Ant(unsigned int vertices) {
  tour = new Tour(vertices);
}

Ant::Ant(const Ant &ant) {
  tour = new Tour(ant.tour->capacity());
  for(unsigned int i=0;i<ant.tour->size();i++) {
    (*tour)[i] = (*ant.tour)[i];
  }
}

Ant &Ant::operator=(const Ant &ant) {
  (*tour) = (*ant.tour);
  return *this;
}

bool Ant::operator<(const Ant &ant) {
  return (*this->tour) < (*ant.tour);
}

Ant::~Ant() {
  delete tour;
}

double Ant::get_tour_length() {
  return tour->get_length();
}

std::vector<unsigned int> Ant::get_vertices() {
  return tour->get_vertices();
}

void Ant::add_vertex_to_tour(OptimizationProblem &op, unsigned int vertex) {
  tour->add_vertex(vertex);
  op.added_vertex_to_tour(vertex);
}

int Ant::current_vertex() {
  if(tour->size() > 0) {
    return (*tour)[tour->size()-1];
  } else {
    return -1;
  }
}

std::multimap<double,unsigned int,Ant::MultiMapComp> Ant::get_feasible_vertices(OptimizationProblem &op, PheromoneMatrix &pheromones, double alpha, double beta) {
  std::map<unsigned int,double> vertices;
  if(current_vertex() == -1) {
    vertices = op.get_feasible_start_vertices();
  } else {
    vertices = op.get_feasible_neighbours(current_vertex());
  }
  double denominator = 0.0;
  std::map<unsigned int,double>::iterator it;
  for(it=vertices.begin();it!=vertices.end();it++) {
    unsigned int vertex = (*it).first;
    double heuristic_value = (*it).second;
    if(current_vertex() == -1) {
      double numerator = pow(pheromones.get(pheromones.size()-1, vertex), alpha) * pow(heuristic_value, beta);
      (*it).second = numerator;
      denominator += numerator;
    } else {
      double numerator = pow(pheromones.get(current_vertex(), vertex), alpha) * pow(heuristic_value, beta);
      (*it).second = numerator;
      denominator += numerator;
    }
  }

  std::multimap<double,unsigned int,MultiMapComp> probabilities;
  for(it=vertices.begin();it!=vertices.end();it++) {
    //std::cout << pheromones.get(pheromones.size()-1, (*it).first) << std::endl;
    unsigned int vertex = (*it).first;
    //double heuristic_value = (*it).second;
    double probability;
    if(denominator == 0) {
      probability = 1.0 / vertices.size();
    } else {
      probability = (*it).second / denominator;
    }
    //std::cout << pheromones.get(current_vertex(), vertex) << " " << heuristic_value << std::endl;
    //std::cout << "vertex: " << vertex << " heuristic: " << heuristic_value << " probability: " << probability << std::endl;
    probabilities.insert(std::pair<double,unsigned int>(probability, vertex));
  }
  //std::cout << "***************************************" << std::endl;
  return probabilities;
}

void Ant::construct_random_proportional_solution(OptimizationProblem &op, PheromoneMatrix &pheromones, double alpha, double beta) {
  while(!op.is_tour_complete(tour->get_vertices())) {
    std::multimap<double,unsigned int,MultiMapComp> vertices = get_feasible_vertices(op, pheromones, alpha, beta);
    unsigned int vertex = choose_next_vertex_with_likelihood(vertices);
    add_vertex_to_tour(op, vertex);
  }
  update_tour_length(op);
  op.cleanup();
}

void Ant::construct_rational_solution(OptimizationProblem &op, PheromoneMatrix &pheromones, double alpha, double beta) {
  while(!op.is_tour_complete(tour->get_vertices())) {
    std::multimap<double,unsigned int,MultiMapComp> vertices = get_feasible_vertices(op, pheromones, alpha, beta);
    unsigned int vertex = (*vertices.begin()).second;
    add_vertex_to_tour(op, vertex);
  }
  update_tour_length(op);
  op.cleanup();
}

void Ant::update_tour_length(OptimizationProblem &op) {
  tour->set_length(op.eval_tour(tour->get_vertices()));
}


bool Ant::MultiMapComp::operator()(const double &p1,const double &p2) const {
  return p1 > p2;
}

unsigned int Ant::choose_next_vertex_with_likelihood(std::multimap<double,unsigned int,MultiMapComp> probabilities) {
  unsigned int vertex = -1;
  unsigned int random_value = Util::random_number(RAND_MAX);
  double tmp = 0;
  std::vector<unsigned int> vertices;
  for(std::multimap<double,unsigned int,MultiMapComp>::iterator it=probabilities.begin();it!=probabilities.end();) {
    vertices.clear();
    double probability = (*it).first;
    while(it!=probabilities.end() && (*it).first == probability) {
      vertices.push_back((*it).second);
      it++;
    }
    if(tmp+probability*vertices.size()*RAND_MAX > random_value) {
      unsigned int v = Util::random_number(vertices.size());
      vertex = vertices[v];
      break;
    }
    tmp += probability*vertices.size()*RAND_MAX;
  }
  return vertex;
}

void Ant::apply_local_search(OptimizationProblem &op) {
  std::vector<unsigned int> vertices = op.apply_local_search(tour->get_vertices());
  tour->set_vertices(vertices);
  update_tour_length(op);
}

void Ant::reset() {
  tour->clear();
}

SimpleAnt::SimpleAnt(unsigned int vertices) : Ant(vertices) {
}

SimpleAnt::SimpleAnt(const SimpleAnt &ant) : Ant(ant) {
}

SimpleAnt &SimpleAnt::operator=(const SimpleAnt &ant) {
  Ant::operator=(ant);
  return (*this);
}

void SimpleAnt::construct_solution(OptimizationProblem &op, PheromoneMatrix &pheromones, double alpha, double beta) {
  construct_random_proportional_solution(op, pheromones, alpha, beta);
}

void SimpleAnt::offline_pheromone_update(OptimizationProblem &op, PheromoneMatrix &pheromones, double weight) {
  for(unsigned int i=0;i<tour->size();i++) {
    double pheromone = op.pheromone_update((*tour)[i], tour->get_length());
    if(i==0) {
      pheromones.add(pheromones.size()-1, (*tour)[i], weight * pheromone);
    } else {
      pheromones.add((*tour)[i-1], (*tour)[i], weight * pheromone);
    }
  }
}

ACSAnt::ACSAnt(unsigned int vertices) : Ant(vertices) {
}

ACSAnt::ACSAnt(const ACSAnt &ant) : Ant(ant) {
}

ACSAnt &ACSAnt::operator=(const ACSAnt &ant) {
  Ant::operator=(ant);
  return (*this);
}

void ACSAnt::construct_pseudorandom_proportional_solution(OptimizationProblem &op, PheromoneMatrix &pheromones, double alpha, double beta) {
  while(!op.is_tour_complete(tour->get_vertices())) {
    std::multimap<double,unsigned int,MultiMapComp> vertices = get_feasible_vertices(op, pheromones, 1.0, beta);
    unsigned int q = Util::random_number(RAND_MAX);
    double q0 = q0_ * RAND_MAX;
    unsigned int vertex;
    if (q < q0) {
      vertex = (*vertices.begin()).second;
    } else {
      vertex = choose_next_vertex_with_likelihood(vertices);
    }
    add_vertex_to_tour(op, vertex);
  }
  update_tour_length(op);
  op.cleanup();

  //local pheromone update
  for(unsigned int i=0;i<tour->size();i++) {
    if(i==0) {
      ((ACSPheromoneMatrix &) pheromones).local_pheromone_update(pheromones.size()-1, (*tour)[i]);
    } else {
      ((ACSPheromoneMatrix &) pheromones).local_pheromone_update((*tour)[i-1], (*tour)[i]);
    }
  }
}

void ACSAnt::construct_solution(OptimizationProblem &op, PheromoneMatrix &pheromones, double alpha, double beta) {
  construct_pseudorandom_proportional_solution(op, pheromones, alpha, beta);
}

void ACSAnt::offline_pheromone_update(OptimizationProblem &op, PheromoneMatrix &pheromones, double weight) {
  for(unsigned int i=0;i<tour->size();i++) {
    double pheromone = op.pheromone_update((*tour)[i], tour->get_length());
    if(i==0) {
      pheromones.add(pheromones.size()-1, (*tour)[i], weight * pheromones.get_evaporation_rate() * pheromone);
    } else {
      pheromones.add((*tour)[i-1], (*tour)[i], weight * pheromones.get_evaporation_rate() * pheromone);
    }
  }
}

void ACSAnt::set_q0(double q0) {
  q0_ = q0;
}

double compute_average_pheromone_update(OptimizationProblem &op) {
  SimpleAnt ant(op.get_max_tour_size());
  PheromoneMatrix matrix(op.number_of_vertices(), 0.0, 1.0);
  ant.construct_rational_solution(op, matrix, 0, 1);
  std::vector<unsigned int> tour = ant.get_vertices();
  double tour_length = op.eval_tour(tour);
  double pheromone_sum = 0.0;
  for(unsigned int i=0;i<tour.size();i++) {
    pheromone_sum += op.pheromone_update(tour[i], tour_length);
  }
  double pheromone_avg = pheromone_sum / tour.size();
  return pheromone_avg;
}

AntColonyConfiguration::AntColonyConfiguration() {
  number_of_ants = 20;
  alpha = 2.0;
  beta = 5.0;
  evaporation_rate = 0.1;
  initial_pheromone = 1.0;
  local_search = LS_ITERATION_BEST;
}

ElitistAntColonyConfiguration::ElitistAntColonyConfiguration() : AntColonyConfiguration() {
  elitist_weight = 2.0;
}

RankBasedAntColonyConfiguration::RankBasedAntColonyConfiguration() : AntColonyConfiguration() {
  elitist_ants = 1;
}

MaxMinAntColonyConfiguration::MaxMinAntColonyConfiguration() : AntColonyConfiguration() {
  best_so_far_frequency = 3;
  a = 5;
}

ACSAntColonyConfiguration::ACSAntColonyConfiguration() : AntColonyConfiguration() {
  xi = 0.1;
  q0 = 0.5;
}

SimpleAntColony::SimpleAntColony(OptimizationProblem *problem, const AntColonyConfiguration &config) : AntColony<SimpleAnt>(problem, config) {
}

void SimpleAntColony::update_pheromones() {
  pheromones_->evaporate_all();
  for(std::list<SimpleAnt>::iterator it=ants_->begin();it!=ants_->end();it++) {
    SimpleAnt &ant = (*it);
    ant.offline_pheromone_update(*problem_, *pheromones_);
  }
}

ElitistAntColony::ElitistAntColony(OptimizationProblem *problem, const ElitistAntColonyConfiguration &config) : AntColony<SimpleAnt>(problem, config) {
  elitist_weight_ = config.elitist_weight;
}

void ElitistAntColony::update_pheromones() {
  pheromones_->evaporate_all();
  best_so_far_->offline_pheromone_update(*problem_, *pheromones_, elitist_weight_);
  for(std::list<SimpleAnt>::iterator it=ants_->begin();it!=ants_->end();it++) {
    SimpleAnt &ant = (*it);
    ant.offline_pheromone_update(*problem_, *pheromones_);
  }
}

RankBasedAntColony::RankBasedAntColony(OptimizationProblem *problem, const RankBasedAntColonyConfiguration &config) : AntColony<SimpleAnt>(problem, config) {
  elitist_ants_ = config.elitist_ants;
}

void RankBasedAntColony::update_pheromones() {
  pheromones_->evaporate_all();
  best_so_far_->offline_pheromone_update(*problem_, *pheromones_, elitist_ants_);
  unsigned int rank = 1;
  for(std::list<SimpleAnt>::iterator it=ants_->begin();it!=ants_->end() && rank != elitist_ants_;it++) {
    SimpleAnt &ant = (*it);
    ant.offline_pheromone_update(*problem_, *pheromones_, elitist_ants_ - rank);
    rank++;
  }
}

MaxMinAntColony::MaxMinAntColony(OptimizationProblem *problem, const MaxMinAntColonyConfiguration &config) : AntColony<SimpleAnt, MaxMinPheromoneMatrix>(problem, config) {
  best_so_far_frequency_ = config.best_so_far_frequency;
  a_ = config.a;
  pheromones_->set_min(config.initial_pheromone / a_);
  pheromones_->set_max(config.initial_pheromone);
}

void MaxMinAntColony::update_pheromones() {
  static unsigned int i = 0;
  pheromones_->evaporate_all();
  unsigned int every_n_iter = best_so_far_frequency_;
  if(i % every_n_iter == 0) {
    best_so_far_->offline_pheromone_update(*problem_, *pheromones_);
  } else {
    best_iteration_->offline_pheromone_update(*problem_, *pheromones_);
  }
  i++;
}

ACSAntColony::ACSAntColony(OptimizationProblem *problem, const ACSAntColonyConfiguration &config) : AntColony<ACSAnt, ACSPheromoneMatrix>(problem, config) {
  pheromones_->set_xi(config.xi);
  for(std::list<ACSAnt>::iterator it=ants_->begin();it!=ants_->end();it++) {
    ACSAnt &ant = (*it);
    ant.set_q0(config.q0);
  }
}

void ACSAntColony::update_pheromones() {
  std::vector<unsigned int> vertices = best_so_far_->get_vertices();
  for(unsigned int i=0;i<vertices.size();i++) {
    if(i==0) {
      pheromones_->evaporate(pheromones_->size()-1, vertices[i]);
    } else {
      pheromones_->evaporate(vertices[i-1], vertices[i]);
    }
  }
  best_so_far_->offline_pheromone_update(*problem_, *pheromones_);
}
