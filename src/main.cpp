#include "master.hpp"
#include <fstream>
#include <jsoncpp/json/json.h>
#include <pthread.h>
#include "interface.hpp"

#define InitFile "master_init_sample.json"
#define MAX_LISTENING 1000

using namespace std;

Master master;
list_head free_client_list, work_client_list, deployed_task_list, uninit_task_list;
pthread_mutex_t mutex_slave_list, mutex_task_list, mutex_uninit_task_list, mutex_task_id;
int task_increment_id = 0;
int increment_slave_id = 1;

//初始化主节点
int startup()
{
    //从初始化文件中获取主节点设置的IP地址与监听端口
    ifstream ifs(InitFile);
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
	local.sin_port = htons(obj["port"].asInt());
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
	master.addr.sin_port = htons(obj["port"].asInt());
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
    master.uninit_task_list_head = uninit_task_list;

	return sock;
}

//从节点连接主节点线程，接收从节点连接并将其加入到从节点管理链表
void* slave_accept(void *arg)
{
    int sock = *(int*)arg;
    struct sockaddr_in client;
	socklen_t len = sizeof(client);
    while(1)
    {
        int new_sock = accept(sock,(struct sockaddr *)&client,&len);

        pthread_mutex_lock(&mutex_slave_list);
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
        pthread_mutex_unlock(&mutex_slave_list);
    }
    return NULL;
}

//分配任务给各个节点
void* task_deploy(void *arg)
{
    while(!list_empty(&uninit_task_list)){
        //从未分配任务链表中取出任务
        pthread_mutex_lock(&mutex_uninit_task_list);
        list_head task = *uninit_task_list.next;
        list_del(uninit_task_list.next);
        pthread_mutex_unlock(&mutex_uninit_task_list);

        //分配子任务执行的从节点

        //将任务插入任务链表
        pthread_mutex_lock(&mutex_task_list);
        list_add_tail(&task, &deployed_task_list);
        pthread_mutex_unlock(&mutex_task_list);

        //更新从节点结构体中的信息

        
    }
    return NULL;
}

int main(){
    int sock = startup();

    pthread_mutex_init(&mutex_slave_list, NULL);
    pthread_mutex_init(&mutex_task_list, NULL);
    pthread_mutex_init(&mutex_uninit_task_list, NULL);
    pthread_mutex_init(&mutex_task_id, NULL);

    pthread_t slave_listen_threadID, bash_input_threadID, bash_output_threadID, task_deploy_threadID;
    pthread_create(&slave_listen_threadID, NULL, slave_accept, &sock);
    pthread_create(&bash_output_threadID, NULL, bash_output, NULL);
    pthread_create(&bash_input_threadID, NULL, bash_input, NULL);
    pthread_create(&task_deploy_threadID, NULL, task_deploy, NULL);
    
    pthread_mutex_destroy(&mutex_slave_list);
    pthread_mutex_destroy(&mutex_task_list);
    pthread_mutex_destroy(&mutex_uninit_task_list);
    pthread_mutex_destroy(&mutex_task_id);
    return 0;
}
