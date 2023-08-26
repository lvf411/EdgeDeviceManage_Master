#include "msg.hpp"
#include <mutex>
#include <sys/time.h>
#include <sstream>

extern Master master;

//从节点消息发送线程
void msg_send(ClientNode *client)
{
    std::string path;
    while(1)
    {
        switch(client->status)
        {
            case INTERACT_STATUS_ROOT:
            {
                //检查是否有被分配新任务，若有，则切换状态请求发送导出的同步文件
                if(client->modified == 1)
                {
                    client->status = INTERACT_STATUS_SUBTASK_SYNCFILE_SEND_REQ;
                }
                //起始状态其他检查项目


                break;
            }
            case INTERACT_STATUS_SUBTASK_SYNCFILE_SEND_REQ:
            {
                //从节点被分配了新任务，导出子任务链表并向从节点发送json文件
                path.clear();
                path = client_task_list_export(client->client_id);
                if(path.empty())
                {
                    printf("client %d, IP:%s, port:%d export client task list error\n", client->client_id, inet_ntoa(client->addr.sin_addr), client->addr.sin_port);
                    return;
                }
                FileInfo info;
                FileInfoInit(&info);
                FileInfoGet(path, &info);
                string msg = FileSendReqMsgEncode(&info);
                send(client->sock, msg.c_str(), msg.length(), 0);

                client->status = INTERACT_STATUS_SUBTASK_SYNCFILE_SEND_WAIT_ACK;
                break;
            }
            case INTERACT_STATUS_SUBTASK_SYNCFILE_SEND_CONNECT:
            {
                //从节点同意了文件传输请求，正式建立连接并新建线程发送文件
                client->file_trans_sock = socket(AF_INET, SOCK_STREAM, 0);
                struct sockaddr_in s_file_addr = {0};
                s_file_addr.sin_family = AF_INET;
                s_file_addr.sin_addr.s_addr = client->addr.sin_addr.s_addr;
                s_file_addr.sin_port = htons(client->file_trans_port);
                if(connect(client->file_trans_sock, (struct sockaddr *)&s_file_addr, sizeof(struct sockaddr)) < 0)
                {
                    //连接目标端口失败，向从节点发送停止传输消息，退回普通状态
                    printf("file send: failed to connect to the target port\n");
                    std::string msg = FileSendCancelMsgEncode();
                    send(client->sock, msg.c_str(), msg.length(), 0);
                    client->status = INTERACT_STATUS_ROOT;
                }
                break;
            }
            case INTERACT_STATUS_SUBTASK_SYNCFILE_SEND_SEND:
            {
                //接收到从节点对主节点发起的tcp连接的ack确认，主节点可以开始发送文件内容
                std::thread filesend_threadID(file_send, client->file_trans_sock, path);
                filesend_threadID.join();
                close(client->file_trans_sock);
                break;
            }
            default:
            {
                //休眠100ms
                usleep(100000);
                break;
            }
        }
        
    }
}

//从节点消息接收线程
void msg_recv(void *arg)
{
    ClientNode *client = (ClientNode *)arg;
    char msg_buf[MSG_BUFFER_SIZE] = {0};

    while(1)
    {
        memset(msg_buf, 0, MSG_BUFFER_SIZE);
        recv(client->sock, msg_buf, MSG_BUFFER_SIZE, 0);
        Json::Value root;
        Json::Reader rd;
        rd.parse(msg_buf, root);

        int msg_type = root["type"].asInt();
        switch(msg_type)
        {
            case MSG_TYPE_FILESEND_REQ_ACK:
            {
                bool ret = root["ret"].asBool();
                if(ret == true)
                {
                    client->file_trans_port = root["listen_port"].asInt();
                    client->status = INTERACT_STATUS_SUBTASK_SYNCFILE_SEND_CONNECT;
                }
                else
                {
                    client->status = INTERACT_STATUS_ROOT;
                }
                
                break;
            }
            case MSG_TYPE_FILESEND_START:
            {
                client->status = INTERACT_STATUS_SUBTASK_SYNCFILE_SEND_SEND;
                break;
            }
            default:
            {
                break;
            }
        }
    }
    
    return;
}

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
std::string FileSendReqMsgEncode(FileInfo *info)
{
    Json::Value root;
    root["type"] = Json::Value(MSG_TYPE_FILESEND_REQ);
    root["src_ip"] = Json::Value(inet_ntoa(master.addr.sin_addr));
    root["src_port"] = Json::Value(ntohs(master.addr.sin_port));
    root["msg_id"] = Json::Value(MsgIDGenerate());
    root["fname"] = Json::Value(info->fname);
    root["exatsize"] = Json::Value(info->exatsize);
    root["md5"] = Json::Value(info->md5);
    //开启base64转码
    root["base64"] = Json::Value(true);
    if(info->exatsize > (FILE_PACKAGE_SIZE / 4) * 3)
    {
        //文件大小大于单个包长度，需进行拆包发送
        root["split"] = Json::Value(true);
        root["pack_num"] = Json::Value(info->exatsize / ((FILE_PACKAGE_SIZE * 3) / 4));
        root["pack_size"] = Json::Value(FILE_PACKAGE_SIZE);
    }
    else
    {
        //文件大小小于单个包长度，不需要拆包发送
        root["split"] = Json::Value(false);
    }
    
    //生成字符串
    Json::FastWriter fw;
    std::stringstream ss;
    ss << fw.write(root);
    return ss.str();
}

/*
    @brief 封装文件发送停止消息
    @return 封装出的消息的内容字符串
*/
std::string FileSendCancelMsgEncode()
{
    Json::Value root;
    root["type"] = Json::Value(MSG_TYPE_FILESEND_CANCEL);
    root["src_ip"] = Json::Value(inet_ntoa(master.addr.sin_addr));
    root["src_port"] = Json::Value(ntohs(master.addr.sin_port));
    root["msg_id"] = Json::Value(MsgIDGenerate());

    //生成字符串
    Json::FastWriter fw;
    std::stringstream ss;
    ss << fw.write(root);
    return ss.str();
}
