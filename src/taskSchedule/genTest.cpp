#include"genTest.h"
#include "..\common\writeToFile.h"
#include "genDAG.h"
#include<algorithm>
#include "..\common\randInfo.h"
#include<fstream>
#include"genTest.h"
//struct TestInfo {
//	int n, m;
//	vector<vector<double>> DAG;
//	vector<double> taskWeight;
//	vector<double> cpuSpeed;
//	vector<vector<double>> NxPw;
//};
vector<vector<double>> getNxPw(vector<double> taskWeight, vector<double> cpuSpeed) {
	int taskNum = taskWeight.size();
	int cpuNum = cpuSpeed.size();
	vector<vector<double>> NxPw(taskNum, vector<double>(cpuNum, 0));
	for (int i = 0; i < NxPw.size(); i++) {
		for (int j = 0; j < NxPw[0].size(); j++) {
			NxPw[i][j] = taskWeight[i] / cpuSpeed[j];
		}
	}
	return NxPw;
}

void writeFile(string filePath, const TestInfo& test) {
	writeToFile(filePath, test.n);
	writeToFile(filePath, test.m, 1);
	writeToFile(filePath, test.DAG, 1);
	writeToFile(filePath, test.taskWeight, 1);
	writeToFile(filePath, test.cpuSpeed, 1);
	writeToFile(filePath, test.NxPw, 1);
}

/**
* @brief	将生成的有向无环图输出到文件
* @param	filePath 文件路径
* @param	n 任务数量
* @param	m 机器数量
* @param	DAG 生成的有向无环图
* @param	taskWeight 任务权重
* @param	cpuspeed cpu速度
* @param	NxPw 任务个数*cpu个数矩阵
*/
void writeFile(string filePath, int n, int m, const vector<vector<double>>& DAG,
	const vector<double>& taskWeight, const vector<double>& cpuSpeed, const vector<vector<double>>& NxPw) {
	writeToFile(filePath, n);
	writeToFile(filePath, m, 1);
	writeToFile(filePath, DAG, 1);
	writeToFile(filePath, taskWeight, 1);
	writeToFile(filePath, cpuSpeed, 1);
	writeToFile(filePath, NxPw, 1);
}

TestInfo generateAtest(int n, int m, double NC, double hc, double ht, double hb, double CCR) {
	//int n = 100, m = 10;
	//double hc = 3;//处理器的异质性=最快速度/最慢速度
	//double ht = 6;//任务量
	//double hb = 1;//传输量
	//double CCR = 1;//任务量均值/传输量均值
	double cpuSpeed_L = 1;
	double taskWeight_L = 1;

	int edgeNum = n * n * NC;

	double cpuSpeed_r = cpuSpeed_L * hc;
	double taskWeight_r = taskWeight_L * ht;
	double edgeWeight_mean = (taskWeight_L + taskWeight_r) / 2 / CCR;
	double edgeWeight_L = (2 * edgeWeight_mean) / (hb + 1);
	double edgeWeight_R = 2 * edgeWeight_mean - edgeWeight_L;

	TestInfo testInfo;
	testInfo.n = n;
	testInfo.m = m;
	testInfo.DAG = genDAG(n, edgeNum, edgeWeight_L, edgeWeight_R);

	testInfo.taskWeight.resize(n);
	std::uniform_real_distribution<> taskWeight_dist(taskWeight_L, taskWeight_r);
	for (int i = 0; i < n; i++) {
		testInfo.taskWeight[i] = taskWeight_dist(Gen);
	}

	testInfo.cpuSpeed.resize(m);
	std::uniform_real_distribution<> cpuSpeed_dist(cpuSpeed_L, cpuSpeed_r);
	for (int i = 0; i < m; i++) {
		testInfo.cpuSpeed[i] = cpuSpeed_dist(Gen);
	}
	sort(testInfo.cpuSpeed.rbegin(), testInfo.cpuSpeed.rend());

	testInfo.NxPw = getNxPw(testInfo.taskWeight, testInfo.cpuSpeed);

	return testInfo;
}


TestInfo generateAtest2(int n, int m, double NC) {
	double taskWeight_L = 1;
	double taskWeight_r = 400;

	TestInfo testInfo;
	testInfo.n = n;
	testInfo.m = m;
		
	vector<int> allCpus{ 80,64,15,30,50,80,72,25,48,64,32,24,16,80,32,
		40,24,32,48,70,550,280,120,180,150,200,300,252,500,600,800,100,
		960,266,240,200,100,264,300,480,2560,1024,2252,1843,1433,1536,
		1228,1126,1198,1945,2355,2560,2662,1638,1740,1843,1331,2252,3276,3072 };
	testInfo.cpuSpeed.resize(m);
	if (m == 10) {
		testInfo.cpuSpeed = { 80,64,200,600,500,1228,1198,1945,800,1331 };
	}
	else if (m == 5) {
		testInfo.cpuSpeed = { 180,800,200,600,1198 };
	}
	else if (m == 15) {
		testInfo.cpuSpeed = { 80,64,200,1198,600,500,1228,2252,960,264,1126,1198,1945,800,1331 };
	}
	else {
		std::uniform_int_distribution<> cpuSpeed_dist(0, allCpus.size() - 1);
		for (int i = 0; i < m; i++) {
			testInfo.cpuSpeed[i] = allCpus[cpuSpeed_dist(Gen)];
		}
	}

	sort(testInfo.cpuSpeed.rbegin(), testInfo.cpuSpeed.rend());


	int edgeNum = n * n * NC;
	testInfo.DAG = genDAG(n, edgeNum, 0.05, 0.1);

	testInfo.taskWeight.resize(n);
	std::uniform_real_distribution<> taskWeight_dist(taskWeight_L, taskWeight_r);
	for (int i = 0; i < n; i++) {
		testInfo.taskWeight[i] = taskWeight_dist(Gen);
	}

	testInfo.NxPw = getNxPw(testInfo.taskWeight, testInfo.cpuSpeed);

	return testInfo;
}

//used for PLC任务分配项目申报（废弃）
TestInfo generateAtest3(int n, int m, double NC, int knownDAG_Flag,const vector<vector<double>>& knownDAG) {
	double taskWeight_L = 100;
	double taskWeight_r = 600;

	TestInfo testInfo;
	testInfo.n = n;
	testInfo.m = m;

	vector<int> allCpus{ 500,600,700 };
	testInfo.cpuSpeed.resize(m);
	std::uniform_int_distribution<> cpuSpeed_dist(0, allCpus.size() - 1);
	if (m == 1) {
		testInfo.cpuSpeed = { 500 };
	}
	else if (m == 2) {
		testInfo.cpuSpeed = { 500,600 };
	}
	else if (m == 3) {
		testInfo.cpuSpeed = { 500,600 ,700 };
	}
	else if (m == 4) {
		testInfo.cpuSpeed = { 500,600 ,700,500 };
	}
	else if (m == 5) {
		testInfo.cpuSpeed = { 500,600 ,700,500,600 };
	}
	else if (m == 6) {
		testInfo.cpuSpeed = { 500,600 ,700,500,600,700 };
	}
	else {
		for (int i = 0; i < m; i++) {
			testInfo.cpuSpeed[i] = allCpus[cpuSpeed_dist(Gen)];
		}
	}
	sort(testInfo.cpuSpeed.rbegin(), testInfo.cpuSpeed.rend());

	int edgeNum = n * n * NC;
	if (!knownDAG_Flag)
		testInfo.DAG = genDAG(n, edgeNum, 0.3, 0.5);
	else
		testInfo.DAG = knownDAG;

	testInfo.taskWeight.resize(n);
	std::uniform_real_distribution<> taskWeight_dist(taskWeight_L, taskWeight_r);
	for (int i = 0; i < n; i++) {
		testInfo.taskWeight[i] = taskWeight_dist(Gen);
	}

	testInfo.NxPw = getNxPw(testInfo.taskWeight, testInfo.cpuSpeed);

	return testInfo;
}