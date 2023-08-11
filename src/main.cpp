#include "master.hpp"
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <jsoncpp/json/json.h>
#include <thread>
#include <mutex>
#include "interface.hpp"
#include "file.hpp"
#include "msg.hpp"

#define InitFile "master_init_sample.json"
#define MAX_LISTENING 1000

using namespace std;

Master master;
list_head free_client_list, work_client_list, deployed_task_list, uninit_task_list;
mutex mutex_slave_list;
map<int, ClientNode *> free_client_list_map, work_client_list_map;
int increment_slave_id = 1;

string work_client_list_export();
string client_task_list_export(int client_id);

//初始化主节点
int startup()
{
    //从初始化文件中获取主节点设置的IP地址与监听端口
    ifstream ifs(InitFile);
    if(!ifs.is_open())
    {
        printf("open init file error!\n");
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
    master.free_client_head = free_client_list;
    master.work_client_num = 0;
    work_client_list = LIST_HEAD_INIT(work_client_list);
    master.work_client_head = work_client_list;
    master.task_num = 0;
    deployed_task_list = LIST_HEAD_INIT(deployed_task_list);
    master.task_list_head = deployed_task_list;
    uninit_task_list = LIST_HEAD_INIT(uninit_task_list);
    master.uninit_task_num = 0;
    master.uninit_task_list_head = uninit_task_list;

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
        list_head *task_list_head = new list_head();
        *task_list_head = LIST_HEAD_INIT(*task_list_head);
        clientNode->head = *task_list_head;
        list_head *self = new list_head();
        *self = LIST_HEAD_INIT(*self);
        clientNode->self = *self;
        list_add_tail(self, &free_client_list);
        free_client_list_map.insert(map<int, ClientNode *>::value_type(clientNode->client_id, clientNode));
        mutex_slave_list.unlock();
        
        clientNode->msg_send_threadID = thread(msg_send, clientNode);
        clientNode->msg_recv_threadID = thread(msg_recv, clientNode);
        clientNode->modified = 0;
        clientNode->status = 0;
    }
    return;
}

int main(){
    int sock = startup();

    thread slave_listen_threadID(slave_accept, sock);
    thread bash_io_threadID(bash_io);
    thread task_deploy_threadID(task_deploy);

    if(slave_listen_threadID.joinable())
    {
        slave_listen_threadID.join();
    }
    if(bash_io_threadID.joinable())
    {
        bash_io_threadID.join();
    }
    if(task_deploy_threadID.joinable())
    {
        task_deploy_threadID.join();
    }

    return 0;
}
