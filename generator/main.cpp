//
//  main.cpp
//  DeploymentOptimization
//
//  Created by Hamilton Turner on 1/29/13.
//  Copyright (c) 2013 Virginia Tech. All rights reserved.
//


#include <boost/mpi.hpp>
#include <boost/mpi/collectives.hpp>
#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <iostream>
#include <fstream>
#include <stdio.h>

// Includes Task and STG parser
#include "main.h"
#include "libs/rapidxml.hpp"
#include "libs/rapidxml_utils.hpp"
#include "libs/rapidxml_print.hpp"

#include "libs/tclap/CmdLine.h"

namespace mpi = boost::mpi;


// Forward declare methods to come
void run_simple_mpi(int argc, char* argv[]);
void build_mpi_from_stl(const char*, const char*);
void build_rankfiles_from_deployment(const char*);

// Simple usage: a.out <input_STG_file_path> <output_file_path> <input_SD_file_Path> <output_rank_file_path>

// Usage: a.out <input.STG> <optimization.xml> <system.xml> <output_directory>
//
// Outputs:
//    output_directory/
//    - input.cpp               (translated software model)
//    - Makefile                (build input.cpp on ataack cloud)
//    - rankfile.{0..N}         (one MPI rankfile for each deployment in optimization.xml)
//    - run_mpi.sh              (sample run script)


// Declare all of the variables that will be parsed by tclap for us
static std::string input_stg;
static std::string input_dep;
static std::string input_system;
static std::string outdir;
static bool DEBUG_LOG = false;

static void parse_options(int argc, char *argv[]) {
  TCLAP::CmdLine cmd("Multi-core Deployment Optimization Model --> MPI Code Generator ", ' ', "0.1");
  TCLAP::ValueArg<std::string> stg_arg("s", "stg", "path to STG file containing a directed acyclic graph describing the software model. Will result in a *.cpp file of the same unqualified name being generated and placed in the output directory", true, "", "STG path");
  cmd.add(stg_arg);
  TCLAP::ValueArg<std::string> dep_arg("d", "deployments", "path to XML file containing all of the deployments", true, "", "Deployment XML");
  cmd.add(dep_arg);
  TCLAP::ValueArg<std::string> sys_arg("y", "system", "path to XML file containing a description of the final deployment system hardware. Used to understand the ID's of processing units used in the deployment XML file", true, "", "system XML");
  cmd.add(sys_arg);
  TCLAP::ValueArg<std::string> dir_arg("o", "output", "path to a directory where output will be placed. Output directory should contain the stg_impl.cpp, Makefile, rankfile.{0..} (one for each deployment) and a sample run_mpi.sh showing how to phrase the running of all the MPI code", true, "", "output dirpath");
  cmd.add(dir_arg);
  TCLAP::SwitchArg debug_arg("", "debug", "Include println statements in the generated stg.cpp code (slows down MPI execution)");
  
  cmd.parse(argc, argv);
  input_stg = stg_arg.getValue();
  input_dep = dep_arg.getValue();
  input_system = sys_arg.getValue();
  outdir = dir_arg.getValue();
  DEBUG_LOG = debug_arg.getValue();
}

int main(int argc, char* argv[])
{
  try {
    parse_options(argc, argv);
  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    exit(EXIT_SUCCESS);
  }
  
  build_rankfiles_from_deployment(input_dep.c_str());
  
  std::string output_stg_cpp_path = outdir.append("/stg_impl.cpp");
  build_mpi_from_stl(input_stg.c_str(),
                     output_stg_cpp_path.c_str());
  
  return 0;
}

// From http://stackoverflow.com/questions/5465227/recursion-problem-in-parsing-with-
// rapidxml-c-class-pointers-side-effect
rapidxml::xml_node<>* get_child_with_id(rapidxml::xml_node<> *inputNode, std::string id)
{
	// cycles every child
  for (rapidxml::xml_node<> *nodeChild = inputNode->first_node(); nodeChild; nodeChild = nodeChild->next_sibling())
  {
    if (strcmp(nodeChild->first_attribute("id")->value(), id.c_str()) == 0)
	return nodeChild;
    
    rapidxml::xml_node<>* x = get_child_with_id(nodeChild, id);
    if (x) 
      return x;
  }
  return 0;
}

// Given a single logical ID, this function searches through the XML until
// that logical ID is located and outputs the physical ID values for node,
// processor, core, and hardware thread that match that logical ID. For
// many inputs the output will be blank e.g. passing in the logical ID
// of a processor will result in only outputs for the physical ID of the
// node and the processor
void parse_ids_from_system_xml(std::string logical_id,
				int &node_pid, 
				int &proc_pid, 
				int &core_pid, 
				int &hwth_pid, 
				std::string &hostname, 
				std::string &ip) {

  rapidxml::file<> xml_system(input_system.c_str());
  rapidxml::xml_document<> sys_doc;
  sys_doc.parse<0>(xml_system.data());
  
  rapidxml::xml_node<>* nodes = sys_doc.first_node("nodes");
  rapidxml::xml_node<>* node = get_child_with_id(nodes, logical_id);
  
  // While we have not returned to the root
  while (strcmp(node->name(), "nodes") != 0) {
  	std::string name = node->name();
		int pindex = atoi( node->first_attribute("pindex")->value() );
    if (name.compare("pu"))
			hwth_pid = pindex;
		else if (name.compare("core"))
			core_pid = pindex;
		else if (name.compare("socket")) 
			proc_pid = pindex;
		else if (name.compare("node")) {
			node_pid = pindex;
			ip = node->first_attribute("ip")->value();
			hostname = node->first_attribute("hostname")->value();
		}
		else
			throw "Unknown tag in XML. Valid values underneath 'nodes' are node,socket,core,pu";
  }
}

void build_rankfiles_from_deployment(const char* deployment_path) {

  // Load XML files
  rapidxml::xml_document<> dep_doc;
  rapidxml::file<> xml_deployment(deployment_path);
  dep_doc.parse<0>(xml_deployment.data());
  
  // Pull out the mapping we are using and translate
  // that into a proper rankfile format
  std::string mapping = dep_doc.first_node("mapping")->first_attribute("to")->value();
  const char* format = "";
  if (mapping.compare("cores") == 0) {
    // rank 1=10.0.2.4 slot=p1:8
    // references physical socket 1 and physical core 8
    format = "rank %d=%s slot=p%d:%d";
  } else if (mapping.compare("nodes") == 0){
    format = "%dsdf";
    throw "only cores supported now";
  } else if (mapping.compare("hw-threads") == 0) {
    format = "%dewr";
    throw "only cores supported now";
  } else if (mapping.compare("processors") == 0) {
    format = "%dasdf";
    throw "only cores supported now";
  } else
    throw "The provided mapping was not recognized. Use one of cores,nodes,processors,hw-threads";
  
  char buffer [50];
  int rank = 0;
  const char* ip = "10.0.2.4";
  int socket = 1;
  int core = 8;
  sprintf(buffer, format, rank, ip, socket, core);
  

  // Locate all deployments
  rapidxml::xml_node<>* deps = dep_doc.first_node("deployments");
  for (rapidxml::xml_node<>* deployment = deps->first_node();
       deployment;
       deployment = deployment->next_sibling()) {
    
    // Start writing the rankfile
    std::string id = deployment->first_attribute("id")->value();
    std::string rankfile = outdir.append(".").append(id);
    std::ofstream rf;
    rf.open(rankfile.c_str());
    
    // Iterate over every mapping in deployment
    for (rapidxml::xml_node<>* mapping = deployment->first_node();
         mapping;
         mapping = mapping->next_sibling()) {
      
      std::string rank = mapping->first_attribute("t")->value();
      std::string logical_id = mapping->first_attribute("u")->value();
      
      std::string hostname;
      std::string ip;
      int node_pid;
      int proc_pid;
      int core_pid;
      int hwth_pid;
      // TODO Call helper function here to fill in the values above
      // This function fills in all other data given the physical ID
      parse_ids_from_system_xml(logical_id, node_pid, proc_pid, core_pid, hwth_pid, hostname, ip);
      
      // Then use format string to write them to buffer, and
      // finally push the buffer out to a file
      
      printf("rank %s=%s slot=p%d:%d", rank.c_str(), hostname.c_str(), 
	proc_pid, core_pid);
    }
  }
  
  std::cout << "Done";

}

void build_mpi_case_for_task(unsigned int tid, unsigned int exectime, std::vector<unsigned int>& pre, std::vector<unsigned int>& post, std::ofstream& out);
void build_mpi_from_stl(const char* stg_path,
                        const char* impl_cpp_outpath) {
  DirectedAcyclicGraph* task_precedence = NULL;
  std::vector<Task>* tasks = parse_stg(stg_path, task_precedence);
  std::ofstream out;
  out.open(impl_cpp_outpath);
  
  // Write header
  const char * header =
    "#include <boost/mpi.hpp>\n"
    "#include <boost/mpi/collectives.hpp>\n"
    "#include <boost/mpi/environment.hpp>\n"
    "#include <boost/mpi/communicator.hpp>\n"
    "namespace mpi = boost::mpi;\n\n";
  out << header;

  const char * start_main =
    "int main(int argc, char* argv[]) {\n"
    "  mpi::environment env(argc, argv);\n"
    "  mpi::communicator world;\n";
  out << start_main;
  
  // Switch start
  out <<
    "  switch (world.rank()) {\n";
  
  // build case (task_id, task_predecessors, task_sucessors, task_execution_time, out)
  for (int i = 0; i < tasks->size(); i++) {
    Task task = tasks->at(i);
    std::vector<unsigned int> pre = task_precedence->get_predecessors(task.int_identifier_);
    std::vector<unsigned int> post = task_precedence->get_successors(task.int_identifier_);
    build_mpi_case_for_task(task.int_identifier_, task.execution_time_, pre, post, out);
  }
  
  // Switch End
  out <<
    "  }\n"
    "  return 0;\n"
    "}\n\n";

}

void build_mpi_case_for_task(unsigned int tid, unsigned int exectime, std::vector<unsigned int>& pre, std::vector<unsigned int>& post, std::ofstream& out) {
  
  // ========= Case Header
  out <<
    "    case " << tid << ": {\n";
  if (DEBUG_LOG)
    out <<
      "      std::cout << \"" << tid << ": Awake\" << std::endl;\n";

  // ========= Receive Predecessors
  if (pre.size() != 0) {
    out <<
      "      mpi::request req[" << pre.size() << "];\n";
    for (int p = 0; p < pre.size(); p++)
      out <<
        "      req[" << p << "] = world.irecv(" << pre[p] << ", 0);\n";
    out <<
      "      mpi::wait_all(req, req + " << pre.size() << ");\n";

    for (int p = 0; p < pre.size(); p++)
      out <<
        "      std::cout << \"" << tid << ": Recv notice from pred " << pre[p] << "\" << std::endl;\n";
  }


  // ========= Perform Computation
  out <<
    "      time_t start;\n"
    "      start = time(NULL);\n";
  
  if (DEBUG_LOG)
    out <<
    "      std::cout << \"" << tid << ": Started compute\" << std::endl;\n";
  out <<
   "      while ((time(NULL)-start) < " << exectime << ");\n";
  if (DEBUG_LOG)
    out <<
    "      std::cout << \"" << tid << ": Finished compute in \" << (time(NULL)-start) << std::endl;\n";
  
  
  // ========= Send to Successors
  if (post.size() != 0) {
    out <<
    "      mpi::request sreq[" << post.size() << "];\n";
    for (int p = 0; p < post.size(); p++)
      out <<
      "      sreq[" << p << "] = world.isend(" << post[p] << ", 0);\n";
    out <<
    "      mpi::wait_all(sreq, sreq + " << post.size() << ");\n";
    
    for (int p = 0; p < post.size(); p++)
      out <<
      "      std::cout << \"" << tid << ": Sent notice to succ " << post[p] << "\" << std::endl;\n";
  } else
    out <<
    "      std::cout << \"" << tid << ": DONE!!\" << std::endl;\n";

  // ========= Case Footer
  out <<
    "      break; }\n";
}

