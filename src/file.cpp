#include "file.hpp"

extern Master master;
extern std::map<int, ClientNode *> free_client_list_map, work_client_list_map;

char SizeUnit[5] = {' ' , 'K', 'M', 'G', 'T'};

//FileInfo 实例初始化
void FileInfoInit(FileInfo *info)
{
    info->fname.clear();
    info->filesize = 0;
    info->exatsize = 0;
    info->unit = ' ';
    info->md5.clear();
}

//根据给定的文件路径获取文件的元数据
bool FileInfoGet(std::string path, FileInfo *info)
{
    std::ifstream f(path.c_str());
    if(!f.good())
    {
        printf("FileInfoGet open file error\n");
        return false;
    }
    info->fname = path;
    f.seekg(0, std::ios::end);
    info->exatsize = f.tellg();
    int u = 0;
    uint32_t size = info->exatsize;
    while(size > 10)
    {
        size /= 1000;
        u = u + 1;
    }
    info->filesize = size;
    info->unit = SizeUnit[u];
    info->md5 = FileDigest(path);
    return true;
}

//将工作从节点链表导出为json文件，返回导出的文件名
string work_client_list_export()
{
    Json::Value root, client;
    root["work_client_num"] = Json::Value((int)work_client_list_map.size());
    std::map<int, ClientNode*>::iterator it = work_client_list_map.begin();
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
    std::ofstream f;
    std::stringstream ss;
    ss << "work_client_list.json";
    string fname = ss.str();
    f.open(fname.c_str(), std::ios::trunc);
    if(!f.is_open())
    {
        printf("open file %s error\n", fname.c_str());
        return "";
    }
    f << sw.write(root);
    f.close();

    return fname;
}

//根据客户端id导出当前分配的子任务链表为json文件，返回导出的文件名
string client_task_list_export(int client_id)
{
    std::map<int, ClientNode *>::iterator it = work_client_list_map.find(client_id);
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
        std::stringstream ss;
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
    std::ofstream f;
    std::stringstream ss;
    ss << client_id << "_subtask_list.json";
    string fname = ss.str();
    f.open(fname.c_str(), std::ios::trunc);
    if(!f.is_open())
    {
        printf("open file %s error\n", fname.c_str());
        return "";
    }
    f << sw.write(root);
    f.close();

    return fname;
}

void file_send(int sock, std::string path)
{
    
}

void file_recv(int sock, int fd)
{

}
