#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/time.h>

#define TASK_ID "1"
#define SUBTASK_ID  "1"
#define OUTPUT_PATH_1 "1_1_2.txt"
#define OUTPUT_PATH_2 "1_1_3.txt"
#define OUTPUT_PATH_3 "1_1_4.txt"

using namespace std;

int main(){
    timeval tv;
    time_t tt;
    tm *t;
    ofstream log, output1, output2, output3;
    string taskid = TASK_ID;
    taskid += "_";
    taskid += SUBTASK_ID;

    log.open(taskid + "_log.txt", ios::app);
    output1.open(OUTPUT_PATH_1, ios::out);
    output2.open(OUTPUT_PATH_2, ios::out);
    output3.open(OUTPUT_PATH_3, ios::out);

    //执行子任务1的操作
    sleep(1);
    cout << "1->2: 1111111111" << endl;
    output1 << "1111111111" << endl;
    gettimeofday(&tv, NULL);
    tt = tv.tv_sec;
    t = localtime(&tt);
    t->tm_year += 1900;
    t->tm_mon += 1;
    log << "[" << t->tm_year << ":" << t->tm_mon << ":" << t->tm_mday << ":"  << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]" << " [send " << OUTPUT_PATH_1 << "] " << "1->2: 1111111111" << endl;
    sleep(1);
    cout << "1->3: 2222222222" << endl;
    output2 << "2222222222" << endl;
    gettimeofday(&tv, NULL);
    tt = tv.tv_sec;
    t = localtime(&tt);
    t->tm_year += 1900;
    t->tm_mon += 1;
    log << "[" << t->tm_year << ":" << t->tm_mon << ":" << t->tm_mday << ":"  << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]" << " [send " << OUTPUT_PATH_2 << "] " << "1->3: 2222222222" << endl;
    sleep(1);
    cout << "1->4: 3333333333" << endl;
    output3 << "3333333333" << endl;
    gettimeofday(&tv, NULL);
    tt = tv.tv_sec;
    t = localtime(&tt);
    t->tm_year += 1900;
    t->tm_mon += 1;
    log << "[" << t->tm_year << ":" << t->tm_mon << ":" << t->tm_mday << ":"  << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]" << " [send " << OUTPUT_PATH_3 << "] " << "1->4: 3333333333" << endl;

    log.close();
    output1.close();
    output2.close();
    output3.close();

    return 0;
}