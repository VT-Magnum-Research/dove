#ifndef __DOVE_XML_H_INCLUDED__
#define __DOVE_XML_H_INCLUDED__

#include <memory> // auto_ptr

// Forward-declare what we can
namespace rapidxml {
  template <typename T> class xml_node;
  template <typename T> class xml_attribute;
  template <typename T> class xml_document;
  template <typename T> class file;
  template <typename T> class memory_pool;
}

namespace dove {
  namespace xml {
    typedef rapidxml::xml_node<char>      node;
    typedef rapidxml::xml_attribute<char> attr;
    typedef rapidxml::file<char>          file;
    typedef rapidxml::xml_document<char>  doc;

    // Used to allocate all memory for rapidxml
    rapidxml::memory_pool<char>* string_pool;

    // Builds a 'safe' string for rapidxml by ensuring
    // the memory is allocated using memory_pool
    char* s(const char* unsafe);
    char* s(int unsafe);
    char* s(std::string unsafe);

    class inner {
      private:
        node* node_;
      public:
        node* get_node() { return node_; }
        void set_node(node* node) { node_ = node; }
    };
    class processor: public inner {
    };

    class core: public inner {
    };

    class hardware_thread: public inner {
    };

    class system {
      private:
        std::auto_ptr<rapidxml::file<char> > xmldata;
      public:
        ~system();
        std::auto_ptr<doc> xml;
        void create(const char* path);
        // TODO return host*, processor*, etc
        std::vector<node*> get_all_hosts() const;
        std::vector<node*> get_all_processors() const;
        std::vector<node*> get_all_cores() const;
        std::vector<node*> get_all_threads() const;
        long get_routing_delay(int from, int to, int lfrom, int lto) const;
        std::string add_routing_delay(
          int fromid, int toid, std::string result, bool dry_run = false);
    };

    class deployment {
      private:
      public:
        std::auto_ptr<doc> xml;
        ~deployment();
        void create(const char* algorithm_name, const char* algorithm_desc);

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
        attr* add_deployment(node* node, int num_deployments);
        attr* allocate_attribute_safe(std::string tag, int value) const;
        attr* allocate_attribute_safe(std::string tag, std::string value) const;
        void complete(const char* filename);
    };
  }
}
#endif