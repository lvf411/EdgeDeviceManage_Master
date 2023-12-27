#include <fstream>
#include <string>
#include <map>
#include <random>
#include <cmath>
#include "writeToFile.h"
#include "heft.h"
#include "genTest.h"
#include "readFileAndConfig.h"
#include "globalVar.h"
#include "genDAG.h"
#include "genTest.h"

std::random_device Rd{};	//初始化随机种子
std::mt19937 Gen{ Rd() };	//伪随机数生成器
std::normal_distribution<> Standard_normal_dist{ 0,1 };		//正态分布随机数
std::uniform_real_distribution<> Uniform_real_dist01(0.0, 1.0);		//均匀分布随机数

int Task_num; int Cpu_num; int Nvars;
vector<double> Task_weight;
vector<double> Cpu_speed;
//vector<vector<int>> DAG;
vector<vector<int>> DAG_glo;

int main(){
    std::vector<std::string> fileList;
    std::ifstream inputFile("testFileList.txt");  // 打开文件
    if (inputFile.is_open()) {  // 检查文件是否成功打开
        std::string line;
        while (std::getline(inputFile, line)) {  // 逐行读取文件内容
            fileList.push_back(line);  // 将每一行内容存储到vector<string>中
        }
        inputFile.close();  // 关闭文件

        // 输出存储的文件名
        for (const auto& fileName : fileList) {
            std::cout << fileName << std::endl;
        }
    }
    else {
        std::cerr << "Failed to open testFileList.txt." << std::endl;  // 打开文件失败时输出错误消息
        assert(1 == 2);
        return 1;
    }
    
    //保存文件相对地址路径
    string filePathPrefix = "/home/lf/Documents/EDM/master/bin/taskSchedule/";
    vector<string> filePaths;
    for (string fileName : fileList) {
        filePaths.push_back(filePathPrefix + fileName);
        //构造相应的测试文件
        writeFile(filePathPrefix + fileName, generateAtest2(100, 9, 0.5));
    }

    std::cout << "DAG generate done" << std::endl;

    for(string fileName : filePaths) {
        	//读入
			readFilAndConfig(fileName);
			//调整输入
			NxPw_TEST = getNxPw(taskWeight_TEST, cpuSpeed_TEST);

			vector<vector<double>> DAG = DAG_TEST;
			vector<vector<double>> NxPw = NxPw_TEST;

			auto heft_res = Heft(NxPw, DAG).run_heft();
			std::cout << fileName << std::endl;
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
    }
}