#include "interface.hpp"

/*
 *  状态机转换实现管理员用户IO界面
 */

//IO管理界面状态，识别当前处于什么界面
enum IOStatus{
    MNEU,
    TaskAddPathGet,
    TaskAddResult,
    SlaveNodeStatusBrief,
    SlaveNodeStatus,
    TaskDeployed,
    TaskUndeployed
};

//特定键入关键字
enum KEY{
    QUIT,
    BACK,
    EXIT,
    REFRESH,
    SWITCH
};

//标记当前界面所处状态
int current_status = MNEU;

extern Master master;

//清屏
void screen_clear()
{
    system("clear");
}

//关键字识别
int key_word_recognize(const char *s)
{
    int ret = -1;
    if(strlen(s) == 1)
    {
        switch(s[0])
        {
            case 'q':
            case 'Q':
                {
                    ret = QUIT;
                    break;
                }
            case 'r':
            case 'R':
                {
                    ret = REFRESH;
                    break;
                }
            case 's':
            case 'S':
                {
                    ret = SWITCH;
                    break;
                }
            case 'b':
            case 'B':
                {
                    ret = BACK;
                    break;
                }
            case 'x':
            case 'X':
            case 'e':
            case 'E':
                {
                    ret = EXIT;
                    break;
                }
            default:
                {
                    ret = -1;
                    break;
                }
        }
    }
    return ret;
}

//检查文件是否存在
bool file_check(const char *path)
{
    return (access(path, F_OK) != -1);
}

//打印总体菜单
void menu_print(){
    printf("==========主节点管理系统==========\n");
    printf("\t[1] 添加任务\n");
    printf("\t[2] 从节点状态\n");
    printf("\t[3] 任务分配状态\n");
    printf("请输入想要执行的操作：");
}

//从节点状态简略显示
void SlaveNodeStatusBrief_print(){
    printf("当前系统内工作从节点数量为：%d,空闲从节点数量为：%d\n", master.work_client_num, master.free_client_num);
    printf("工作从节点：\n");
    list_head head = master.work_client_head, *temp = master.work_client_head.next;
    int i = 0;
    while(temp != &head && i < master.work_client_num)
    {
        i++;
        ClientNode *node = (ClientNode *)(list_entry(temp, ClientNode, self));
        printf("[%d] ID:%d %s:%d\n", i, node->client_id, inet_ntoa(node->addr.sin_addr) \
                        , node->addr.sin_port);
        temp = temp->next;
    }
    if(i == master.work_client_num)
    {
        printf(" work client number error!\n");
        return;
    }

    head = master.free_client_head, temp = master.free_client_head.next;
    i = 0;
    while(temp != &head && i < master.free_client_num)
    {
        i++;
        ClientNode *node = (ClientNode *)(list_entry(temp, ClientNode, self));
        printf("[%d] ID:%d %s:%d\n", i, node->client_id, inet_ntoa(node->addr.sin_addr) \
                        , node->addr.sin_port);
        temp = temp->next;
    }
    if(i == master.free_client_num)
    {
        printf(" free client number error!\n");
        return;
    }
}

//从节点状态详细显示
void SlaveNodeStatus_print(){
    printf("当前系统内工作从节点数量为：%d,空闲从节点数量为：%d\n", master.work_client_num, master.free_client_num);
    printf("工作从节点：\n");
    list_head head = master.work_client_head, *temp = master.work_client_head.next;
    int i = 0;
    while(temp != &head && i < master.work_client_num)
    {
        i++;
        ClientNode *node = (ClientNode *)(list_entry(temp, ClientNode, self));
        printf("\t[%d] ID:%d %s:%d 算力:%d 已分配任务%d个\n", i, node->client_id, inet_ntoa(node->addr.sin_addr) \
                        , node->addr.sin_port, node->ability, node->subtask_num);
        if(node->flag == -1)
        {
            printf("当前从节点接下来没有被分配任务\n");
        }
        else
        {
            int j = 0;
            list_head task_head = node->head, *task_temp = node->head.next;
            printf("\t[编号] (任务编号):(子任务编号)\n");
            while(j < node->subtask_num && task_temp != NULL)
            {
                j++;
                SubTaskNode *subtask_node = (SubTaskNode *)(list_entry(task_temp, SubTaskNode, self));
                printf("\t\t[%d] %d:%d\n", j, subtask_node->root_id, subtask_node->subtask_id);
                task_temp = task_temp->next;
            }
        }
        temp = temp->next;
    }
    if(i == master.work_client_num)
    {
        printf(" work client number error!\n");
        return;
    }

    head = master.free_client_head, temp = master.free_client_head.next;
    i = 0;
    while(temp != &head && i < master.free_client_num)
    {
        i++;
        ClientNode *node = (ClientNode *)(list_entry(temp, ClientNode, self));
        printf("[%d] ID:%d %s:%d\n", i, node->client_id, inet_ntoa(node->addr.sin_addr) \
                        , node->addr.sin_port);
        temp = temp->next;
    }
    if(i == master.free_client_num)
    {
        printf(" free client number error!\n");
        return;
    }
}

//操作终端页面实现与人交互
void* bash_output(void *arg)
{


    return NULL;
}

void* bash_input(void *arg){
    int buf_size = 1024;
    char buf[buf_size];
    while(1){
        switch(current_status)
        {
            case MNEU:
                {
                    scanf("%s", buf);
                    if(strlen(buf) > 1)
                    {
                        screen_clear();
                        printf("请输入正确的操作码！\n");
                        menu_print();
                    }
                    else{
                        int op = atoi(buf);
                        switch(op)
                        {
                            case 1:
                                {
                                    screen_clear();
                                    printf("请输入待添加任务的描述文件(*.json):");
                                    current_status = TaskAddPathGet;
                                    break;
                                }
                            case 2:
                                {
                                    screen_clear();
                                    current_status = SlaveNodeStatusBrief;
                                    break;
                                }
                            case 3:
                                {
                                    screen_clear();
                                    current_status = SlaveNodeStatusBrief;
                                }
                            default:
                                {
                                    screen_clear();
                                    printf("请输入正确的操作码！\n");
                                    menu_print();
                                    break;
                                }
                        }
                    }
                    break;
                }
            case TaskAddPathGet:
                {
                    memset(buf, 0, buf_size);
                    scanf("%s", buf);
                    int ret = key_word_recognize(buf);
                    if(ret == BACK || ret == QUIT)
                    {
                        current_status = MNEU;
                        screen_clear();
                        menu_print();
                        break;
                    }
                    
                    //检查文件是否存在
                    if(file_check(buf))
                    {
                        //根据json文件将任务假如到待分配任务链表
                        if(task_add(buf)){
                            current_status = TaskAddResult;
                            screen_clear();
                            printf("任务添加成功\n");
                            printf("按[q/Q]返回目录\n");
                            break;
                        }
                    }
                    printf("选择任务描述文件有误,请重新输入,或按[b/B]返回目录\n");
                    printf("任务描述文件路径：");
                    break;
                }
            case TaskAddResult:
                {
                    memset(buf, 0, buf_size);
                    scanf("%s", buf);
                    int ret = key_word_recognize(buf);
                    if(ret == QUIT)
                    {
                        current_status = MNEU;
                        screen_clear();
                        menu_print();
                        break;
                    }
                    break;
                }
            case SlaveNodeStatusBrief:
                {
                    screen_clear();
                    SlaveNodeStatusBrief_print();
                    printf("按[s/S]切换为详细显示，按[b/B]返回目录\n");
                    memset(buf, 0, buf_size);
                    scanf("%s", buf);
                    int ret = key_word_recognize(buf);
                    switch (ret)
                    {
                    case SWITCH:
                        {
                            current_status = SlaveNodeStatus;
                            break;
                        }
                    case BACK:
                        {
                            current_status = MNEU;
                            screen_clear();
                            menu_print();
                            break;
                        }
                    default:
                        {
                            printf("请输入正确的操作码!\n");
                            break;
                        }
                    }
                    break;
                }
            case SlaveNodeStatus:
                {
                    screen_clear();
                    SlaveNodeStatus_print();
                    printf("按[s/S]切换为简略显示，按[b/B]返回目录\n");
                    memset(buf, 0, buf_size);
                    scanf("%s", buf);
                    int ret = key_word_recognize(buf);
                    switch (ret)
                    {
                    case SWITCH:
                        {
                            current_status = SlaveNodeStatusBrief;
                            break;
                        }
                    case BACK:
                        {
                            current_status = MNEU;
                            screen_clear();
                            menu_print();
                            break;
                        }
                    default:
                        {
                            printf("请输入正确的操作码!\n");
                            break;
                        }
                    }
                    break;
                }
            case TaskDeployed:
                {
                    screen_clear();
                    break;
                }
            case TaskUndeployed:
                {
                    break;
                }
            default:
                {
                    break;
                }
        }

    }
    return NULL;
}