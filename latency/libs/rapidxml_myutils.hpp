
// Copyright (C) 2013 Hamilton Turner
// Creates some simple functions for very high
// level XML parsing based off of Rapidxml 1.13

// All functions under rapidxml namespace are 
// generic, while all functions under DOVE are 
// specific to the Deployment Optimization Validation 
// Engine

#include "rapidxml.hpp"
#include <string>

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

#include <string.h>
namespace dove 
{

  enum hardware_type { 
    HOST      = 4, 
    PROCESSOR = 3, 
    PROC      = PROCESSOR,
    SOCKET    = PROCESSOR,
    SOCK      = PROCESSOR, 
    CORE      = 2,
    HW_THREAD = 1,
    UNKNOWN   = 0
  };

  // Describes a hardware component as completely as 
  // possible. For identifying a high-level hardware construct, 
  // like a host, this will obviously have no information on
  // processor/core
  struct hardware_component {
      int node_pid;
      int proc_pid; 
      int core_pid; 
      int hwth_pid; 
      std::string hostname; 
      std::string ip;
      hardware_type type;

      hardware_component(): node_pid(-1),
        proc_pid(-1), core_pid(-1),
        hwth_pid(-1), hostname(""),
        ip(""), type(UNKNOWN) { }
  };

  // Given the system.xml and the logical ID of a tag within the system.xml, 
  // this returns all identifiers needed to uniquely identify the hardware 
  // component referenced by that tag. A component could be a host, processor, 
  // core, or hardware thread. 
  //
  // For higher-level components e.g. processor, there will be no returned 
  // values for the lower-level components contained within the higher-level
  // component. For example, passing a logical ID that corresponds to a 
  // core will return physical identifiers for the host, processor, and 
  // core, but will return no identifying information about a hardware 
  // thread. 
  //
  void parse_pids(rapidxml::xml_document<char> &system, 
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

  hardware_component parse_pids(rapidxml::xml_document<char> &system, 
      std::string logical_id) {
    hardware_component com;
    parse_pids(system, logical_id, 
        com.node_pid, com.proc_pid, com.core_pid,
        com.hwth_pid, com.hostname, com.ip);
    return com;
  }

  // Code to build rankfile lines from all different kinds of logical IDs 
  std::string build_rankline_core(int task, hardware_component com) {
    return build_rankline_core(task, com.hostname, com.proc_pid, com.core_pid);
  }
  std::string build_rankline_core(int task, std::string host, int procpid, int corepid) {
    // Cores are "rank %s=%s slot=p%d:%d\n" 
    // rank 1=10.0.2.4 slot=p1:8
    // references physical socket 1 and physical core 8
    out << "rank " << task << "=" << host << " slot=p" << procpid << ":" << corepid << std::endl;
  }
  
  // Given a hardware component logical ID, the task number, and 
  // the system.XML file, this will build the rankline to 
  // properly deploy that task onto that hardware component. 
  // 
  // Automatically detects if the ID is for a core, hardware thread,
  // or processor, and builds the rankline appropriately. If for an 
  // entire host, then the rankline simply lists that the task can be 
  // bound to any available processor on that host
  std::string build_rankline(rapidxml::xml_document<char> &system,
      int taskid, 
      std::string id) {
    hardware_component com = parse_pids(system, id);

    int type = com.type;
    switch (type) {
    case UNKNOWN:
      throw "Unknown hardware component type";
    case HW_THREAD:
      // TODO support threads
      throw "Only supports cores for now";
      break;
    case CORE:
      return build_rankline_core(task, com);
    case SOCKET:
      // TODO support sockets
      throw "Only supports cores for now";
      break;
    case HOST: 
      throw "Only supports cores for now";
      // TODO support hosts
      break;
    }
  }


}

