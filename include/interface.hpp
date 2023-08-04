#ifndef __INTERFACE_HPP
#define __INTERFACE_HPP

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include "master.hpp"
#include "kbhit.hpp"

void* bash_output(void *arg);

void* bash_input(void *arg);

#endif //__INTERFACE_HPP
