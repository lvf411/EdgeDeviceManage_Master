#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/time.h>

#define TASK_ID "1"
#define SUBTASK_ID  "4"
#define INPUT_PATH "1_1_4.txt"
#define OUTPUT_PATH "1_4_5.txt"

using namespace std;

int main(){
    timeval tv;
    time_t tt;
    tm *t;
    ofstream log, output1;
    ifstream input1;
    string logfile_name = TASK_ID;
    logfile_name += "_";
    logfile_name += SUBTASK_ID;
    logfile_name += "_log.txt";
    log.open(logfile_name.c_str(), ios::app);
    input1.open(INPUT_PATH, ios::in);
    output1.open(OUTPUT_PATH, ios::out);

    //执行子任务2的操作
    char para1[1024] = {0};
    while(input1.get(para1, sizeof(para1)))
    {
        
        gettimeofday(&tv, NULL);
        tt = tv.tv_sec;
        t = localtime(&tt);
        t->tm_year += 1900;
        t->tm_mon += 1;
        log << "[" << t->tm_year << ":" << t->tm_mon << ":" << t->tm_mday << ":"  << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]" << " [read " << INPUT_PATH << "] "<< para1 << endl;
        cout << "read:" << para1 << endl;
    }
    sleep(1);
    cout << "4->5: 4444444444" << endl;
    output1 << "4444444444" << endl;
    gettimeofday(&tv, NULL);
    tt = tv.tv_sec;
    t = localtime(&tt);
    t->tm_year += 1900;
    t->tm_mon += 1;
    log << "[" << t->tm_year << ":" << t->tm_mon << ":" << t->tm_mday << ":"  << t->tm_hour << ":" << t->tm_min << ":" << t->tm_sec << "]" << " [send " << OUTPUT_PATH << "] " << "4->5: 4444444444" << endl;

    log.close();
    input1.close();
    output1.close();

    return 0;
}