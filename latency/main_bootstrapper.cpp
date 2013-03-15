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
// white filters that only allow the logical IDs within them. 
// 
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
  using namespace std;
  for (int i = 1; i < argc; i++) {
      for (int j = 1; j < argc; j++) {
          if (i != j) {
              cout << get_latency(argv[i], argv[j]) << endl;
          }
      }
  }
  return 0;
}

std::string get_latency(std::string from, std::string to) {
    using namespace std;

    string mpirun_bin = "mpirun";
    string rankfile = make_rankfile(to, from);
    string mpiflags = "-np 2 --rankfile " + rankfile;
    string latency_bin = "latency_impl/latency";
#   ifdef LATENCY_BIN
        latency_bin = LATENCY_BIN;
#   endif
    string command = mpirun_bin + " " + mpiflags + " " + latency_bin;

    string result = exec(command);

    string regex_line = "Avg round trip time = [0-9]+";
    string regex_number = "[0-9]+";
    string microsec_line = match(result, regex_line);
    string microsec_str = match(microsec_line, regex_number);
    remove(rankfile.c_str());
    return "<cd f=\"" + from +
           "\"  t=\""   + to +
           "\"  v=\""   + microsec_str + "\" />";
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

// Given parsed command-line options, this builds the lists that 
// are used to generate pair-pair combinations
void build_main_filter(bool all, bool host, bool socket, bool core, 
    bool thread) {
  if (all || host) {
  }
  if (all || socket) {
  }
  if (all || core) {
  }
  if (all || thread) {
  }
}

static void parse_options(int argc, char *argv[]) {
  TCLAP::CmdLine cmd("Latency Analyzer", ' ', "0.1");
  TCLAP::ValueArg<std::string> xml_arg("x", "xml", "System XML file containing the "
      "logical IDs that are referenced from all other flags. Used to gather "
      "additional information about the system that is required for OpenMPI.",
      true, "filepath");
  cmd.add(xml_arg);

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
  std::vector<Arg*> filter_list;
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
    rapidxml::file<> xml_file(xml_arg.getValue());
    xml.parse<0>(xml_file.data());
  } catch (rapidxml::parse_error err) {
    std::cout << "Could not parse XML file. Error was: " << std::endl;
    std::cout << err.what() << std::endl;
    std::cout << err.where() << std::endl;
    TCLAP::ArgException e("Unable to parse XML", err.what());
    throw e;
  } 

  // TODO Use the high-level filter (all, cores, etc) to read in the XML 
  // file and build a list of all items that need to be compared. 
  build_main_filter(all_filter.getValue(), host_filter.getValue(), 
      socket_filter.getValue(), core_filter.getValue(),
      thread_filter.getValue());

  // TODO start by comparing the lowest level items and work up. E.g. 
  // for (threadpair in threads)
  //     resolve all pieces of thread
  //     for (filter in filters)
  //        returnif (filter not passed)
  //
  // 
  
}


std::string build_rankline_for_logical_id() {

  // Load XML files
  rapidxml::xml_document<> dep_doc;
  rapidxml::file<> xml_deployment(deployment_xml_path.c_str());
  dep_doc.parse<0>(xml_deployment.data());
  
  // Pull out the mapping we are using and translate
  // that into a proper rankfile format
  std::string mapping = dep_doc.first_node("optimization")->
  first_node("mapping")->first_attribute("to")->value();
  void (*write_rank_line) (std::ofstream&, int, std::string, int, int);

  if (mapping.compare("cores") == 0) 
    write_rank_line = &write_rank_core;
  else if (mapping.compare("nodes") == 0){
    throw "only cores supported now";
  } else if (mapping.compare("hw-threads") == 0) {
    throw "only cores supported now";
  } else if (mapping.compare("processors") == 0) {
    throw "only cores supported now";
  } else
    throw "The provided mapping was not recognized. Use one of cores,nodes,processors,hw-threads";
  
  // Locate all deployments
  rapidxml::xml_node<>* deps = dep_doc.first_node("optimization")->
  first_node("deployments");
  for (rapidxml::xml_node<>* deployment = deps->first_node();
       deployment;
       deployment = deployment->next_sibling()) {
    
    // Start writing the rankfile
    std::string id = deployment->first_attribute("id")->value();
    std::string rankfile = outdir;
    rankfile.append("rankfile.").append(id);
    std::ofstream rf;
    rf.open(rankfile.c_str());
    
    // Iterate over every mapping in deployment
    for (rapidxml::xml_node<>* mapping = deployment->first_node();
         mapping;
         mapping = mapping->next_sibling()) {
     
      // Skip anything that's not a 'deploy' tag
      if (strcmp(mapping->name(), "deploy") != 0)
        continue;

      int rank = atoi( mapping->first_attribute("t")->value() );
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
      
      // Then write them to the file
      write_rank_line(rf, rank, hostname, proc_pid, core_pid);

    } // Done with mappings for one deployment
       
    rf.close();
  } // Done with all deployments
  

}
