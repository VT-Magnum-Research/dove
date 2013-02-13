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
#include "main.h" // Includes Task and STG parser

namespace mpi = boost::mpi;

// Forward declare methods to come
void run_simple_mpi(int argc, char* argv[]);
void build_mpi_from_stl(char* file_path, char* outfile_p);

bool SAMPLE = false;

// Simple usage: a.out <input_STG_file_path> <output_file_path>
int main(int argc, char* argv[])
{
  if (SAMPLE)
    run_simple_mpi(argc, argv);
  else
    build_mpi_from_stl(argv[1], argv[2]);
  
  return 0;
}

bool DEBUG_LOG = true;
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
  
  // Execution time array
//  out <<
//    "  long execution_times [" << tasks->size() << "] = {";
//  
//  unsigned long end = tasks->size();
//  for (int i = 0; i < end; i++)
//    out << tasks->at(i).execution_time_ <<
//      (i==(end-1) ? "};\n\n" : ", ");
  
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




// TODO: Sends need to be non-blocking. Perhaps also replace the current recv with receive all so that there are
// not unexpected items. Also add in the ability to remove the data on the send
void run_simple_mpi(int argc, char* argv[]) {
  
  mpi::environment env(argc, argv);
  mpi::communicator world;
  long execution_times [4] = {1, 6, 1, 2};
  
  switch (world.rank()) {
    case 0: {
      std::cout << "0: Awake" << std::endl;
      time_t start;
      start = time (NULL);
      std::cout << "0: Started compute" << std::endl;
      while ((time(NULL)-start) < execution_times[0]);
      
      std::cout << "0: Finished compute in " << (time(NULL)-start) << std::endl;
      
      mpi::request reqs[2];
      reqs[0] = world.isend(1, 0);
      reqs[1] = world.isend(2, 0);
      mpi::wait_all(reqs, reqs + 2);
      std::cout << "0: Sent tag 1 to rank 2" << std::endl;
      std::cout << "0: Sent tag 0 to rank 1" << std::endl;
      
      break; }
    case 1: {
      std::cout << "1: Awake" << std::endl;
      world.recv(0, 0); // Recv tag '0' from rank 0
      std::cout << "1: Recv tag 0 from rank 0" << std::endl;
      time_t start;
      start = time (NULL);
      std::cout << "1: Started compute" << std::endl;
      while ((time(NULL)-start) < execution_times[1]);
      std::cout << "1: Finished compute in " << (time(NULL)-start) << std::endl;
      world.send(3, 0);
      break; }
    case 2: {
      std::cout << "2: Awake" << std::endl;
      world.recv(0, 0);
      std::cout << "2: Recv tag 0 from rank 0" << std::endl;
      time_t start;
      start = time (NULL);
      std::cout << "2: Started compute" << std::endl;
      while ((time(NULL)-start) < execution_times[2]);
      std::cout << "2: Finished compute in " << (time(NULL)-start) << std::endl;
      world.send(3, 0);
      break; }
    case 3: {
      std::cout << "3: Awake" << std::endl;
      
      mpi::request req[2];
      req[0] = world.irecv(1, 0);
      req[1] = world.irecv(2, 0);
      mpi::wait_all(req, req + 2);
      std::cout << "3: Recv tag 0 from rank 1" << std::endl;
      std::cout << "3: Recv tag 0 from rank 2" << std::endl;
      time_t start;
      start = time (NULL);
      std::cout << "3: Started compute" << std::endl;
      while ((time(NULL)-start) < execution_times[3]);
      std::cout << "3: Finished compute in " << (time(NULL)-start) << std::endl;
      
      std::cout << "3: DONE!!" << std::endl;
      break; }
  }
}

