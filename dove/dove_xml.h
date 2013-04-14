#ifndef __DOVE_XML_H_INCLUDED__
#define __DOVE_XML_H_INCLUDED__

// TODO: I can move this to a different namespace, or even
// put it back in dove.cpp.
// I'm just keeping it simple because it's easy to change.

namespace dove {
  namespace xml {
    typedef rapidxml::xml_node<char> xml_node;
    typedef rapidxml::xml_node<char> node;
    typedef std::vector<xml_node*> xml_node_vector; 
    typedef rapidxml::xml_attribute<char> attr;

    rapidxml::memory_pool<char>* string_pool;

    // Builds a 'safe' string for rapidxml
    char* s(const char* unsafe);
    char* s(int unsafe);
    char* s(std::string unsafe);

    class system {
      private:
        rapidxml::file<char>* xmldata;
      public:
        rapidxml::xml_document<char>* xml;
        void create(const char* path);
        std::vector<node*> get_all_hosts();
        std::vector<node*> get_all_processors();
        std::vector<node*> get_all_cores();
        std::vector<node*> get_all_threads();
        long get_routing_delay(int from, int to, int lfrom, int lto);
    };

    class deployment {
      private:
      public:
        rapidxml::xml_document<char>* xml;
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
        rapidxml::xml_attribute<char>*
        add_deployment(rapidxml::xml_node<char>* node, int num_deployments);
        void complete(const char* filename);
    };
    }
}
#endif
