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
#include "base64.hpp"

#define FILEBUF_MAX_LENGTH 3072     //3KB
#define FILE_PACKAGE_SIZE 4096      //4KB

struct FileInfo
{
    std::string fname;
    uint exatsize;
    int filesize;
    char unit;
    std::string md5;
};

struct FileTransInfo
{
    FileInfo info;              //文件元数据
    bool splitflag;             //是否进行拆包发送标志，拆包为TRUE，未拆包为FALSE
    bool base64flag;            //包内数据是否进行base64转码
    int packnum;                //拆包后的分包总数量
    int packsize;               //每个包所含数据的大小
    int file_type;              //文件类型
    int dst_rootid;             //指示发送给哪个任务的文件
    int dst_subtaskid;          //指示发送给 dst_rootid 的哪一个子任务的文件
};

#define FILE_TYPE_ORDINARY      0
#define FILE_TYPE_EXE           1
#define FILE_TYPE_INPUT         2
#define FILE_TYPE_OUTPUT        3
#define FILE_TYPE_KEY           4

void FileInfoInit(FileInfo *info);

bool FileInfoGet(std::string path, FileInfo *info);

std::string client_task_list_export(int client_id);

std::string work_client_list_export();

void file_send(int sock, std::string path);

void file_recv(int sock, FileInfo *info, std::ofstream& ofs, std::string& res_md5);

#endif //__FILE_HPP
