#ifndef __WRITE_TO_FILE_H
#define __WRITE_TO_FILE_H

#include<iostream>
#include<fstream>
#include<vector>
#include<assert.h>
using namespace std;

template<typename T>
void writeToFile(string path, T val, bool appendFlag = 0) {
	ofstream ofs;
	if (appendFlag)
		ofs = ofstream(path, ios::app);
	else
		ofs = ofstream(path, ios::trunc);
	if (!ofs) {
		assert(1 == 2);
		return;
	}

	ofs << val << endl;
	ofs.close();
}

template<typename T>
void writeToFile(string path, vector<T> val, bool appendFlag = 0) {
	ofstream ofs;
	if (appendFlag)
		ofs = ofstream(path, ios::app);
	else
		ofs = ofstream(path, ios::trunc);
	if (!ofs) {
		assert(1 == 2);
		return;
	}

	for (int i = 0; i < val.size(); i++) {
		ofs << val[i] << " ";
	}
	ofs << endl;
}

template<typename T>
void writeToFile(string path, vector<vector<T>> val, bool appendFlag = 0) {
	ofstream ofs;
	if (appendFlag)
		ofs = ofstream(path, ios::app);
	else
		ofs = ofstream(path, ios::trunc);
	if (!ofs) {
		assert(1 == 2);
		return;
	}

	for (int i = 0; i < val.size(); i++) {
		for (int j = 0; j < val[i].size(); j++) {
			ofs << val[i][j] << " ";
		}
		ofs << endl;
	}
}

template<typename T,typename R>
void writeToFile(string path, vector<pair<T, R>> val, bool appendFlag = 0) {
	ofstream ofs;
	if (appendFlag)
		ofs = ofstream(path, ios::app);
	else
		ofs = ofstream(path, ios::trunc);
	if (!ofs) {
		assert(1 == 2);
		return;
	}

	for (int i = 0; i < val.size(); i++) {
		ofs << val[i].first << " " << val[i].second << endl;
	}
	ofs.close();
}

#endif //__WRITE_TO_FILE_H
