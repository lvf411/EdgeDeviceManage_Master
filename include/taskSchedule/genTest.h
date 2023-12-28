#ifndef __GENTEST_H
#define __GENTEST_H

#include<vector>
#include<iostream>
using namespace std;
struct TestInfo {
	int n, m;
	vector<vector<double>> DAG;
	vector<double> taskWeight;
	vector<double> cpuSpeed;
	vector<vector<double>> NxPw;
};
void writeFile(string filePath, const TestInfo& test);
TestInfo generateAtest(int n = 100, int m = 10, double NC = 0.3, double hc = 3, double ht = 6, double hb = 1, double CCR = 1);
TestInfo generateAtest2(int n, int m, double NC);
TestInfo generateAtest3(int n, int m, double NC, int knownDAG_Flag = 0, const vector<vector<double>>& DAG = vector<vector<double>>());
vector<vector<double>> getNxPw(vector<double> taskWeight, vector<double> cpuSpeed);
void writeFile(string filePath, int n, int m, const vector<vector<double>>& DAG,
	const vector<double>& taskWeight, const vector<double>& cpuSpeed, const vector<vector<double>>& NxPw);

#endif //__GENTEST_H