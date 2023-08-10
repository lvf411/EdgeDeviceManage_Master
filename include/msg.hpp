#ifndef __MSG_HPP
#define __MSG_HPP

#include "master.hpp"
#include "file.hpp"

#define FILE_PACKAGE_SIZE 40960     //4KB

std::string FileInfoMsgEncode(FileInfo *info);

#endif //__MSG_HPP