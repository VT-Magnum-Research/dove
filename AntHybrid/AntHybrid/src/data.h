//
//  data.h
//  AntHybrid
//
//  Created by Hamilton Turner on 11/9/12.
//  Copyright (c) 2012 Virginia Tech. All rights reserved.
//

#ifndef AntHybrid_data_h
#define AntHybrid_data_h

#include <sstream>

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
  
  Task() : pred_level_(0), scheduled_(false), execution_time_(0), identifier_("") {}
  
  const char* get_cstr() {
    std::stringstream ss;
    ss << "[" << identifier_ << "," << pred_level_ << "]";
    return ss.str().c_str();
  }
};

struct Core {
  // A unique identifier for this Task, such as an ID from an
  // external data file. If none is provided, this is initialized
  // to a unique number
  std::string identifier_;
  
  unsigned int current_task_priority;
  unsigned int current_task_completion_time;
};

#endif
