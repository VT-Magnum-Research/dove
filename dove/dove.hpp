
// Copyright (C) 2013 Hamilton Turner
//
// Deployment Optimization Validation Engine (dove)
//
// Depends upon Rapidxml 1.13
//

#include "rapidxml.hpp"
#include <string>
#include <sstream>

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

#include <string.h>
#include <vector>
namespace dove 
{
  typedef rapidxml::xml_node<char> xml_node;
  typedef rapidxml::xml_node<char> node;
  typedef std::vector<xml_node*> xml_node_vector; 
  typedef rapidxml::xml_attribute<char> attr;
  typedef rapidxml::node_element node_element

  // Hardware component types
  enum hwcom_type { 
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
  struct hwcom {
      int node_pid;
      int proc_pid; 
      int core_pid; 
      int hwth_pid; 
      std::string hostname; 
      std::string ip;
      hwcom_type type;

      hwcom(): node_pid(-1),
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
  //TODO make this set type on hardware component
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
  hwcom parse_pids(rapidxml::xml_document<char> &system, 
      std::string logical_id) {
    hwcom com;
    parse_pids(system, logical_id, 
        com.node_pid, com.proc_pid, com.core_pid,
        com.hwth_pid, com.hostname, com.ip);
    return com;
  }

  std::string build_rankline_core(int task, 
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
  // Code to build rankfile lines from all different kinds of logical IDs 
  std::string build_rankline_core(int task, hwcom com) {
    return build_rankline_core(task, com.hostname, com.proc_pid, com.core_pid);
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
    hwcom com = parse_pids(system, id);

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

  std::vector<rapidxml::xml_node<char>*> get_all_hosts(
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

 std::vector<rapidxml::xml_node<char>*> get_all_processors(
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

  std::vector<rapidxml::xml_node<char>*> get_all_cores(
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

  std::vector<rapidxml::xml_node<char>*> get_all_threads(
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
  // Allows an external codebase to request a number N of 
  // hardware components and the routing delay between these
  // components. HW components are referencable by 0...N-1. 
  // The routing delays come from the system.xml file. 
  // This class maintains the list of mappings from logicalids
  // (used in the system.xml) and the 0...N-1 count of "compute
  // units" that are traditionally used in optimization problems
  class hwprofile {
    public: 

      hwprofile(hwcom_type type, int compute_units, 
          rapidxml::xml_document<char> &system) {
      }

      // Given the 0...N-1 id, this will return the logical ID that
      // is used in the system.xml file. 
      //
      // throws exception if id is outside 0...N-1
      int get_logical_id(int id) {
        return 0;
      }

      long get_routing_delay(int from, int to) {
        return 0;
      }
  };

  // Simple data storage class to allow a user to iteratively 
  // build a deployment
  class deployment {
    
    private: 
      // First is task ID, second is hardware logical ID
      std::vector<std::pair<int, int> > deployment;
      std::vector<std::pair<std::string, std::string> > metrics;
      rapidxml::xml_document<> doc;
      hwprofile profile;
     
      // Builds a 'safe' string for rapidxml
      inline char* s(const char* unsafe) {
        return rapidxml::doc.allocate_string(unsafe);
      }
      char* s(int unsafe) {
        std::string st = std::to_string(unsafe);
        return s(st.c_str());
      } 

    public: 
      
      deployment(hwprofile prof, 
          rapidxml::xml_document<char> &system) {
        profile = prof;
        doc = system;
      }

      // Builds the xml to represent this deployment
      // TODO add metrics to the XML :-)
      node* get_xml() {  
        node* deployment_xml = doc.allocate_node(node_element, s("deployment"));
        
        std::vector<std::pair<int, int> >::iterator it;
        for (it = deployment.begin();
            it != deployment.end();
            it++) {
          node* deploy = doc.allocate_node(node_element, s("deploy"));
          attr* task = doc.allocate_attribute(s("t"), s((*it).first));
          attr* unit = doc.allocate_attribute(s("u"), s((*it).second));
          deploy->append_attribute(task);
          deploy->append_attribute(unit);
          deployment_xml->append_node(deploy);
        }

        return deployment_xml;
      }

      // Add a deployment of a task to a hardware compute
      // unit, using the 0..N-1 id's from the hardware profile
      // to describe hardware components
      void add_task_deploment(int task, int hardware) {
        // TODO handle exceptions here if the hardware id is bad
        int logical_id = profile.get_logical_id(hardware);
        std::pair<int, int> map = std::make_pair (task, logical_id);
        deployment.push_back(map);
      }

      // While an algorithm can add any metrics desired, there are 
      // some common metrics that allow dove to automatically 
      // analyze the algorithm. These currently include: 
      // - makespan: End to end optimization time that dove uses 
      // to determine if runtime and optimization metrics are correlated
      // - iteration: The iteration of the optimization algorithm
      // that produced this deployment. This allows dove to try and 
      // determine a cutoff point after which continued algorithm
      // iterations tend to yield a benefit that is below some 
      // threshold
      void add_metric(std::string name, std::string value) {
        metrics.push_back(std::make_pair(name, value));
      }
  };
  
  // Interfaces the DOVE validation suite with an optimization 
  // algorithm. An algorithm creates a deployment_optimization
  // object, requesting a number of compute units (e.g. 4 cores). 
  // DOVE uses the input real-system description to find 4 
  // cores, and the algorithm can request the routing delay
  // from every unit to every other unit. 
  //
  // The algorithm must inform DOVE ofall of the incremental 
  // deploymen solutions, and then call complete() to output 
  // an XML file that can be read into the rest of the DOVE 
  // codebase to generate a real STG implementation and 
  // validate the optimization algorithm results
  class deployment_optimization {

    private:
      hwprofile profile;
      int task_count;
      const char* output;
      rapidxml::xml_document<> doc;

      // Builds a 'safe' string for rapidxml
      inline char* s(const char* unsafe) {
        return rapidxml::doc.allocate_string(unsafe);
      }

    public: 

      deployment_optimization(int tasks, 
        int compute_units,
        hwcom_type compute_type,
        const char* output_filename,
        const char* algorithm_name,
        const char* algortihm_desc = "",
        rapidxml::xml_document<> &system) {
         
        task_count = tasks;
        profile = hwprofile(compute_type, compute_units, system);
        
        node *root = doc.allocate_node(node_element, s("optimization"));
        doc.append_node(root);
        attr *name = doc.allocate_attribute(s("name"), s(algorithm_name));
        root->append_attribute(name);
        attr *desc = doc.allocate_attribute(s("desc"), s(algorithm_desc));
        root->append_attribute(desc);
        node *deployments = doc.allocate_node(node_element, s("deployments"));
        root.append_node(deployments);
      }
    
      // Dove allocates [0...compute_units] computation units 
      // when created. This function returns the real routing delay
      // from one unit to another
      long get_routing_delay(int from, int to) {
        return profile.get_routing_delay(from, to);
      }

      // An algorithm must inform dove of each deployment. This
      // function returns an empty deployment plan that the algorithm
      // can then fill with it's task to hardware mappings and any
      // calculated metrics 
      deployment get_empty_deployment() {
        return deployment(profile);
      }
      
      // An algorithm must inform dove of each deployment. This
      // adds a single deployment plan to this optimization
      void add_deployment(deployment d) {
        node* = doc.first_node("deployments");
        node->append_node(d.get_xml());
      }
   
      // Informs dove that the algorithm has completed and all 
      // data should be written to file
      void complete() {
        std::stream output;
        output.open (output_filename, ios::out | ios::trunc);

        rapidxml::print(output, doc);
      }
  };
}

