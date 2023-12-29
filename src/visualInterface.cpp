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

std::string visualizationTaskGenerateAndDeploy(int taskNum, double NC)
{
    TestInfo tinfo = DAGGenterate("10_10.txt", taskNum, master.work_client_num, NC);

    auto scheduleRes = HEFT_task_schedule("10_10.txt");

    return scheduleResExport(taskNum, scheduleRes);
}

int testserver()
{
    httplib::Server server;

    server.Options("/perform_calculation", [&](const httplib::Request& req, httplib::Response& res) {
        // 设置 CORS 头
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        // 返回成功状态码
        res.status = 200;
    });

    server.Post("/perform_calculation", [&](const httplib::Request& req, httplib::Response& res) {
        // 解析接收到的JSON数据
        Json::CharReaderBuilder readerBuilder;
        Json::Value data;
        std::istringstream is(req.body);
        std::string errs;
        Json::parseFromStream(readerBuilder, is, &data, &errs);

        std::cout << "get req:" << is.str() << std::endl;

        // 从JSON中获取输入数据
        int subtaskNum = data["subtaskNum"].asInt();
        double dagEdgeDensity = data["dagEdgeDensity"].asDouble();

        std::cout << "subtaskNum:" << subtaskNum << std::endl << "nc: " << dagEdgeDensity << std::endl;

        std::string result = visualizationTaskGenerateAndDeploy(subtaskNum, dagEdgeDensity);

        // 设置 CORS 头，允许所有来源，也可以指定具体的域
        res.set_header("Access-Control-Allow-Origin", "*");
        // 其他响应头设置，根据需要添加
        res.set_header("Content-Type", "application/json");
        // 返回响应
        res.status = 200;
        
        res.set_content(result, "application/json");

        std::cout << "ack: " << result << std::endl;
    });

    // 启动服务器并监听端口
    server.listen("192.168.80.128", 8080);

    return 0;
}
