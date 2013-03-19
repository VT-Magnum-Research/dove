//
//  Created by Brandon Amos
//  Modified by Hamilton Turner
//  Copyright (c) 2013 Virginia Tech. All rights reserved.
//
#include <string>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h> // mkstemp

// Provides regex and exec 
#include "helper.hpp"
#include "main_bootstrapper.hpp"

// XML parsing
#include "libs/rapidxml.hpp"
#include "libs/rapidxml_utils.hpp"
#include "libs/rapidxml_print.hpp"

// DOVE codebase (incl some rapidXML extensions)
#include "libs/rapidxml_myutils.hpp"

// Enable or disable debug statement printing
#define DEBUG true

// Function declarations
void calculate_latency(std::vector<int> ids);

// Everything that is below this line and before main either 
// enables command line parsing or are variables initialized 
// during the parsing of arguments
#include "libs/tclap/CmdLine.h"
static void parse_options(int argc, char *argv[]);

// XML file used describe the system we are testing
rapidxml::xml_document<> xml;

// Logical IDs of all hardware that should have
// latency profiled. Each vector is used to generate
// pair-pair combinations
static std::vector<int> hosts;
static std::vector<int> sockets;
static std::vector<int> cores;
static std::vector<int> threads;

// If these are non-empty after options have been parsed, then they are 
// white filters for any values generated from the pair-pair combinations
// above. For example, cores may be {2, 15, 7}, but if core 2 is situated 
// on node 5, and node 5 is not contained within the list below, then
// any pair-pair combination containing core 2 will be skipped
static std::vector<int> host_filter;
static std::vector<int> sock_filter;
static std::vector<int> core_filter;
static std::vector<int> thread_filter;

int main(int argc, char** argv) {
  try {
    parse_options(argc, argv);
  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    exit(EXIT_SUCCESS);
  }

  calculate_latency(cores);

  // TODO start by comparing the lowest level items and work up. E.g. 
  // for (threadpair in threads)
  //     resolve all pieces of thread
  //     for (filter in filters)
  //        returnif (filter not passed)
  //
  
  // TODO build a model of progress by calculating the total number of 
  // permutations for each hardwrae component type e.g. (# cores P 2)
  
  // TODO build logic for after-parsing options
  //using namespace std;
  
  //return 0;
}
std::string make_rankfile(std::string to, std::string from) {
    char sfn[21] = ""; FILE* sfp; int fd = -1;
     
    strncpy(sfn, "/tmp/rankfile.XXXXXX", sizeof sfn);
    fd = mkstemp(sfn);
    if (fd == -1) return "";

    sfp = fdopen(fd, "w+");
    if (sfp == NULL) {
        unlink(sfn);
        close(fd);
        return "";
    }

    fprintf(sfp, "rank 0=10.0.2.4 slot=p0:%s\n", to.c_str());
    fprintf(sfp, "rank 1=10.0.2.4 slot=p0:%s\n", from.c_str());

    // TODO: This isn't returning a value on the stack, is it?
    return std::string(sfn);
}
std::string get_latency(std::string from, std::string to) {
    using namespace std;

    string mpirun_bin = "mpirun";
    string rankfile = make_rankfile(to, from);
    string mpiflags = "-np 2 --rankfile " + rankfile;
    string latency_bin = "latency_impl/latency";
    // TODO move this to a global variable at the top
#   ifdef LATENCY_BIN
        latency_bin = LATENCY_BIN;
#   endif
    string command = mpirun_bin + " " + mpiflags + " " + latency_bin;
    string result("-1");
// TODO make this a command-line option
#define DRY_RUN false
    if (DRY_RUN)
      cout << command << endl;
    else
      result = exec(command);
    // TODO somehow deal with both cases e.g. latency_bin built with and 
    // without DEBUG
    //string regex_line = "Avg round trip time = [0-9]+";
    //string regex_number = "[0-9]+";
    //string microsec_line = match(result, regex_line);
    //string microsec_str = match(microsec_line, regex_number);
    remove(rankfile.c_str());
    return "<cd f=\"" + from +
           "\"  t=\""   + to +
           "\"  v=\""   + result + "\" />";
}

void calculate_latency(std::vector<int> ids) {
  // TODO make previous functions accept integers 
  // First step is to calculate all permutations
  for (unsigned int i = 0; i < ids.size(); i++) {
      for (unsigned int j = 0; j < ids.size(); j++) {
          if (i != j) {
            std::stringstream si;
            std::stringstream sj;
            si << ids[i];
            sj << ids[j];
            std::string ssi(si.str());
            std::string ssj(sj.str());
            std::cout << get_latency(ssi,ssj) << std::endl;
          }
      }
  }


}

// Given parsed command-line options, this builds the lists that 
// are used to generate pair-pair combinations
void build_main_filter(bool all, bool host, bool socket, bool core, 
    bool thread) {
  typedef rapidxml::xml_node<char> xml_node;
  typedef std::vector<xml_node*> xml_node_vector; 

  if (all || host) {
    xml_node_vector xhosts = dove::get_all_hosts(xml);
    for (xml_node_vector::iterator it = xhosts.begin();
        it != xhosts.end();
        ++it)
    hosts.push_back(atoi((*it)->first_attribute("id")->value()));
  }
  if (all || socket) {
    xml_node_vector xprocs = dove::get_all_processors(xml);
    for (xml_node_vector::iterator it = xprocs.begin();
        it != xprocs.end();
        ++it)
    sockets.push_back(atoi((*it)->first_attribute("id")->value()));
  }
  if (all || core) {
    xml_node_vector xcores = dove::get_all_cores(xml);
    for (xml_node_vector::iterator it = xcores.begin();
        it != xcores.end();
        ++it)
    cores.push_back(atoi((*it)->first_attribute("id")->value()));
  }
  if (all || thread) {
    xml_node_vector xhw = dove::get_all_threads(xml);
    for (xml_node_vector::iterator it = xhw.begin();
        it != xhw.end();
        ++it)
    threads.push_back(atoi((*it)->first_attribute("id")->value()));
  }

  if (DEBUG) {
    std::cerr << "Size of host: " << hosts.size() << std::endl;
    std::cerr << "Size of socket: " << sockets.size() << std::endl;
    std::cerr << "Size of core: " << cores.size() << std::endl;
    std::cerr << "Size of thread: " << threads.size() << std::endl;
  }
}

static void parse_options(int argc, char *argv[]) {
  TCLAP::CmdLine cmd("Latency Analyzer", ' ', "0.1");
  TCLAP::ValueArg<std::string> xml_arg("x", "xml", "System XML file containing the "
      "logical IDs that are referenced from all other flags. Used to gather "
      "additional information about the system that is required for OpenMPI.",
      true, "system.xml", "filepath", cmd);

  // Setup the list of required filters
  TCLAP::SwitchArg all_filter("", "all", "Indicates that latency tests will "
      "be performed for all host-host,socket-socket,core-core,and thread-thread "
      "pairs in the XML file. May take a substantial amount of time. "
      "Can be used with additional filters such as core, node, thread.", 
      false);
  TCLAP::SwitchArg host_filter("", "hosts", "Indicates that latency tests will "
      "only be performed between hosts outlined in the XML."
      "Can be used with additional filters such as core, node, thread.", 
      false);
  TCLAP::SwitchArg socket_filter("", "sockets", "Indicates that latency tests "
      "will be performed for all socket-socket pairs in the XML file. "
      "Can be used with additional filters such as core, node, thread.",
      false);
  TCLAP::SwitchArg core_filter("", "cores", "Indicates that latency tests "
      "will be performed for all core-core pairs in the XML file. "
      "Can be used with additional filters such as core, node, thread.",
      false);
  TCLAP::SwitchArg thread_filter("", "threads", "Indicates that latency tests "
      "will be performed for all hardware_thread-hardware_thread pairs in the "
      "XML file. "
      "Can be used with additional filters such as core, node, thread.",
      false);
  std::vector<TCLAP::Arg*> filter_list;
  filter_list.push_back(&all_filter);
  filter_list.push_back(&host_filter);
  filter_list.push_back(&socket_filter);
  filter_list.push_back(&core_filter);
  filter_list.push_back(&thread_filter);
  cmd.xorAdd(filter_list);

  // List all of the partial filters that are possible
  TCLAP::MultiArg<int> node_arg("n", "node", "An additional filter for "
      "limiting the nodes eligible for node-nodepairs to the ones "
      "explicitely listed using this option. Can be listed multiple times "
      "e.g. `-n 3 -n 2` to specify multiple nodes. All IDs are the "
      "logical IDs provided by the XML file. Note that providing only one "
      "value will do nothing, as there are zero pairs possible. ", false, 
      "int", cmd);
  TCLAP::MultiArg<int> sock_arg("s", "socket", "An additional filter for "
      "limiting the sockets eligible for socket-socket pairs to the ones "
      "explicitely listed using this option. Can be listed multiple times "
      "e.g. `-s 1 -s 2` to specify multiple sockets. All IDs are the "
      "logical IDs provided by the XML file. Note that providing only one "
      "value will do nothing, as there are zero pairs possible. ", false, 
      "int", cmd);
  TCLAP::MultiArg<int> core_arg("c", "core", "An additional filter for "
      "limiting the cores eligible for core-core pairs to the ones "
      "explicitely listed using this option. Can be listed multiple times "
      "e.g. `-c 1 -c 2` to specify multiple cores. All IDs are the "
      "logical IDs provided by the XML file. Note that providing only one "
      "value will do nothing, as there are zero pairs possible. ", false, 
      "int", cmd);
  TCLAP::MultiArg<int> thread_arg("t", "thread", "An additional filter for "
      "limiting the hw threads eligible for thread-thread pairs to the ones "
      "explicitely listed using this option. Can be listed multiple times "
      "e.g. `-t 1 -t 2` to specify multiple threads. All IDs are the "
      "logical IDs provided by the XML file. Note that providing only one "
      "value will do nothing, as there are zero pairs possible. ", false, 
      "int", cmd);
 
  cmd.parse(argc, argv);

  // Ensure that the given XML path is accessible by rapidxml
  try {
    rapidxml::file<> xml_file(xml_arg.getValue().c_str());
    xml.parse<0>(xml_file.data());
  } catch (rapidxml::parse_error err) {
    std::cout << "Could not parse XML file. Error was: " << std::endl;
    std::cout << err.what() << std::endl;
    TCLAP::ArgException e("Unable to parse XML", err.what());
    throw e;
  } 

  // Use the high-level filter (all, cores, etc) to read in the XML 
  // file and build a list of all items that need to be compared. 
  build_main_filter(all_filter.getValue(), host_filter.getValue(), 
      socket_filter.getValue(), core_filter.getValue(),
      thread_filter.getValue());
  
   
  
}

