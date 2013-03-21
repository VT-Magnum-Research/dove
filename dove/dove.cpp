
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
std::vector<rapidxml::xml_node<char>*> dove::get_all_hosts(
    rapidxml::xml_document<char> &system) {
  rapidxml::xml_node<>* nodes = system.first_node("system")->
    first_node("nodes");
  
  std::vector<rapidxml::xml_node<char>*> result;
  for (rapidxml::xml_node<char> *child = nodes->first_node();
      child;
      child = child->next_sibling()) {
    if (strcmp(child->name(), "node")==0)
      result.push_back(child);
  }
  
  return result;
}

std::vector<rapidxml::xml_node<char>*> dove::get_all_processors(
    rapidxml::xml_document<char> &system) {

  std::vector<rapidxml::xml_node<char>*> result;
  std::vector<rapidxml::xml_node<char>*> hosts = 
    get_all_hosts(system);
  std::vector<rapidxml::xml_node<char>*>::iterator it;
  for (it = hosts.begin();
      it != hosts.end();
      ++it) {
    rapidxml::xml_node<char>* host = *it;
    for (rapidxml::xml_node<char>* proc = host->first_node();
        proc;
        proc = proc->next_sibling()) {
      if (strcmp(proc->name(), "socket")==0)
        result.push_back(proc);
    }
  }
  
  return result;
}

std::vector<rapidxml::xml_node<char>*> dove::get_all_cores(
    rapidxml::xml_document<char> &system) {

  xml_node_vector result;
  xml_node_vector procs = get_all_processors(system);
  xml_node_vector::iterator it;
  for (it = procs.begin();
      it != procs.end();
      ++it) {
    rapidxml::xml_node<char>* proc = *it;
    for (rapidxml::xml_node<char>* core = proc->first_node();
        core;
        core = core->next_sibling()) {
      if (strcmp(core->name(), "core")==0)
        result.push_back(core);
    }
  }
  
  return result;
}

std::vector<rapidxml::xml_node<char>*> dove::get_all_threads(
    rapidxml::xml_document<char> &system) {

  xml_node_vector result;
  xml_node_vector cores = get_all_cores(system);
  xml_node_vector::iterator it;
  for (it = cores.begin();
      it != cores.end();
      ++it) {
    rapidxml::xml_node<char>* core = *it;
    for (rapidxml::xml_node<char>* hwth = core->first_node();
        hwth;
        hwth = hwth->next_sibling()) {
      if (strcmp(hwth->name(), "pu")==0)
        result.push_back(hwth);
    }
  }
  
  return result;
}

// TODO add in a strategy for choosing specific hardware 
// components e.g. ones on the same machine, etc
dove::hwprofile::hwprofile(hwcom_type type, int compute_units, 
    rapidxml::xml_document<char> &system) {
}

int dove::hwprofile::get_logical_id(int id) {
  return 0;
}

long dove::hwprofile::get_routing_delay(int from, int to) {
  return 0;
}
      
char* dove::deployment::s(const char* unsafe) {
  return doc->allocate_string(unsafe);
}
     
char* dove::deployment::s(int unsafe) {
  std::stringstream st;
  st << unsafe;
  return s(st.str().c_str());
} 

dove::deployment::deployment(hwprofile* prof, 
    rapidxml::xml_document<char>* system) {
  profile = prof;
  doc = system;
}

// TODO add metrics to the XML :-)
node* dove::deployment::get_xml() {  
  node* deployment_xml = doc->allocate_node(rapidxml::node_element, s("deployment"));
  
  std::vector<std::pair<int, int> >::iterator it;
  for (it = plan.begin();
      it != plan.end();
      it++) {
    node* deploy = doc->allocate_node(rapidxml::node_element, s("deploy"));
    attr* task = doc->allocate_attribute(s("t"), s((*it).first));
    attr* unit = doc->allocate_attribute(s("u"), s((*it).second));
      deploy->append_attribute(task);
      deploy->append_attribute(unit);
      deployment_xml->append_node(deploy);
    }

    return deployment_xml;
}

void dove::deployment::add_task_deploment(int task, int hardware) {
  // TODO handle exceptions here if the hardware id is bad
  int logical_id = profile->get_logical_id(hardware);
  std::pair<int, int> map = std::make_pair (task, logical_id);
  plan.push_back(map);
}

void dove::deployment::add_metric(std::string name, std::string value) {
  metrics.push_back(std::make_pair(name, value));
}

char* dove::deployment_optimization::s(const char* unsafe) {
  return doc->allocate_string(unsafe);
}

dove::deployment_optimization::deployment_optimization(int tasks, 
  int compute_units,
  hwcom_type compute_type,
  const char* output_filename,
  const char* algorithm_name,
  rapidxml::xml_document<char> &system,
  const char* algorithm_desc) {

  this->output_filename = output_filename;
  task_count = tasks;
  profile = new hwprofile(compute_type, compute_units, system);
 
  doc = &system;
  node *root = doc->allocate_node(rapidxml::node_element, s("optimization"));
  doc->append_node(root);
  attr *name = doc->allocate_attribute(s("name"), s(algorithm_name));
  root->append_attribute(name);
  attr *desc = doc->allocate_attribute(s("desc"), s(algorithm_desc));
  root->append_attribute(desc);
  node *deployments = doc->allocate_node(rapidxml::node_element, s("deployments"));
  root->append_node(deployments);
}
    
long dove::deployment_optimization::get_routing_delay(int from, int to) {
  return profile->get_routing_delay(from, to);
}

dove::deployment dove::deployment_optimization::get_empty_deployment() {
  return deployment(profile, doc);
}

void dove::deployment_optimization::add_deployment(deployment d) {
  node* deps = doc->first_node("deployments");
  deps->append_node(d.get_xml());
}

void dove::deployment_optimization::complete() {
  std::ofstream output;
  output.open (output_filename, std::ios::out | std::ios::trunc);

  output << doc;
}

