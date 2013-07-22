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

// The dove namespace wraps domain-specific data container, utliity functions,
// 
// The dove framework uses XML files to communicate between different components. 
// While these are visible to the user on the file system, they should not have to 
// interact with these XML files for typical usage of dove--internally the rapidxml
// library is used to write and parse all XML files. Files of interest: 
//   - system.xml : Provides a mapping between the fully unique logical IDs dove uses 
//                  to refer to hardware components such as cores and the physical 
//                  IDs that are used when performing OpenMPI processor affinity
//   - deployment.xml: The collection of deployment solutions that an optimization 
//                     algorithm has generated. Used dove's logical IDs for the 
//                     hardware components 
//
//   - 
//
// Classes in the dove namespace:
//
//   - validator: An optimization algorithm builds a dove::validator when building its
//                solution, and the validator can later perform the actual validation. 
//                This is one of the main entry points into dove and therefore controls
//                a good portion of the memory
//
//   TODO Build dove::config object(singleton?) that can be passed around to access the configuration
//   of the system (access to various XML files...)
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

  // Describes a hardware component using physical machine-specific IDs
  //
  // For high-level hardware construct such as host, there will be no 
  // information on processor/core
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

  // Extracts physical IDs from the system.xml file for a logical hardware
  // component ID. 
  //
  // Provides the minimal information needed to uniquely 
  // identify that logical component (e.g. a if the logical_id is a processor, 
  // no information on cores/threads will be provided)
  // TODO move this into XML handling area
  hwcom parse_pids(rapidxml::xml_document<char> &system, 
      std::string logical_id);
  void parse_pids(rapidxml::xml_document<char> &system, 
      std::string logical_id,
      int &node_pid, 
      int &proc_pid, 
      int &core_pid, 
      int &hwth_pid, 
      std::string &hostname, 
      std::string &ip);
  
  
  // Returns a configuration string that will assign a task to a physical core
  //
  // Returns: One line of an OpenMPI rankfile
  std::string build_rankline_core(int task, hwcom com);
  std::string build_rankline_core(int task, std::string host, int procpid, int corepid);
  
  // Builds one line of the OpenMPI configuration (rankfile) to map the 
  // provided taskid onto the provided logical hardware component 
  // 
  // Specifically, this automatically detects the type of hardware component 
  // represented by the logical ID (e.g.  core, hardware thread,
  // or processor) and builds the rankline appropriately. If the ID is for an 
  // entire host, then the rankline lists that the task can be 
  // bound to any available processor on that host
  std::string build_rankline(rapidxml::xml_document<char> &system,
      int taskid, 
      std::string logical_id);

  std::vector<rapidxml::xml_node<char>*> get_all_hosts(
      rapidxml::xml_document<char> &system);
  std::vector<rapidxml::xml_node<char>*> get_all_processors(
      rapidxml::xml_document<char> &system);
  std::vector<rapidxml::xml_node<char>*> get_all_cores(
      rapidxml::xml_document<char> &system);
  std::vector<rapidxml::xml_node<char>*> get_all_threads(
      rapidxml::xml_document<char> &system);

  // Represents a collection of hardware components. Used by algorithms to 
  // request N hardware components, where the components can be N cores, 
  // N processors, N machines, etc. 
  //
  // Internally this uses a strategy to choose which of the available 
  // physical components (e.g. out of all components listed in the 
  // system.xml file) will be exposed to the optimization algorithm. 
  // The algorithm then uses this class to request information about the 
  // created hardware profile, such as querying the routing time between
  // two hardware components or the execution time. 
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

  void xlog(const char* msg, int level); 
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
      // TODO look up C++ singleton and use it here for dove::configuration or
      // dove::xml_config. The next 4 variables are all configuration items,
      // along with a ton of other items in this class
      rapidxml::xml_document<char>* system_;

      // Must keep source text around for rapidxml
      rapidxml::file<char>* xmldata;
      // Build the deployments.xml into this 
      rapidxml::xml_document<char>* deployment_;
      // So that it can eventually be printed into this
      std::string deployment_filename;
      
      // Builds a 'safe' string for rapidxmli
      // TODO why is this here? It should only be in dove::xml
      // and should be used inside of all of those methods (e.g. to 
      // be hidden from the dove::xml user)
      char* s(const char* unsafe);
      
      // Keeps track of the number of times add_deployment has
      // been called, so that we can append an ID to each deployment
      int number_deployments_;

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

      // TODO make the above constructor also automatically 
      // "own" the XML files in the associated directories, and 
      // associated modifications to the dove data structures 
      // will be reflected in the XML files. E.g. make all the 
      // different little modules (generator, runner, etc) call
      // through dove instead of having to manage rapidxml code
      // on their own. Dove destructor can handle removing the 
      // memory for the files. We will need a finish call to 
      // write all modifications to disk

      // TODO create debug arguments for DOVE. Perhaps use an int
      // called DEBUG_LEVEL and write functions for log/debug/verbose
      // that automatically set DEBUG_LEVEL properly and then internally
      // call log(const char*, int)

      // TODO can this provide a score function, using a set of assumptions
      // when calculating the score so that we can easily modify the mechanism
      // for which it's calculated? 
      
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
      // adds a single deployment plan to this optimization. 
      // 
      // Note: There is an inherent assumption that most algorithms 
      // using dove are iterative, and this function is first called
      // with the initial deployment and then itertively called with
      // all others. It assigns an increasing id number to each 
      // passed deployment, which is later used to automatically 
      // determine how many iterations should be run before the 
      // deployment will terminate
      void add_deployment(deployment d);

      // Informs dove that the algorithm has completed and all 
      // data should be written to file
      void complete();
  };


} // End dove namespace

#endif

