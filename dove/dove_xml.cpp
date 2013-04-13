#include <stdio.h>
#include <cstring> // strcmp
#include <fstream> // basic_ofstream
// #include <sstream>
// #include <iostream>

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include "rapidxml_utils.hpp"

#include "dove_xml.h"
#include "dove.h"

char* dove_xml::s(const char* unsafe) {
   return xml->allocate_string(unsafe);
}

char* dove_xml::s(std::string unsafe) {
  return s(unsafe.c_str());
}

char* dove_xml::s(int unsafe) {
  std::stringstream st;
  st << unsafe;
  return s(st.str().c_str());
}

void system_xml::create(const char* path) {
 xmldata = new rapidxml::file<char>(path);
 xml = new rapidxml::xml_document<char>();
 xml->parse<0>(xmldata->data());
}

// TODO consider creating dove::xml and moving all of my 
// helper functions into that namespace to keep it clean
std::vector<rapidxml::xml_node<char>*> system_xml::get_all_hosts() {
  dove::xdebug("Getting all hosts from system.xml");
  rapidxml::xml_node<>* nodes = xml->first_node("system")->
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

std::vector<rapidxml::xml_node<char>*> system_xml::get_all_processors() {
  dove::xdebug("Getting all processors from system.xml");
  std::vector<rapidxml::xml_node<char>*> result;
  std::vector<rapidxml::xml_node<char>*> hosts = get_all_hosts();
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

std::vector<rapidxml::xml_node<char>*> system_xml::get_all_cores() {
  dove::xdebug("Getting all cores from system.xml");
  xml_node_vector result;
  xml_node_vector procs = get_all_processors();
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

std::vector<rapidxml::xml_node<char>*> system_xml::get_all_threads() {
  dove::xdebug("Getting all threads from system.xml");
  xml_node_vector result;
  xml_node_vector cores = get_all_cores();
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

long system_xml::get_routing_delay(int from, int to, int lfrom, int lto) {
  rapidxml::xml_node<>* system = xml->first_node("system");
  //if (system != 0)
  //  info("Got system");
  //else
  //  info("Tried to fetch system, but it was not returned");
  rapidxml::xml_node<>* delays = system->last_node("routing_delays");
  if (delays == 0)
    dove::info("Delays does not exist, expect segfaults next!");

  for (rapidxml::xml_node<char> *delay = 
        delays->first_node(); 
        delay; 
        delay = delay->next_sibling()) {
    char* sfrom = delay->first_attribute("f")->value();
    std::istringstream ssfrom(sfrom);
    int from;
    ssfrom >> from;
    if (from != lfrom) 
      continue;
    char* sto = delay->first_attribute("t")->value();
    std::istringstream ssto(sto);
    int to;
    ssto >> to;
    if (to == lto) {
      char* sval = delay->first_attribute("v")->value();
      std::istringstream ssval(sval);
      int val;
      ssval >> val;
      return val;
    }
  }

  throw "No route was found between the two id's";
}

void deployment_xml::create(const char* algorithm_name,
    const char* algorithm_desc) {
  xml = new rapidxml::xml_document<char>();
  node *root = xml->allocate_node(rapidxml::node_element,
    s("optimization"));
  xml->append_node(root);
  attr *name = xml->allocate_attribute(s("name"), s(algorithm_name));
  root->append_attribute(name);
  attr *desc = xml->allocate_attribute(s("desc"), s(algorithm_desc));
  root->append_attribute(desc);
  node *deployments = xml->allocate_node(rapidxml::node_element, s("deployments"));
  root->append_node(deployments);
}

void deployment_xml::complete(const char* filename) {
  dove::xdebug("Complete was called on deployment_optimization");
  std::ofstream output(filename, std::ios::out | std::ios::trunc);
  if (output.is_open())
  {
    dove::info("About to write to file...");
    dove::info(filename);
    output << *xml;
    output.close();
    dove::info("File written");
  } else {
    dove::error("Unable to save file to following location: ");
    dove::error(filename);
    // TODO complete this by 
    // 1) stripping any directory component, 
    // 2) creating deployments.XXXXXX
    // 3) using mkstemp to make that a unique path
    // 4) Trying to write to that path...
    //fd = mkstemp(sfn); 
    //mkstemp(tmpname);
    //ofstream f(tmpname);
    //char tmpname[] = "tmp.XXXXXX";
    //FILE *fpt = fdopen(mkstemp(tmpname), "w");
  }
}
