//
//  MpSchedule.h
//  AntHybrid
//
//  Created by Hamilton Turner on 11/9/12.
//  Copyright (c) 2012 Virginia Tech. All rights reserved.
//

#ifndef __AntHybrid__MpSchedule__
#define __AntHybrid__MpSchedule__

#include <iostream>
#include <map>
#include <vector>
#include <memory>
#include <sstream>
#include "data.h"

// Used to hold a complete time-domain representation of a tour. Allows rapid querying of
// core and task state at any time point from 0 to end_time, which is useful for verifying that a
// scheduled tour is not violating any constraints.
// Tries to be memory-friendly, as we don't want to increase our memory load by
// O(cores*tasks) needlessly. However this means that the underlying tasks are only available for
// the lifetime of the MpsProblem this is based upon.
// It does use ant/iteration-independent variables, so it can be safely held across these
class MpSchedule;
class MpsProblem;

class ScheduleItem {
private:
  Task* task_;
  
public:
  unsigned int start_;
  unsigned int end_;
  Task* get_task() {
    return task_;
  };
  friend class MpSchedule;
  friend class MpsProblem;
  
  const char * get_cstr() {
    std::stringstream ss;
    ss << "[" << start_ << "," << end_ << "," << get_task()->get_cstr() << "]";
    return ss.str().c_str();
  }
  
  unsigned int pred_level() {
    return get_task()->pred_level_;
  }
};

class IdleTask : ScheduleItem {
private:
  Task static_task_;
public:
  IdleTask(unsigned int start, unsigned int end) {
    static_task_.pred_level_ = 0;
    static_task_.identifier_ = "idle";
    static_task_.execution_time_ = (end - start);
  }
  Task* get_task() {
    return &static_task_;
  }
  friend class MpSchedule;
  friend class MpsProblem;
};


class MpSchedule {
private:
  
  std::map<unsigned int, std::vector<ScheduleItem> > schedule_;
  
  // Adds a task to core
  void add_task(unsigned int core, ScheduleItem scheduled_task);
  
  // For a provided priority, this reports the last completion time of
  // any item with this priority
  unsigned int get_earliest_start_time(Task* t, unsigned int core_id, bool maintain_routing_time = false);
  
  // Based upon the current tasks, get the time that this core will complete
  unsigned int get_current_completion_time(unsigned int coreid);
  
public:
  
  // Prints a schedule in the format
  // Core <identifier>
  // <task1>
  // <task2>
  // Core <identifier>
  void print_schedule_as_page();
  
  // Any querying of times beyond this point will likely throw exceptions
  // Returns the time that this schedule will complete
  unsigned int get_completion_time();
  
  Task* get_task_on_core_at_t(unsigned int core, unsigned int time);
  
  std::vector<ScheduleItem>* get_schedule_for_core(unsigned int coreid);
  
  friend class MpsProblem;
};

#endif /* defined(__AntHybrid__MpSchedule__) */
