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
    for(int i = 1; i <= tinfo.n; i++)
    {
        long cycleNum = tinfo.taskWeight[i - 1];
        cycleNum *= TEST_TASK_CYCLE_PARAM;
        std::string taskFileName = "../task/" + std::to_string(taskid) + '_' + std::to_string(i);
        std::string taskFilePath = taskFileName + ".cpp";
        std::string taskOutputpath = std::to_string(taskid) + '_' + std::to_string(i) + ".txt";
        std::ofstream ofs(taskFilePath, std::ios::trunc);
        ofs << "#include <iostream>" << std::endl;
        ofs << "#include <chrono>" << std::endl;
        ofs << "using namespace std;" << std::endl;
        ofs << "int main(){" << std::endl;
        // ofs << "auto currenttime = std::chrono::system_clock::now();" << std::endl;
        // ofs << "auto timestamp = std::chrono::system_clock::to_time_t(currenttime);" << std::endl;
        // ofs << "auto timestampMS = std::chrono::duration_cast<std::chrono::milliseconds>(currenttime.time_since_epoch()).count();" << std::endl;
        // ofs << "char buffer[80];" << std::endl;
        // ofs << "std::strftime(buffer, sizeof(buffer), \"%Y-%m-%d %H:%M:%S\", std::localtime(&timestamp));" << std::endl;
        // ofs << "std::cout << \"ststart timestampMS: \" << timestampMS << std::endl;" << std::endl;
        // ofs << "std::cout << \"ststart timestamp: \" << buffer << std::endl;" << std::endl;
        ofs << "volatile long sum = 0;" << std::endl;
        ofs << "long i;for(i = 0; i < " << cycleNum << "; i++){" << std::endl;
        // ofs << "sum++;}system(\"touch " + taskOutputpath + "\");return 0;}" << std::endl;
        ofs << "sum++;}system(\"echo a > " + taskOutputpath +"\");" <<std::endl;
        // ofs << "currenttime = std::chrono::system_clock::now();" << std::endl;
        // ofs << "timestamp = std::chrono::system_clock::to_time_t(currenttime);" << std::endl;
        // ofs << "timestampMS = std::chrono::duration_cast<std::chrono::milliseconds>(currenttime.time_since_epoch()).count();" << std::endl;
        // ofs << "std::strftime(buffer, sizeof(buffer), \"%Y-%m-%d %H:%M:%S\", std::localtime(&timestamp));" << std::endl;
        // ofs << "std::cout << \"stend   timestampMS: \" << timestampMS << std::endl;" << std::endl;
        // ofs << "std::cout << \"stend   timestamp: \" << buffer << std::endl;" << std::endl;
        ofs << "return 0;}" << std::endl;
        ofs.close();
        std::string compileInst;
        compileInst.append(CROSS_COMPILE_TOOL_PATH);
        compileInst.append(" ");
        compileInst.append(taskFilePath);
        compileInst.append(" -o ");
        compileInst.append(taskFileName);
        compileInst.append(" -std=gnu++11 ");
        std::cout << compileInst << std::endl;
        system(compileInst.c_str());

    }
}

extern Master master;
extern int task_increment_id;
extern std::mutex mutex_task_list, mutex_uninit_task_list, mutex_task_id;
extern map<int, Task *>deployedTaskListMap; 

//返回系统内分配的任务id
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
        node->subtask_id = i + 1;
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
                newprev->fname = to_string(task->id) + '_' + to_string(j + 1) + ".txt";
                newprev->next = temp->next;
                temp->next = newprev;
                temp = newprev;
            }
        }
        node->prev_num = prevnum;

        //（i，i）~（i，n-1）若有非0值代表有后继关系
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
        //taskDeploy 为从0开始单独编号的所有工作节点上被分配的当前任务的子任务序列
        //假如有工作节点被撤销或掉线的情况需要另外处理，此处未考虑
        int subtasknum = taskDeploy[i].size();
        for(int j = 0; j < subtasknum; j++)
        {
            int index = taskDeploy[i].front();
            deployArray[index] = i + 1;
            taskDeploy[i].pop();
            taskDeploy[i].push(index);
        }
    }

    cerr << "tinfo.n:" << tinfo.n << "\ttinfo.m:" << tinfo.m << endl;
    for(int i = 0; i < tinfo.m; i++)
    {
        cerr << deployArray[i] << ":";
    }

    //补充从节点任务链表中自身、前驱与后继被分配的节点信息
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
        slave[deployArray[i] - 1]->subtask_num++;
        slave[deployArray[i] - 1]->modified = true;
        subtaskNode->clienthead = slave[deployArray[i] - 1]->head;
        subtaskNode->clientself = LIST_HEAD_INIT(subtaskNode->clientself);
        list_add_tail(&subtaskNode->clientself, slave[deployArray[i] - 1]->head);
        taskTemp = taskTemp->next;
    }

    list_head *subt_head = task->subtask_head, *subt_temp = task->subtask_head->next;
    while(subt_temp != subt_head)
    {
        int j = 0;
        SubTaskNode *subt = (SubTaskNode *)(list_entry(subt_temp, SubTaskNode, taskself));
        SubTaskResult *res_temp = subt->prev_head->next;
        while(j < subt->prev_num)
        {
            res_temp->client_id = slave[deployArray[res_temp->subtask_id - 1] - 1]->client_id;
            res_temp = res_temp->next;
            j++;
        }
        j = 0;
        res_temp = subt->succ_head->next;
        while(j < subt->next_num)
        {
            res_temp->client_id = slave[deployArray[res_temp->subtask_id - 1] - 1]->client_id;
            res_temp = res_temp->next;
            j++;
        }
        subt_temp = subt_temp->next;
    }

    //将task插入到master系统中的任务队列中
    task->self = LIST_HEAD_INIT(task->self);
    mutex_task_list.lock();
    list_add_tail(&task->self, master.task_list_head);
    deployedTaskListMap.insert(map<int, Task *>::value_type(task->id, task));
    master.task_num++;
    mutex_task_list.unlock();

    return task->id;
}
