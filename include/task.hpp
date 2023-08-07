#ifndef __TASK_HPP
#define __TASK_HPP

#include <string>
#include <fstream>
#include <pthread.h>
#include <jsoncpp/json/json.h>
#include "master.hpp"
#include "list.hpp"

bool task_add(std::string path);

#endif //__TASK_HPP