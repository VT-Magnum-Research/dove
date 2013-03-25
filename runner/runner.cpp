#include <stdio.h>
#include <papi.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <string>

// XML parsing
#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"
#include "rapidxml_print.hpp"

// XML file used describe the deployments
// TODO please set this up in parse_options
rapidxml::xml_document<> xml;

// Command-line argument parsing
#include "libs/tclap/CmdLine.h"

#define NUM_EVENTS 1

using namespace std;

// Declare all of the variables that will be parsed by tclap for us
static std::string rank_path;
static std::string outdir;
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
// TODO Instead of "looping on the rankfiles", make this read from 
// the deployments.xml document. Look for 
// all deployment tags, read the 'id' attribute from 
// them, and assume that a rankfile exists that has 
// the name rankfile.<id> that corresponds to that 
// deployment. Use code like this: 
//
// Locate all deployments
//  rapidxml::xml_node<>* deps = dep_doc.first_node("optimization")->
//  first_node("deployments");
//  for (rapidxml::xml_node<>* deployment = deps->first_node();
//       deployment;
//       deployment = deployment->next_sibling()) {
//    
//    // Build the rankfile path
//    std::string id = deployment->first_attribute("id")->value();
//    std::string rankfile = outdir;
//    rankfile.append("rankfile.").append(id);
//
//    // TODO run kbest here, 
//
//    // TODO then add a new child node to the 
//    // deployment called kbest. 
//    // See http://rapidxml.sourceforge.net/manual.html#namespacerapidxml_1modifying_dom_tree
//    // for some examples of how to add a  new node
//  }

  //loop on the rank files, for each iteration, k-best scheme is applied
  int i;
  for(i=0;i<fl_count;i++)
  {
    std::cerr << "Running rankfile." << i << std::endl; 
    kbest(i,ranks,k,M,E);
  }
}


static void parse_options(int argc, char *argv[]) 
{                            
  TCLAP::CmdLine cmd("Multi-core Deployment Optimization Model -->  Runner",' ',"1.0");
  TCLAP::ValueArg<std::string> inp_arg("i", "inputdir", "Path to directory containing rankfiles, deployment.xml. Must end with a slash", true, "ranks/", "inputdir path", cmd);
  // TODO it's not clear what rankfiles.temp is or why we need an output directory path
  // for it. If this is only needed internally for running MPI, then please just use the 
  // standard C++ mkstemp command internally. There is example code for this at 
  // latency/main_bootstrapper.cpp:93:    fd = mkstemp(sfn);
  TCLAP::ValueArg<std::string> out_arg("o", "outputdir", "Path of output directory where rankfiles.temp is written", false, "ranks/", "output directory path", cmd);
  TCLAP::ValueArg<int> k_arg("k", "kvalue", "minimum number of runs",false,3,"integer: k value of k best scheme"); 
  cmd.add(k_arg);
  TCLAP::ValueArg<int> m_arg("m", "maximum", "maximum number of runs",false,10,"integer: M value of k best scheme"); 
  cmd.add(m_arg);
  TCLAP::ValueArg<double> e_arg("t", "tolerance", "tolerance required to stop runs",false,0.8,"double value for tolerance"); 
  cmd.add(e_arg);
  cmd.parse(argc, argv);
  rank_path = inp_arg.getValue();
  outdir = out_arg.getValue();
  k=k_arg.getValue();
  M=m_arg.getValue();
  E=e_arg.getValue();

  // TODO use rapidxml to parse the deployments.xml document, using 
  // something like this: 
  //
  // Ensure that the given XML path is accessible by rapidxml
  //try {
  //  rapidxml::file<> xml_file(xml_arg.getValue().c_str());
  //  xml.parse<0>(xml_file.data());
  //} catch (rapidxml::parse_error err) {
  //  std::cout << "Could not parse XML file. Error was: " << std::endl;
  //  std::cout << err.what() << std::endl;
  //  TCLAP::ArgException e("Unable to parse XML", err.what());
  //  throw e;
  //} 
}

//returns number of lines in a rankfile
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

void kbest(int turn,int ranks,int k,int M,double E)
{
  int times[1000]; 
  string cmd;
  string fname;

  //prepare the mpi command to send to system	
  stringstream s1,s2;
  cmd = "mpirun --rankfile rankfile.";
  s1 << turn;
  cmd += s1.str() + " --hostfile hostfile.txt -np ";
  s2 << ranks;
  cmd += s2.str() + " impl >/dev/null 2>&1";
  cerr << cmd << endl; 
  
  fname = outdir+"rankfile."+s1.str()+".temp";
  //configure the PAPI counters to be monitored
  
  int Events[NUM_EVENTS]={PAPI_TOT_CYC}, EventSet=PAPI_NULL;
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
  tf.open(fname.c_str());
  tf << "Run\tExec. Cycles\n-------------------------------\n";

  int i,j;
  for(i=0;i<M;i++) times[i]=9999999;	//initialize executions times to big number

  for(i=0;i<M;i++)   //each iteration is one monitored run
  {   
    cerr << "run:" << i+1 << endl;     
    retval = PAPI_start(EventSet);
    system(cmd.c_str());	//code to be evaluated
    retval = PAPI_stop(EventSet,values);
    times[i]=values[0];
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
    if(((double)(times[k-1]-times[0])/times[0])<= E)
    {
      tf << "Number of runs = " << i+1 << endl;
      tf << "Fastest execution = " << times[0] << " cycles." << endl;
      tf << k << "th fastest execution = " << times[k-1] << " cycles." << endl;
      tf << k << "th fastest = " << 1+((double)(times[k-1]-times[0])/times[0])  << "of the fastest." << endl;
      break;
    }
  } // End of iterations

  if(i==M) //in case we did the maximum number of runs
  {
    tf << "Could not converge!!" << endl;
    tf << "Fastest execution = " << times[0] << "cycles." << endl;
  }
  tf.close();
}


