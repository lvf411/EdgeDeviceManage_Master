#ifndef __TASK_SCHEDULE_HPP
#define __TASK_SCHEDULE_HPP

#include <string>
#include <map>
#include <vector>
#include <queue>
#include "genTest.h"

#define FILEPATH_PREFIX "./taskSchedule/"
#define TASK_PROGRAM_PATH_PREFIX "../task/"

//测试程序内容为循环taskweight取整后乘上TEST_TASK_CYCLE_PARAM次
#define TEST_TASK_CYCLE_PARAM 10000000

//交叉编译器路径
#define CROSS_COMPILE_TOOL_PATH "/home/lf/Downloads/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-g++"

TestInfo DAGGenterate(string DAGfilename, int tasknum, int nodenum, double edgeNumConpensation);

std::vector<std::queue<int>> HEFT_task_schedule(string DAGfilename);

void test_task_exeprogram_generate(const TestInfo &tinfo, int taskid);

int test_task_info_import(const TestInfo &tinfo, std::vector<std::queue<int>> &taskDeploy);

#endif //__TASK_SCHEDULE_HPP