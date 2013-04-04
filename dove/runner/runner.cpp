#include <stdio.h>
#include <papi.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <cstdio>

#include <iostream>
#include <fstream>
#include <string>

// XML parsing
#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"
#include "rapidxml_print.hpp"

// Command-line argument parsing
#include "libs/tclap/CmdLine.h"

#define NUM_EVENTS 1

using namespace std;

// Declare all of the variables that will be parsed by tclap for us
static std::string rank_path;
static bool delete_rankfiles = false;
static bool store_logs = false;
static std::string dove_workspace;
static rapidxml::xml_document<char>* deployments;
static rapidxml::file<char>* deps_data;
static std::string deployments_path;


static int k;
static int M;
static double E;


//prototypes of functions 
static void parse_options(int argc, char *argv[]);
int get_rank_count();
int get_rankfile_count();
void kbest(int turn,int ranks,int k,int M,double E);

main(int argc,char* argv[])
{
  try 
  {
    parse_options(argc, argv);
  } 
  catch (TCLAP::ArgException &e) {
    cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
  }
  int ranks=get_rank_count();
  int fl_count=get_rankfile_count();
  cout << ranks << fl_count << endl;

  //loop on the rank files, for each iteration, k-best scheme is applied
  int i;
  for(i=0;i<fl_count;i++)
  {
    std::cerr << "Running rankfile." << i << std::endl; 
    kbest(i,ranks,k,M,E);
  }

  if (delete_rankfiles) {
    for(i=0;i<fl_count;i++) {
      std::stringstream s1;
      std::string file = dove_workspace;
      file += "rankfile.";
      s1 << i;
      file += s1.str();
      if (0 != remove(file.c_str()))
        std::cerr << "Unable to remove " << file << std::endl;
      else
        std::cout << "Removed " << file << std::endl;
    }
  }

  // Write the updated deployment.xml
  std::ofstream output(deployments_path.c_str(), 
      std::ios::out | std::ios::trunc);
  if (output.is_open()) {
    output << *deployments;
    output.close();
  } else
    std::cerr << "Unable to open deployments.xml file for writing" 
      << std::endl;

  delete deps_data;
  delete deployments;
}


static void parse_options(int argc, char *argv[]) 
{                            
  TCLAP::CmdLine cmd("Multi-core Deployment Optimization Model -->  Runner",' ',"1.0");
  TCLAP::ValueArg<std::string> inp_arg("d", "dove", "Path to dove's workflow "
      "directory. Should contain rankfiles and deployment.xml. Must end with "
      "a slash. ", true, "ranks/", "inputdir path", cmd);
  TCLAP::SwitchArg logs_arg("l","storelogs", "In addition to storing the final metrics "
      "(currently just total_cpu_cycles) found inside of the deployment.xml, passing "
      "this flag will cause the runner to also create a log file for every rankfile "
      "that shows some output data from the runner and all of the scores that "
      "existed before kbest was used to compress them to one number", cmd);
  TCLAP::SwitchArg remove_ranks_arg("","rmrankfiles", "After finishing all other "
      "commands successfully, remove all rankfiles from dove workspace", cmd);

  TCLAP::ValueArg<int> k_arg("k", "kvalue", "minimum number of runs",false,3,"integer: k value of k best scheme", cmd); 
  TCLAP::ValueArg<int> m_arg("m", "maximum", "maximum number of runs",false,10,"integer: M value of k best scheme"); 
  cmd.add(m_arg);
  TCLAP::ValueArg<double> e_arg("t", "tolerance", "tolerance required to stop runs",false,0.8,"double value for tolerance"); 
  cmd.add(e_arg);
  cmd.parse(argc, argv);
  rank_path = inp_arg.getValue();
  k=k_arg.getValue();
  M=m_arg.getValue();
  E=e_arg.getValue();

  delete_rankfiles = remove_ranks_arg.getValue();
  store_logs = logs_arg.getValue();
  dove_workspace = inp_arg.getValue();
  deployments_path = dove_workspace;
  deployments_path.append("deployments.xml");

  // TODO use rapidxml to parse the deployments.xml document, using 
  // something like this: 
  //
  // Ensure that the given XML path is accessible by rapidxml
  try {
    deployments = new rapidxml::xml_document<char>();
    deps_data = new rapidxml::file<char>(deployments_path.c_str());
    deployments->parse<0>(deps_data->data());
  } catch (rapidxml::parse_error err) {
    std::cerr << "Could not parse XML file. Error was: " << std::endl;
    std::cerr << err.what() << std::endl;
    TCLAP::ArgException e("Unable to parse XML", err.what());
    throw e;
  } 
}

// Returns number of lines in a rankfile (synonymous to 
// get_number_cores_used)
int get_rank_count()
{
  ifstream fin;
  // TODO - this should really be checked when parsing the 
  // command line arguments. Can you please make rank_count
  // and rankfile_count global variables and call these 
  // functions inside of parse_options, throwing a TCLAP::ArgException
  // if the necessary files are not available? 
  string filename = rank_path+"rankfile.0";
  fin.open(filename.c_str());
  if (!fin) {
    string error ("File was not readable: ");
    error.append(filename);
    throw error.c_str();
  }
  string line;
  int ln_count=0;

  while(getline(fin,line))
    ln_count++;
  
  fin.close();
  return ln_count;
}
   
//returns number of rankfiles, assumes this number to be less than 1000 
int get_rankfile_count()
{
  DIR *d;
  struct dirent *fs;
  int fl_count=0;
  d=opendir(rank_path.c_str());
  while(fs=readdir(d))
  {
    if(strncmp(fs->d_name,"rankfile.",9)==0 && strlen(fs->d_name)<13)
       fl_count++;
  } 
  closedir(d);
  return fl_count;
}

void add_metric_to_deployment(int deployment_id, 
    const char* metric_name, 
    const char* metric_value) {

  std::stringstream d_id;
  d_id << deployment_id;

  // First find the deployment node
  rapidxml::xml_node<char>* dep = NULL;
  // Cycles every child
  for (rapidxml::xml_node<char> *nodeChild = 
        deployments->first_node("optimization")->
        first_node("deployments")->first_node(); 
      nodeChild; 
      nodeChild = nodeChild->next_sibling())
  {
    rapidxml::xml_attribute<char>* attribute = 
      nodeChild->first_attribute("id");
    if (attribute == 0) {
      std::cerr << "Found a deployment with no id!"
        << std::endl;
      continue;
    }
    if (strcmp(attribute->value(), 
          d_id.str().c_str()) == 0) {
      dep = nodeChild;
      break;
    }
  }
  if (dep == NULL) {
    std::cerr << "Did not find a deployment with id "
      << deployment_id << std::endl;
    return;
  }

  // Build a sub-node to add to the deployment
  rapidxml::xml_node<char>* rmetric = deployments->
    allocate_node(rapidxml::node_element, deployments->
        allocate_string("rmetric"));
  rmetric->append_attribute(deployments->allocate_attribute(
        deployments->allocate_string("name"),
        deployments->allocate_string(metric_name)));
  rmetric->append_attribute(deployments->allocate_attribute(
        deployments->allocate_string("value"),
        deployments->allocate_string(metric_value)));

  dep->append_node(rmetric);
}

void kbest(int turn,int ranks,int k,int M,double E)
{
  int times[1000]; 
  string cmd;
  string fname;

  //prepare the mpi command to send to system	
  stringstream s_file,s_rank;
  cmd = "mpirun --rankfile ";
  cmd += dove_workspace;
  cmd += "rankfile.";
  s_file << turn;
  cmd += s_file.str();
  cmd +=" --hostfile ";
  cmd += dove_workspace;
  cmd += "hostfile.txt";
  cmd += " -np ";
  s_rank << ranks;
  cmd += s_rank.str() + " ";
  cmd += dove_workspace;
  cmd += "impl ";
  cerr << cmd << endl; 
  
  fname = dove_workspace + "rankfile." + s_file.str() + ".log";
  //configure the PAPI counters to be monitored
  
  int Events[NUM_EVENTS]={PAPI_TOT_CYC};
  int EventSet=PAPI_NULL;
  long_long values[NUM_EVENTS];
  /* Initialize the Library */
  int retval = PAPI_library_init(PAPI_VER_CURRENT);
  /* Allocate space for the new eventset and do setup */
  retval = PAPI_create_eventset(&EventSet);
  /* Add Flops and total cycles to the eventset */
  retval = PAPI_add_events(EventSet,Events,NUM_EVENTS);

  //prepare the file to save the monitored events
  ofstream tf;
  stringstream buf;
  if (store_logs) {
    tf.open(fname.c_str());
    tf << "Run\tExec. Cycles\n-------------------------------\n";
  }

  int i,j;
  for(i=0;i<M;i++) 
    times[i]=9999999;	//initialize executions times to big number

  for(i=0;i<M;i++)   //each iteration is one monitored run
  {   
    cerr << "run:" << i+1 << endl;     
    retval = PAPI_start(EventSet);
    system(cmd.c_str());	//code to be evaluated
    retval = PAPI_stop(EventSet,values);
    times[i]=values[0];
    if (store_logs) 
      tf << i << "\t" << times[i] << endl; 
    //place new run time to its location in sorted array
    int ii=i;
    for(j=i-1;j>=0;j--,ii--)
      if(times[ii]<times[j]) //swap
      {
          int t=times[ii];
          times[ii]=times[j];
          times[j]=t;
      }
     
    //now we r sure execution times array is sorted
    //we can compare the best time to the kth best time
    double range_of_top_k_scores = ((double)(times[k-1]-times[0])/times[0]);  
    if(range_of_top_k_scores <= E)
    {
      if (store_logs) {
        tf << "Number of runs = " << i+1 << endl;
        tf << "Fastest execution = " << times[0] << " cycles." << endl;
        tf << k << "th fastest execution = " << times[k-1] << " cycles." << endl;
        tf << k << "th fastest = " << 1 + range_of_top_k_scores  << "of the fastest." << endl;
      }
      break;
    }
  } // End of iterations

  if(i==M && store_logs) //in case we did the maximum number of runs
  {
    tf << "Could not converge!!" << endl;
    tf << "Fastest execution = " << times[0] << "cycles." << endl;
  }


  std::stringstream ktime;
  ktime << times[0];
  add_metric_to_deployment(turn, 
      "total_cpu_cycles", ktime.str().c_str());

  if (store_logs) 
    tf.close();
}


