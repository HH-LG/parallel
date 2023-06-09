#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include<omp.h>
#include"Adp.h"
#include <mpi.h>

bool isParallel_in = false;
// hash分段实现
struct HashBucket {// 一个hash段，记录他在倒排列表中的起始位置
	int begin = -1;
	int end = -1;
};
HashBucket** hashBucket;
// 预处理，将倒排列表放入hash段里
void preprocessing(vector<InvertedIndex>& index, int count) {
	hashBucket = new HashBucket * [count];
	for (int i = 0; i < count; i++)
		hashBucket[i] = new HashBucket[101600];// 26000000/65536

	for (int i = 0; i < count; i++) {
		for (int j = 0; j < index[i].docIdList.size(); j++) {
			int value = index[i].docIdList[j];// 拿出当前列表第j个值
			int hashValue = value / 256;// TODO:check
			if (hashBucket[i][hashValue].begin == -1) // 该段内还没有元素
				hashBucket[i][hashValue].begin = j;
			hashBucket[i][hashValue].end = j;// 添加到该段尾部
		}
	}
}
InvertedIndex HASH(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 选出最短的倒排列表
	int count = 0;
	for (int i = 1; i < num; i++) {// 与剩余列表求交
		// SVS的思想，将s挨个按表求交
		count = 0;
		int length = s.docIdList.size();
		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j] / 256;
			// 找到该hash值在当前待求交列表中对应的段
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// 该值肯定找不到，不用往后做了
				continue;
			}
			else {
				for (begin; begin <= end; begin++) {
					if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
						// 在该段中找到了当前值
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin]) {
						break;
					}
				}
				if (isFind) {
					// 覆盖
					s.docIdList[count++] = s.docIdList[j];
				}
			}
		}
		if (count < length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
	}

	return s;
}

InvertedIndex HASH_MPI(int* queryList, vector<InvertedIndex>& index, int num, int rank, int size) {
	// 取最短的列表
	int initialLength = index[queryList[0]].length;
	InvertedIndex s;// 取工作范围1/4
	int start = (initialLength / size) * rank,
		end = rank != size - 1 ? (initialLength / size) * (rank + 1) : initialLength;// 末尾特殊处理
	s.docIdList.assign(index[queryList[0]].docIdList.begin() + start, index[queryList[0]].docIdList.begin() + end);
	s.length = s.docIdList.size();

	for (int i = 1; i < num; i++) {// 与剩余列表求交
		// SVS的思想，将s挨个按表求交
		int count = 0;
		int length = s.length;

		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j] / 256;
			// 找到该hash值在当前待求交列表中对应的段
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// 该值肯定找不到，不用往后做了
				continue;
			}
			else {
				for (begin; begin <= end; begin++) {
					if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
						// 在该段中找到了当前值
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin]) {
						break;
					}
				}
				if (isFind) {
					// 覆盖
					s.docIdList[count++] = s.docIdList[j];
				}
			}
		}
		if (count < length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}

	// 进程通信
	// 用0号进程做收集
	if (rank == 0) {
		for (int i = 1; i < size; i++) {
			int count;
			MPI_Recv(&count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			unsigned int* temp;// 转成int数组发送
			temp = new unsigned int[count];
			// 接受数据
			MPI_Recv(temp, count, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			s.docIdList.insert(s.docIdList.end(), temp, temp + count);
			delete[]temp;
		}
		s.length = s.docIdList.size();
	}
	// 非0号线程则把vector发送过去
	else {
		unsigned int* temp;// 转成int数组发送
		temp = new unsigned[s.length];
		copy(s.docIdList.begin(), s.docIdList.end(), temp);
		MPI_Send(&s.length, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);// 先发一个长度过去
		MPI_Send(temp, s.length, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD);// 再把数据发过去
		delete[]temp;
	}

	return s;
}

InvertedIndex HASH_omp(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// 选出最短的倒排列表
	int count = 0;
#pragma omp parallel num_threads(NUM_THREADS),shared(count)
	for (int i = 1; i < num; i++) {// 与剩余列表求交
		// SVS的思想，将s挨个按表求交
		count = 0;
		int length = s.docIdList.size();
#pragma omp for 
		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j] / 64;
			// 找到该hash值在当前待求交列表中对应的段
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// 该值肯定找不到，不用往后做了
				continue;
			}
			else {
				for (begin; begin <= end; begin++) {
					if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
						// 在该段中找到了当前值
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin]) {
						break;
					}
				}
#pragma omp critical
				if (isFind) {
					// 覆盖
					s.docIdList[count++] = s.docIdList[j];
				}
			}
		}
#pragma omp single
		if (count < length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
	}

	return s;
}