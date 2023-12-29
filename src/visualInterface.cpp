#include "visualInterface.hpp"
#include <chrono>
#include <ctime>

extern Master master;

string scheduleResExport(int taskNum, vector<queue<int>> &scheduleRes)
{
    Json::Value root;
    root["messageID"] = Json::Value(1);

    auto currentTime = chrono::system_clock::now();
    time_t ct = chrono::system_clock::to_time_t(currentTime);
    tm* timeInfo = localtime(&ct);
    char timebuf[50] = {0};
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", timeInfo);
    root["timestamp"] = Json::Value(timebuf);

    root["machineNum"] = Json::Value((int)scheduleRes.size());
    root["taskNum"] = Json::Value(taskNum);
    Json::Value jsonSchedules, jsonMachines;
    for(int i = 0; i < scheduleRes.size(); i++)
    {
        Json::Value jsonMachine, jsonSubtasks;
        int subtaskNum = scheduleRes[i].size();
        for(int j = 0; j < subtaskNum; j++)
        {
            int temp = scheduleRes[i].front();
            jsonSubtasks.append(temp);
            scheduleRes[i].pop();
            scheduleRes[i].push(temp);
        }
        jsonMachine["machineID"] = Json::Value(i + 1);
        jsonMachine["subtasks"] = jsonSubtasks;
        jsonMachine["subtaskNum"] = subtaskNum;
        jsonSchedules.append(jsonMachine);
    }
    root["taskSchedule"] = jsonSchedules;

    Json::StyledWriter sw;
    strstream ss;
    ss << sw.write(root) << endl;

    return ss.str();
}

void visualizationTaskGenerateAndDeploy(int taskNum, double NC)
{
    TestInfo tinfo = DAGGenterate("10_10.txt", taskNum, master.work_client_num, NC);
cout << "DAG" << endl;
    auto scheduleRes = HEFT_task_schedule("10_10.txt");
cout << "heft" << endl;
    cout << scheduleResExport(taskNum, scheduleRes) << endl;
}
