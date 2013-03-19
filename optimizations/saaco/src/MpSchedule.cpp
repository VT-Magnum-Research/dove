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
#include <iostream>
#include <sstream>


#include "data.h"
#include "graph.h"

MpSchedule::MpSchedule(SymmetricMatrix<unsigned int>* routing_costs,
                       Matrix<unsigned int>*          run_times,
                       DirectedAcyclicGraph*          task_precedence):
                    run_times_(run_times),
                    routing_costs_(routing_costs),
                    task_precedence_(task_precedence)
{
}

void MpSchedule::add_task(unsigned int core, ScheduleItem scheduled_task) {
  schedule_[core].push_back(scheduled_task);
}

std::string to_string(int & a) {
  std::stringstream ss;
  ss << a;
  return ss.str();
}
std::string to_string(unsigned int & a) {
  std::stringstream ss;
  ss << a;
  return ss.str();
}
std::string to_string(const unsigned int & a) {
  std::stringstream ss;
  ss << a;
  return ss.str();
}

unsigned int MpSchedule::get_earliest_start_time(Task* task, unsigned int core_id, std::vector<std::string>& delays, bool maintain_routing_time) {

  // No one can start before a core is free
  unsigned int completion_time = get_current_completion_time(core_id);
  std::string completion_delay("+Core ");
  completion_delay.append(to_string(core_id)).append(" will be idle at ").append(to_string(completion_time));
  delays.push_back(completion_delay);
  
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
        unsigned int routing_delay = core_task->end_ + routing_costs_->operator[](core_id)[core->first];

        // Lots of logging information
        std::string predecessor_info("");
        if (core_task->end_ > completion_time)
          predecessor_info.append("+");
        predecessor_info.append("Predecessor ")
        .append(core_task->task_->identifier_)
        .append(" on Core ")
        .append(to_string(core->first))
        .append(" completes at ")
        .append(to_string(core_task->end_));
        delays.push_back(predecessor_info);
        
        // We cannot run if a predecessor is not complete
        completion_time = std::max(completion_time, core_task->end_);
        
        if (!maintain_routing_time)
          continue;
        
        // More logging information
        std::string predecessor_route("");
        if (routing_delay > completion_time)
          predecessor_route.append("+");
        predecessor_route.append("For predecessor ")
        .append(core_task->task_->identifier_)
        .append(", routing from core ")
        .append(to_string(core->first))
        .append(" to ")
        .append(to_string(core_id)
        .append(" completes at ")
        .append(to_string(routing_delay)));
        delays.push_back(predecessor_route);
        
        // We cannot run if the result of a predecessor has not been routed to us
        completion_time = std::max(completion_time, routing_delay);
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


