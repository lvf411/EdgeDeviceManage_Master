/*
heft:
1. calc ranku
2. insersion-based allocation
*/
#include <iostream>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <queue>
#include <utility>
#include <climits>

using namespace std;
class Heft {
    struct Subtask {
        double avgw, avgc;
        double ranku;
        double est, eft;
        int allocatedFlag, cpuno;
        Subtask() :avgw(0), avgc(0), ranku(0), est(0), eft(0), cpuno(0), allocatedFlag(0) {};
    };
    struct Processor {
        vector<pair<double, double>> taskIntervals;
    };
    const vector<vector<double>> NxPw;
    const vector<vector<double>> DAG;
    unsigned tasknum, cpunum;
    vector<Subtask> subtasks;

    //逆拓扑排序依次计算各ranku
    void calc_rankus() {
        vector<int> outdegree(tasknum, 0);
        for (unsigned i = 0; i < DAG.size(); i++) {
            for (unsigned j = 0; j < DAG.size(); j++) {
                if (DAG[i][j]) {
                    outdegree[i]++;
                }
            }
        }

        vector<int> stk;
        for (unsigned i = 0; i < outdegree.size(); i++) {
            if (outdegree[i] == 0)
                stk.push_back(i);
        }


        while (!stk.empty()) {
            //出一，删其"入"度
            int idx = stk.back(); stk.pop_back();
            for (unsigned i = 0; i < DAG.size(); i++) {
                if (DAG[i][idx]) {
                    if (--outdegree[i] == 0) {
                        stk.push_back(i);
                    }
                }
            }
            //赋值tasks[idx].ranku
            subtasks[idx].ranku = subtasks[idx].avgw;
            double maxLatter = 0;//maxLatter = max(cij+ranku(j)) j��succ(idx)
            for (unsigned j = 0; j < DAG.size(); j++) {
                if (DAG[idx][j]) {
                    maxLatter = max(DAG[idx][j] + subtasks[j].ranku, maxLatter);
                }
            }
            subtasks[idx].ranku += maxLatter;
        }
    }
public:
    Heft(const vector<vector<double>>& NxPw, const vector<vector<double>>& DAG) :NxPw(NxPw), DAG(DAG) {
        tasknum = DAG.size();
        cpunum = NxPw[0].size();
        subtasks = vector<Subtask>(tasknum);
        //init avgw
        for (unsigned i = 0; i < tasknum; i++) {
            int avai_cpunum = 0;
            subtasks[i].avgw = 0;
            for (unsigned j = 0; j < cpunum; j++) {
                if (NxPw[i][j] != INT_MAX) {
                    avai_cpunum++;
                    subtasks[i].avgw += NxPw[i][j];
                }
            }
            subtasks[i].avgw /= avai_cpunum;
        }
        //init avgc
        for (unsigned i = 0; i < DAG.size(); i++) {
            subtasks[i].avgc = 0;
            for (unsigned j = 0; j < DAG.size(); j++) {
                if (DAG[i][j] > 0) {
                    subtasks[i].avgc += DAG[i][j];
                }
            }
            subtasks[i].avgc /= tasknum;
        }
        calc_rankus();

    }

    pair<vector<queue<int>>, double>  run_heft() {
        vector<int> sortedIdx(tasknum);
        for (int i = 0; i < tasknum; i++)
            sortedIdx[i] = i;
        sort(sortedIdx.begin(), sortedIdx.end(), [this](int idx1, int idx2) {
            return this->subtasks[idx1].ranku > this->subtasks[idx2].ranku;
        });

        vector<Processor> processors(cpunum);
        for (int& curIdx : sortedIdx) {
            assert(subtasks[curIdx].allocatedFlag == 0);

            int bestPidx = -1;
            double bestAvaiTime = 0;
            //find best processor
            for (unsigned i = 0; i < processors.size(); i++) {
                //realease time of subtask[curIdx] when allocated on processors[i]
                double releaseTime = 0;
                for (unsigned pred = 0; pred < DAG.size(); pred++) {
                    if (DAG[pred][curIdx] > 0) {//if is predecessor
                        assert(subtasks[pred].allocatedFlag == 1);
                        releaseTime = max(releaseTime, subtasks[pred].eft + ((subtasks[pred].cpuno == i) ? 0 : DAG[pred][curIdx]));
                    }
                }

                //insertion-based allocation, find first okay intervalL
                double intervalL = 0, intervalR = 0;
                for (unsigned j = 0; j < processors[i].taskIntervals.size(); j++) {
                    intervalR = processors[i].taskIntervals[j].first;
                    if (intervalL >= releaseTime && intervalR - intervalL >= NxPw[curIdx][i]) {
                        break;//find the okay interval
                    }
                    intervalL = processors[i].taskIntervals[j].second;
                }
                if (intervalL >= intervalR)//not find
                    intervalL = max(releaseTime, intervalL);

                if (bestPidx == -1 || intervalL + NxPw[curIdx][i] < bestAvaiTime + NxPw[curIdx][bestPidx]) {
                    bestPidx = i;
                    bestAvaiTime = intervalL;
                }
            }
            //update according to bestPidx and bestAvaiTime
            if (processors[bestPidx].taskIntervals.empty() || bestAvaiTime >= processors[bestPidx].taskIntervals.back().second)
                processors[bestPidx].taskIntervals.push_back({ bestAvaiTime,bestAvaiTime + NxPw[curIdx][bestPidx] });
            else {
                for (auto iter = processors[bestPidx].taskIntervals.begin(); iter != processors[bestPidx].taskIntervals.end(); iter++) {
                    if (bestAvaiTime < iter->first) {
                        processors[bestPidx].taskIntervals.insert(iter, { bestAvaiTime,bestAvaiTime + NxPw[curIdx][bestPidx] });
                        break;
                    }
                }
            }
            subtasks[curIdx].allocatedFlag = 1;
            subtasks[curIdx].cpuno = bestPidx;
            subtasks[curIdx].est = bestAvaiTime;
            subtasks[curIdx].eft = subtasks[curIdx].est + NxPw[curIdx][bestPidx];
        }

    //    // DEBUG
    //    for (auto idx : sortedIdx) {
    //        cout << idx << ",";
    //}
    //    cout << endl;
    //    for (auto subTask : subtasks) {
    //        cout << subTask.cpuno << ",";
    //    }
    //    cout << endl;

        pair<vector<queue<int>>, double> res;
        res.second = subtasks[0].eft;
        for (int i = 0; i < tasknum; i++) {
            assert(subtasks[i].allocatedFlag = 1);
            res.second = max(res.second, subtasks[i].eft);
        }
        vector<vector<int>> allocation(cpunum);
        for (int i = 0; i < subtasks.size(); i++) {
            allocation[subtasks[i].cpuno].push_back(i);
        }
        for (int i = 0; i < allocation.size(); i++) {
            sort(allocation[i].begin(), allocation[i].end(), [this](int taskidx1, int taskidx2) {
                return this->subtasks[taskidx1].est < this->subtasks[taskidx2].est;
            });
        }
        res.first.resize(cpunum);
        for (int i = 0; i < res.first.size(); i++) {
            for (int j = 0; j < allocation[i].size(); j++) {
                res.first[i].push(allocation[i][j]);
            }
        }
        return res;
    }
};
