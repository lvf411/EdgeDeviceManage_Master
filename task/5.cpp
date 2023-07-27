#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/time.h>

#define TASK_ID "1"
#define SUBTASK_ID  "5"
#define INPUT_PATH_1 "1_2_5.txt"
#define INPUT_PATH_2 "1_3_5.txt"
#define INPUT_PATH_3 "1_4_5.txt"

using namespace std;

int main(){
    timeval tv;
    time_t tt;
    tm *t;
    ofstream log;
    ifstream input1, input2, input3;
    string subtaskid = TASK_ID;
    subtaskid += "_";
    subtaskid += SUBTASK_ID;

    log.open(subtaskid + "_log.txt", ios::app);
    input1.open(INPUT_PATH_1, ios::in);
    input2.open(INPUT_PATH_2, ios::in);
    input3.open(INPUT_PATH_3, ios::in);

    //执行子任务2的操作
    char para1[1024] = {0};
    while(input1.get(para1, sizeof(para1)))
    {
        
        gettimeofday(&tv, NULL);
        tt = tv.tv_sec;
        t = localtime(&tt);
        t->tm_year += 1900;
        t->tm_mon += 1;
        log << "[" << t->tm_year << ":" << t->tm_mon << ":" << t->tm_mday << ":"  << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]" << " [read " << INPUT_PATH_1 << "] "<< para1 << endl;
        cout << "read:" << para1 << endl;
    }
    while(input2.get(para1, sizeof(para1)))
    {
        
        gettimeofday(&tv, NULL);
        tt = tv.tv_sec;
        t = localtime(&tt);
        t->tm_year += 1900;
        t->tm_mon += 1;
        log << "[" << t->tm_year << ":" << t->tm_mon << ":" << t->tm_mday << ":"  << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]" << " [read " << INPUT_PATH_2 << "] "<< para1 << endl;
        cout << "read:" << para1 << endl;
    }
    while(input3.get(para1, sizeof(para1)))
    {
        
        gettimeofday(&tv, NULL);
        tt = tv.tv_sec;
        t = localtime(&tt);
        t->tm_year += 1900;
        t->tm_mon += 1;
        log << "[" << t->tm_year << ":" << t->tm_mon << ":" << t->tm_mday << ":"  << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]" << " [read " << INPUT_PATH_3 << "] "<< para1 << endl;
        cout << "read:" << para1 << endl;
    }

    log.close();
    input1.close();
    input2.close();
    input3.close();

    return 0;
}