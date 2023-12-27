#include<iostream>
#include<vector>
#include<fstream>
#include<assert.h>
#include"globalVar.h"
using namespace std;

void readFilAndConfig(string path) {
    ifstream ifs(path);
    if (!ifs)
        assert(1 == 2);
    int n, m; ifs >> n >> m;
    tasknum_TEST = n; cpunum_TEST = m;

    DAG_TEST = vector<vector<double>>(n, vector<double>(n, 0));
    taskWeight_TEST = vector<double>(n, 0);
    cpuSpeed_TEST = vector<double>(m, 0);
    NxPw_TEST = vector<vector<double>>(n, vector<double>(m, 0));

    for (unsigned i = 0; i < DAG_TEST.size(); i++) {
        for (unsigned j = 0; j < DAG_TEST[0].size(); j++) {
            ifs >> DAG_TEST[i][j];
        }
    }
    for (unsigned i = 0; i < taskWeight_TEST.size(); i++) {
        ifs >> taskWeight_TEST[i];
    }
    for (unsigned i = 0; i < cpuSpeed_TEST.size(); i++) {
        ifs >> cpuSpeed_TEST[i];
    }
    for (unsigned i = 0; i < NxPw_TEST.size(); i++) {
        for (unsigned j = 0; j < NxPw_TEST[0].size(); j++) {
            ifs >> NxPw_TEST[i][j];
        }
    }
    ifs.close();
}