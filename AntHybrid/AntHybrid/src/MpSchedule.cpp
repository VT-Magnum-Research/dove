//
//  MpSchedule.cpp
//  AntHybrid
//
//  Created by Hamilton Turner on 11/9/12.
//  Copyright (c) 2012 Virginia Tech. All rights reserved.
//

#include "MpSchedule.h"
#include <algorithm>

void MpSchedule::add_task(unsigned int core, ScheduleItem scheduled_task) {
  schedule_[core].push_back(scheduled_task);
}

unsigned int MpSchedule::get_priority_end_time(unsigned int priority) {

  unsigned int completion_time = 0;
  std::map<unsigned int, std::vector<ScheduleItem> >::iterator it;
  for (it = schedule_.begin(); it != schedule_.end(); it++)
  {
    std::vector<ScheduleItem> tasks = it->second;
    std::vector<ScheduleItem>::iterator it2;
    for (it2 = tasks.begin(); it2 != tasks.end(); it2++)
      if (it2->priority()==priority)
        completion_time = std::max(completion_time, it2->end_);
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


