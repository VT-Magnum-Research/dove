#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cfloat>
#include <climits>
#include <csignal>
#include "tclap/CmdLine.h"
#include "ants.h"

#include "mps.h"

enum StagnationMeasureType { STAG_NONE, STAG_VARIATION_COEFFICIENT, STAG_LAMBDA_BRANCHING_FACTOR };

static std::string filepath;
static std::string out_filepath;
static unsigned int ants = 10;
static unsigned int iterations = UINT_MAX;
static double alpha = 1.0;
static double beta = 1.0;
static double rho = 0.1;
static double initial_pheromone = -1.0;
static bool print_tour_flag = false;
static bool stag_variance_flag = false;
static bool stag_lambda_flag = false;
static StagnationMeasureType stagnation_measure = STAG_NONE;
static double time_limit = DBL_MAX;
static bool simple_as_flag = false;
static bool elitist_as_flag = false;
static bool rank_as_flag = false;
static bool maxmin_as_flag = false;
static bool acs_as_flag = false;

static unsigned int cores_used = 2;
static unsigned int processor_heterogenity = 1;
static bool use_grid_routing = false;
static unsigned int routing_heterogenity = 1;
static unsigned int routing_default=0;
static unsigned int task_heterogenity = 1;

static double elitist_weight = 2.0;
static unsigned int ranked_ants = 1;

static unsigned int maxmin_frequency = 5;
static double maxmin_a = 2.0;

static double acs_q0 = 0.5;
static double acs_xi = 0.1;

static AntColony<Ant> *colony;
static MpsProblem* mpsproblem;

static void parse_options(int argc, char *argv[]) {
  TCLAP::CmdLine cmd("Ant Colony Optimization for the Multiprocessor Scheduling Problem", ' ', "0.1");
  TCLAP::ValueArg<unsigned int> ants_arg ("m", "ants", "number of ants", false, ants, "integer");
  TCLAP::ValueArg<unsigned int> iterations_arg ("i", "iterations", "number of iterations", false, iterations, "positive integer");
  TCLAP::ValueArg<double> alpha_arg ("a", "alpha", "alpha (influence of pheromone trails)", false, alpha, "double");
  TCLAP::ValueArg<double> beta_arg("b", "beta", "beta (influence of heuristic information)", false, beta, "double");
  TCLAP::ValueArg<double> rho_arg("r", "rho", "pheromone trail evaporation rate", false, rho, "double");
  TCLAP::ValueArg<double> initial_pheromone_arg("p", "pheromone", "initial pheromone value", false, initial_pheromone, "double");
  std::vector<unsigned int> allowed;
  allowed.push_back(0);
  allowed.push_back(1);
  TCLAP::ValuesConstraint<unsigned int> allowed_values( allowed );
  TCLAP::ValueArg<std::string> filepath_arg("f", "file", "path to the input file", true, "", "filepath");
  TCLAP::ValueArg<unsigned int> cores_used_arg("c", "cores", "number of homogeneous processing cores. Defaults to 2", false, 2, "positive integer");
  TCLAP::ValueArg<unsigned int> processor_h_arg("","core_heter", "Processor heterogeneity. 1 specifies homogeneous processors, <int> specifies a limit on processor upper bound that is randomly queried to build a set of heterogeneous processors. Default is 1", false, 1, "positive integer");
  TCLAP::SwitchArg print_tour_arg("o", "printord", "print best elimination ordering in iteration");
  TCLAP::ValueArg<unsigned int> task_harg("", "task_heter", "task heterogeneity. 1 specifies to leave task homogenity alone, <int> specifies a limit on the upper bound a task completion time can be multiplied by. Default is 1", false, 1, "positive integer");
  
  TCLAP::SwitchArg is_gridroute("g","use_grid_routing", "Should we use the default (routing heterogeneity route cost calculation), or NSEW grid routing route cost calculation e.g. taxicab distance between cores * route_default. Note that using grid routing requires the number of cores to be evenly sqrt-able e.g. 4, 9, 16, 25, 36, etc...");
    
  TCLAP::ValueArg<unsigned int> routing_h_arg("","routing_heter", "routing heterogeneity. 1 specifies homogeneous routing delay, <int> specifies a limit on routing delay upper bounds that is randomly queried to build a set of routing delays between the processors. Default is 1", false, 1, "positive integer");
  TCLAP::ValueArg<unsigned int> routing_def_arg("","route_default", "base routing cost between cores. Used as base for both routing heterogeneity and grid routing. Default is 0", false, 0, "positive integer");
  
  TCLAP::ValueArg<std::string> out_filepath_arg("", "outfile", "path to store a detailed output file, which will include the full final mapping and the full makespan calculation. Allows redirection to file,stdout,stderr,or none (no detailed output). Default is none", false, "none", "filepath|out|err|none");
  
  TCLAP::SwitchArg stag_variance_arg("", "stag_variance", "compute and print variation coefficient stagnation");
  TCLAP::SwitchArg stag_lambda_arg("", "stag_lambda", "compute and print lambda branching factor stagnation");
  TCLAP::ValueArg<double> time_limit_arg("t", "time", "terminate after n seconds (after last iteration is finished)", false, time_limit, "double");
  TCLAP::SwitchArg simple_as_arg("", "simple", "use Simple Ant System");
  TCLAP::ValueArg<double> elitist_as_arg("", "elitist", "use Elitist Ant System with given weight", false, elitist_weight, "double");
  TCLAP::ValueArg<unsigned int> rank_as_arg("", "rank", "use Rank-Based Ant System and let the top n ants deposit pheromone", false, ranked_ants, "positive integer");
  TCLAP::SwitchArg maxmin_as_arg("", "maxmin", "use Max-Min Ant System");
  TCLAP::ValueArg<unsigned int> maxmin_frequency_arg("", "maxmin_frequency", "frequency of pheromone updates of best-so-far ant in Max-Min Ant System", false, maxmin_frequency, "double");
  TCLAP::ValueArg<double> maxmin_a_arg("", "maxmin_a", "parameter a in Max-Min Ant System", false, maxmin_a, "double");
  TCLAP::SwitchArg acs_as_arg("", "acs", "use Ant Colony System");
  TCLAP::ValueArg<double> acs_q0_arg("", "acs_q0", "q0 parameter for Ant Colony System", false, acs_q0, "double");
  TCLAP::ValueArg<double> acs_xi_arg("", "acs_xi", "xi parameter for Ant Colony System", false, acs_xi, "double");
  std::vector<TCLAP::Arg *> as_variants;
  as_variants.push_back(&simple_as_arg);
  as_variants.push_back(&elitist_as_arg);
  as_variants.push_back(&rank_as_arg);
  as_variants.push_back(&maxmin_as_arg);
  as_variants.push_back(&acs_as_arg);
  cmd.add(ants_arg);
  cmd.add(iterations_arg);
  cmd.add(alpha_arg);
  cmd.add(beta_arg);
  cmd.add(rho_arg);
  cmd.add(initial_pheromone_arg);
  cmd.add(filepath_arg);
  cmd.add(print_tour_arg);
  cmd.add(stag_variance_arg);
  cmd.add(stag_lambda_arg);
  cmd.add(time_limit_arg);
  cmd.add(maxmin_frequency_arg);
  cmd.add(maxmin_a_arg);
  cmd.add(acs_q0_arg);
  cmd.add(acs_xi_arg);
  cmd.xorAdd(as_variants);
  
  cmd.add(cores_used_arg);
  cmd.add(processor_h_arg);
  cmd.add(is_gridroute);
  cmd.add(routing_h_arg);
  cmd.add(routing_def_arg);
  cmd.add(task_harg);
  cmd.add(out_filepath_arg);

  cmd.parse(argc, argv);
  
  ants = ants_arg.getValue();
  iterations = iterations_arg.getValue();
  alpha = alpha_arg.getValue();
  beta = beta_arg.getValue();
  rho = rho_arg.getValue();
  initial_pheromone = initial_pheromone_arg.getValue();
  filepath = filepath_arg.getValue();
  print_tour_flag = print_tour_arg.getValue();
  stag_variance_flag = stag_variance_arg.getValue();
  stag_lambda_flag = stag_lambda_arg.getValue();
  time_limit = time_limit_arg.getValue();
  simple_as_flag = simple_as_arg.isSet();
  elitist_as_flag = elitist_as_arg.isSet();
  elitist_weight = elitist_as_arg.getValue();
  rank_as_flag = rank_as_arg.isSet();
  ranked_ants = rank_as_arg.getValue();
  maxmin_as_flag = maxmin_as_arg.isSet();
  maxmin_frequency = maxmin_frequency_arg.getValue();
  maxmin_a = maxmin_a_arg.getValue();
  acs_as_flag = acs_as_arg.isSet();
  acs_q0 = acs_q0_arg.getValue();
  acs_xi = acs_xi_arg.getValue();
  cores_used = cores_used_arg.getValue();
  processor_heterogenity=processor_h_arg.getValue();
  routing_heterogenity=routing_h_arg.getValue();
  routing_default=routing_def_arg.getValue();
  task_heterogenity = task_harg.getValue();
  use_grid_routing = is_gridroute.getValue();
  out_filepath = out_filepath_arg.getValue();
  
  if(stag_variance_arg.isSet()) {
    stagnation_measure = STAG_VARIATION_COEFFICIENT;
  } else if(stag_lambda_arg.isSet()) {
    stagnation_measure = STAG_LAMBDA_BRANCHING_FACTOR;
  }
}

static void set_config(AntColonyConfiguration &config) {
  config.number_of_ants = ants;
  config.alpha = alpha;
  config.beta = beta;
  config.evaporation_rate = rho;
  if (initial_pheromone != -1)
    config.initial_pheromone = initial_pheromone;
  else
    config.initial_pheromone = 1;
}

static void set_initial_pheromone(OptimizationProblem *problem, AntColonyConfiguration &config) {
  //if(config.initial_pheromone == -1.0) {
  //  double initial_pheromone = compute_average_pheromone_update(*problem) * config.number_of_ants;
  //  config.initial_pheromone = initial_pheromone * (1.0 / ants);
  //}
  config.initial_pheromone = 1;
}

AntColony<Ant> *get_ant_colony(OptimizationProblem *problem) {
  AntColony<Ant> *colony;
  
  if(simple_as_flag) {
    AntColonyConfiguration config;
    set_config(config);
    set_initial_pheromone(problem, config);
    colony = (AntColony<Ant> *) new SimpleAntColony(problem, config);
  } else if(elitist_as_flag) {
    ElitistAntColonyConfiguration config;
    set_config(config);
    set_initial_pheromone(problem, config);
    config.elitist_weight = elitist_weight;
    colony = (AntColony<Ant> *) new ElitistAntColony(problem, config);
  } else if(rank_as_flag) {
    RankBasedAntColonyConfiguration config;
    set_config(config);
    set_initial_pheromone(problem, config);
    config.elitist_ants = ranked_ants;
    colony = (AntColony<Ant> *) new RankBasedAntColony(problem, config);
  } else if(maxmin_as_flag) {
    MaxMinAntColonyConfiguration config;
    set_config(config);
    set_initial_pheromone(problem, config);
    config.best_so_far_frequency = maxmin_frequency;
    config.a = maxmin_a;
    colony = (AntColony<Ant> *) new MaxMinAntColony(problem, config);
  } else if(acs_as_flag) {
    ACSAntColonyConfiguration config;
    set_config(config);
    set_initial_pheromone(problem, config);
    config.q0 = acs_q0;
    config.xi = acs_xi;
    colony = (AntColony<Ant> *) new ACSAntColony(problem, config);
  }
  return colony;
}

static void terminate(int signal) {
  //std::cout << std::endl;
  //std::cout << "best\tordering" << std::endl;
  //std::cout << colony->get_best_tour_length() << "\t";
  //mpsproblem->print_tour(colony->get_best_tour());
  //std::cout << std::endl;
  delete colony;
  exit(EXIT_FAILURE);
}

double timer() {
  static bool initialized_time = false;
  static clock_t time;
  if(!initialized_time) {
    time = clock();
    initialized_time = true;
    return 0.0;
  } else {
    clock_t time_diff = clock() - time;
    double elapsed_time = time_diff * 1.0 / CLOCKS_PER_SEC;
    return elapsed_time;
  }
}

double timer2() {
  static bool initialized_time2 = false;
  static clock_t time2;
  if(!initialized_time2) {
    time2 = clock();
    initialized_time2 = true;
    return 0.0;
  } else {
    clock_t time_diff = clock() - time2;
    double elapsed_time = time_diff * 1.0 / CLOCKS_PER_SEC;
    return elapsed_time;
  }
}


bool precedence_sort(Task a, Task b) {
  return a.pred_level_ < b.pred_level_;
}

bool identifier_sort(Task a, Task b) {
  return a.int_identifier_ < b.int_identifier_;
}

double run_entire_aco(MpsProblem* problem) {
  
  //problem->get_feasible_start_vertices();
  std::ostream* info = &std::cerr;
  
  *info << "iter\ttime\tbest\tbest_it\talpha\tbeta";
  *info << ((stagnation_measure != STAG_NONE) ? "\tstagnation" : "");
  *info << (print_tour_flag ? "\tordering" : "");
  *info << std::endl;
  
  timer();
  timer2();
  for(unsigned int i=0;i<iterations && timer() < time_limit;i++) {
    colony->run();
    *info << (i+1) << "\t";
    *info << timer() << "\t";
    *info << colony->get_best_tour_length() << "\t";
    *info << colony->get_best_tour_length_in_iteration() << "\t";
    *info << colony->alpha_ << "\t" << colony->beta_ << "\t";
    
    // colony->alpha_ = colony->alpha_ * 1.25;
    //if (colony->beta_ != 0)
    //  colony->beta_ = colony->beta_ - 10;
    
    if(stagnation_measure == STAG_VARIATION_COEFFICIENT) {
      *info << colony->get_variation_coefficient();
    }
    
    if(stagnation_measure == STAG_LAMBDA_BRANCHING_FACTOR) {
      *info << colony->get_lambda_branching_factor();
    }
    
    if(print_tour_flag) {
      //std::cout << "\t";
      // TODO mpsproblem->print_tour(colony->get_best_tour_in_iteration());
    }
    
    *info << std::endl;
  }
  *info << std::endl;
  *info << "best\tordering" << std::endl;
  *info << colony->get_best_tour_length() << "," << timer2();
  
  std::cout << colony->get_best_tour_length() << "," << timer2();
  
  //mpsproblem->print_tour(colony->get_best_tour());
  
  //MpSchedule schedule = mpsproblem->convert_tour_to_schedule(colony->get_best_tour());
  //bool verify = mpsproblem->verify_schedule_passes_constraints(schedule);
  
  //schedule.print_schedule_as_page();
  //return colony->get_best_tour_length();
  
  return timer2();
}

double unifRand()
{
  return rand() / double(RAND_MAX);
}
//
// Generate a random number in a real interval.
// param a one end point of the interval
// param b the other end of the interval
// return a inform rand numberin [a,b].
unsigned int unifRand(double a, double b)
{
  return (unsigned int)((b-a)*1.0*unifRand() + a);
}

unsigned int task_count;
unsigned int core_count;
inline unsigned int get_vertex_for(unsigned int core, unsigned int task) {
  return core * task_count + task;
}

inline void get_task_and_core_from_vertex(unsigned int vertex_id, unsigned int & task, unsigned int & core) {
  task = vertex_id % task_count;
  core = vertex_id / task_count;
}

// if the difference of old score and new_score is < 500 and current_temp is <=1000, then
// this will always return P[0...1]. P[1] is returned if the move is better, and
// a worse move will be accepted (with a max probability of ~0.5) based upon the current
// temperature
inline double should_accept(double old_energy, double new_energy, int current_temp) {
  if (new_energy < old_energy)
    return new_energy;
  return 1.0 / (1.0 + exp((old_energy - new_energy) / current_temp));
}


// Returns the best found mapping
std::vector<unsigned int>* run_annealing(DirectedAcyclicGraph* task_precedence,
                   SymmetricMatrix<unsigned int>* routing_costs,
                   Matrix<unsigned int>* run_times,
                   std::vector<unsigned int>* task_scheduling_order,
                   double& annealing_makespan) {
  
  std::ostream* info = &std::cerr;
  
  *info << "Running anneal" << std::endl;
  
  MpsProblem *problem = new MpsProblem(routing_costs, run_times, task_precedence, task_scheduling_order);
  // Perform a little setup so that we can re-use the methods I've already got for
  // schedule evaluation
  AntColony<Ant>* initial_colony = get_ant_colony(problem);
  initial_colony->set_local_search_type(AntColonyConfiguration::LS_NONE);
  problem->set_ant_colony(initial_colony);
  initial_colony->run();
  
  std::vector<unsigned int> tour = initial_colony->get_best_tour_no_ls();

  // We always remember the absolute best we did :-)
  double global_best = problem->eval_tour(tour);
  std::vector<unsigned int>* global_best_mapping = new std::vector<unsigned int>(problem->get_mapping_from_task_to_core().size());
  std::vector<unsigned int> global_best_tour(tour);
  
  // Emperically determined
  unsigned int temp = 5000;
  unsigned int counts_until_restart = 200;
  double current_solution_score = global_best;
  unsigned int print_flag = 250;
  while (temp-- != 0) {
  
    if (print_flag-- == 0) {
      print_flag = 750;
      *info << "Temperature is now " << temp << " and local best is " << current_solution_score << std::endl;
    }
    
    // Randomly choose a vertex to modify
    unsigned int vertex_pos = 0;
    while (vertex_pos == 0 || vertex_pos == task_count-1)
      vertex_pos = (int) (rand() % (int) tour.size());
    
    unsigned int vertex = tour[vertex_pos];
    unsigned int task,core;
    get_task_and_core_from_vertex(vertex, task, core);
      
    // Randomly choose a new core for the task
    unsigned int new_core = core;
    while (new_core == core)
      new_core = rand() % core_count;
      
    // Build the new vertex
    unsigned int new_vertex = get_vertex_for(new_core, task);
    tour[vertex_pos] = new_vertex;
    problem->get_mapping_from_task_to_core()[task] = new_core;
      
    // Evaluate the new tour
    double new_time = problem->eval_tour(tour);

    // Don't waste time copying the best until we've made some improvements
    if (new_time < global_best) {
      global_best = new_time;
      counts_until_restart = 200;
      if (temp < 3500) {
        *info << "Updated Global Best " << global_best << std::endl;
        global_best_mapping->assign(problem->get_mapping_from_task_to_core().begin(),
                                   problem->get_mapping_from_task_to_core().end());
        global_best_tour.assign(tour.begin(), tour.end());
      }
    } else if (temp == 3500) {
      global_best_mapping->assign(problem->get_mapping_from_task_to_core().begin(),
                                 problem->get_mapping_from_task_to_core().end());
      global_best_tour.assign(tour.begin(), tour.end());
    }
    
    if (should_accept(current_solution_score, new_time, temp) > (rand() / RAND_MAX)) {
      current_solution_score = new_time;
    } else { // undo
      tour[vertex_pos] = vertex;
      problem->get_mapping_from_task_to_core()[task] = core;
      
      if (temp < 3500 && --counts_until_restart == 0) {
        problem->get_mapping_from_task_to_core().assign(global_best_mapping->begin(),
                                                        global_best_mapping->end());
        tour.assign(global_best_tour.begin(), global_best_tour.end());
        current_solution_score = global_best;
        counts_until_restart = 200;
      }
    }
  }

  std::cerr << "Found global best of " << global_best << " using the anneal" << std::endl;
  annealing_makespan = global_best;

  // Takes care of deleting the problem
  delete initial_colony;
  
  return global_best_mapping;
}

void print_detailed_schedule(std::vector<unsigned int> tour, std::ostream &out, DirectedAcyclicGraph* task_precedence,
                             SymmetricMatrix<unsigned int>* routing_costs,
                             Matrix<unsigned int>* run_times,
                             std::vector<Task>* tasks) {

  // Put tasks back into linear order
  std::sort(tasks->begin(), tasks->end(), identifier_sort);
  
  // Flatten the scheduling order into a real schedule
  MpSchedule schedule(routing_costs, run_times, task_precedence);
  std::vector<unsigned int>::const_iterator it = tour.begin();
  unsigned int task_i, core_i;
  Task* task;
  
  out << "# " << std::endl;
  out << "# Task-Order Fully Detailed Printing" << std::endl;
  out << "# Printed in the order items are scheduled" << std::endl;
  out << "# Format is Task <<task>>, Core <<core>>\\n\\t<<delay item>>" << std::endl;
  out << "# Any delay item that delays computation will be prefixed with +" << std::endl;
  out << "# Note that the delay items are not independent e.g. any item that independently causes delay has +" << std::endl;
  
  for (it = tour.begin(); it < tour.end(); it++) {
    get_task_and_core_from_vertex(*it, task_i, core_i);
    task = &(*tasks)[task_i];
    
    ScheduleItem si;
    si.task_ = task;
    std::vector<std::string> delays;
    si.start_ = schedule.get_earliest_start_time(task, core_i, delays);
    si.delays = delays;
    si.end_ = si.start_ + run_times->operator[](task->int_identifier_)[core_i];
    schedule.add_task(core_i, si);
    
    out << "Task " << task->identifier_ << ", Core " << core_i << " completes at " << si.end_ << std::endl;
    for (int i = 0; i < delays.size() ; i++)
      out << "\t" << delays[i] << std::endl;
    
    out.flush();
  }
}

int main(int argc, char *argv[]) {
  signal(SIGINT, terminate);
  try {
    parse_options(argc, argv);
  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    exit(EXIT_SUCCESS);
  }
  
  std::vector<Core> touse(cores_used);
  for (int i = 0; i < cores_used; i++)
  {
    std::ostringstream oss;
    oss << "Core " << i;
    touse[i].identifier_ = oss.str();
    touse[i].speed_multiplier_ = 1;
    if (processor_heterogenity != 1.0)
      touse[i].speed_multiplier_ *= unifRand(1, processor_heterogenity);
  }

  std::cerr << "Parsing " << filepath << std::endl;
  
  DirectedAcyclicGraph* task_precedence = NULL;
  std::vector<Task>* tasks = Parser::parse_stg(filepath.c_str(), task_precedence);
  // TODO - return task heterogeneity to the uniform value and stop adding randomness
  for (int i =0; i<tasks->size(); i++)
    tasks->at(i).execution_time_ = tasks->at(i).execution_time_ * unifRand(1, task_heterogenity);
  task_count = (int) tasks->size();
  core_count = cores_used;
  std::sort(tasks->begin(), tasks->end(), identifier_sort);
  
  // Setup route table using either grid or heterogeneity routing
  SymmetricMatrix<unsigned int>* routing_costs = new SymmetricMatrix<unsigned int>((int) touse.size(), routing_default);
  if (use_grid_routing) {
    int grid_size = (int) std::sqrt(cores_used);
    if (cores_used != std::pow(grid_size, 2.0))
      throw "Unable to complete, using grid routing but the number of cores is not a clean square root";
    for (int i =0; i<cores_used; i++)
    for (int j =0; j<cores_used; j++) {
      int rowdiff = std::abs(i / grid_size - j / grid_size);
      int coldiff = std::abs(i % grid_size - j % grid_size);
      (*routing_costs)[i][j] = rowdiff + coldiff;
    }
  } else {
    for (int i =0; i<cores_used; i++)
      for (int j =0; j<cores_used; j++)
          (*routing_costs)[i][j] *= routing_default * unifRand(1, routing_heterogenity);
  }
  
  // Build the run times by combining information about cores and tasks
  Matrix<unsigned int>* run_times = new Matrix<unsigned int>((int) tasks->size(), (int) touse.size(), 0);
  for (int task = 0; task < tasks->size(); task++)
    for (int core = 0; core < touse.size(); core++)
      (*run_times)[task][core] = tasks->at(task).execution_time_ * touse[core].speed_multiplier_;

  // Initially reorder tasks by precedence to create at least a simple but somewhat reasonable sorting order
  // then flatten into scheduling order. If you change this it must be changed below
  std::sort(tasks->begin(), tasks->end(), precedence_sort);
  std::vector<unsigned int> scheduling_order(tasks->size());
  for (int task = 0; task < tasks->size(); task++)
    scheduling_order[task] = tasks->at(task).int_identifier_;
  
  // As long as we are not running a sequential benchmark
  std::vector<unsigned int>* mapping = NULL;
  double annealing_makespan = 999999999.9;
  if (cores_used != 1)
    mapping = run_annealing(task_precedence,
                            routing_costs,
                            run_times,
                            &scheduling_order,
                            annealing_makespan);
  
  MpsProblem *problem = new MpsProblem(routing_costs, run_times, task_precedence, &scheduling_order);
  colony = get_ant_colony(problem);
  problem->set_ant_colony(colony);
  PheromoneMatrix* pheromones = colony->get_pheromone_matrix();
  
  // As long as we are not running a sequential benchmark
  std::vector<unsigned int> annealing_best_tour;
  if (cores_used != 1) {
    ACSAnt* ant = (ACSAnt *) colony->get_ant();
    for (unsigned int cur = 0; cur < mapping->size(); cur++)
    {
      unsigned int task = scheduling_order[cur];
      unsigned int core = mapping->at(task);
      unsigned int vertex = get_vertex_for(core, task);
      ant->offline_add_vertex_to_tour(vertex);
      annealing_best_tour.push_back(vertex);
    }
    ant->initial_offline_pheromone_update(pheromones, 10.0);
  }
  
  // Get all results and print
  double runtime = run_entire_aco(problem);
  double makespan =  colony->get_best_tour_length();
  std::vector<unsigned int> best_tour = colony->get_best_tour();
  if (makespan > annealing_makespan) {
    std::cout << std::endl << "Using SA best score over ACO best" << std::endl;
    best_tour = annealing_best_tour;
    makespan = annealing_makespan;
  }
  
  std::cout << runtime << "," << makespan << std::endl << std::endl;
  
  std::streambuf * buf;
  std::ofstream of;
  if (out_filepath == "out")
    buf = std::cout.rdbuf();
  else if (out_filepath == "err")
    buf = std::cout.rdbuf();
  else if (out_filepath != "none") {
    of.open(out_filepath.c_str());
    buf = of.rdbuf();
  }
  
  if (out_filepath != "none") {
    std::ostream out(buf);
    out << "# Full makespan" << std::endl << makespan << std::endl;
    out << "# Full runtime" << std::endl << runtime << std::endl;
    
    problem->print_mapping(best_tour, out);
    out.flush();
    
    print_detailed_schedule(best_tour, out, task_precedence, routing_costs, run_times, tasks);
  }
  
  //problem->print_tour(colony->get_best_tour())

  
  //bool verify = mpsproblem->verify_schedule_passes_constraints(schedule);
  
  //schedule.print_schedule_as_page();
  //return colony->get_best_tour_length();
  
  
  delete task_precedence;
  delete routing_costs;
  delete run_times;
  
  delete colony;
}



























