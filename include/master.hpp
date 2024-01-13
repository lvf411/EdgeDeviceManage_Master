#ifndef __MASTER_HPP
#define __MASTER_HPP

#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <mutex>
#include "list.hpp"
#include "file.hpp"
#include "semaphore.hpp"

#define WORK_CLIENT_LIST_UPDATE_GAP_TIME 3

#define SLAVE_ABILITY_DEFAULT 10

//子任务间结果传递
struct SubTaskResult{
    int client_id;                      //对应前驱和后继被分配的从节点ID
    int subtask_id;                     //子任务的ID
    std::string fname;                  //传递的结果的文件名
    struct SubTaskResult *next;
};

//子任务描述节点
struct SubTaskNode{
    int subtask_id;                     //标记当前子任务在整个任务中的编号
    int root_id;                        //标记整个任务在系统中的编号
    int client_id;                      //标记被分配到的设备编号
    int prev_num;                       //标记运行当前子任务需要传来参数的前驱的数量
    struct SubTaskResult *prev_head;    //运行当前子任务需要传来参数的前驱的输出结果链表头节点
    int next_num;                       //标记当前子任务需要向后传递的后继数量
    struct SubTaskResult *succ_head;    //当前子任务需要向后传递的后继信息链表头结点
    struct list_head *taskhead;         //整个任务的子任务链表表头
    struct list_head taskself;          //指向自身在整个任务的子任务链表中的指针
    struct list_head *clienthead;       //子任务分配执行的客户端的子任务链表表头
    struct list_head clientself;        //指向自身在分配执行的客户端的子任务链表中的指针
    std::string exepath;                //子任务执行文件路径
    bool done;                          //执行完成标记，true为执行完成
};

//客户端链表节点
struct ClientNode{
    int client_id;                      //设备编号
    int sock;                           //与客户端通信的文件描述符
    struct sockaddr_in addr;            //客户端的地址信息
    int listen_port;                    //客户端监听的端口地址
    int flag;                           //表示当前该进程是否在运行，若为-1表示空闲；若大于0，表示分配给编号为flag的任务运行
    int ability;                        //执行任务的效率，能力，越大越强
    int subtask_num;                    //分配到的子任务数量
    struct list_head *head;             //子任务链表头地址，若flag为-1，为空闲节点；若大于0，为分配给该从节点的子任务链表表头
    struct list_head self;              //指向自身在客户端链表中的指针
    std::thread msg_send_threadID;      //消息发送线程ID
    std::thread msg_recv_threadID;      //消息接收线程ID
    bool modified;                      //修改标记，当值为0时表示没有受到修改，没有分配新的子任务；当被置为1时，表示被分配了新的子任务，需要同步任务链表
    bool work_client_cahange_flag;      //指示工作从节点链表是否有变动，是否需要重新同步从节点的工作从节点链表信息
    int status;                         //分配的发送/接收线程状态，用以指示状态机运行以及部分同步问题
    std::mutex mutex_status;            //保障对发送/接收线程状态参数的互斥访问
    std::string file_trans_fname;       //文件传输时正在传输的文件的文件名
    FileTransInfo *transinfo;           //文件传输时正在传输的文件的信息
    int file_trans_sock;                //文件传输时与从节点建立的新连接
    int file_trans_port;                //文件传输时从节点提供的端口号
    Semaphore sem;                      //实现消息发送/接收线程同步的信号量
};

//任务描述
struct Task{
    int id;                             //系统内的任务编号
    std::string task_id;                //任务自身编号
    int subtask_num;                    //可分解的子任务个数
    struct list_head *subtask_head;     //子任务链表表头
    struct list_head self;              //自身在任务链表中的指针
    int downloadSubtaskNum;             //已下载完成子任务个数
    std::mutex mutexDownloadSubtaskNum; //对已下载子任务个数进行保护的锁
    int doneSubtaskNum;                 //执行完成的子任务个数
    std::mutex mutexDoneSubtaskNum;     //对执行完成子任务个数进行保护的锁
};

//服务端信息结构体
struct Master{
    int sock;                           //监听的文件描述符
    struct sockaddr_in addr;            //服务端的地址信息
    int free_client_num;                //空闲设备数量
    struct list_head *free_client_head; //空闲设备链表表头
    int work_client_num;                //工作设备数量
    struct list_head *work_client_head; //工作设备链表表头
    int task_num;                       //已分配的任务数量
    struct list_head *task_list_head;   //已分配的任务链表表头
    int uninit_task_num;                //未分配的任务数量
    struct list_head *uninit_task_list_head;    //未分配的任务的链表表头
    int doneTaskNum;                    //已完成的任务数量
    struct list_head *doneTaskListHead; //已完成的任务链表表头
};

#endif //__MASTER_HPP