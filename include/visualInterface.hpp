#ifndef __VISUAL_INTERFACE_HPP
#define __VISUAL_INTERFACE_HPP

#include <iostream>
#include "master.hpp"
#include "task.hpp"
#include "jsoncpp/json/json.h"
#include "taskSchedule.hpp"
#include <strstream>

string scheduleResExport(int taskNum, vector<queue<int>> &scheduleRes);

void visualizationTaskGenerateAndDeploy(int taskNum, double NC);

#endif //__VISUAL_INTERFACE_HPP