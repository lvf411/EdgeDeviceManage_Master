#ifndef __FILE_HPP
#define __FILE_HPP

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <jsoncpp/json/json.h>
#include <map>
#include "md5.hpp"
#include "master.hpp"

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

string client_task_list_export(int client_id);

string work_client_list_export();

void file_send(int sock, std::string path);

void file_recv(int sock, int fd);

#endif //__FILE_HPP
