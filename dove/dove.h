#ifndef __DOVE_H_INCLUDED__
#define __DOVE_H_INCLUDED__

// Forward-declare what we can
namespace rapidxml {
  template <typename T> class xml_node;
  template <typename T> class xml_attribute;
  template <typename T> class xml_document;
  template <typename T> class file;
}

#include <iostream>
#include <vector>
#include <string>
#include <map>

typedef rapidxml::xml_node<char> xml_node;
typedef rapidxml::xml_node<char> node;
typedef std::vector<xml_node*> xml_node_vector; 
typedef rapidxml::xml_attribute<char> attr;

namespace dove {

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
  void parse_pids(rapidxml::xml_document<char> &system, 
      std::string logical_id,
      int &node_pid, 
      int &proc_pid, 
      int &core_pid, 
      int &hwth_pid, 
      std::string &hostname, 
      std::string &ip);
  hwcom parse_pids(rapidxml::xml_document<char> &system, 
      std::string logical_id);
  
  std::string build_rankline_core(int task, 
      std::string host, int procpid, int corepid);
  
  // Code to build rankfile lines from all different kinds of logical IDs 
  std::string build_rankline_core(int task, hwcom com);

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
      std::string id);
  std::vector<rapidxml::xml_node<char>*> get_all_hosts(
      rapidxml::xml_document<char> &system);
  std::vector<rapidxml::xml_node<char>*> get_all_processors(
      rapidxml::xml_document<char> &system);
  std::vector<rapidxml::xml_node<char>*> get_all_cores(
      rapidxml::xml_document<char> &system);
  std::vector<rapidxml::xml_node<char>*> get_all_threads(
      rapidxml::xml_document<char> &system);

  // Allows an external codebase to request a number N of 
  // hardware components and the routing delay between these
  // components. HW components are referencable by 0...N-1. 
  // The routing delays come from the system.xml file. 
  // This class maintains the list of mappings from logicalids
  // (used in the system.xml) and the 0...N-1 count of "compute
  // units" that are traditionally used in optimization problems
  class hwprofile {
    hwcom_type type_;
    // Maps from 0...N to the actual logical ID
    std::map<int, int> ids_;
    rapidxml::xml_document<char>* system_;

    public:
      hwprofile(hwcom_type type, int compute_units, 
        rapidxml::xml_document<char>* system);
  
      // Given the 0...N-1 id, this will return the logical ID that
      // is used in the system.xml file. 
      //
      // throws exception if id is outside 0...N-1
      int get_logical_id(int id);
      
      long get_routing_delay(int from, int to);

  };
  
  // Simple data storage class to allow a user to iteratively 
  // build a deployment
  class deployment {
    private: 
      // First is task ID, second is hardware logical ID
      // TODO why did I make this a vector and not just a map? 
      std::vector<std::pair<int, int> > plan;
      std::map<std::string, std::string> metrics;
      rapidxml::xml_document<char>* system_;
      rapidxml::xml_document<char>* deployments_;
      hwprofile* profile;
      
      // Builds a 'safe' string for rapidxml
      char* s(const char* unsafe);
      char* s(int unsafe);
      char* s(std::string unsafe);

    public:
      deployment(hwprofile* prof, 
          rapidxml::xml_document<char>* system,
          rapidxml::xml_document<char>* deployment);
      
      // Builds the xml to represent this deployment
      node* get_xml();
    
      // Add a deployment of a task to a hardware compute
      // unit, using the 0..N-1 id's from the hardware profile
      // to describe hardware components
      void add_task_deployment(int task, int hardware);

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
      void add_metric(std::string name, std::string value);
      void add_metric(const char* name, std::string value);
      void add_metric(const char* name, const char* value);
      void add_metric(const char* name, int value);

      // TODO override add_metric to support other types of 
      // values
  };

  void log(const char* msg, int level); 
  void xdebug(const char* msg);
  void info(const char* msg); 
  void error(const char* msg);

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
  class validator {

    private:
      hwprofile* profile;
      int task_count;
      rapidxml::xml_document<char>* system_;
      // Must keep source text around for rapidxml
      rapidxml::file<char>* xmldata;
      // Build the deployments.xml into this 
      rapidxml::xml_document<char>* deployment_;
      // So that it can eventually be printed into this
      std::string deployment_filename;
      
      // Builds a 'safe' string for rapidxml
      char* s(const char* unsafe);

    public:
      validator(int tasks, 
        int compute_units,
        hwcom_type compute_type,
        const char* output_filename,
        const char* algorithm_name,
        const char* system_xml_path,
        const char* algorithm_desc = "");

      // TODO create constructor that accepts the 
      // <dove> directory, checks for system.xml, software.stg, 
      // and plans to output deployments.xml. Should read in the
      // STG to automatically figure out how many tasks

      // TODO create debug arguments for DOVE. Perhaps use an int
      // called DEBUG_LEVEL and write functions for log/debug/verbose
      // that automatically set DEBUG_LEVEL properly and then internally
      // call log(const char*, int)

      // Dove allocates [0...compute_units] computation units 
      // when created. This function returns the real routing delay
      // from one unit to another
      long get_routing_delay(int from, int to);

      // An algorithm must inform dove of each deployment. This
      // function returns an empty deployment plan that the algorithm
      // can then fill with it's task to hardware mappings and any
      // calculated metrics 
      deployment get_empty_deployment();

      // An algorithm must inform dove of each deployment. This
      // adds a single deployment plan to this optimization
      void add_deployment(deployment d);

      // Informs dove that the algorithm has completed and all 
      // data should be written to file
      void complete();
  };


} // End dove namespace

#endif

