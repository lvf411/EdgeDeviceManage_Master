#ifndef __MASTER_HPP
#define __MASTER_HPP

#include <iostream>
#include <list>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//链表节点
struct list_head{
    struct list_head *prev, *next;
    void *source;                       //指向该链表节点对应的节点实例的首地址
};

//参数描述
struct Parameter{
    int para_type;                      //该参数计量的类型
    int para_size;                      //该参数最大字节数
    struct Parameter *next;             //参数链表后继
};

//子任务需要的输出参数结果
struct  SubTaskParameter{
    void *owner;                        //生成该输出结果的子任务描述节点指针
    int owner_id;                       //生成该输出结果的子任务的ID
    int para_num;                       //生成输出结果中的参数个数
    struct Parameter *head;             //参数描述链表头地址
    struct SubTaskParameter *next;      //输出结果链表后继
};

//后继信息节点
struct SubTaskSuccessorListNode{
    int successor_id;                   //后继子任务的ID
    struct SubTaskSuccessorListNode *next;
};

//子任务描述节点
struct SubTaskNode{
    int task_id;                        //标记当前子任务在整个任务中的编号
    int root_id;                        //标记整个任务在系统中的编号
    int client_id;                      //标记被分配到的设备编号
    int prev_num;                       //标记运行当前子任务需要传来参数的前驱的数量
    struct SubTaskParameter *prev_head; //运行当前子任务需要传来参数的前驱的输出结果链表头节点
    int next_num;                       //标记当前子任务需要向后传递的后继数量
    struct SubTaskSuccessorListNode *head;  //当前子任务需要向后传递的后继信息链表头结点
    struct list_head head;              //任务链表表头
    struct list_head self;              //指向自身在链表中的指针
};

//客户端链表节点
struct ClientNode{
    int sock;                           //与客户端通信的文件描述符
    struct sockaddr_in addr;            //客户端的地址信息
    int flag;                           //表示当前该进程是否在运行，若为-1表示空闲；若大于0，表示分配给编号为flag的任务运行
    struct list_head head;              //链表头地址，若flag为-1，为空闲链表表头；若大于0，为任务链表表头
    struct list_head self;              //指向自身在链表中的指针
    struct ClientNode *source;          //指向自身的指针
};

//任务描述
struct Task{
    int task_id;                        //任务编号
    int subtask_num;                    //可分解的子任务个数
    struct list_head subtask_head;      //子任务链表表头
    struct list_head head;              //任务链表表头
    struct list_head self;              //自身在链表中的指针
};

//服务端信息结构体
struct Server{
    int sock;                           //监听的文件描述符
    struct sockaddr_in addr;            //服务端的地址信息
    int free_client_num;                //空闲设备数量
    struct list_head free_client_head;  //空闲设备链表表头
    int work_client_num;                //工作设备数量
    struct list_head work_client_head;  //工作设备链表表头
    int task_num;                       //任务数量
    struct list_head task_list_head;    //任务链表表头
    struct list_head uninit_task_list_head;     //未分配的任务的链表表头
};

#endif //__MASTER_HPP