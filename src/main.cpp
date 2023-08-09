#include "master.hpp"
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <jsoncpp/json/json.h>
#include <pthread.h>
#include "interface.hpp"

#define InitFile "master_init_sample.json"
#define MAX_LISTENING 1000

using namespace std;

Master master;
list_head free_client_list, work_client_list, deployed_task_list, uninit_task_list;
pthread_mutex_t mutex_slave_list, mutex_task_list, mutex_uninit_task_list, mutex_task_id;
map<int, ClientNode *> free_client_list_map, work_client_list_map;
int task_increment_id = 0;
int increment_slave_id = 1;

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

//从节点消息发送线程
void *msg_send(void *arg)
{

}

//从节点消息接收线程
void *msg_recv(void *arg)
{

}

//从节点连接主节点线程，接收从节点连接并将其加入到从节点管理链表，为每个从节点连接分配单独的消息收发线程
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
        free_client_list_map.insert(map<int, ClientNode *>::value_type(clientNode->client_id, clientNode));
        pthread_mutex_unlock(&mutex_slave_list);
        
        pthread_t msg_send_threadID, msg_recv_threadID;
        pthread_create(&msg_send_threadID, NULL, msg_send, (void *)clientNode);
        pthread_create(&msg_recv_threadID, NULL,msg_recv, (void*)clientNode);
        clientNode->msg_send_threadID = msg_send_threadID;
        clientNode->msg_recv_threadID = msg_recv_threadID;
    }
    return NULL;
}

//分配任务给各个节点
void* task_deploy(void *arg)
{
    while(!list_empty(&uninit_task_list)){
        //0、保证至少有两个设备可供分配
        while(master.work_client_num < 2 && master.free_client_num > 0){
            pthread_mutex_lock(&mutex_slave_list);
            list_head *temp = free_client_list.next;
            ClientNode *slave = (ClientNode *)(list_entry(temp, ClientNode, self));
            list_del(free_client_list.next);
            free_client_list_map.erase(slave->client_id);
            master.free_client_num--;
            list_add_tail(temp, &(master.work_client_head));
            work_client_list_map.insert(map<int, ClientNode*>::value_type(slave->client_id, slave));
            master.work_client_num++;
            pthread_mutex_unlock(&mutex_slave_list);
        }
        //没有大于2个从节点设备可供调配，休眠一段时间等待有无新从节点加入
        if(master.work_client_num < 2)
        {
            printf("从节点数量不足\n");
            sleep(10);
            continue;
        }
        
        //1、从未分配任务链表中取出任务
        pthread_mutex_lock(&mutex_uninit_task_list);
        list_head task_node = *uninit_task_list.next;
        list_del(uninit_task_list.next);
        pthread_mutex_unlock(&mutex_uninit_task_list);

        //2、分配子任务执行的从节点，并更新对应从节点结构体中的信息
        //此处将子任务交替依次分配给工作从节点链表头两个节点
        int i = 0;
        Task *task = (Task *)(list_entry(&task_node, Task, self));
        list_head subt_head = task->subtask_head, *subt_temp = task->subtask_head.next;
        ClientNode *slave[2];
        slave[0] = (ClientNode *)(list_entry(master.work_client_head.next, ClientNode, self));
        slave[1] = (ClientNode *)(list_entry(master.work_client_head.next->next, ClientNode, self));
        int pick = 0;   //指定当前子任务分配给slave[0]还是slave[1]
        vector<int> task_workclient_a;      //记录每个子任务按顺序被分配的执行从节点
        while(i < task->subtask_num)
        {
            i++;
            SubTaskNode *subt = (SubTaskNode *)(list_entry(&subt_temp, SubTaskNode, self));
            subt->client_id = slave[pick]->client_id;
            slave[pick]->subtask_num++;
            if(slave[pick]->flag == -1)
            {
                slave[pick]->flag = 0;
            }
            list_add_tail(subt_temp, &(slave[pick]->head));
            task_workclient_a.push_back(pick);
            pick = 1 - pick;
            subt_temp = subt_temp->next;
        }

        //3、更新子任务节点中的前驱后继内容
        subt_temp = subt_head.next;
        while(subt_temp != &subt_head)
        {
            int j = 0;
            SubTaskNode *subt = (SubTaskNode *)(list_entry(&subt_temp, SubTaskNode, self));
            SubTaskResult *res_temp = subt->prev_head->next;
            while(j < subt->prev_num)
            {
                j++;
                res_temp->client_id = slave[task_workclient_a[res_temp->subtask_id]]->client_id;
                res_temp = res_temp->next;
            }
            j = 0;
            SubTaskResult *res_temp = subt->succ_head->next;
            while(j < subt->next_num)
            {
                j++;
                res_temp->client_id = slave[task_workclient_a[res_temp->subtask_id]]->client_id;
                res_temp = res_temp->next;
            }
        }

        //4、将任务插入任务链表
        pthread_mutex_lock(&mutex_task_list);
        list_add_tail(&task_node, &deployed_task_list);
        pthread_mutex_unlock(&mutex_task_list);
    }
    return NULL;
}

//将工作从节点链表导出为json文件
string work_client_list_export()
{
    Json::Value root, client;
    root["work_client_num"] = Json::Value((int)work_client_list_map.size());
    map<int, ClientNode*>::iterator it = work_client_list_map.begin();
    while(it != work_client_list_map.end())
    {
        ClientNode *node = it->second;
        Json::Value json_node;
        json_node["client_id"] = Json::Value(node->client_id);
        json_node["ip"] = Json::Value(inet_ntoa(node->addr.sin_addr));
        json_node["listen_port"] = Json::Value(node->addr.sin_port);
        client.append(json_node);
        it++;
    }
    root["work_client"] = client;

    Json::StyledWriter sw;
    ofstream f;
    stringstream ss;
    ss << "work_client_list.json";
    string fname = ss.str();
    f.open(fname.c_str(), ios::trunc);
    if(!f.is_open())
    {
        printf("open file %s error\n", fname.c_str());
        return NULL;
    }
    f << sw.write(root);
    f.close();
}

//根据客户端id导出当前分配的子任务链表为json文件
string client_task_list_export(int client_id)
{
    map<int, ClientNode *>::iterator it = work_client_list_map.find(client_id);
    if(it == work_client_list_map.end()){
        printf("目标从节点查找失败\n");
        return NULL;
    }
    ClientNode *client = it->second;
    Json::Value root;
    root["client_id"] = Json::Value(client_id);
    root["ip"] = Json::Value(inet_ntoa(client->addr.sin_addr));
    root["port"] = Json::Value(client->addr.sin_port);
    root["master_ip"] = Json::Value(inet_ntoa(master.addr.sin_addr));
    root["master_port"] = Json::Value(master.addr.sin_port);
    root["subtask_num"] = Json::Value(client->subtask_num);
    Json::Value json_subtask;
    int i = 0;
    list_head subtask_head = client->head, *subtask_temp = client->head.next;
    while(i < client->subtask_num && subtask_temp != &subtask_head)
    {
        i++;
        SubTaskNode *node = (SubTaskNode *)(list_entry(subtask_temp, SubTaskNode, self));
        Json::Value json_temp;
        json_temp["root_id"] = Json::Value(node->root_id);
        json_temp["subtask_id"] = Json::Value(node->subtask_id);
        stringstream ss;
        ss << node->root_id << '_' << node->subtask_id;
        string fname = ss.str();
        json_temp["exe_name"] = Json::Value(fname);
        json_temp["input_src_num"] = Json::Value(node->prev_num);
        Json::Value prev, next;
        SubTaskResult *res_temp = node->prev_head->next;
        int j = 0;
        while(res_temp != NULL && j < node->prev_num)
        {
            Json::Value temp;
            temp["subtask_id"] = Json::Value(res_temp->subtask_id);
            temp["client_id"] = Json::Value(res_temp->client_id);
            prev.append(temp);
        }
        json_temp["input_src"] = prev;
        json_temp["output_dst_num"] = node->next_num;
        res_temp = node->succ_head->next;
        j = 0;
        while(res_temp != NULL && j < node->next_num)
        {
            Json::Value temp;
            temp["subtask_id"] = Json::Value(res_temp->subtask_id);
            temp["client_id"] = Json::Value(res_temp->client_id);
            next.append(temp);
        }
        json_temp["output_dst"] = next;
        json_subtask.append(json_temp);
    }
    root["subtask"] = json_subtask;
    
    Json::StyledWriter sw;
    ofstream f;
    stringstream ss;
    ss << client_id << "_subtask_list.json";
    string fname = ss.str();
    f.open(fname.c_str(), ios::trunc);
    if(!f.is_open())
    {
        printf("open file %s error\n", fname.c_str());
        return NULL;
    }
    f << sw.write(root);
    f.close();
}

int main(){
    int sock = startup();

    pthread_mutex_init(&mutex_slave_list, NULL);
    pthread_mutex_init(&mutex_task_list, NULL);
    pthread_mutex_init(&mutex_uninit_task_list, NULL);
    pthread_mutex_init(&mutex_task_id, NULL);

    pthread_t slave_listen_threadID, bash_input_threadID, bash_output_threadID, task_deploy_threadID,\
        master2slave_instruction_threadID, master2slave_file_threadID, slave2master_response_threadID, slave2master_file_threadID;
    pthread_create(&slave_listen_threadID, NULL, slave_accept, &sock);
    pthread_create(&bash_output_threadID, NULL, bash_io, NULL);
    pthread_create(&task_deploy_threadID, NULL, task_deploy, NULL);
    
    pthread_mutex_destroy(&mutex_slave_list);
    pthread_mutex_destroy(&mutex_task_list);
    pthread_mutex_destroy(&mutex_uninit_task_list);
    pthread_mutex_destroy(&mutex_task_id);
    return 0;
}
