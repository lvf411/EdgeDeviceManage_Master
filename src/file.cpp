#include "file.hpp"
#include "master.hpp"

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
std::string work_client_list_export()
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
        json_node["listen_port"] = Json::Value(node->listen_port);
        client.append(json_node);
        it++;
    }
    root["work_client"] = client;

    Json::StyledWriter sw;
    std::ofstream f;
    std::stringstream ss;
    ss << WORK_CLIENT_LIST_FNAME;
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

std::string GetFnameFromPath(std::string path)
{
    size_t last_index = path.find_last_of("\\/");
    if(last_index != std::string::npos)
    {
        return path.substr(last_index + 1);
    }
    else
    {
        return path;
    }
}

//根据客户端id导出当前分配的子任务链表为json文件，返回导出的文件名
std::string client_task_list_export(int client_id)
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
    root["port"] = Json::Value(ntohs(client->addr.sin_port));
    root["master_ip"] = Json::Value(inet_ntoa(master.addr.sin_addr));
    root["master_port"] = Json::Value(ntohs(master.addr.sin_port));
    root["subtask_num"] = Json::Value(client->subtask_num);
    Json::Value json_subtask;
    int i = 0;
    list_head *subtask_head = client->head, *subtask_temp = client->head->next;
    while(i < client->subtask_num && subtask_temp != subtask_head)
    {
        i++;
        SubTaskNode *node = (SubTaskNode *)(list_entry(subtask_temp, SubTaskNode, clientself));
        Json::Value json_temp;
        json_temp["root_id"] = Json::Value(node->root_id);
        json_temp["subtask_id"] = Json::Value(node->subtask_id);
        json_temp["exe_name"] = Json::Value(GetFnameFromPath(node->exepath));         //从路径中取出最终的文件名
        json_temp["input_num"] = Json::Value(node->prev_num);
        Json::Value prev, next;
        SubTaskResult *res_temp = node->prev_head->next;
        int j = 0;
        while(res_temp != NULL && j < node->prev_num)
        {
            Json::Value temp;
            temp["subtask_id"] = Json::Value(res_temp->subtask_id);
            temp["client_id"] = Json::Value(res_temp->client_id);
            temp["fname"] = Json::Value(res_temp->fname);
            prev.append(temp);
            j++;
            res_temp = res_temp->next;
        }
        json_temp["input"] = prev;
        json_temp["output_num"] = node->next_num;
        res_temp = node->succ_head->next;
        j = 0;
        while(res_temp != NULL && j < node->next_num)
        {
            Json::Value temp;
            temp["subtask_id"] = Json::Value(res_temp->subtask_id);
            temp["client_id"] = Json::Value(res_temp->client_id);
            temp["fname"] = Json::Value(res_temp->fname);
            next.append(temp);
            j++;
            res_temp = res_temp->next;
        }
        json_temp["output"] = next;
        json_subtask.append(json_temp);
        subtask_temp = subtask_temp->next;
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
    FileInfo info;
    FileInfoInit(&info);
    FileInfoGet(path, &info);
    int packid = 0, packnum = ((info.exatsize / 3) * 4) / FILE_PACKAGE_SIZE + 1;
    ifstream ifs(path, std::ios::binary);
    char file_readbuf[FILEBUF_MAX_LENGTH + 1];
    char sendbuf[FILE_PACKAGE_SIZE + 1];
    char recvbuf[1024];
    while(packid < packnum)
    {
        uint32_t sendlength;
        int ack = 0;
        packid++;
        memset(file_readbuf, 0, FILEBUF_MAX_LENGTH);
        memset(sendbuf, 0, FILE_PACKAGE_SIZE);
        ifs.read(file_readbuf, FILEBUF_MAX_LENGTH);
        Base64_Encode(file_readbuf, ifs.gcount(), sendbuf, &sendlength);
        std::cout << "file_read: " << file_readbuf << "sendlength:" << sendlength << std::endl;
        do{
            send(sock, sendbuf, strlen(sendbuf), 0);
            std::cout << "filesend: " << sendbuf << std::endl;
            memset(recvbuf, 0, 1024);
            recv(sock, recvbuf, 1024, 0);
            std::cout << "filerecv: " << recvbuf << std::endl;
            Json::Value root;
            Json::Reader rd;
            rd.parse(recvbuf, root);
            ack = root["ret"].asInt();
        }while(ack < packid);
    }
    ifs.close();
}

//接收了对方的文件传输套接字的连接，还没发送确认信息,res_md5传引用
void file_recv(int sock, FileInfo *info, std::ofstream& ofs, std::string& res_md5)
{
    // std::ofstream ofs(info->fname, std::ios::binary | std::ios::app);
    char recvbuf[FILE_PACKAGE_SIZE + 1];
    char file_writebuf[FILEBUF_MAX_LENGTH + 1];
    char sendbuf[1024];
    long long int  whole_length = 0;
    uint32_t file_length = 0, recv_length = 0;
    int packid = 0, packnum = ((info->exatsize / 3) * 4) / FILE_PACKAGE_SIZE + 1;
    while(packid < packnum)
    {
        recv_length = recv(sock, recvbuf, FILE_PACKAGE_SIZE, 0);
        std::cout << "filerecv: " << recvbuf << std::endl;
        Base64_Decode(recvbuf, recv_length, file_writebuf, &file_length);
        std::cout << "file_write: " << file_writebuf << std::endl;
        ofs.write(file_writebuf, file_length);
        whole_length += file_length;
        packid++;
        Json::Value root;
        root["ret"] = Json::Value(packid);
        Json::FastWriter fw;
        std::stringstream ss;
        ss << fw.write(root);
        send(sock, ss.str().c_str(), ss.str().length(), 0);
    }
    ofs.close();
    std::ifstream ifs(info->fname, std::ios::binary);
    MD5 md5;
    std::streamsize length;
    char buf[1024];
    while(!(ifs.eof()))
    {
        ifs.read(buf, 1024);
        length = ifs.gcount();
        if(length > 0)
        {
            md5.update(buf, length);
        }
    }
    ifs.close();
    res_md5 = md5.toString();
}
