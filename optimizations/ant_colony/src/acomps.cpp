#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cfloat>
#include <climits>
#include <csignal>
#include <fstream>
#include "tclap/CmdLine.h"
#include "ants.h"

#include "mps.h"
#include "dove.h"

enum StagnationMeasureType { STAG_NONE, STAG_VARIATION_COEFFICIENT, STAG_LAMBDA_BRANCHING_FACTOR };

// Arguments for ant colony
static unsigned int ants = 10;
static unsigned int iterations = 100;
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
static double elitist_weight = 2.0;
static unsigned int ranked_ants = 1;
static unsigned int maxmin_frequency = 5;
static double maxmin_a = 2.0;
static double acs_q0 = 0.5;
static double acs_xi = 0.1;

// Arguments for deployment optimization
static dove::config config;
// static std::string stg_filepath;
static unsigned int cores_used = 2;
//static unsigned int processor_heterogenity = 1;
//static unsigned int routing_heterogenity = 1;
//static unsigned int routing_default=0;
//static unsigned int task_heterogenity = 1;

// Data structures for optimization
std::vector<Task>* tasks = NULL;
DirectedAcyclicGraph* task_precedence = NULL;
static AntColony<Ant> *colony;
static MpsProblem* mpsproblem;

// Data structures for validation
dove::validator* validation = NULL;

static void parse_options(int argc, char *argv[]) {
  TCLAP::CmdLine cmd("Ant Colony Optimization for the Multiprocessor Scheduling Problem", ' ', "0.1");
  TCLAP::ValueArg<unsigned int> ants_arg ("m", "ants", "number of ants", false, ants, "integer");
  TCLAP::ValueArg<unsigned int> iterations_arg ("i", "iterations", "number of iterations", false, iterations, "positive integer");
  TCLAP::ValueArg<double>       alpha_arg ("a", "alpha", "alpha (influence of pheromone trails)", false, alpha, "double");
  TCLAP::ValueArg<double>       beta_arg("b", "beta", "beta (influence of heuristic information)", false, beta, "double");
  TCLAP::ValueArg<double>       rho_arg("r", "rho", "pheromone trail evaporation rate", false, rho, "double");
  TCLAP::ValueArg<double>       initial_pheromone_arg("p", "pheromone", "initial pheromone value", false, initial_pheromone, "double");
  std::vector<unsigned int> allowed;
  allowed.push_back(0);
  allowed.push_back(1);
  TCLAP::ValuesConstraint<unsigned int> allowed_values( allowed );
  TCLAP::ValueArg<unsigned int> cores_used_arg("c", "cores", "number of homogeneous processing cores. Defaults to 2", false, 2, "positive integer", cmd);
//  TCLAP::ValueArg<unsigned int> processor_h_arg("","core_heter", "Processor heterogeneity. 1 specifies homogeneous processors, <int> specifies a limit on processor upper bound that is randomly queried to build a set of heterogeneous processors. Default is 1", false, 1, "positive integer", cmd);
  TCLAP::SwitchArg              print_tour_arg("o", "printord", "print best elimination ordering in iteration");
//  TCLAP::ValueArg<unsigned int> task_harg("", "task_heter", "task heterogeneity. 1 specifies to leave task homogenity alone, <int> specifies a limit on the upper bound a task completion time can be multiplied by. Default is 1", false, 1, "positive integer");
//  TCLAP::ValueArg<unsigned int> routing_h_arg("","routing_heter", "routing heterogeneity. 1 specifies homogeneous routing delay, <int> specifies a limit on routing delay upper bounds that is randomly queried to build a set of routing delays between the processors. Default is 1", false, 1, "positive integer");
//  TCLAP::ValueArg<unsigned int> routing_def_arg("","route_default", "base routing cost between cores. Default is 0", false, 0, "positive integer");
  TCLAP::SwitchArg              stag_variance_arg("", "stag_variance", "compute and print variation coefficient stagnation");
  TCLAP::SwitchArg              stag_lambda_arg("", "stag_lambda", "compute and print lambda branching factor stagnation");
  TCLAP::ValueArg<double>       time_limit_arg("t", "time", "terminate after n seconds (after last iteration is finished)", false, time_limit, "double");
  TCLAP::SwitchArg              simple_as_arg("", "simple", "use Simple Ant System");
  TCLAP::ValueArg<double>       elitist_as_arg("", "elitist", "use Elitist Ant System with given weight", false, elitist_weight, "double");
  TCLAP::ValueArg<unsigned int> rank_as_arg("", "rank", "use Rank-Based Ant System and let the top n ants deposit pheromone", false, ranked_ants, "positive integer");
  TCLAP::SwitchArg              maxmin_as_arg("", "maxmin", "use Max-Min Ant System");
  TCLAP::ValueArg<unsigned int> maxmin_frequency_arg("", "maxmin_frequency", "frequency of pheromone updates of best-so-far ant in Max-Min Ant System", false, maxmin_frequency, "double");
  TCLAP::ValueArg<double>       maxmin_a_arg("", "maxmin_a", "parameter a in Max-Min Ant System", false, maxmin_a, "double");
  TCLAP::SwitchArg              acs_as_arg("", "acs", "use Ant Colony System");
  TCLAP::ValueArg<double>       acs_q0_arg("", "acs_q0", "q0 parameter for Ant Colony System", false, acs_q0, "double");
  TCLAP::ValueArg<double>       acs_xi_arg("", "acs_xi", "xi parameter for Ant Colony System", false, acs_xi, "double");
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
  cmd.add(print_tour_arg);
  cmd.add(stag_variance_arg);
  cmd.add(stag_lambda_arg);
  cmd.add(time_limit_arg);
  cmd.add(maxmin_frequency_arg);
  cmd.add(maxmin_a_arg);
  cmd.add(acs_q0_arg);
  cmd.add(acs_xi_arg);
  cmd.xorAdd(as_variants);
  //cmd.add(routing_h_arg);
  //cmd.add(routing_def_arg);
  //cmd.add(task_harg);

  config = dove::use_tclap(cmd, argc, argv);
  
  ants = ants_arg.getValue();
  iterations = iterations_arg.getValue();
  alpha = alpha_arg.getValue();
  beta = beta_arg.getValue();
  rho = rho_arg.getValue();
  initial_pheromone = initial_pheromone_arg.getValue();
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
  //processor_heterogenity=processor_h_arg.getValue();
  //routing_heterogenity=routing_h_arg.getValue();
  //routing_default=routing_def_arg.getValue();
  //task_heterogenity = task_harg.getValue();

  tasks = Parser::parse_stg(config.stg_filepath.c_str(), task_precedence);

  validation = new dove::validator(tasks->size(), 
    cores_used,
    dove::CORE, 
    config.deps.c_str(),
    "Ant Colony Optimization",
    config.sys.c_str());
  
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
    config.initial_pheromone = 0;
}

static void set_initial_pheromone(OptimizationProblem *problem, AntColonyConfiguration &config) {
  //if(config.initial_pheromone == -1.0) {
  //  double initial_pheromone = compute_average_pheromone_update(*problem) * config.number_of_ants;
  //  config.initial_pheromone = initial_pheromone * (1.0 / ants);
  //}
  config.initial_pheromone = 2;
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
  delete validation;
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

void run_entire_aco(DirectedAcyclicGraph* task_precedence,
                    SymmetricMatrix<unsigned int>* routing_costs,
                    Matrix<unsigned int>* run_times,
                    std::vector<unsigned int>* task_scheduling_order) {
  
  MpsProblem *problem = new MpsProblem(routing_costs, run_times, task_precedence, task_scheduling_order);
  colony = get_ant_colony(problem);
  problem->set_ant_colony(colony);
  
  problem->get_feasible_start_vertices();
  std::ostream* info = &std::cerr;
  
  *info << "iter\ttime\tbest\tbest_it\talpha\tbeta";
  *info << ((stagnation_measure != STAG_NONE) ? "\tstagnation" : "");
  *info << (print_tour_flag ? "\tordering" : "");
  *info << std::endl;
  
  timer();
  timer2();
  for(unsigned int i=0;i<iterations && timer() < time_limit;i++) {
    colony->run();
    std::vector<unsigned int> tour = colony->get_best_tour_in_iteration();
    unsigned int task;
    unsigned int core;
    std::vector<unsigned int>::iterator it;
    dove::deployment deployment = validation->get_empty_deployment();
    for (it = tour.begin();
        it != tour.end();
        it++) {
      problem->get_task_and_core_from_vertex(*it, task, core);
      deployment.add_task_deployment(task, core);
    }
    double score = colony->get_best_tour_length_in_iteration();
    std::stringstream strs;
    strs << std::fixed << std::setprecision(19) << (score * 1000000000.0);
    deployment.add_metric("makespan", strs.str().c_str());
    validation->add_deployment(deployment);

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
      std::cout << "\t";
      // TODO mpsproblem->print_tour(colony->get_best_tour_in_iteration());
    }
    
    *info << std::endl;
  }
  *info << std::endl;
  *info << "best\tordering" << std::endl;
  *info << colony->get_best_tour_length() << "," << timer2();
  std::cout << colony->get_best_tour_length() << "," << timer2();
  
  validation->complete();

  //mpsproblem->print_tour(colony->get_best_tour());
  
  //MpSchedule schedule = mpsproblem->convert_tour_to_schedule(colony->get_best_tour());
  //bool verify = mpsproblem->verify_schedule_passes_constraints(schedule);
  
  //schedule.print_schedule_as_page();
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

int main(int argc, char *argv[]) {
  signal(SIGINT, terminate);
  try {
    parse_options(argc, argv);
  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    exit(EXIT_SUCCESS);
  } catch (dove::parse_error &e) {
    std::cerr << "error: " << e.what() << std::endl;
    exit(EXIT_SUCCESS);
  }
  
  // TODO potentially make DOVE internally contain all of the options to 
  // randomly generate a system? 
  // Before DOVE, this was used to store all of the cores.
  // TODO add a way to get descriptive name from a component in dove, and 
  // add a way to set a descriptive name for a component in dove, so that 
  // tracking program flow is easier. 
  /*std::vector<Core> touse(cores_used);
  for (int i = 0; i < cores_used; i++)
  {
    std::ostringstream oss;
    oss << "Core " << i;
    touse[i].identifier_ = oss.str();
    touse[i].speed_multiplier_ = 1;
    if (processor_heterogenity != 1.0)
      touse[i].speed_multiplier_ *= unifRand(1, processor_heterogenity);
  }*/
  
  // tasks was created in parse_options
  for (int i =0; i<tasks->size(); i++)
    // Before DOVE, this was multiplied by  
    //* unifRand(1, task_heterogenity);
    tasks->at(i).execution_time_ = tasks->at(i).execution_time_; 
  
  std::sort(tasks->begin(), tasks->end(), identifier_sort);
  
  SymmetricMatrix<unsigned int>* routing_costs = 
    new SymmetricMatrix<unsigned int>(cores_used, 0); //routing_default);
  for (int i =0; i < cores_used; i++)
      for (int j =0; j < cores_used; j++) {
        (*routing_costs)[i][j] = validation->get_routing_delay(i, j);

        // Before DOVE, routing costs were created like so
        // TODO make it so this can run both with and without dove
        // (*routing_costs)[i][j] *= routing_default * unifRand(1, routing_heterogenity);
      }
  
  // Build the run times by combining information about cores and tasks
  Matrix<unsigned int>* run_times = 
    new Matrix<unsigned int>((int) tasks->size(), cores_used, 0);
  for (int task = 0; task < tasks->size(); task++)
    for (int core = 0; core < cores_used; core++)
      // Before DOVE
      // (*run_times)[task][core] = tasks->at(task).execution_time_ * touse[core].speed_multiplier_;

      // With DOVE, there is no multiplier currently because the generator creates an amount 
      // of time delay, not an amount of work
      (*run_times)[task][core] = tasks->at(task).execution_time_;  

  // Initially reorder tasks by precedence to create at least a simple but somewhat reasonable sorting order
  // then flatten into scheduling order
  std::sort(tasks->begin(), tasks->end(), precedence_sort);
  std::vector<unsigned int> scheduling_order(tasks->size());
  for (int task = 0; task < tasks->size(); task++)
    scheduling_order[task] = tasks->at(task).int_identifier_;
  run_entire_aco(task_precedence, routing_costs, run_times, &scheduling_order);

  delete task_precedence;
  delete routing_costs;
  delete run_times;
  delete validation;  
  delete colony;
}
