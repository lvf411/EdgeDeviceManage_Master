#include "msg.hpp"
#include <mutex>
#include <sys/time.h>
#include <sstream>
#include <assert.h>

extern Master master;
extern bool slave_list_export_file_flag;
extern std::mutex mutex_slave_change, mutex_task_list, mutexDoneTaskList;
extern pthread_rwlock_t workClientListRWLock;
extern std::map<int, ClientNode *> work_client_list_map;
extern std::map<int, Task *>deployedTaskListMap; 

long long int MsgIDGenerate();
std::string FileSendReqMsgEncode(FileTransInfo *info);
std::string FileSendCancelMsgEncode();

//从节点消息发送线程
void msg_send(ClientNode *client)
{
    std::cout << "msgsend" << std::endl;
    //先发送主节点分配给该从节点的从节点ID
    Json::Value root;
    root["slaveID"] = Json::Value(client->client_id);
    Json::FastWriter fw;
    std::stringstream ss;
    ss << fw.write(root);
    send(client->sock, ss.str().c_str(), ss.str().length(), 0);
    //进入正常工作状态前先睡眠 WORK_CLIENT_LIST_UPDATE_GAP_TIME * 2 秒，防止系统第一次启动时批量从节点请求的 work_client_list.txt 文件可能不同
    sleep(WORK_CLIENT_LIST_UPDATE_GAP_TIME * 2);
    while(1)
    {
        switch(client->status)
        {
            case INTERACT_STATUS_ROOT:
            {
                //检查是否需要重新同步从节点链表信息
                if(client->work_client_cahange_flag == true)
                {
                    client->mutex_status.lock();
                    client->status = INTERACT_STATUS_SLAVELIST_EXPORT_FILE_GETFILE;
                    client->mutex_status.unlock();
                    break;
                }
                //检查是否有被分配新任务，若有，则切换状态请求发送导出的同步文件
                if(client->modified == true)
                {
                    client->mutex_status.lock();
                    client->status = INTERACT_STATUS_SUBTASK_SYNCFILE_GETFILE;
                    client->mutex_status.unlock();
                    break;
                }
                //起始状态其他检查项目


                break;
            }
            case INTERACT_STATUS_SUBTASK_SYNCFILE_GETFILE:
            {
                //从节点被分配了新任务，导出子任务链表并向从节点发送json文件
                client->file_trans_fname.clear();
                client->file_trans_fname = client_task_list_export(client->client_id);
                if(client->file_trans_fname.empty())
                {
                    printf("client %d, IP:%s, port:%d export client task list error\n", client->client_id, inet_ntoa(client->addr.sin_addr), client->addr.sin_port);
                    return;
                }
                client->transinfo = new FileTransInfo();
                FileInfoInit(&client->transinfo->info);
                FileInfoGet(client->file_trans_fname, &client->transinfo->info);
                client->transinfo->file_type = FILE_TYPE_KEY;
                client->transinfo->dst_rootid = 0;
                client->transinfo->dst_subtaskid = 0;
                client->mutex_status.lock();
                client->status = INTERACT_STATUS_FILESEND_SEND_REQ;
                client->mutex_status.unlock();
                client->modified = false;
                break;
            }
            case INTERACT_STATUS_SLAVELIST_EXPORT_FILE_GETFILE:
            {
                //工作从节点链表发生改变，需要重新同步
                // //检查当前的从节点链表导出文件是否最新
                // mutex_slave_change.lock();
                // if(slave_list_export_file_flag == true)
                // {
                //     work_client_list_export();
                //     client->work_client_cahange_flag = true;
                // }
                // mutex_slave_change.unlock();

                //从节点被分配了新任务，导出子任务链表并向从节点发送json文件
                client->file_trans_fname.clear();
                client->file_trans_fname = WORK_CLIENT_LIST_FNAME;

                if(client->transinfo == NULL)
                {
                    client->transinfo = new FileTransInfo();
                }
                else
                {
                    //清空transinfo
                    FileInfoInit(&client->transinfo->info);
                    client->transinfo->splitflag = false;
                    client->transinfo->base64flag = false;
                    client->transinfo->packnum = 0;
                    client->transinfo->packsize = 0;
                    client->transinfo->file_type = 0;
                    client->transinfo->dst_rootid = 0;
                    client->transinfo->dst_subtaskid = 0;
                }
                pthread_rwlock_rdlock(&workClientListRWLock);
                FileInfoInit(&client->transinfo->info);
                FileInfoGet(client->file_trans_fname, &client->transinfo->info);
                client->transinfo->file_type = FILE_TYPE_WORK_CLIENT_LIST;
                client->transinfo->dst_rootid = 0;
                client->transinfo->dst_subtaskid = 0;
                client->work_client_cahange_flag = false;

            }
            case INTERACT_STATUS_FILESEND_SEND_REQ:
            {
                string msg = FileSendReqMsgEncode(client->transinfo);
                send(client->sock, msg.c_str(), msg.length(), 0);
                std::cout << msg << std::endl;

                client->mutex_status.lock();
                client->status = INTERACT_STATUS_FILESEND_WAIT_ACK;
                client->mutex_status.unlock();
                break;
            }
            case INTERACT_STATUS_FILESEND_CONNECT:
            {
                //从节点同意了文件传输请求，正式建立连接并新建线程发送文件
                client->file_trans_sock = socket(AF_INET, SOCK_STREAM, 0);
                std::cout << "client:" << client->client_id << "socket file_trans_sock:" << client->file_trans_sock << std::endl;
                struct sockaddr_in s_file_addr = {0};
                s_file_addr.sin_family = AF_INET;
                s_file_addr.sin_addr.s_addr = client->addr.sin_addr.s_addr;
                s_file_addr.sin_port = htons(client->file_trans_port);
                int count = 0;
                while(count < 10)
                {
                    count++;
                    std::cout << "client:" << client->client_id << "count:" << count << std::endl;
                    int ret = connect(client->file_trans_sock, (struct sockaddr *)&s_file_addr, sizeof(struct sockaddr));
                    std::cout << "client:" << client->client_id << "file_trans_sock:" << client->file_trans_sock << "ret:" << ret << std::endl;
                    if(ret == 0)
                    {
                        std::cout << "ret:" << ret << std::endl;
                        break;
                    }
                    
                }
                if(count >= 10)
                {
                    //连接目标端口失败，向从节点发送停止传输消息，退回普通状态
                    printf("file send: failed to connect to the target port\n");
                    std::string msg = FileSendCancelMsgEncode();
                    send(client->sock, msg.c_str(), msg.length(), 0);
                    std::cout << msg << std::endl;
                    client->mutex_status.lock();
                    client->status = INTERACT_STATUS_ROOT;
                    client->mutex_status.unlock();
                }
                client->sem.Wait();
                break;
            }
            case INTERACT_STATUS_FILESEND_SENDFILE:
            {
                //接收到从节点对主节点发起的tcp连接的ack确认，主节点可以开始发送文件内容
                std::thread filesend_threadID(file_send, client->file_trans_sock, client->file_trans_fname);
                filesend_threadID.join();
                if(client->transinfo->file_type == FILE_TYPE_WORK_CLIENT_LIST)
                {
                    pthread_rwlock_unlock(&workClientListRWLock);
                }
                close(client->file_trans_sock);
                client->sem.Wait();
                // if(client->status == INTERACT_STATUS_FILESEND_SENDFILE)
                // {
                //     continue;
                // }
                // close(client->file_trans_sock);
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
void msg_recv(ClientNode *client)
{
    char msg_buf[MSG_BUFFER_SIZE] = {0};

    while(1)
    {
        memset(msg_buf, 0, MSG_BUFFER_SIZE);
        recv(client->sock, msg_buf, MSG_BUFFER_SIZE, 0);
        std::cout << "recv:" << msg_buf << std::endl;
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
                    client->mutex_status.lock();
                    client->status = INTERACT_STATUS_FILESEND_CONNECT;
                    client->mutex_status.unlock();
                }
                else
                {
                    client->mutex_status.lock();
                    client->status = INTERACT_STATUS_ROOT;
                    client->mutex_status.unlock();
                }
                
                break;
            }
            case MSG_TYPE_FILESEND_START:
            {
                client->mutex_status.lock();
                client->status = INTERACT_STATUS_FILESEND_SENDFILE;
                client->sem.Signal();
                client->mutex_status.unlock();
                break;
            }
            case MSG_TYPE_FILESEND_RES:
            {
                int res = root["res"].asBool();
                if(res == false)
                {
                    client->mutex_status.lock();
                    client->status = INTERACT_STATUS_FILESEND_SEND_REQ;
                    //根据文件名重新获取当前传输文件信息
                    //初始化当前传输文件信息
                    FileInfoInit(&client->transinfo->info);
                    client->transinfo->splitflag = false;
                    client->transinfo->base64flag = false;
                    client->transinfo->packnum = 0;
                    client->transinfo->packsize = 0;
                    // client->transinfo->file_type = 0;
                    // client->transinfo->dst_rootid = 0;
                    // client->transinfo->dst_subtaskid = 0;

                    FileInfoGet(client->file_trans_fname, &client->transinfo->info);
                    client->mutex_status.unlock();

                }
                else
                {
                    client->status = INTERACT_STATUS_ROOT;
                }
                client->sem.Signal();
                break;
            }
            case MSG_TYPE_FILEREQ_REQ:
            {
                int flag;
                std::string fname = root["fname"].asString();
                std::ifstream ifs(TASK_STORE_PATH + fname);
                if(ifs.good())
                {
                    if(client->status == INTERACT_STATUS_ROOT)
                    {
                        //只有客户端状态空闲时可以响应文件传输的请求
                        client->mutex_status.lock();
                        client->transinfo->file_type = FILE_TYPE_EXE;
                        client->transinfo->dst_rootid = root["rootid"].asInt();
                        client->transinfo->dst_subtaskid = root["subtaskid"].asInt();
                        FileInfoInit(&client->transinfo->info);
                        FileInfoGet(TASK_STORE_PATH + fname, &client->transinfo->info);
                        client->status = INTERACT_STATUS_FILESEND_SEND_REQ;
                        client->mutex_status.unlock();
                        client->file_trans_fname = TASK_STORE_PATH + fname;
                        flag = FILEREQ_ACK_OK;
                    }
                    else
                    {
                        flag = FILEREQ_ACK_WAIT;
                    }
                }
                else
                {
                    flag = FILEREQ_ACK_ERROR;
                }
                Json::Value msg;
                msg["type"] = Json::Value(MSG_TYPE_FILEREQ_ACK);
                msg["src_ip"] = Json::Value(inet_ntoa(master.addr.sin_addr));
                msg["src_port"] = Json::Value(ntohs(master.addr.sin_port));
                msg["msg_id"] = Json::Value(MsgIDGenerate());
                msg["fname"] = Json::Value(fname);
                msg["ret"] = Json::Value(flag);
                msg["rootid"] = Json::Value(root["rootid"].asInt());
                msg["subtaskid"] = Json::Value(root["subtaskid"].asInt());
                Json::FastWriter fw;
                std::stringstream ss;
                ss << fw.write(msg);
                send(client->sock, ss.str().c_str(), ss.str().length(), 0);
                std::cout << ss.str() << std::endl;
                break;
            }
            case MSG_TYPE_SUBTASK_RESULT:
            {
                //接收到从节点任务执行完成上报报文
                //需删除对应从节点结构中的相应任务信息，归档任务链表中的对应信息
                int rootID = root["root_id"].asInt();
                int subtaskID = root["subtask_id"].asInt();
                int slaveID = root["slaveID"].asInt();

                Task *task = NULL;
                //处理从节点结构体中的子任务信息
                // auto it = work_client_list_map.find(slaveID);
                // assert(it != work_client_list_map.end());
                // ClientNode *client = it->second;
                list_head *subtaskList = client->head;
                list_head *temp = subtaskList->next;
                while (temp != subtaskList)
                {
                    SubTaskNode *subtaskNode = list_entry(temp, SubTaskNode, clientself);
                    std::cout << "rootID:" << subtaskNode->root_id << "subtaskID:" << subtaskNode->subtask_id << std::endl;
                    auto it = deployedTaskListMap.find(rootID);
                    task = it->second;
                    assert(task != NULL);
                    if(subtaskNode->root_id == rootID)
                    {
                        if(subtaskNode->subtask_id == subtaskID)
                        {
                            //找到目标子任务结构体，进行清除
                            subtaskNode->done = true;
                            list_del(&subtaskNode->clientself);
                            client->mutex_status.lock();
                            client->subtask_num--;
                            client->mutex_status.unlock();
                            //修改任务结构体中的已执行数量
                            std::cout << "slaveID:" << client->client_id << ":" << task->task_id << std::endl;
                            task->mutexDoneSubtaskNum.lock();
                            task->doneSubtaskNum++;
                            task->mutexDoneSubtaskNum.unlock();
                            break;
                        }
                    }
                    temp = temp->next;
                    std::cout << "slaveID:" << client->client_id << std::endl;
                }
                if(temp == subtaskList)
                {
                    std::cout << "Error subtask result!!! " << msg_buf << std::endl;
                    break;
                }
                
                //归档任务链表中对应信息
                assert(task != NULL);
                if(task->doneSubtaskNum == task->subtask_num)
                {
                    mutex_task_list.lock();
                    list_del(&task->self);
                    master.task_num--;
                    mutex_task_list.unlock();

                    mutexDoneTaskList.lock();
                    list_add_tail(&task->self, master.doneTaskListHead);
                    master.doneTaskNum++;
                    mutexDoneTaskList.unlock();
                }
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
std::string FileSendReqMsgEncode(FileTransInfo *transinfo)
{
    Json::Value root;
    root["type"] = Json::Value(MSG_TYPE_FILESEND_REQ);
    root["src_ip"] = Json::Value(inet_ntoa(master.addr.sin_addr));
    root["src_port"] = Json::Value(ntohs(master.addr.sin_port));
    root["msg_id"] = Json::Value(MsgIDGenerate());
    root["fname"] = Json::Value(GetFnameFromPath(transinfo->info.fname));
    root["exatsize"] = Json::Value(transinfo->info.exatsize);
    root["md5"] = Json::Value(transinfo->info.md5);
    //开启base64转码
    root["base64"] = Json::Value(true);
    root["file_type"] = Json::Value(transinfo->file_type);
    root["dst_rootid"] = Json::Value(transinfo->dst_rootid);
    root["dst_subtaskid"] = Json::Value(transinfo->dst_subtaskid);
    if(transinfo->info.exatsize > (FILE_PACKAGE_SIZE / 4) * 3)
    {
        //文件大小大于单个包长度，需进行拆包发送
        root["split"] = Json::Value(true);
        root["pack_num"] = Json::Value(transinfo->info.exatsize / ((FILE_PACKAGE_SIZE * 3) / 4) + 1);
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
