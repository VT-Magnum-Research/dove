#ifndef __DOVE_H_INCLUDED__
#define __DOVE_H_INCLUDED__

// Forward-declare what we can
namespace rapidxml {
  template <typename T> class xml_node;
  template <typename T> class xml_attribute;
  template <typename T> class xml_document;
  template <typename T> class file;
//   template <typename T> class memory_pool;
}

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <stdexcept>

#include "libs/tclap/CmdLine.h"
#include "dove_xml.h"

typedef rapidxml::xml_node<char> xml_node;
typedef rapidxml::xml_node<char> node;
typedef std::vector<xml_node*> xml_node_vector; 
typedef rapidxml::xml_attribute<char> attr;

namespace dove {
  class parse_error : public std::runtime_error {
    public:
      parse_error(std::string msg) : std::runtime_error(msg) { }
  };
  
  void xlog(const char* msg, int level); 
  void xdebug(const char* msg);
  void info(const char* msg); 
  void error(const char* msg);

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

  struct config {
    std::string stg_filepath;
    std::string deps;
    std::string sys;
  } config_;

  // Given a TCLAP CmdLine pointer, `use_tclap` adds dove's command
  // line options, calls TCLAP::CmdLine::parse, and returns a 
  // completed dove::config. This should replace any calls to 
  // TCLAP::CmdLine::parse
  // TODO check if you can call parse multiple times without adverse
  // effects
  config use_tclap(TCLAP::CmdLine &cmd, int argc, char * argv[]);
  
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
  
  // Builds an OpenMPI rankline to describe mapping the 
  // provided task to the provided core existing on 
  // the host/processor
  std::string build_rankline_core(int task, 
      std::string host, int procpid, int corepid);
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

  // Allows an external codebase to request a number N of 
  // hardware components, and the routing delay between these
  // components. HW components are referencable by 0...N-1. 
  // The routing delays come from the system.xml file. 
 class hwprofile {
    hwcom_type type_;
    
    // Mappings from logicalid (used in the system.xml) and 
    // the 0...N-1 count of "compute units" that is typically 
    // used in optimization problems
    std::map<int, int> ids_;
    const xml::system* system_;

    public:
      hwprofile(hwcom_type type, int compute_units, 
        xml::system* system);
  
      // Given the 0...N-1 id, this will return the logical ID that
      // is used in the system.xml file. Adds the ID if  
      //
      // throws exception if id is outside 0...N-1
      int get_logical_id(int id) const;
      
      long get_routing_delay(int from, int to) const;
  };
  
  // Allow a user to iteratively build a deployment of 
  // tasks to cores
  class deployment {
    private: 
      // First is task ID, second is hardware logical ID
      // TODO update this to be a map
      std::vector<std::pair<int, int> > plan;
      std::map<std::string, std::string> metrics;
      const xml::system* system_;
      const xml::deployment* deployments_;
      hwprofile* profile;
     
    public:
      deployment(hwprofile* prof, 
          const xml::system* system,
          const xml::deployment* deployment);
      
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
      // TODO define a custom exception to throw if the given metric
      // already exists
      // TODO build variants that accept double, long, etc
      void add_metric(std::string name, std::string value);
      void add_metric(const char* name, std::string value);
      void add_metric(const char* name, const char* value);
      void add_metric(const char* name, int value);
  };

  // Interfaces the DOVE validation suite with an optimization 
  // algorithm. An algorithm creates a validator
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
      xml::system* system_;

      // Keeps track of the number of times add_deployment has
      // been called, so that we can append an ID to each deployment
      int number_deployments_;

      // Must keep source text around for rapidxml
//       rapidxml::file<char>* xmldata;
      // Build the deployments.xml into this 
      xml::deployment* deployment_;
      // So that it can eventually be printed into this
      std::string deployment_filename;
      
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

      // Dove allocates [0...compute_units] computation units 
      // when created. This function returns the real routing delay
      // from one unit to another
      // TODO make this accept a variable indicating the units that
      // the caller expects back. There will be some problems because some
      // very large routing delays (network-level) cannot easily be 
      // represented as small values, like nano_seconds, because a 
      // long is just not long enough. Some very small delays, like
      // nanoseconds, cannot be returned as large units e.g. seconds
      // without using a floating point type. It may be hard to come
      // up with one return type for all functions without defining a
      // new type or using something like timespec
      long get_routing_delay(int from, int to) const;

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

