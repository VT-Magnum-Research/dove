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
#include <iostream>
#include <unistd.h> // unlink, close

// Provides regex and exec 
#include "helper.hpp"

// XML parsing
#include "libs/rapidxml.hpp"
#include "libs/rapidxml_utils.hpp"
#include "libs/rapidxml_print.hpp"

#include "dove.h"

// Each pass of the -v flag raises this by one
// 0 prints errors
// 1 prints info
// 2 prints debug
static int log_level = 0;
void log(const char* msg, int level) {  if (level <= log_level) std::cout << 
  "profile: " << msg << std::endl; }
void info(const char* msg) { log(msg, 1); }
void error(const char* msg) { log(msg, 0); }
void debug(const char* msg) { log(msg, 2); }

// Function declarations
void calculate_latency(std::vector<int> ids);
std::string get_latency(std::string to, std::string from);
std::string make_rankfile(std::string to, std::string from);

// Everything that is below this line and before main either 
// enables command line parsing or are variables initialized 
// during the parsing of arguments
#include "libs/tclap/CmdLine.h"
static void parse_options(int argc, char *argv[]);

// XML file used describe the system we are testing
rapidxml::xml_document<char>* xml;
std::string xml_filepath;
rapidxml::file<char>* xml_file;

// If true, the latency is not actually profiled 
static bool dry_run = false;

// If true, progress will be printed as latency is calculated
static bool print_progress = false;

// The path to the `latency` binary.
// Overwritten if LATENCY_BIN is defined.
static std::string latency_bin = "latency_impl/latency";

// Logical IDs of all hardware that should have
// latency profiled. Each vector is used to generate
// pair-pair combinations
static std::vector<int> hosts;
static std::vector<int> sockets;
static std::vector<int> cores;
static std::vector<int> threads;

// TODO re-enable after programming options
// If these are non-empty after options have been parsed, then they are 
// white filters for any values generated from the pair-pair combinations
// above. For example, cores may be {2, 15, 7}, but if core 2 is situated 
// on node 5, and node 5 is not contained within the list below, then
// any pair-pair combination containing core 2 will be skipped
//static std::vector<int> host_filter;
//static std::vector<int> sock_filter;
//static std::vector<int> core_filter;
//static std::vector<int> thread_filter;

int main(int argc, char** argv) {
# ifdef LATENCY_BIN
    latency_bin = LATENCY_BIN;
# endif
  xml = new rapidxml::xml_document<char>();
  try {
    parse_options(argc, argv);
  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    delete xml;
    exit(EXIT_SUCCESS);
  }

  
  // TODO for the low-level filters, start by comparing the 
  // lowest level items and work up. E.g. 
  // for (threadpair in threads)
  //     resolve all pieces of thread
  //     for (filter in filters)
  //        returnif (filter not passed)
  //
 
  // Prepare for writing a file
  // TODO Create a temporary file at the start, flush updated XML to 
  // the temporary file every latency measurement so we don't loose 
  // everything if we crash
  xml_filepath.append(".updated");
  std::ofstream output(xml_filepath.c_str(), std::ios::trunc | std::ios::out);
  if (!dry_run && !output.is_open())
  {
    error("Refusing to profile latency: Unable to open xml file for output:");
    error(xml_filepath.c_str());
    delete xml;
    exit(EXIT_FAILURE);
  }

  // TODO update rapidxml_myutils to enable building of 
  // ranklines for these other options
  //
  //calculate_latency(hosts);
  //calculate_latency(sockets);
  //calculate_latency(threads);
  calculate_latency(cores);

  if (!dry_run) {
    output << *xml;
    output.close();
  }


  delete xml;
  exit(EXIT_SUCCESS);
}

std::string make_rankfile(int to, int from) {
    char sfn[21] = "/tmp/rankfile.XXXXXX"; FILE* sfp; int fd = -1;
     
    fd = mkstemp(sfn);
    if (fd == -1) return "";

    sfp = fdopen(fd, "w+");
    if (sfp == NULL) {
        unlink(sfn);
        close(fd);
        return "";
    }

    // TODO either use the logic in rapidxml_myutils.hpp to 
    // build ranklines, or move this logic there
    fprintf(sfp, "rank 0=10.0.2.5 slot=p0:%d\n", to);
    fprintf(sfp, "rank 1=10.0.2.5 slot=p0:%d\n", from);

    return std::string(sfn);
}

// Makes a string 'safe' for use with rapidxml
char* s(const char* unsafe) {
  return xml->allocate_string(unsafe);
}

// Given the to and from, this writes them into the XML file at
// the proper location and also returns the XML string written
std::string write_latency(int to, int from) {
    using namespace std;

    string mpirun_bin = "mpirun";
    string rankfile = make_rankfile(to, from);
    string mpiflags = "-np 2 --rankfile " + rankfile;
    string command = mpirun_bin + " " + mpiflags + " " + latency_bin;
    string result("-1");
    if (dry_run)
      info(command.c_str());
    else
      result = exec(command);
    
    // TODO somehow deal with both cases e.g. latency_bin built with and 
    // without DEBUG
    //
    //string regex_line = "Avg round trip time = [0-9]+";
    //string regex_number = "[0-9]+";
    //string microsec_line = match(result, regex_line);
    //string microsec_str = match(microsec_line, regex_number);
    remove(rankfile.c_str());

    rapidxml::xml_node<char>* delay = xml->
      allocate_node(rapidxml::node_element, "d");
    rapidxml::xml_attribute<> *fattr = xml->allocate_attribute("f", 
        s(to_string((long long) from).c_str()));
    rapidxml::xml_attribute<> *tattr = xml->allocate_attribute("t", 
        s(to_string((long long) to).c_str()));
    rapidxml::xml_attribute<> *vattr = xml->allocate_attribute("v", 
        s(result.c_str()));
    delay->append_attribute(fattr);
    delay->append_attribute(tattr);
    delay->append_attribute(vattr);

    if (!dry_run) {
      // Find the right place in the XML
      rapidxml::xml_node<char>* system = xml->first_node("system");
      if (system == 0)
      {
        error("Unable to find system!");
        error("Printing all XML so we can debug....");
        rapidxml::print(std::cerr, *xml, 0);
        error("Now to segfault...");
      }
      rapidxml::xml_node<char>* delays = system->first_node("routing_delays");
      if (delays == 0) {
        delays = xml->allocate_node(rapidxml::node_element, "routing_delays");
        xml->first_node("system")->append_node(delays);
      }
    
      delays->append_node(delay);
    }

    // Print to string using output iterator
    std::string s;
    rapidxml::print(std::back_inserter(s), *delay, 0);
    return s;
}

void calculate_latency(std::vector<int> ids) {
  // Use (N Permutation 2) formula, where N = ids.size
  // However this makes huge numbers, so we take a shortcut and
  // realize that N P 2 = N * N-1
  unsigned long total = (unsigned long) ids.size()* ((unsigned long) ids.size() - 1);
  unsigned long progress = 0;
  time_t start = time(0);

  // First step is to calculate all permutations
  for (unsigned int i = 0; i < ids.size(); i++) {
      for (unsigned int j = 0; j < ids.size(); j++) {
          if (i != j) {
            info(write_latency(ids[i], ids[j]).c_str());
            progress++;
            if (print_progress)
            {
              double p = (double) progress / (double) total;
              p = p * 1000;
              p = (double) ((int) p);
              p = p / 10;
              time_t end = time(0);
              std::cout << "profile: Progress: " << p << "% (" << 
                progress << "/" << total << ") in " <<
                (int) difftime(end, start) <<
                " seconds" << std::endl;
            }
          }
      }
  }
  info("Done calculating all latency!");
}

// Given parsed command-line options, this builds the lists that 
// are used to generate pair-pair combinations
void build_main_filter(bool all, bool host, bool socket, bool core, 
    bool thread) {
  typedef rapidxml::xml_node<char> xml_node;
  typedef std::vector<xml_node*> xml_node_vector; 

  if (all || host) {
    xml_node_vector xhosts = dove::get_all_hosts(*xml);
    for (xml_node_vector::iterator it = xhosts.begin();
        it != xhosts.end();
        ++it)
    hosts.push_back(atoi((*it)->first_attribute("id")->value()));
  }
  if (all || socket) {
    xml_node_vector xprocs = dove::get_all_processors(*xml);
    for (xml_node_vector::iterator it = xprocs.begin();
        it != xprocs.end();
        ++it)
    sockets.push_back(atoi((*it)->first_attribute("id")->value()));
  }
  if (all || core) {
    xml_node_vector xcores = dove::get_all_cores(*xml);
    for (xml_node_vector::iterator it = xcores.begin();
        it != xcores.end();
        ++it)
    cores.push_back(atoi((*it)->first_attribute("id")->value()));
  }
  if (all || thread) {
    xml_node_vector xhw = dove::get_all_threads(*xml);
    for (xml_node_vector::iterator it = xhw.begin();
        it != xhw.end();
        ++it)
    threads.push_back(atoi((*it)->first_attribute("id")->value()));
  }

  //std::cerr << "Size of host: " << hosts.size() << std::endl;
  //std::cerr << "Size of socket: " << sockets.size() << std::endl;
  //std::cerr << "Size of core: " << cores.size() << std::endl;
  //std::cerr << "Size of thread: " << threads.size() << std::endl;
}

// TODO add flag for output
// TODO program logic for sub-filter (e.g. run_latency --cores -n 1 -n 2 
// would run all cores, but only those that reside on nodes 1 and two). 
// Once this logic is programmed re-enable all of the options
static void parse_options(int argc, char *argv[]) {
  TCLAP::CmdLine cmd("Latency Analyzer", ' ', "0.1");
  TCLAP::ValueArg<std::string> xml_arg("x", "xml", "System XML file containing the "
      "logical IDs that are referenced from all other flags. Used to gather "
      "additional information about the system that is required for OpenMPI.",
      true, "system.xml", "filepath", cmd);
  TCLAP::SwitchArg clear_xml("", "clearxml", "Removes all delay (d) tags from "
      "the XML file. Effectively indicates that this profiling pass will be "
      "overwriting any prior profiling information. Without this flag delay "
      "tags will be appended. If two or more delay tags exist with the same "
      "to/from values that will *not* throw errors, and later behaviour of "
      "dove is undefined", cmd);
  TCLAP::SwitchArg show_progress("p", "progress", "With this flag, progress "
      "will be reported as the program is executing", cmd);

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

  TCLAP::SwitchArg dry_filter("", "dryrun", "No latency tests will "
      "be performed, but all mpirun commands will be printed and temporary "
      "files will be created. Implies at least one instance of the verbosity "
      "flag e.g. -v. Passed XML file will not be modified at all", cmd);
  TCLAP::MultiSwitchArg verbosity("v","verbose","Enables printing of any "
    "verbose ouptut. Passing the flag multiple times e.g. -vvv increases "
    "verbosity further. With no -v flag only errors or final summaries "
    "are printed", cmd, 0);

  // List all of the partial filters that are possible
  /*TCLAP::MultiArg<int> node_arg("n", "node", "An additional filter for "
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
  */
  cmd.parse(argc, argv);

  // Ensure that the given XML path is accessible by rapidxml
  try {
    info("Trying to parse the following xml file:");
    xml_filepath = xml_arg.getValue();
    info(xml_filepath.c_str());
    xml_file = new rapidxml::file<char>(xml_arg.getValue().c_str());
    xml->parse<0>(xml_file->data());
  } catch (rapidxml::parse_error err) {
    std::cout << "Could not parse XML file. Error was: " << std::endl;
    std::cout << err.what() << std::endl;
    TCLAP::ArgException e("Unable to parse XML", err.what());
    throw e;
  }

  print_progress = show_progress.getValue();
  dry_run = dry_filter.getValue();
  log_level = verbosity.getValue();
  // Require at least info logging if we are doing a dry run
  if (dry_run && log_level==0)
    log_level = 1;

  if (clear_xml.getValue()) {
    if (dry_run) 
      info("Refusing to clear XML due to this being a dry run");
    else {
      rapidxml::xml_node<char>* delays = 
        xml->first_node("system")->first_node("routing_delays");
      if (delays != 0) {
        xml->first_node("system")->remove_node(delays);
        info("Removed the routing_delays node from the xml");
      }
    }
  }

  // Use the high-level filter (all, cores, etc) to read in the XML 
  // file and build a list of all items that need to be compared. 
  build_main_filter(all_filter.getValue(), host_filter.getValue(), 
      socket_filter.getValue(), core_filter.getValue(),
      thread_filter.getValue()); 
}


