#include "master.hpp"
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <jsoncpp/json/json.h>
#include <thread>
#include <mutex>
#include "cmdInterface.hpp"
#include "file.hpp"
#include "msg.hpp"
#include "visualInterface.hpp"
#include <random>

#define InitFile "master_init_sample.json"
#define MAX_LISTENING 1000

using namespace std;

Master master;
list_head free_client_list, work_client_list, deployed_task_list, uninit_task_list, doneTaskList;
mutex mutex_slave_list;
map<int, ClientNode *> free_client_list_map, work_client_list_map;
map<int, Task *>deployedTaskListMap; 
int increment_slave_id = 1;
bool slave_list_export_file_flag = true;   //指示当前的从节点链表导出文件是否为最新的
mutex mutex_slave_change;
pthread_rwlock_t workClientListRWLock;

std::random_device Rd{};	//初始化随机种子
std::mt19937 Gen{ Rd() };	//伪随机数生成器
std::normal_distribution<> Standard_normal_dist{ 0,1 };		//正态分布随机数
std::uniform_real_distribution<> Uniform_real_dist01(0.0, 1.0);		//均匀分布随机数

//初始化主节点
int startup()
{
    //从初始化文件中获取主节点设置的IP地址与监听端口
    ifstream ifs(InitFile);
    if(!ifs.is_open())
    {
        perror("open init file error!\n");
        exit(0);
    }
    Json::Reader reader;
    Json::Value obj;
    reader.parse(ifs, obj);
    
    //1.初始化 socket
    int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock < 0){
		perror("socket");
		exit(1);
	}

    //2.绑定监听 socket ip与端口
	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_port = htons(obj["listen_port"].asInt());
	local.sin_addr.s_addr = inet_addr(obj["ip"].asCString());
    //closesocket 后不经历 TIME_WAIT 的过程，继续重用该socket
	bool bReuseaddr=true;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(const char*)&bReuseaddr,sizeof(bReuseaddr));
    
    //3.绑定 socket
	if(bind(sock,(struct sockaddr*)&local,sizeof(local)) < 0)
	{
		perror("bind");
		exit(2);
	}
	else{cout<<"Bind success."<<endl;}
	if(listen(sock, MAX_LISTENING) < 0)
	{
		perror("listen");
		exit(3);
	}
	else{cout<<"Listening..."<<endl;}

    //初始化 master 变量信息
    master.sock = sock;
    master.addr.sin_family = AF_INET;
	master.addr.sin_port = htons(obj["listen_port"].asInt());
    master.addr.sin_addr.s_addr = inet_addr(obj["ip"].asCString());
    master.task_num = 0;
    master.free_client_num = 0;
    free_client_list = LIST_HEAD_INIT(free_client_list);
    master.free_client_head = &free_client_list;
    master.work_client_num = 0;
    work_client_list = LIST_HEAD_INIT(work_client_list);
    master.work_client_head = &work_client_list;
    master.task_num = 0;
    deployed_task_list = LIST_HEAD_INIT(deployed_task_list);
    master.task_list_head = &deployed_task_list;
    uninit_task_list = LIST_HEAD_INIT(uninit_task_list);
    master.uninit_task_num = 0;
    master.uninit_task_list_head = &uninit_task_list;
    doneTaskList = LIST_HEAD_INIT(doneTaskList);
    master.doneTaskNum = 0;
    master.doneTaskListHead = &doneTaskList;

	return sock;
}

//从节点连接主节点线程，接收从节点连接并将其加入到从节点管理链表，为每个从节点连接分配单独的消息收发线程
void slave_accept(int sock)
{
    struct sockaddr_in client;
	socklen_t len = sizeof(client);
    while(1)
    {
        int new_sock = accept(sock,(struct sockaddr *)&client,&len);

        mutex_slave_list.lock();
        ClientNode *clientNode = new ClientNode();
        clientNode->client_id = increment_slave_id;
        increment_slave_id++;
        clientNode->sock = new_sock;
        clientNode->addr.sin_family = client.sin_family;
        clientNode->addr.sin_addr.s_addr = client.sin_addr.s_addr;
        clientNode->addr.sin_port = client.sin_port;
        clientNode->flag = -1;      //节点空闲
        clientNode->subtask_num = 0;
        clientNode->ability = SLAVE_ABILITY_DEFAULT;
        clientNode->head = new list_head();
        *clientNode->head = LIST_HEAD_INIT(*clientNode->head);
        clientNode->self = LIST_HEAD_INIT(clientNode->self);
        // list_add_tail(&clientNode->self, master.free_client_head);
        // master.free_client_num++;
        // free_client_list_map.insert(map<int, ClientNode *>::value_type(clientNode->client_id, clientNode));
        list_add_tail(&clientNode->self, master.work_client_head);
        master.work_client_num++;
        work_client_list_map.insert(map<int, ClientNode *>::value_type(clientNode->client_id, clientNode));
        slave_list_export_file_flag = true;
        mutex_slave_list.unlock();

        char recvbuf[100] = {0};
        recv(clientNode->sock, recvbuf, 100, 0);
        Json::Value root;
        Json::Reader rd;
        rd.parse(recvbuf, root);
        clientNode->listen_port = root["listen_port"].asInt();
        sleep(1);
        
        clientNode->msg_send_threadID = thread(msg_send, clientNode);
        clientNode->msg_recv_threadID = thread(msg_recv, clientNode);
        clientNode->modified = false;
        clientNode->status = INTERACT_STATUS_ROOT;

        clientNode->work_client_cahange_flag = true;

        

    }
    return;
}

//自动更新节点上的工作节点链表文件
void workClientListUpdate()
{
    while(1)
    {
        sleep(WORK_CLIENT_LIST_UPDATE_GAP_TIME);
        if(slave_list_export_file_flag == true)
        {
            //对 work_client_list 加写锁
            pthread_rwlock_wrlock(&workClientListRWLock);
            work_client_list_export();
            pthread_rwlock_unlock(&workClientListRWLock);

            mutex_slave_list.lock();
            std::map<int, ClientNode*>::iterator it;
            for (it = work_client_list_map.begin(); it != work_client_list_map.end(); ++it) {
                it->second->work_client_cahange_flag = true;
            }
            slave_list_export_file_flag = false;
            mutex_slave_list.unlock();
            
        }
        
    }
}

int main(){
    int sock = startup();

    thread slave_listen_threadID(slave_accept, sock);
    thread bash_io_threadID(bash_io);
    //thread task_deploy_threadID(task_deploy);
    thread HTTPServer_threadID(testserver);
    thread workClientListUpdate_threadID(workClientListUpdate);

    if(slave_listen_threadID.joinable())
    {
        slave_listen_threadID.join();
    }
    if(bash_io_threadID.joinable())
    {
        bash_io_threadID.join();
    }
    // if(task_deploy_threadID.joinable())
    // {
    //     task_deploy_threadID.join();
    // }
    if(HTTPServer_threadID.joinable())
    {
        HTTPServer_threadID.join();
    }
    if(workClientListUpdate_threadID.joinable())
    {
        workClientListUpdate_threadID.join();
    }

    return 0;
}
