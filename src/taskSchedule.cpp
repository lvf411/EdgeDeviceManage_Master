#include <fstream>
#include <random>
#include <cmath>
#include "writeToFile.h"
#include "heft.h"
#include "readFileAndConfig.h"
#include "globalVar.h"
#include "genDAG.h"
#include "taskSchedule.hpp"
#include "master.hpp"

extern std::random_device Rd;	//初始化随机种子
extern std::mt19937 Gen;	//伪随机数生成器
extern std::normal_distribution<> Standard_normal_dist;		//正态分布随机数
extern std::uniform_real_distribution<> Uniform_real_dist01;		//均匀分布随机数

string filePathPrefix = FILEPATH_PREFIX;

string taskProgramPathPrefix = TASK_PROGRAM_PATH_PREFIX;

TestInfo DAGGenterate(string DAGfilename, int tasknum, int nodenum, double edgeNumConpensation)
{
    //保存文件相对地址路径
    string filepath(filePathPrefix + DAGfilename);
    //构造相应的测试文件
    TestInfo tinfo = generateAtest2(tasknum, nodenum, edgeNumConpensation);
    writeFile(filepath, tinfo);

    std::cout << "DAG generate done" << std::endl;

    return tinfo;
}

std::vector<std::queue<int>> HEFT_task_schedule(string DAGfilename)
{
    //保存文件相对地址路径
    string filepath(filePathPrefix + DAGfilename);

    //读入
    readFilAndConfig(filepath);
    //调整输入
    NxPw_TEST = getNxPw(taskWeight_TEST, cpuSpeed_TEST);

    std::vector<std::vector<double>> DAG = DAG_TEST;
    std::vector<std::vector<double>> NxPw = NxPw_TEST;

    auto heft_res = Heft(NxPw, DAG).run_heft();
    std::cout << filepath << std::endl;
    std::cout << "HEFT_Result:" << heft_res.second << std::endl;
    for (auto i = 0; i <  heft_res.first.size(); i++)
    {
        int qsize = heft_res.first[i].size();
        std::cout << i << ": ";
        for (auto j = 0; j < qsize; j++)
        {
            int allocateTask = heft_res.first[i].front();
            std::cout << allocateTask << ' ';
            heft_res.first[i].pop();
            heft_res.first[i].push(allocateTask);
        }
        std::cout << std::endl;
    }

    return heft_res.first;
}

void test_task_exeprogram_generate(const TestInfo &tinfo, int taskid)
{
    for(int i = 1; i <= tinfo.m; i++)
    {
        int cycleNum = tinfo.taskWeight[i];
        cycleNum *= TEST_TASK_CYCLE_PARAM;
        std::string taskFileName = "../task/" + std::to_string(taskid) + '_' + std::to_string(i);
        std::string taskFilePath = taskFileName + ".c";
        std::string taskOutputpath = std::to_string(taskid) + '_' + std::to_string(i) + ".txt";
        std::ofstream ofs(taskFilePath, std::ios::trunc);
        ofs << "#include <stdio.h>" << std::endl;
        ofs << "int main(){" << std::endl;
        ofs << "volatile long sum = 0;" << std::endl;
        ofs << "long i;for(i = 0; i < " << cycleNum << "; i++){" << std::endl;
        ofs << "sum++;}system(\"touch " + taskOutputpath + "\")return 0;}" << std::endl;
        ofs.close();
        std::string compileInst;
        compileInst.append(CROSS_COMPILE_TOOL_PATH);
        //compileInst.append("gcc");
        compileInst.append(" ");
        compileInst.append(taskFilePath);
        compileInst.append(" -o ");
        compileInst.append(taskFileName);
        std::cout << compileInst << std::endl;
        system(compileInst.c_str());

    }
}

extern Master master;
extern int task_increment_id;
extern std::mutex mutex_task_list, mutex_uninit_task_list, mutex_task_id;

int test_task_info_import(const TestInfo &tinfo, std::vector<std::queue<int>> &taskDeploy)
{
    Task *task = new Task();
    mutex_task_id.lock();
    task->id = task_increment_id;
    task_increment_id++;
    mutex_task_id.unlock();
    //task->task_id.append("");   //生成任务不需要自身的task id，只要有系统内的即可
    task->subtask_num = tinfo.n;
    //构造任务执行程序
    test_task_exeprogram_generate(tinfo, task->id);

    list_head *subtask_list_head = new list_head();
    *subtask_list_head = LIST_HEAD_INIT(*subtask_list_head);
    task->subtask_head = subtask_list_head;

    for(int i = 0; i < tinfo.n; i++)
    {
        SubTaskNode *node = new SubTaskNode();
        node->subtask_id = i;
        node->root_id = task->id;
        node->exepath = "../task/" + to_string(task->id) + '_' + to_string(i + 1);

        node->prev_head = new SubTaskResult();
        node->prev_head->next = NULL;
        SubTaskResult *temp = node->prev_head;
        int prevnum = 0;
        //DAG矩阵为上三角矩阵，（0，i）~（i，i）若有非0值代表有前驱关系
        for(int j = 0; j < i; j++)
        {
            if(tinfo.DAG[j][i] > 0)
            {
                prevnum++;
                SubTaskResult *newprev = new SubTaskResult();
                newprev->client_id = 0;
                newprev->subtask_id = j + 1;
                newprev->fname = to_string(task->id) + '_' + to_string(i + 1) + ".txt";
                newprev->next = temp->next;
                temp->next = newprev;
                temp = newprev;
            }
        }
        node->prev_num = prevnum;

        node->succ_head = new SubTaskResult();
        node->succ_head->next = NULL;
        temp = node->succ_head;
        int nextnum = 0;
        for(int j = i; j < tinfo.n; j++)
        {
            if(tinfo.DAG[i][j] > 0)
            {
                nextnum++;
                SubTaskResult *newnext = new SubTaskResult();
                newnext->client_id = 0;
                newnext->subtask_id = j + 1;
                newnext->fname = to_string(task->id) + '_' + to_string(i + 1) + ".txt";
                newnext->next = temp->next;
                temp->next = newnext;
                temp = newnext;
            }
            node->next_num = nextnum;
        }
        node->taskhead = task->subtask_head;
        node->taskself = LIST_HEAD_INIT(node->taskself);
        list_add_tail(&node->taskself, task->subtask_head);
    }

    //补充任务部署设备信息并更新设备任务链表
    int deployArray[tinfo.n] = {0};
    ClientNode *slave[tinfo.m];
    for(int i = 0; i < tinfo.m; i++)
    {
        while(!taskDeploy[i].empty())
        {
            deployArray[taskDeploy[i].front()] = i;
            taskDeploy[i].pop();
        }
    }
    list_head *clientTemp = master.work_client_head->next;
    for(int i = 0; i < tinfo.m; i++)
    {
        slave[i] = (ClientNode *)list_entry(clientTemp, ClientNode, self);
        clientTemp = clientTemp->next;
    }

    list_head *taskTemp = task->subtask_head->next;
    for(int i = 0; i < tinfo.n; i++)
    {
        SubTaskNode *subtaskNode = (SubTaskNode *)list_entry(taskTemp, SubTaskNode, taskself);
        subtaskNode->client_id = deployArray[i];
        slave[deployArray[i]]->subtask_num++;
        slave[deployArray[i]]->modified = true;
        subtaskNode->clienthead = slave[deployArray[i]]->head;
        subtaskNode->clientself = LIST_HEAD_INIT(subtaskNode->clientself);
        list_add_tail(&subtaskNode->clientself, slave[deployArray[i]]->head);
        taskTemp = taskTemp->next;
    }

    //将task插入到master系统中的任务队列中
    task->self = LIST_HEAD_INIT(task->self);
    mutex_uninit_task_list.lock();
    list_add_tail(&task->self, master.uninit_task_list_head);
    mutex_uninit_task_list.unlock();

    return 0;
}
