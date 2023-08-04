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

//打印总体菜单
void menu_print(){
    printf("==========主节点管理系统==========\n");
    printf("\t[1] 添加任务\n");
    printf("\t[2] 从节点状态\n");
    printf("\t[3] 任务分配状态\n");
    printf("请输入想要执行的操作：");
}

//操作终端页面实现与人交互
void* bash_output(void *arg)
{


    return;
}

void* bash_input(void *arg){
    char buf[1024];
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
                    memset(buf, 0, 1024);
                    scanf("%s", buf);
                    if(key_word_recognize(buf) == BACK)
                    {
                        current_status = MNEU;
                        screen_clear();
                        menu_print();
                        break;
                    }
                    
                    break;
                }
            case TaskAddResult:
                {
                    break;
                }
            case SlaveNodeStatusBrief:
                {
                    break;
                }
            case SlaveNodeStatus:
                {
                    break;
                }
            case TaskDeployed:
                {
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
    return;
}