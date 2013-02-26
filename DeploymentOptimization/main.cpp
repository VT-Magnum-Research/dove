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

// Includes Task and STG parser
#include "main.h"
#include "libs/rapidxml.hpp"
#include "libs/rapidxml_utils.hpp"
#include "libs/rapidxml_print.hpp"

#include "tclap/CmdLine.h"


namespace mpi = boost::mpi;

// Forward declare methods to come
void run_simple_mpi(int argc, char* argv[]);
void build_mpi_from_stl(char* file_path, char* outfile_p);
void build_rankfiles_from_deployment(char* sd_path, char* output);

// Simple usage: a.out <input_STG_file_path> <output_file_path> <input_SD_file_Path> <output_rank_file_path>

// Usage: a.out <input.STG> <optimization.xml> <system.xml> <output_directory>
//
// Outputs:
//    output_directory/
//    - input.cpp               (translated software model)
//    - Makefile                (build input.cpp on ataack cloud)
//    - rankfile.{0..N}         (one MPI rankfile for each deployment in optimization.xml)
//    - run_mpi.sh              (sample run script)


// TODO - Advanced Usage
// a.out --system <xml> --stg <stg> --recursive(STG recursive) --deployments <xml> --outdir <dir>
// Automatically detect STG files recursively


// Declare all of the variables that will be parsed by tclap for us
static std::string input_stg;
static std::string input_dep;
static std::string input_sys;
static std::string input_gendir;
static bool DEBUG_LOG = false;

static void parse_options(int argc, char *argv[]) {
  TCLAP::CmdLine cmd("Multi-core Deployment Optimization Model --> MPI Code Generator ", ' ', "0.1");
  TCLAP::ValueArg<std::string> stg_arg("s", "stg", "path to STG file containing a directed acyclic graph describing the software model. Will result in a *.cpp file of the same unqualified name being generated and placed in the output directory", true, "", "STG path");
  cmd.add(stg_arg);
  TCLAP::ValueArg<std::string> dep_arg("d", "deployments", "path to XML file containing all of the deployments", true, "", "Deployment XML");
  cmd.add(dep_arg);
  TCLAP::ValueArg<std::string> sys_arg("y", "system", "path to XML file containing a description of the final deployment system hardware. Used to understand the ID's of processing units used in the deployment XML file", true, "", "system XML");
  cmd.add(sys_arg);
  TCLAP::ValueArg<std::string> dir_arg("o", "output", "path to a directory where output will be placed. Output directory should contain the stg.cpp, Makefile, rankfile.{0..} (one for each deployment) and a sample run_mpi.sh showing how to phrase the running of all the MPI code", true, "", "output dirpath");
  cmd.add(dir_arg);
  TCLAP::SwitchArg debug_arg("", "debug", "Include println statements in the generated stg.cpp code (slows down MPI execution)");
  
  cmd.parse(argc, argv);
  input_stg = stg_arg.getValue();
  input_dep = dep_arg.getValue();
  input_sys = sys_arg.getValue();
  input_gendir = dir_arg.getValue();
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
  
  build_rankfiles_from_deployment(argv[3], argv[4]);
  build_mpi_from_stl(argv[1], argv[2]);
  
  return 0;
}

void build_rankfiles_from_deployment(char* sd_path, char* output) {
  rapidxml::file<> xmlFile(sd_path);
  rapidxml::xml_document<> doc;
  doc.parse<0>(xmlFile.data());

  rapidxml::xml_node<> *node = doc.first_node("optimization")
                                  ->first_node("deployments")
                                  ->first_node("deployment");
  std::cout << "Name of my first node is: " << node->name() << "\n";
  std::cout << "Node value " << node->value() << "\n";
  for (rapidxml::xml_attribute<> *attr = node->first_attribute();
       attr; attr = attr->next_attribute())
  {
    std::cout << "Node has attribute " << attr->name() << " ";
    std::cout << "with value " << attr->value() << "\n";
  }
  
  // Try to add a unit
  rapidxml::xml_node<> *unit = doc.allocate_node(rapidxml::node_element, "unit");
  node->append_node(unit);
  rapidxml::xml_attribute<> *attr = doc.allocate_attribute("type", "GPP");
  unit->append_attribute(attr);
  
  std::cout << doc;
  
  std::cout << "Done";

}



void build_mpi_case_for_task(unsigned int tid, unsigned int exectime, std::vector<unsigned int>& pre, std::vector<unsigned int>& post, std::ofstream& out);
void build_mpi_from_stl(char* filepath, char* outfilepath) {
  DirectedAcyclicGraph* task_precedence = NULL;
  std::vector<Task>* tasks = parse_stg(filepath, task_precedence);
  std::ofstream out;
  out.open(outfilepath);
  
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

