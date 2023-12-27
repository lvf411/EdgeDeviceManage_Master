#include <iostream>
#include <unordered_map>
#include <vector>
#include <assert.h>
#include <algorithm>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <cmath>
#include <queue>
#include <numeric>
#include <fstream>

using namespace std;

extern std::mt19937 Gen;
//尝试添加edgeNum次边即可
vector<vector<double>> genDAG(int vnum, int edge_num, double edgewL = 1, double edgewR = 1) {
	vector<vector<double>> DAG(vnum, vector<double>(vnum, 0));
	vector<int> indegree(vnum, 0);
	vector<int> outdegree(vnum, 0);
	//随机找两个点，小序号点 连 大序号点
	std::uniform_int_distribution<> int_dist(0, vnum - 1);
	std::uniform_real_distribution<> real_dist(edgewL, edgewR);
	for (int itera = 0; itera < edge_num; itera++) {
		//rand 2 different vertex
		int v1 = int_dist(Gen);
		int v2 = int_dist(Gen);
		while (v1 == v2) {
			v2 = int_dist(Gen);
		}

		if (v1 < v2)
			DAG[v1][v2] = real_dist(Gen);
		else if (v1 > v2)
			DAG[v2][v1] = real_dist(Gen);
		else
			assert(1 == 2);
	}
	return DAG;
}


//要求边的数量达到edgeNum为止
vector<vector<double>> genDAG_ver2(int vnum, int edge_num, double edgewL = 1, double edgewR = 1) {
	vector<vector<double>> DAG(vnum, vector<double>(vnum, 0));
	vector<int> indegree(vnum, 0);
	vector<int> outdegree(vnum, 0);
	//随机找两个点，小序号点 连 大序号点
	std::uniform_int_distribution<> int_dist(0, vnum - 1);
	std::uniform_real_distribution<> real_dist(edgewL, edgewR);
	int ednum = 0;
	assert(edge_num < vnum* vnum / 2);//最多只能连这么多边
	
	vector<pair<int, int>> canConnectedEdge;//哪些点现在还有出边.(点号，出边数)。会erase
	for (int i = 0; i < vnum; i++) {
		canConnectedEdge.push_back({ i,vnum - (i + 1) });
	}
	canConnectedEdge.pop_back();//最后一个点没有能连的边
	vector<vector<int>> notConnectedEdge(vnum);//每个点对应还有哪些出边。外层vector不会erase，下标即点号。内层会erase
	for (int i = 0; i < vnum; i++) {
		for (int j = i+1; j < vnum; j++) {
			notConnectedEdge[i].push_back(j);
		}
	}
	//notConnectedEdge[1].erase(find(notConnectedEdge[1].begin(), notConnectedEdge[1].end(), 1));
	while(ednum<edge_num){
		std::uniform_int_distribution<> out_intDist(0, canConnectedEdge.size() - 1);//随机一出点
		int randPos= out_intDist(Gen);
		int outPoint = canConnectedEdge[randPos].first;
		int outPoint_notConnNum = canConnectedEdge[randPos].second;
		assert(outPoint_notConnNum > 0);
		assert(outPoint_notConnNum == notConnectedEdge[outPoint].size());
		std::uniform_int_distribution<> in_intDist(0, notConnectedEdge[outPoint].size()-1);//随机一入点
		int randPos2 = in_intDist(Gen);
		int inPoint = notConnectedEdge[outPoint][randPos2];

		//update canConnectedEdge and notConnectedEdge
		if (--canConnectedEdge[randPos].second == 0)
			canConnectedEdge.erase(find(canConnectedEdge.begin(), canConnectedEdge.end(), pair<int, int>(outPoint, 0)));
		notConnectedEdge[outPoint].erase(find(notConnectedEdge[outPoint].begin(), notConnectedEdge[outPoint].end(), inPoint));

		if (outPoint < inPoint)
			DAG[outPoint][inPoint] = real_dist(Gen);
		else if (inPoint < outPoint)
			DAG[inPoint][outPoint] = real_dist(Gen);
		else
			assert(1 == 2);
		ednum++;
	}
	return DAG;
}

/*
1—2
  /
3

4—5
*/
vector<vector<int>> getTopo(const vector<vector<int>>& DAG) {
	int vnum = DAG.size();
	vector<int> indegree(vnum, 0);
	for (int i = 0; i < vnum; i++) {
		for (int j = 0; j < vnum; j++) {
			if (DAG[i][j] == 1) {
				indegree[j]++;
			}
		}
	}
	//收集第一列
	vector<vector<int>> topo;
	vector<int> curCol;
	for (int i = 0; i < vnum; i++) {
		if (indegree[i] == 0) {
			curCol.push_back(i);
			indegree[i] = -1;
		}
	}
	topo.push_back(curCol);
	
	while (!curCol.empty()) {
		//清除当前列的出度
		while (!curCol.empty()) {
			int vno = curCol.back(); curCol.pop_back();
			for (int j = 0; j < vnum; j++) {
				if (DAG[vno][j] == 1) {
					indegree[j]--;
				}
			}
		}
		//收集当前列
		for (int i = 0; i < vnum; i++) {
			if (indegree[i] == 0) {
				curCol.push_back(i);
				indegree[i] = -1;
			}
		}
		topo.push_back(curCol);
	}
	return topo;
}

vector<vector<double>> getRangeViaTopo(const vector<vector<int>>& topo, int nvar) {
	int lb = 0, ub = 0;
	vector<vector<double>> bounds(nvar, vector<double>(2, 0));
	for (int i = 0; i < topo.size(); i++) {
		lb = ub + 1;
		ub = lb + topo[i].size();
		for (int j = 0; j < topo[i].size(); j++) {
			bounds[topo[i][j]][0] = lb;
			bounds[topo[i][j]][1] = ub;
		}
	}
	return bounds;
}


pair<vector<int>,vector<int>> generateTestSample(int tasknum, int cpunum, int edgenum, string outputFilePath) {
	vector<int> allCpus{ 80,64,15,30,50,80,72,25,48,64,32,24,
	16,80,32,40,24,32,48,70,550,280,120,180,150,200,300,252,500,
	600,800,100,960,266,240,200,100,264,300,480,2560,1024,2252,
	1843,1433,1536,1228,1126,1198,1945,2355,2560,2662,1638,1740,1843,
	1331,2252,3276,3072 };

	vector<int> cpuspeed(cpunum);
	std::uniform_int_distribution<> int_dist1(0, allCpus.size() - 1);
	for (int i = 0; i < cpunum; i++) {
		cpuspeed[i] = allCpus[int_dist1(Gen)];
	}

	vector<int> taskweight(tasknum);
	std::uniform_int_distribution<> int_dist2(30, 3000);
	for (int i = 0; i < tasknum; i++) {
		taskweight[i] = int_dist2(Gen);
	}

	vector<vector<double>> DAG = genDAG(tasknum, edgenum);

	ofstream ofs(outputFilePath, ios::out);
	if (!ofs)
		assert(1 == 2);
	ofs << tasknum << " " << cpunum << endl;
	for (int i = 0; i < taskweight.size(); i++) {
		ofs << taskweight[i] << " ";
	}
	ofs << endl;
	for (int i = 0; i < cpuspeed.size(); i++) {
		ofs << cpuspeed[i] << " ";
	}
	ofs << endl;
	for (int i = 0; i < DAG.size(); i++) {
		for (int j = 0; j < DAG[0].size(); j++) {
			ofs << DAG[i][j] << " ";
		}
		ofs << endl;
	}
	return { taskweight,cpuspeed };
}


void generateTestSample(vector<int> taskweight, vector<int> cpuspeed, int edgenum, string outputFilePath) {
	vector<int> allCpus{ 80,64,15,30,50,80,72,25,48,64,32,24,
	16,80,32,40,24,32,48,70,550,280,120,180,150,200,300,252,500,
	600,800,100,960,266,240,200,100,264,300,480,2560,1024,2252,
	1843,1433,1536,1228,1126,1198,1945,2355,2560,2662,1638,1740,1843,
	1331,2252,3276,3072 };

	int cpunum = cpuspeed.size();
	int tasknum = taskweight.size();

	vector<vector<double>> DAG = genDAG(tasknum, edgenum);

	ofstream ofs(outputFilePath, ios::out);
	if (!ofs)
		assert(1 == 2);
	ofs << tasknum << " " << cpunum << endl;
	for (int i = 0; i < taskweight.size(); i++) {
		ofs << taskweight[i] << " ";
	}
	ofs << endl;
	for (int i = 0; i < cpuspeed.size(); i++) {
		ofs << cpuspeed[i] << " ";
	}
	ofs << endl;
	for (int i = 0; i < DAG.size(); i++) {
		for (int j = 0; j < DAG[0].size(); j++) {
			ofs << DAG[i][j] << " ";
		}
		ofs << endl;
	}
}