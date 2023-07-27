#include "master.hpp"
#include <jsoncpp/json/json.h>
#include <pthread.h>

#define InitFile "master_init_sample.json"
#define MAX_LISTENING 1000

using namespace std;

Master master;
list_head free_client_list, work_client_list, task_list, uninit_task_list;
pthread_mutex_t mutex_slave_list, mutex_task_list, mutex_uninit_task_list, mutex_task_id;
int task_increment_id = 0;

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
    task_list = LIST_HEAD_INIT(task_list);
    master.task_list_head = task_list;
    uninit_task_list = LIST_HEAD_INIT(uninit_task_list);
    master.uninit_task_list_head = uninit_task_list;

	return sock;
}

//根据任务描述文件添加任务
bool task_add(string path){
    ifstream ifs(path);
    Json::Reader reader;
    Json::Value obj;
    reader.parse(ifs, obj);

    Task *task = new(Task);
    pthread_mutex_lock(&mutex_task_id);
    task->id = ++task_increment_id;
    pthread_mutex_unlock(&mutex_task_id);
    task->task_id = obj["task_id"].asString();
    task->subtask_num = obj["subtask_num"].asInt();

    list_head *subtask_list_head = new(list_head);
    *subtask_list_head = LIST_HEAD_INIT(*subtask_list_head);
    task->subtask_head = *subtask_list_head;
    //解析描述文件取出任务内容
    if(!obj["subtask"].isArray()){
        perror("desc file error");
        return false;
    }
    for(int i = 0; i < obj["subtask"].size(); i++)
    {
        SubTaskNode *node = new(SubTaskNode);
        node->subtask_id = obj["subtask"][i]["subtaskid"].asInt();
        node->root_id = task->id;
        node->exepath = obj["subtask"][i]["exe_path"].asString();
        node->prev_num = obj["subtask"][i]["input_src_num"].asInt();
        if(node->prev_num ==0)
        {
            node->prev_head = NULL;
        }
        else{
            node->prev_head = new(SubTaskResult);
            SubTaskResult *temp = node->prev_head;
            temp->next = NULL;
            for(int j = 0; j < node->prev_num; j++)
            {
                SubTaskResult *n = new(SubTaskResult);
                n->dir = 0;
                n->subtask_id = obj["subtask"][i]["input_src"][j].asInt();
                n->next = temp->next;
                temp->next = n;
            }
        }
        node->next_num = obj["subtask"][i]["output_dst_num"].asInt();
        if(node->next_num ==0)
        {
            node->succ_head = NULL;
        }
        else{
            node->succ_head = new(SubTaskResult);
            SubTaskResult *temp = node->succ_head;
            temp->next = NULL;
            for(int j = 0; j < node->next_num; j++)
            {
                SubTaskResult *n = new(SubTaskResult);
                n->dir = 0;
                n->subtask_id = obj["subtask"][i]["output_dst"][j].asInt();
                n->next = temp->next;
                temp->next = n;
            }
        }

        node->head = task->subtask_head;
        list_head *s = new(list_head);
        *s = LIST_HEAD_INIT(*s);
        node->self = *s;
        list_add_tail(s, &task->subtask_head);
    }

    //将任务节点插入到待分配任务链表尾部
    list_head *self = new(list_head);
    *self = LIST_HEAD_INIT(*self);
    task->self = *self;
    pthread_mutex_lock(&mutex_uninit_task_list);
    list_add_tail(self, &uninit_task_list);
    pthread_mutex_unlock(&mutex_uninit_task_list);

    return true;
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
        clientNode->sock = new_sock;
        clientNode->addr.sin_family = client.sin_family;
        clientNode->addr.sin_addr.s_addr = client.sin_addr.s_addr;
        clientNode->addr.sin_port = client.sin_port;
        clientNode->flag = -1;      //节点空闲
        clientNode->subtask_num = 0;
        //clientNode->ability = 
        list_head *task_list_head = new list_head();
        *task_list_head = LIST_HEAD_INIT(*task_list_head);
        clientNode->head = *task_list_head;
        list_head *self = new list_head();
        *self = LIST_HEAD_INIT(*self);
        clientNode->self = *self;
        list_add_tail(self, &free_client_list);
        pthread_mutex_unlock(&mutex_slave_list);
    }
}

//操作终端页面实现与人交互
void* bash_io(void *arg)
{

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
        list_add_tail(&task, &task_list);
        pthread_mutex_unlock(&mutex_task_list);
    }
}

int main(){
    int sock = startup();

    pthread_mutex_init(&mutex_slave_list, NULL);
    pthread_mutex_init(&mutex_task_list, NULL);
    pthread_mutex_init(&mutex_uninit_task_list, NULL);
    pthread_mutex_init(&mutex_task_id, NULL);

    pthread_t slave_listen_threadID, io_interface_threadID;
    pthread_create(&slave_listen_threadID, NULL, slave_accept, &sock);
    pthread_create(&io_interface_threadID, NULL, bash_io, NULL);
    
    pthread_mutex_destroy(&mutex_slave_list);
    pthread_mutex_destroy(&mutex_task_list);
    pthread_mutex_destroy(&mutex_uninit_task_list);
    pthread_mutex_destroy(&mutex_task_id);
    return 0;
}
