#ifndef __GENDAG_H
#define __GENDAG_H

#include<iostream>
#include<vector>
using namespace std;
vector<vector<double>> getTopo(const vector<vector<double>>& DAG);
vector<vector<double>> getRangeViaTopo(const vector<vector<int>>& topo, int nvar);
pair<vector<int>, vector<int>> generateTestSample(int tasknum, int cpunum, int edgenum, string outputFilePath);
void generateTestSample(vector<int> taskweight, vector<int> cpuspeed, int edgenum, string outputFilePath);
vector<vector<double>> genDAG(int vnum, int edge_num, double edgewL = 1, double edgewR = 1);

vector<vector<double>> genDAG_ver2(int vnum, int edge_num, double edgewL = 1, double edgewR = 1);

#endif //__GENDAG_H