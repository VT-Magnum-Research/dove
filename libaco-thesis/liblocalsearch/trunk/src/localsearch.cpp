#include <iostream>
#include <liblocalsearch/localsearch.h>

unsigned int random_number(unsigned int range) {
  static bool seeded = false;
  if(!seeded) {
    srand(time(0));
    seeded = true;
  }
  return (rand() % range);
}

void Neighbourhood::reset() {
  this->set_solution(this->get_solution());
}

void TwoOptNeighbourhood::set_solution(std::vector<unsigned int> solution) {
  solution_ = solution;
  neighbour_ = solution;
  i_ = 0;
  j_ = 1;
  prev_i_ = 0;
  prev_j_ = 0;
}

std::vector<unsigned int> TwoOptNeighbourhood::get_solution() {
  return solution_;
}

bool TwoOptNeighbourhood::has_next_neighbour_solution() {
  return j_ < solution_.size();
}

std::vector<unsigned int> TwoOptNeighbourhood::next_neighbour_solution() {
  swap(neighbour_, prev_i_, prev_j_);
  swap(neighbour_, i_, j_);
  prev_i_ = i_;
  prev_j_ = j_;
  if(j_ < solution_.size()-1) {
    j_++;
  } else {
    i_++;
    j_ = i_+1;
  }
  return neighbour_;
}

void TwoOptNeighbourhood::swap(std::vector<unsigned int> &solution, unsigned int i, unsigned int j) {
  unsigned int tmp = solution[i];
  solution[i] = solution[j];
  solution[j] = tmp;
}

LocalSearch::LocalSearch(std::vector<unsigned int> initial_solution, EvaluationFunction &eval_func, Neighbourhood &neighbourhood) {
  best_so_far_solution_ = initial_solution;
  best_so_far_quality_ = eval_func.eval_solution(best_so_far_solution_);
  eval_func_ = &eval_func;
  neighbourhood_ = &neighbourhood;
  neighbourhood_->set_solution(initial_solution);
}

std::vector<unsigned int> LocalSearch::get_best_so_far_solution() {
  return best_so_far_solution_;
}

double LocalSearch::get_best_so_far_quality() {
  return best_so_far_quality_;
}

void LocalSearch::set_solution(std::vector<unsigned int> solution) {
  best_so_far_solution_ = solution;
  best_so_far_quality_ = this->eval_solution(solution);
  neighbourhood_->set_solution(solution);
}

double LocalSearch::eval_solution(const std::vector<unsigned int> &solution) {
  return eval_func_->eval_solution(solution);
}

void LocalSearch::search_iterations(int iterations) {
  for(int i=0;i<iterations;i++) {
    search_neighbourhood();
  }
}

void LocalSearch::search_iterations_without_improve(int iterations_without_improve) {
  int no_improve_counter = 0;
  double quality = best_so_far_quality_;
  while(no_improve_counter < iterations_without_improve) {
    search_neighbourhood();

    if(best_so_far_quality_ > quality) {
      no_improve_counter = 0;
      quality = best_so_far_quality_;
    } else {
      no_improve_counter++;
    }
  }
}

HillClimbing::HillClimbing(std::vector<unsigned int> initial_solution, EvaluationFunction &eval_func, Neighbourhood &neighbourhood) : LocalSearch(initial_solution, eval_func, neighbourhood) {
}

void HillClimbing::search_neighbourhood() {
  while(neighbourhood_->has_next_neighbour_solution()) {
    const std::vector<unsigned int> &solution = neighbourhood_->next_neighbour_solution();
    double quality = eval_func_->eval_solution(solution);
    if(quality > best_so_far_quality_) {
      best_so_far_solution_ = solution;
      best_so_far_quality_ = quality;
      neighbourhood_->set_solution(best_so_far_solution_);
      break;
    }
  }
  neighbourhood_->reset();
}

IterativeLocalSearch::IterativeLocalSearch(LocalSearch *local_search, IterativeLocalSearchClient *it_ls_client) {
  local_search_ = local_search;
  it_ls_client_ = it_ls_client;
  best_solution_ = local_search->get_best_so_far_solution();
  best_quality_ = local_search->get_best_so_far_quality();
}

void IterativeLocalSearch::run(int iterations_without_improve, int ls_iterations_without_improve) {
  std::vector<unsigned int> best_solution = local_search_->get_best_so_far_solution();
  std::vector<unsigned int> next_solution;
  double best_quality_ = local_search_->get_best_so_far_quality();
  int no_improve_counter = 0;
  while(no_improve_counter < iterations_without_improve) {
    local_search_->search_iterations_without_improve(ls_iterations_without_improve);
    std::vector<unsigned int> new_solution = local_search_->get_best_so_far_solution();
    double new_quality = local_search_->get_best_so_far_quality();
    if(new_quality > best_quality_) {
      best_quality_ = new_quality;
      best_solution_ = new_solution;
      no_improve_counter = 0;
    } else {
      next_solution = best_solution_;
      no_improve_counter++;
    }

    if(it_ls_client_->is_solution_accepted(new_quality, best_quality_)) {
      next_solution = new_solution;
    }

    next_solution = it_ls_client_->perturbate(next_solution);
    local_search_->set_solution(next_solution);
  }
}

std::vector<unsigned int> IterativeLocalSearch::get_best_solution() {
  return this->best_solution_;
}
