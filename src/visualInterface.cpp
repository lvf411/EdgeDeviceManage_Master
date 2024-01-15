#include "visualInterface.hpp"
#include <chrono>
#include <ctime>

extern Master master;
extern map<int, ClientNode *> work_client_list_map;
extern map<int, Task *> deployedTaskListMap; 

string scheduleResExport(int taskNum, vector<queue<int>> &scheduleRes, int taskID)
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
    root["result"] = true;
    root["taskID"] = taskID;
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

std::string visualizationTaskGenerateAndDeploy(int taskNum, double NC)
{
    TestInfo tinfo = DAGGenterate("10_10.txt", taskNum, master.work_client_num, NC);

    auto scheduleRes = HEFT_task_schedule("10_10.txt");

    int taskID = test_task_info_import(tinfo, scheduleRes);

    return scheduleResExport(taskNum, scheduleRes, taskID);
}

int testserver()
{
    httplib::Server server;

    server.Options("/task_gen", [&](const httplib::Request& req, httplib::Response& res) {
        // 设置 CORS 头
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        // 返回成功状态码
        res.status = 200;
    });

    //下发任务数量和边密度，随机生成对应数量任务和边密度的有向无环图，调度后将结果返回
    server.Post("/task_gen", [&](const httplib::Request& req, httplib::Response& res) {
        // 解析接收到的JSON数据
        Json::CharReaderBuilder readerBuilder;
        Json::Value data;
        std::istringstream is(req.body);
        std::string errs;
        Json::parseFromStream(readerBuilder, is, &data, &errs);

        std::cout << "get task_gen req:" << is.str() << std::endl;
        
        // 从JSON中获取输入数据
        int subtaskNum = data["subtaskNum"].asInt();
        double dagEdgeDensity = data["dagEdgeDensity"].asDouble();

        std::cout << "subtaskNum:" << subtaskNum << std::endl << "nc: " << dagEdgeDensity << std::endl;
        
        std::string result;
        if(master.work_client_num == 0)
        {
            Json::Value root;
            root["result"] = false;
            root["messageID"] = Json::Value(1);

            auto currentTime = chrono::system_clock::now();
            time_t ct = chrono::system_clock::to_time_t(currentTime);
            tm* timeInfo = localtime(&ct);
            char timebuf[50] = {0};
            strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", timeInfo);
            root["timestamp"] = Json::Value(timebuf);

            root["machineNum"] = Json::Value(master.work_client_num);
            root["taskNum"] = Json::Value(subtaskNum);

            Json::StyledWriter sw;
            strstream ss;
            ss << sw.write(root) << endl;
        }
        else
        {
            result = visualizationTaskGenerateAndDeploy(subtaskNum, dagEdgeDensity);
        }

        // 设置 CORS 头，允许所有来源，也可以指定具体的域
        res.set_header("Access-Control-Allow-Origin", "*");
        // 其他响应头设置，根据需要添加
        res.set_header("Content-Type", "application/json");
        // 返回响应
        res.status = 200;
        
        res.set_content(result, "application/json");

        std::cout << "ack: " << result << std::endl;
    });

    server.Options("/run_subtasks", [&](const httplib::Request& req, httplib::Response& res) {
        // 设置 CORS 头
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        // 返回成功状态码
        res.status = 200;
    });

    server.Post("/run_subtasks", [&](const httplib::Request& req, httplib::Response& res) {
        // 解析接收到的JSON数据
        //Json::CharReaderBuilder readerBuilder;
        //Json::Value data;
        //std::istringstream is(req.body);
        //std::string errs;
        //Json::parseFromStream(readerBuilder, is, &data, &errs);

        //std::cout << "get req:" << is.str() << std::endl;

        std::cout << "get run_subtasks req:" << std::endl;
        
        for(auto iter = work_client_list_map.begin(); iter != work_client_list_map.end(); ++iter){
            ClientNode *c = iter->second;
            c->runFlag = true;
        }

        Json::Value root;
        root["result"] = true;
        Json::StyledWriter sw;
        strstream ss;
        ss << sw.write(root) << endl;

        // 设置 CORS 头，允许所有来源，也可以指定具体的域
        res.set_header("Access-Control-Allow-Origin", "*");
        // 其他响应头设置，根据需要添加
        res.set_header("Content-Type", "application/json");
        // 返回响应
        res.status = 200;
        res.set_content(ss.str(), "application/json");

        std::cout << "ack: " << ss.str() << std::endl;
        
    });

    server.Options("/check_executable", [&](const httplib::Request& req, httplib::Response& res) {
        // 设置 CORS 头
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        // 返回成功状态码
        res.status = 200;
    });

    server.Post("/check_executable", [&](const httplib::Request& req, httplib::Response& res) {
        // 解析接收到的JSON数据
        Json::CharReaderBuilder readerBuilder;
        Json::Value data;
        std::istringstream is(req.body);
        std::string errs;
        Json::parseFromStream(readerBuilder, is, &data, &errs);

        std::cout << "get check_executable req:" << is.str() << std::endl;
        int taskID = data["taskID"].asInt();
        auto it = deployedTaskListMap.find(taskID);
        Task *task = it->second;     

        Json::Value root;
        if(task->downloadSubtaskNum == task->subtask_num)
        {
            root["result"] = true;
        }
        else
        {
            root["result"] = false;
        }
        Json::StyledWriter sw;
        strstream ss;
        ss << sw.write(root) << endl;

        // 设置 CORS 头，允许所有来源，也可以指定具体的域
        res.set_header("Access-Control-Allow-Origin", "*");
        // 其他响应头设置，根据需要添加
        res.set_header("Content-Type", "application/json");
        // 返回响应
        res.status = 200;
        res.set_content(ss.str(), "application/json");

        std::cout << "ack: " << ss.str() << std::endl;
        
    });

    // 启动服务器并监听端口
    server.listen("192.168.80.128", 8080);

    return 0;
}
