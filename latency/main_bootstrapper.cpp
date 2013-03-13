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

// Command line parsing
#include "libs/tclap/CmdLine.h"
static void parse_options(int argc, char *argv[]);
static std::vector<int> sockets;
static std::vector<int> cores;

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
  sockets = sock_arg.getValue();
  cores = core_arg.getValue();
}

// Given an XML node, find the child node with id equaling the passed id
// http://stackoverflow.com/questions/5465227
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

  rapidxml::file<> xml_system(system_xml_path.c_str());
  rapidxml::xml_document<> sys_doc;
  sys_doc.parse<0>(xml_system.data());
  
  rapidxml::xml_node<>* nodes = sys_doc.first_node("system")->
  first_node("nodes");
  rapidxml::xml_node<>* node = get_child_with_id(nodes, logical_id);
  
  // While we have not returned to the root
  while (strcmp(node->name(), "nodes") != 0) {
    std::string name = node->name();
    int pindex = atoi( node->first_attribute("pindex")->value() );
    if (name.compare("pu") == 0)
      hwth_pid = pindex;
    else if (name.compare("core") == 0)
      core_pid = pindex;
    else if (name.compare("socket") == 0) 
      proc_pid = pindex;
    else if (name.compare("node") == 0) {
      node_pid = pindex;
      ip = node->first_attribute("ip")->value();
      hostname = node->first_attribute("hostname")->value();
    }
    else
      throw "Unknown tag in XML. Valid values underneath 'nodes' are node,socket,core,pu";
    
    node = node->parent();
  }   
}

void write_rank_core(std::ofstream& out, int rank, std::string host, int procpid, int corepid) {
  // Cores are "rank %s=%s slot=p%d:%d\n" 
  // rank 1=10.0.2.4 slot=p1:8
  // references physical socket 1 and physical core 8
  out << "rank " << rank << "=" << host << " slot=p" << procpid << ":" << corepid << std::endl;
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
