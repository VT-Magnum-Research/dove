
// Copyright (C) 2013 Hamilton Turner
//
// Deployment Optimization Validation Engine (dove)
//
// Depends upon Rapidxml 1.13
//

#include "dove.h"


#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string.h>
#include <map>

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include "rapidxml_utils.hpp"


// We extend rapidxml with some high-level functions
namespace rapidxml
{
  // Given an XML node, recursively find the first child node (e.g. not including the 
  // passed node) that has an attribute with the passed name and value. 
  // Uses string matching on the attribute value. If no value is found 
  // 0 is returned
  //
  // http://stackoverflow.com/questions/5465227
  template<class Ch>
  inline xml_node<Ch>* get_child_with_attribute(xml_node<Ch> *node, 
    std::string attribute_name, std::string attribute_value)
  {
    // Cycles every child
    for (xml_node<> *nodeChild = 
        node->first_node(); 
        nodeChild; 
        nodeChild = nodeChild->next_sibling())
    {
      xml_attribute<Ch>* attribute = nodeChild->first_attribute(attribute_name.c_str());
      if (attribute != 0 && strcmp(attribute->value(), attribute_value.c_str()) == 0)
        return nodeChild;

      xml_node<Ch>* x = get_child_with_attribute(nodeChild, 
          attribute_name, attribute_value);
      if (x != 0) 
        return x;
    }
    return 0;
  }
}

// Set log_level as high as you want to enable that logging level
// and all below it. 999 will be very verbose
#define LOG_LEVEL 999
#define LOG_ERROR  10
#define LOG_DEBUG  30
#define LOG_INFO  100

// TODO: Make logs outside of this main namespace?
// Or, is the design idea to keep everything inside of here?
// Java has made me keen on smaller-sized files.
void dove::xlog(const char* msg, int level) {
  if (level <= LOG_LEVEL)
    std::cout << "dove: " << msg << std::endl;
}

void dove::info(const char* msg) { xlog(msg, LOG_INFO); }
void dove::error(const char* msg) { xlog(msg, LOG_ERROR); }
void dove::xdebug(const char* msg) { xlog(msg, LOG_DEBUG); }

dove::config dove::use_tclap(TCLAP::CmdLine &cmd, int argc, char *argv[]) {
  info("Entering add_tclap");
  // TODO remove any short option flags (f/d) to avoid TCLAP conflicts for 
  // other projects
  info("Constructor done");
  TCLAP::ValueArg<std::string> dove_dir_arg =
    TCLAP::ValueArg<std::string>("d", "dove", "Path to DOVE directory. "  
      "Provide this if you want to use the deployment optimization validation "
      "engine to validate that your arguments + this algorithm are in fact "
      "resulting in optimizations on a real system."
      "If provided, then the following files are assumed to exist: \n" 
      "<dove>/system.xml - Contains accurate profile of the hardware system" 
      "dove will use to validate the output of this algorithm. \n" 
      "<dove>/software.stg - the software STG model.\n" 
      "The <dove>/deployments.xml file will be created as this algorithm is " 
      "executed. ", false, "", "dirpath");
  // TODO if we are going to be providing this argument, then the STG parser should
  // be part of the dove framework, perhaps under dove::parsers::stg
  TCLAP::ValueArg<std::string> stg_path_arg =
    TCLAP::ValueArg<std::string>
    ("f", "stg", "path to the input STG file", true, "", "filepath");
  std::vector<TCLAP::Arg *> stg_variants;
  stg_variants.push_back(&dove_dir_arg);
  stg_variants.push_back(&stg_path_arg);
  info("Arguments addedi");
  cmd.xorAdd(stg_variants);

  cmd.parse(argc, argv);
  info("Returning...");
 
  std::string dove = dove_dir_arg.getValue();
  info("Dove string is...");
  info(dove.c_str());
  config_.sys = dove;
  config_.sys.append("system.xml");
  std::string stg = dove;
  stg.append("software.stg");
  config_.deps = dove;
  config_.deps.append("deployments.xml");
  if (dove.compare("") != 0)
  {
    std::ifstream ifile(config_.sys.c_str());
    if (!ifile) 
      throw TCLAP::ArgException("<dove>/system.xml not found");
    std::ifstream ifile2(stg.c_str());
    if (!ifile2)
      throw TCLAP::ArgException("<dove>/software.stg not found"); 

    config_.stg_filepath = stg;
  } else {
    config_.stg_filepath = stg_path_arg.getValue();
  }
  info("Returning...");
  info(config_.stg_filepath.c_str());
  return config_;
}

// TODO make this set type on hardware component
void dove::parse_pids(rapidxml::xml_document<char> &system, 
  std::string logical_id,
  int &node_pid, 
  int &proc_pid, 
  int &core_pid, 
  int &hwth_pid, 
  std::string &hostname, 
  std::string &ip) {

rapidxml::xml_node<>* nodes = system.first_node("system")->
  first_node("nodes");
rapidxml::xml_node<>* node = get_child_with_attribute(nodes, "id", 
    logical_id);

// While we have not returned to the root
while (strcmp(node->name(), "nodes") != 0) {
  std::string name = node->name();
  int pindex = atoi( node->first_attribute("pindex")->value() );
  if (name.compare("pu") == 0) {
    hwth_pid = pindex;
  }
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

// TODO make this set type on hardware component
dove::hwcom dove::parse_pids(rapidxml::xml_document<char> &system, 
    std::string logical_id) {
  dove::hwcom com;
  parse_pids(system, logical_id, 
      com.node_pid, com.proc_pid, com.core_pid,
      com.hwth_pid, com.hostname, com.ip);
  return com;
}

std::string dove::build_rankline_core(int task, 
    std::string host, int procpid, int corepid) {
  // Cores are "rank %s=%s slot=p%d:%d\n" 
  // rank 1=10.0.2.4 slot=p1:8
  // references physical socket 1 and physical core 8
  std::ostringstream out;
  out << "rank " 
    << task << "=" << host 
    << " slot=p" << procpid << ":" 
    << corepid << std::endl;

  return out.str();
}

std::string dove::build_rankline_core(int task, dove::hwcom com) {
  return build_rankline_core(task, com.hostname, com.proc_pid, com.core_pid);
}

std::string dove::build_rankline(rapidxml::xml_document<char> &system,
    int taskid, 
    std::string id) {
  xdebug("Building rankline");
  dove::hwcom com = parse_pids(system, id);

  int type = com.type;
  switch (type) {
  case HW_THREAD:
    // TODO support threads
    throw "Only supports cores for now";
    break;
  case CORE:
    return build_rankline_core(taskid, com);
  case SOCKET:
    // TODO support sockets
    throw "Only supports cores for now";
    break;
  case HOST: 
    throw "Only supports cores for now";
    // TODO support hosts
    break;
  case UNKNOWN:
  default: 
    throw "Unknown hardware component type";
    break;
  }
  throw "Unknown state";
}

// TODO consider creating dove::xml and moving all of my 
// helper functions into that namespace to keep it clean
// std::vector<rapidxml::xml_node<char>*> dove::get_all_hosts(
//     rapidxml::xml_document<char> &system) {
//   xdebug("Getting all hosts from system.xml");
//   rapidxml::xml_node<>* nodes = system.first_node("system")->
//     first_node("nodes");
//   
//   std::vector<rapidxml::xml_node<char>*> result;
//   for (rapidxml::xml_node<char> *child = nodes->first_node();
//       child;
//       child = child->next_sibling()) {
//     if (strcmp(child->name(), "node")==0)
//       result.push_back(child);
//   }
//   
//   return result;
// }
// 
// std::vector<rapidxml::xml_node<char>*> dove::get_all_processors(
//     rapidxml::xml_document<char> &system) {
//   xdebug("Getting all processors from system.xml");
//   std::vector<rapidxml::xml_node<char>*> result;
//   std::vector<rapidxml::xml_node<char>*> hosts = 
//     get_all_hosts(system);
//   std::vector<rapidxml::xml_node<char>*>::iterator it;
//   for (it = hosts.begin();
//       it != hosts.end();
//       ++it) {
//     rapidxml::xml_node<char>* host = *it;
//     for (rapidxml::xml_node<char>* proc = host->first_node();
//         proc;
//         proc = proc->next_sibling()) {
//       if (strcmp(proc->name(), "socket")==0)
//         result.push_back(proc);
//     }
//   }
// 
//   return result;
// }
// 
// std::vector<rapidxml::xml_node<char>*> dove::get_all_cores(
//     rapidxml::xml_document<char> &system) {
//   xdebug("Getting all cores from system.xml");
//   xml_node_vector result;
//   xml_node_vector procs = get_all_processors(system);
//   xml_node_vector::iterator it;
//   for (it = procs.begin();
//       it != procs.end();
//       ++it) {
//     rapidxml::xml_node<char>* proc = *it;
//     for (rapidxml::xml_node<char>* core = proc->first_node();
//         core;
//         core = core->next_sibling()) {
//       if (strcmp(core->name(), "core")==0)
//         result.push_back(core);
//     }
//   }
//   
//   return result;
// }
// 
// std::vector<rapidxml::xml_node<char>*> dove::get_all_threads(
//     rapidxml::xml_document<char> &system) {
//   xdebug("Getting all threads from system.xml");
//   xml_node_vector result;
//   xml_node_vector cores = get_all_cores(system);
//   xml_node_vector::iterator it;
//   for (it = cores.begin();
//       it != cores.end();
//       ++it) {
//     rapidxml::xml_node<char>* core = *it;
//     for (rapidxml::xml_node<char>* hwth = core->first_node();
//         hwth;
//         hwth = hwth->next_sibling()) {
//       if (strcmp(hwth->name(), "pu")==0)
//         result.push_back(hwth);
//     }
//   }
//   
//   return result;
// }

// TODO add in a strategy for choosing specific hardware 
// components e.g. ones on the same machine, etc
dove::hwprofile::hwprofile(hwcom_type type, int compute_units, 
    dove::xml::system* const system) {
  xdebug("Creating new hardware profile");
 
  system_ = system;
  type_ = type;

  std::vector<rapidxml::xml_node<char>*> nodes;
  switch (type) {
    case HOST: 
      throw "Only cores are supported";
      break;
    case PROC:
      throw "Only cores are supported";
      break;
    case CORE:
      info("Getting all cores");
      nodes = system_->get_all_cores();
      info("Done getting cores");
      break;
    case HW_THREAD:
      throw "Only cores are supported";
      break;
    default:
      throw "Hardware component type must be know to create a hardware profile";
      break;
  }
  
  // Our first 'strategy' for choosing the components is quite 
  // simple: whichever ones appear first in the XML ;-)
  if (nodes.size() < compute_units)
    throw "There are not enough available compute units";
  info("Choosing cores");
  for (int u = 0; u < compute_units; u++) {
    char* logid_str = nodes[u]->first_attribute("id")->value();
    std::istringstream stream(logid_str);
    int logid;
    stream >> logid;
    ids_[u] = logid;
  }
}

int dove::hwprofile::get_logical_id(int id) {
  if (id >= ids_.size() || id < 0)
    throw "Invalid ID passed to get_logical_id";
  return ids_[id];
}

long dove::hwprofile::get_routing_delay(int from, int to) {
  int lfrom = get_logical_id(from);
  int lto = get_logical_id(to);
  if (lfrom == lto)
    return 0;
  
//   rapidxml::xml_node<>* system = system_->first_node("system");
//   //if (system != 0)
//   //  info("Got system");
//   //else
//   //  info("Tried to fetch system, but it was not returned");
//   rapidxml::xml_node<>* delays = system->last_node("routing_delays");
//   if (delays == 0)
//     info("Delays does not exist, expect segfaults next!");
// 
//   for (rapidxml::xml_node<char> *delay = 
//         delays->first_node(); 
//         delay; 
//         delay = delay->next_sibling()) {
//     char* sfrom = delay->first_attribute("f")->value();
//     std::istringstream ssfrom(sfrom);
//     int from;
//     ssfrom >> from;
//     if (from != lfrom) 
//       continue;
//     char* sto = delay->first_attribute("t")->value();
//     std::istringstream ssto(sto);
//     int to;
//     ssto >> to;
//     if (to == lto) {
//       char* sval = delay->first_attribute("v")->value();
//       std::istringstream ssval(sval);
//       int val;
//       ssval >> val;
//       return val;
//     }
//   }
//   throw "No route was found between the two id's";
  return system_->get_routing_delay(from, to, lfrom, lto);
}
      
// char* dove::deployment::s(const char* unsafe) {
//    return string_pool->allocate_string(unsafe);
// }
// 
// char* dove::deployment::s(std::string unsafe) {
//   return s(unsafe.c_str());
// }
// 
// char* dove::deployment::s(int unsafe) {
//   std::stringstream st;
//   st << unsafe;
//   return s(st.str().c_str());
// } 

dove::deployment::deployment(hwprofile* prof, 
    const dove::xml::system* system,
    const dove::xml::deployment* deployment) {
  //xdebug("Creating new deployment");
  profile = prof;
  system_ = system;
  deployments_ = deployment;
}

node* dove::deployment::get_xml() {
  //xdebug("Getting XML for deployment");
  node* deployment_xml = system_->xml->allocate_node(rapidxml::node_element,
      dove::xml::s("deployment"));
  
  std::vector<std::pair<int, int> >::iterator it;
  for (it = plan.begin();
      it != plan.end();
      it++) {
    node* deploy = deployments_->xml->allocate_node(rapidxml::node_element, 
        dove::xml::s("deploy"));
//     attr* task = deployments_->xml->allocate_attribute(
//       dove::xml::s("t"), dove::xml::s((*it).first));
//     attr* unit = deployments_->xml->allocate_attribute(
//       dove::xml::s("u"), dove::xml::s((*it).second));
    attr* task = deployments_->allocate_attribute_safe("t", (*it).first);
    attr* unit = deployments_->allocate_attribute_safe("u", (*it).second);
    deploy->append_attribute(task);
    deploy->append_attribute(unit);
    deployment_xml->append_node(deploy);
  }

  std::map<std::string, std::string>::iterator it2;
  for (it2 = metrics.begin();
      it2 != metrics.end();
      it2++) {
    node* metric = deployments_->xml->allocate_node(rapidxml::node_element, 
        dove::xml::s("metric"));
//     attr* name = deployments_->xml->allocate_attribute(
//       dove::xml::s("name"), dove::xml::s(it2->first));
//     attr* value = deployments_->xml->allocate_attribute(
//       dove::xml::s("value"), dove::xml::s(it2->second));
    attr* name = deployments_->allocate_attribute_safe("name", it2->first);
    attr* value = deployments_->allocate_attribute_safe("value", it2->second);
    metric->append_attribute(name);
    metric->append_attribute(value);
    deployment_xml->append_node(metric);
  }

  return deployment_xml;
}

void dove::deployment::add_task_deployment(int task, int hardware) { 
  // TODO handle exceptions here if the hardware id is bad
  int logical_id = profile->get_logical_id(hardware);
  std::pair<int, int> map = std::make_pair (task, logical_id);
  plan.push_back(map);
}

void dove::deployment::add_metric(std::string name, std::string value) {
  metrics[name] = value;
}

void dove::deployment::add_metric(const char* name, const char* value) {
  std::string n(name);
  std::string v(value);
  add_metric(n, v);
}

void dove::deployment::add_metric(const char* name, std::string value) {
  std::string n(name);
  add_metric(n, value);
}
void dove::deployment::add_metric(const char* name, int value) {
  std::string n(name);
  std::ostringstream val;
  val << value;
  add_metric(n, val.str());
}

// char* dove::validator::s(const char* unsafe) {
//   return system_->allocate_string(unsafe);
// }

dove::validator::validator(int tasks, 
        int compute_units,
        hwcom_type compute_type,
        const char* deployment_output_filename,
        const char* algorithm_name,
        const char* system_xml_path,
        const char* algorithm_desc) {
  xdebug("Creating new deployment_optimization 1");

  number_deployments_ = 0;

  // TODO I need to copy all of the char* i receive into my own memory, as 
  // simply copying the pointer to that memory likely means that it will 
  // go away soo
  this->deployment_filename.assign(deployment_output_filename);
  task_count = tasks;
  // TODO I am 100% leaking all the new memory 
  info("Testing if system xml can be parsed");

//   xmldata = new rapidxml::file<char>(system_xml_path);
//   system_ = new system_xml();
//   system_->parse<0>(xmldata->data());
  system_->create(system_xml_path);
  info("Storing system.xml into system_");
  profile = new hwprofile(compute_type, compute_units, system_);
  
  info("Creating header for deployments");
  deployment_->create(algorithm_name, algorithm_desc);
  xdebug("Done creating deployment_optimization");
}

long dove::validator::get_routing_delay(int from, int to) {
  return profile->get_routing_delay(from, to);
}

dove::deployment dove::validator::get_empty_deployment() {
  return deployment(profile, system_, deployment_);
}

// TODO: This one's tricky to place in another class.
void dove::validator::add_deployment(deployment d) {
  node* deps = deployment_->xml->first_node("optimization")->
    first_node("deployments");
  node* deployment = d.get_xml();
//   std::stringstream idtochar;
//   idtochar << number_deployments_;
//   number_deployments_++;
//   attr* id = deployment_->xml->allocate_attribute(
//       dove::xml::s("id"), 
//       dove::xml::s(idtochar.str().c_str()));
  attr* id = deployment_->allocate_attribute_safe("id", number_deployments_);
  deployment->append_attribute(id);
  deps->append_node(deployment);
}

void dove::validator::complete() {
  deployment_->complete(deployment_filename.c_str());
}
