#include "msg.hpp"
#include <mutex>
#include <sys/time.h>
#include <sstream>

extern Master master;

/*
    @brief 自动生成64位消息ID，高32位表示生成ID时的秒数，低32位为循环自增的消息ID序号
    @return 生成的消息ID
*/
long long int MsgIDGenerate()
{
    long long int ret = 0;
    static uint32_t count = 0;  //循环自增序号
    static std::mutex mutex_count;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t sec = tv.tv_sec;
    
    mutex_count.lock();
    count++;
    ret = (sec << 32) | count;
    mutex_count.unlock();

    return ret;
}

/*
    @brief 根据给定的文件信息封装出文件发送请求消息的内容
    @param info 待填入的文件信息的结构体指针
    @return 封装出的消息的内容字符串
*/
std::string FileInfoMsgEncode(FileInfo *info)
{
    Json::Value root;
    root["type"] = Json::Value(1);
    root["src_ip"] = Json::Value(inet_ntoa(master.addr.sin_addr));
    root["src_port"] = Json::Value(master.addr.sin_port);
    root["msg_id"] = Json::Value(MsgIDGenerate());
    root["fname"] = Json::Value(info->fname);
    root["exatsize"] = Json::Value(info->exatsize);
    root["md5"] = Json::Value(info->md5);
    //开启base64转码
    root["base64"] = Json::Value(0);
    if(info->exatsize > (FILE_PACKAGE_SIZE / 4) * 3)
    {
        //文件大小大于单个包长度，需进行拆包发送
        root["split"] = Json::Value(1);
        root["pack_num"] = Json::Value(info->exatsize / ((FILE_PACKAGE_SIZE * 3) / 4));
    }
    else
    {
        //文件大小小于单个包长度，不需要拆包发送
        root["split"] = Json::Value(0);
    }
    
    //生成字符串
    Json::FastWriter fw;
    std::stringstream ss;
    ss << fw.write(root);
    return ss.str();
}