#ifndef __FILE_HPP
#define __FILE_HPP

#include <string>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <jsoncpp/json/json.h>
#include "md5.hpp"

struct FileInfo
{
    std::string fname;
    long long int exatsize;
    int filesize;
    char unit;
    std::string md5;
};

void FileInfoInit(FileInfo *info);

bool FileInfoGet(std::string path, FileInfo *info);

#endif //__FILE_HPP
