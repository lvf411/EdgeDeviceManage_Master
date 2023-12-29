#ifndef __VISUAL_INTERFACE_HPP
#define __VISUAL_INTERFACE_HPP

#include <iostream>
#include <string>
#include "master.hpp"
#include "task.hpp"
#include "jsoncpp/json/json.h"
#include "taskSchedule.hpp"
#include <strstream>
#include "httplib.h"

string scheduleResExport(int taskNum, vector<queue<int>> &scheduleRes);

std::string visualizationTaskGenerateAndDeploy(int taskNum, double NC);

int testserver();

#endif //__VISUAL_INTERFACE_HPP