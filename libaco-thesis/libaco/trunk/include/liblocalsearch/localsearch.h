#include <vector>

class Neighbourhood {
  public:
    virtual void set_solution(std::vector<unsigned int> solution) = 0;
    virtual std::vector<unsigned int> get_solution() = 0;
    virtual bool has_next_neighbour_solution() = 0;
    virtual const std::vector<unsigned int> &next_neighbour_solution() = 0;
};

class TwoOptNeighbourhood : public Neighbourhood {
  private:
    std::vector<unsigned int> solution_;
    std::vector<unsigned int> neighbour_;
    unsigned int prev_i_, prev_j_, i_,j_;
  public:
    void set_solution(std::vector<unsigned int> solution);
    std::vector<unsigned int> get_solution();
    bool has_next_neighbour_solution();
    const std::vector<unsigned int> &next_neighbour_solution();
    void swap(std::vector<unsigned int> &solution, unsigned int i, unsigned int j);
};

class EvaluationFunction {
  public:
    virtual double eval_solution(const std::vector<unsigned int> &solution) = 0;
};

class LocalSearch {
  protected:
    std::vector<unsigned int> best_so_far_solution_;
    double best_so_far_quality_;
    EvaluationFunction *eval_func_;
    Neighbourhood *neighbourhood_;
  public:
    LocalSearch(std::vector<unsigned int> initial_solution, EvaluationFunction &eval_func, Neighbourhood &neighbourhood);
    virtual void search_neighbourhood() = 0;
};

class HillClimbing : public LocalSearch {
  public:
    HillClimbing(std::vector<unsigned int> initial_solution, EvaluationFunction &eval_func, Neighbourhood &neighbourhood);
    void search_neighbourhood();
};
