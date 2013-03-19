//
//  main.h
//  DeploymentOptimization
//
//  Created by Hamilton Turner on 2/13/13.
//  Copyright (c) 2013 Virginia Tech. All rights reserved.
//

#ifndef DeploymentOptimization_main_h
#define DeploymentOptimization_main_h

const char *default_makefile = ""
"CFLAGS= -O3 -I/usr/include/openmpi-x86_64\n"
"LD_LIBRARY_PATH=/usr/local/lib:/usr/lib64/openmpi/lib\n"
"LDFLAGS=-L/usr/local/lib -L/usr/lib64/openmpi/lib\n"
"LIBS = $(LDFLAGS) -lmpi -lmpi_cxx -lboost_serialization -lboost_mpi \n"
"\n"
"all:\n"
"\tg++ $(CFLAGS) $(LIBS) stg_impl.cpp -o impl\n"
"\n"
"clean:\n"
"\trm -f impl\n\n";

const char* default_run = ""
"#!/bin/sh\n"
"RANKS=$(wc -l rankfile.0 | cut --delimiter=\\  -f 1)\n"
"for file in $(ls | grep -e \"rankfile.[0-9]*$\")\n"
"do\n" 
"  echo \"Running $file\"\n"
"  PROC_TIME=$(time (mpirun --rankfile $file --hostfile hostfile.txt -np $RANKS impl >/dev/null 2>&1) 2>&1)\n"
"  echo \"Took $PROC_TIME\"\n"
"  echo $PROC_TIME > $file.time\n"
"done\n";

#include "libs/graph.h"

struct Task {
  // This is analogous to pred_level
  // 1 is 'highest' priority and will be executed before 2,
  // which is before 3 ... etc
  unsigned int pred_level_;
  
  bool scheduled_;
  
  // TODO temporary, I need to implement the time matrix
  unsigned int execution_time_;
  
  // A unique identifier for this Task, such as an ID from an
  // external data file. If none is provided, this is initialized
  // to a unique number
  std::string identifier_;
  
  // Another possible unique identifier
  unsigned int int_identifier_;
  
  Task() : pred_level_(0), scheduled_(false), execution_time_(0), identifier_("") {}
  
  const char* get_cstr() {
    std::stringstream ss;
    ss << "[" << identifier_ << "," << pred_level_ << "]";
    return ss.str().c_str();
  }
};

std::vector<Task>* parse_stg(const char *filepath, DirectedAcyclicGraph* &graph) {
  enum section {
    HEADER,
    GRAPH,
    FOOTER
  };
  
  std::vector<Task>* tasks = NULL;
  
  section s = HEADER;
  std::ifstream file(filepath);
  
  if(!file)
    throw filepath;
  
  unsigned int task_count = 0;
  while(file.good()) {
    if(s == HEADER) {
      file >> task_count;
      if (task_count !=0) {
        // STG format always includes two dummy nodes
        task_count = task_count + 2;
        graph = new DirectedAcyclicGraph(task_count);
        tasks = new std::vector<Task>(task_count);
        s = GRAPH;
      }
    } else if(s == GRAPH) {
      if (task_count-- == 0) {
        s = FOOTER;
        continue;
      }
      unsigned int task_id, task_processing_time, predecessor_count;
      file >> task_id;
      file >> task_processing_time;
      file >> predecessor_count;
      
      Task* task = &(tasks->at(task_id));
      if (task_id == 0)
        task->pred_level_ = 1;
      
      for (int i = 0; i < predecessor_count; i++) {
        unsigned int pred_id;
        file >> pred_id;
        graph->add_edge(pred_id, task_id);
        
        task->pred_level_ = std::max(task->pred_level_,
                                     tasks->at(pred_id).pred_level_ + 1);
      }
      
      task->execution_time_ = task_processing_time;
      std::ostringstream oss;
      oss << "t" << task_id;
      task->identifier_ = oss.str();
      task->int_identifier_ = task_id;
      
    } else if (s == FOOTER) {
      std::string line;
      file >> line;
    }
  }
  
  file.close();
  
  // I want to keep my start and end tasks, so that I know the DAG begins at
  // (*graph)[0][0] and I can request successors from there
  //std::sort(tasks->begin(), tasks->end(), precedence_sort);
  //tasks->pop_back();
  //tasks->erase(tasks->begin());
  
  return tasks;
}


#endif
