
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
  template<lass Ch>
  inline xml_node<Ch>* get_child_with_attribute(xml_node<Ch> *node, 
    ::string attribute_name, std::string attribute_value)
  {
    // Cycles every child
    for (xml_node<> *nodeChild = 
        node->first_node(); 
        nodeChild; 
        nodeChild = nodeChild->next_sibling())
    {
      xml_attribute<Ch>* attribute = nodeChild->first_attribute(attribute_name);
      if (attribute != 0 && strcmp(attribute->value(), attribute_value) == 0)
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
  void parse_pids(rapid_xml::xml_document<char> system, 
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

  // Describes a 
  class hardware_component {
      int node_pid;
      int proc_pid; 
      int core_pid; 
      int hwth_pid; 
      std::string hostname; 
  }

  std::string build_rankline_core(int rank, std::string host, int procpid, int corepid) {
    // Cores are "rank %s=%s slot=p%d:%d\n" 
    // rank 1=10.0.2.4 slot=p1:8
    // references physical socket 1 and physical core 8
    out << "rank " << rank << "=" << host << " slot=p" << procpid << ":" << corepid << std::endl;
  }
  
}

#endif
