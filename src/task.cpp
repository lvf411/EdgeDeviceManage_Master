#include "task.hpp"

extern Master master;
extern list_head uninit_task_list;
extern int task_increment_id;
extern pthread_mutex_t mutex_uninit_task_list, mutex_task_id;

//根据任务描述文件添加任务
bool task_add(std::string path){
    std::ifstream ifs(path);
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
                n->client_id = 0;
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
                n->client_id = 0;
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

    //任务数量加一
    master.uninit_task_num++;
    
    return true;
}
