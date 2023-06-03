#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include<omp.h>
#include"Adp.h"
#include <mpi.h>

bool isParallel_in = false;
// hash�ֶ�ʵ��
struct HashBucket {// һ��hash�Σ���¼���ڵ����б��е���ʼλ��
	int begin = -1;
	int end = -1;
};
HashBucket** hashBucket;
// Ԥ�����������б����hash����
void preprocessing(vector<InvertedIndex>& index, int count) {
	hashBucket = new HashBucket * [count];
	for (int i = 0; i < count; i++)
		hashBucket[i] = new HashBucket[101600];// 26000000/65536

	for (int i = 0; i < count; i++) {
		for (int j = 0; j < index[i].docIdList.size(); j++) {
			int value = index[i].docIdList[j];// �ó���ǰ�б��j��ֵ
			int hashValue = value / 256;// TODO:check
			if (hashBucket[i][hashValue].begin == -1) // �ö��ڻ�û��Ԫ��
				hashBucket[i][hashValue].begin = j;
			hashBucket[i][hashValue].end = j;// ��ӵ��ö�β��
		}
	}
}
InvertedIndex HASH(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// ѡ����̵ĵ����б�
	int count = 0;
	for (int i = 1; i < num; i++) {// ��ʣ���б���
		// SVS��˼�룬��s����������
		count = 0;
		int length = s.docIdList.size();
		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j] / 256;
			// �ҵ���hashֵ�ڵ�ǰ�����б��ж�Ӧ�Ķ�
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// ��ֵ�϶��Ҳ�����������������
				continue;
			}
			else {
				for (begin; begin <= end; begin++) {
					if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
						// �ڸö����ҵ��˵�ǰֵ
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin]) {
						break;
					}
				}
				if (isFind) {
					// ����
					s.docIdList[count++] = s.docIdList[j];
				}
			}
		}
		if (count < length)// ������ɾ��
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
	}

	return s;
}

InvertedIndex HASH_MPI(int* queryList, vector<InvertedIndex>& index, int num, int rank, int size) {
	// ȡ��̵��б�
	int initialLength = index[queryList[0]].length;
	InvertedIndex s;// ȡ������Χ1/4
	int start = (initialLength / size) * rank,
		end = rank != size - 1 ? (initialLength / size) * (rank + 1) : initialLength;// ĩβ���⴦��
	s.docIdList.assign(index[queryList[0]].docIdList.begin() + start, index[queryList[0]].docIdList.begin() + end);
	s.length = s.docIdList.size();

	for (int i = 1; i < num; i++) {// ��ʣ���б���
		// SVS��˼�룬��s����������
		int count = 0;
		int length = s.length;

		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j] / 256;
			// �ҵ���hashֵ�ڵ�ǰ�����б��ж�Ӧ�Ķ�
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// ��ֵ�϶��Ҳ�����������������
				continue;
			}
			else {
				for (begin; begin <= end; begin++) {
					if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
						// �ڸö����ҵ��˵�ǰֵ
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin]) {
						break;
					}
				}
				if (isFind) {
					// ����
					s.docIdList[count++] = s.docIdList[j];
				}
			}
		}
		if (count < length)// ������ɾ��
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}

	// ����ͨ��
	// ��0�Ž������ռ�
	if (rank == 0) {
		for (int i = 1; i < size; i++) {
			int count;
			MPI_Recv(&count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			unsigned int* temp;// ת��int���鷢��
			temp = new unsigned int[count];
			// ��������
			MPI_Recv(temp, count, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			s.docIdList.insert(s.docIdList.end(), temp, temp + count);
			delete[]temp;
		}
		s.length = s.docIdList.size();
	}
	// ��0���߳����vector���͹�ȥ
	else {
		unsigned int* temp;// ת��int���鷢��
		temp = new unsigned[s.length];
		copy(s.docIdList.begin(), s.docIdList.end(), temp);
		MPI_Send(&s.length, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);// �ȷ�һ�����ȹ�ȥ
		MPI_Send(temp, s.length, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD);// �ٰ����ݷ���ȥ
		delete[]temp;
	}

	return s;
}

InvertedIndex HASH_omp(int* queryList, vector<InvertedIndex>& index, int num) {
	InvertedIndex s = index[queryList[0]];// ѡ����̵ĵ����б�
	int count = 0;
#pragma omp parallel num_threads(NUM_THREADS),shared(count)
	for (int i = 1; i < num; i++) {// ��ʣ���б���
		// SVS��˼�룬��s����������
		count = 0;
		int length = s.docIdList.size();
#pragma omp for 
		for (int j = 0; j < length; j++) {
			bool isFind = false;
			int hashValue = s.docIdList[j] / 64;
			// �ҵ���hashֵ�ڵ�ǰ�����б��ж�Ӧ�Ķ�
			int begin = hashBucket[queryList[i]][hashValue].begin;
			int end = hashBucket[queryList[i]][hashValue].end;
			if (begin < 0 || end < 0) {// ��ֵ�϶��Ҳ�����������������
				continue;
			}
			else {
				for (begin; begin <= end; begin++) {
					if (s.docIdList[j] == index[queryList[i]].docIdList[begin]) {
						// �ڸö����ҵ��˵�ǰֵ
						isFind = true;
						break;
					}
					else if (s.docIdList[j] < index[queryList[i]].docIdList[begin]) {
						break;
					}
				}
#pragma omp critical
				if (isFind) {
					// ����
					s.docIdList[count++] = s.docIdList[j];
				}
			}
		}
#pragma omp single
		if (count < length)// ������ɾ��
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
	}

	return s;
}