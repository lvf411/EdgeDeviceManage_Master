#ifndef __SLAVE_HPP
#define __SLAVE_HPP

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
#include <map>
#include <string>
#include <vector>

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

//前驱输入
struct PrevInput{
    int source_id;                      //传递该输入的任务ID
    bool get_flag;                      //是否已经接收该输入
    std::string path;                   //输入文件存储路径
    int para_num;                       //该输入中参数的数量
    struct Parameter *head;             //参数描述链表头地址
    struct PrevInput *next;             //输入信息后继
};

//分配到的任务
struct Task{
    int root_id;                        //整个任务的ID
    int task_id;                        //当前子任务在整个任务中的ID
    int prev_num;                       //运行当前子任务需要传来输入的前驱的数量
    struct PrevInput *recved_head;      //已接收到的输入链表
    struct PrevInput *unrecv_head;      //未接收到的输入链表
    int next_num;                       //生成结果要输出的目标个数
    std::vector<int> next_id;           //生成结果要输出的目标的编号
    struct list_head head;              //任务链表的头节点
    struct list_head self;              //自身在任务链表中的节点
};

struct Master{
    int sock;                           //与服务端通信的文件描述符
    struct sockaddr_in addr;            //服务端地址信息
    int slave_id;                       //客户端编号
    int task_num;                       //待执行的任务数量
    struct list_head task;              //任务链表
    struct Task *current_task;          //当前正在执行的任务
    struct Task *next_task;             //下一个执行的任务
    std::map<int, struct sockaddr_in> work_slave_addr;          //按照客户端编号存储的正在运行的所有客户端节点地址信息
};

#endif //__SLAVE_HPP