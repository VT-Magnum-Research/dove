//
//  MpSchedule.cpp
//  AntHybrid
//
//  Created by Hamilton Turner on 11/9/12.
//  Copyright (c) 2012 Virginia Tech. All rights reserved.
//

#include "MpSchedule.h"
#include "mps.h"
#include <algorithm>

void MpSchedule::add_task(unsigned int core, ScheduleItem scheduled_task) {
  schedule_[core].push_back(scheduled_task);
}

unsigned int MpSchedule::get_earliest_start_time(Task* task, unsigned int core_id, bool maintain_routing_time) {

  // No one can start before a core is free
  unsigned int completion_time = get_current_completion_time(core_id);
  
  // No predecessors, so we can start as soon as the core is free
  if (task->pred_level_ == 1)
    return completion_time;
  
  unsigned int predecessor_priority = task->pred_level_ - 1;
  std::map<unsigned int, std::vector<ScheduleItem> >::iterator core;
  for (core = schedule_.begin(); core != schedule_.end(); core++)
  {
    std::vector<ScheduleItem> core_tasks = core->second;
    std::vector<ScheduleItem>::iterator core_task;
    for (core_task = core_tasks.begin(); core_task != core_tasks.end(); core_task++)
      if (core_task->pred_level()==predecessor_priority) {
        // We cannot run until this task is finished
        completion_time = std::max(completion_time, core_task->end_);
        
        
        // TODO We cannot run until the task result is routed to us!
        //if (maintain_routing_time)
        //  completion_time = std::max(completion_time, core_task->end_ +
        //                            MpsProblem::get_routing_time(core_id, core->first));
          
      }
  }
  
  return completion_time;
}

void MpSchedule::print_schedule_as_page() {
  std::cout << std::endl << std::endl;
  std::map<unsigned int, std::vector<ScheduleItem> >::iterator it;
  for (it = schedule_.begin(); it != schedule_.end(); it++)
  {
    std::cout << "Core " << it->first << std::endl;
    std::vector<ScheduleItem> tasks = it->second;
    std::vector<ScheduleItem>::iterator it2;
    for (it2 = tasks.begin(); it2 != tasks.end(); it2++)
      std::cout << "\t" << it2->get_cstr() << std::endl;
  }
}

unsigned int MpSchedule::get_current_completion_time(unsigned int coreid) {
  if (schedule_[coreid].size() == 0)
    return 0;
  ScheduleItem task = schedule_[coreid].back();
  return task.end_;
}

unsigned int MpSchedule::get_completion_time() {
  unsigned int max = 0;
  std::map<unsigned int, std::vector<ScheduleItem> >::iterator it;
  for (it = schedule_.begin(); it != schedule_.end(); it++)
    max = std::max(it->second.back().end_, max);
  return max;
}

std::vector<ScheduleItem>* MpSchedule::get_schedule_for_core(unsigned int coreid) {
  return &(schedule_[coreid]);
}


