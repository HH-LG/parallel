#include <stdio.h>
#include <mpi.h>
#include "util.h"
#include<fstream>
#include<string>
#include<vector>
#include<sstream>
#include<Windows.h>
#include<algorithm>
#include"Adp.h"
//#include"BitMap.h"
//#include"Hash.h"
#include"SvS.h"
#include"SvS_SSE.h"
#include"Adp_SSE.h"
using namespace std;


// ���رȽϷ����Գ��������������
bool operator<(const InvertedIndex& i1, const InvertedIndex& i2) {
	return i1.length < i2.length;
}


// �ѵ����б���������
void sorted(int* list, vector<InvertedIndex>& idx, int num) {
	for (int i = 0; i < num - 1; i++) {
		for (int j = 0; j < num - i - 1; j++) {
			if (idx[list[j]].length > idx[list[j + 1]].length) {
				int tmp = list[j];
				list[j] = list[j + 1];
				list[j + 1] = tmp;
			}
		}
	}
}

int main(int argc, char* argv[])
{
	// ��ȡ�������ļ�
	fstream file;
	file.open("ExpIndex.bin", ios::binary | ios::in);
	if (!file.is_open()) {
		printf("Wrong in opening file!\n");
		return 0;

	}
	static vector<InvertedIndex>* invertedLists = new vector<InvertedIndex>();
	for (int i = 0; i < 2000; i++)		//�ܹ���ȡ2000����������
	{
		InvertedIndex* t = new InvertedIndex();				//һ����������
		file.read((char*)&t->length, sizeof(t->length));
		for (int j = 0; j < t->length; j++)
		{
			unsigned int docId;			//�ļ�id
			file.read((char*)&docId, sizeof(docId));
			t->docIdList.push_back(docId);//���������ű�
		}
		sort(t->docIdList.begin(), t->docIdList.end());//���ĵ��������
		invertedLists->push_back(*t);		//����һ�����ű�
	}
	file.close();

	// ��ȡ��ѯ����
	file.open("ExpQuery.txt", ios::in);
	static int query[1000][5] = { 0 };// ������ѯ���5��docId,ȫ����ȡ
	string line;
	int count = 0;

	while (getline(file, line)) {// ��ȡһ��
		stringstream ss; // �ַ���������
		ss << line; // ������һ��
		int i = 0;
		int temp;
		ss >> temp;
		while (!ss.eof()) {
			query[count][i] = temp;
			i++;
			ss >> temp;
		}
		count++;// �ܲ�ѯ��
	}


	//Ԥ����
	//preprocessing(*invertedLists, 2000);
	//------��ʼ��MPI------ 

	int requr = MPI_THREAD_MULTIPLE, provd;
	MPI_Init(&argc, &argv);

	int rank, proNum;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &proNum);

	//------��------
	long long head, tail, freq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time

	//int length = floor(count / proNum);// ÿ�����̴����length
	//int start = length * rank;
	//int end = min(length * (rank + 1), count);// ����ĩβ

	//preprocessing(*invertedLists, 2000);
	for (int i = 0; i < count; i++) {// count����ѯ
		int num = 0;// query��ѯ����
		for (int j = 0; j < 5; j++) {
			if (query[i][j] != 0) {
				num++;
			}
		}
		int* list = new int[num];// Ҫ�����list
		for (int j = 0; j < num; j++) {
			list[j] = query[i][j];
		}
		sorted(list, *(invertedLists), num);// ����������
		
		InvertedIndex res = SVS(list, *invertedLists, num);
		//printf("%d\n", i);
	}

	//MPI_Barrier(MPI_COMM_WORLD);// ����ͬ��

	// �ý������
	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
	printf("%f\n", (tail - head) * 1000.0 / freq);

	MPI_Finalize();
}

//int main(int argc, char* argv[])
//{
//	// ��ȡ�������ļ�
//	fstream file;
//	file.open("ExpIndex.bin", ios::binary | ios::in);
//	if (!file.is_open()) {
//		printf("Wrong in opening file!\n");
//		return 0;
//
//	}
//	static vector<InvertedIndex>* invertedLists = new vector<InvertedIndex>();
//	for (int i = 0; i < 2000; i++)		//�ܹ���ȡ2000����������
//	{
//		InvertedIndex* t = new InvertedIndex();				//һ����������
//		file.read((char*)&t->length, sizeof(t->length));
//		for (int j = 0; j < t->length; j++)
//		{
//			unsigned int docId;			//�ļ�id
//			file.read((char*)&docId, sizeof(docId));
//			t->docIdList.push_back(docId);//���������ű�
//		}
//		sort(t->docIdList.begin(), t->docIdList.end());//���ĵ��������
//		invertedLists->push_back(*t);		//����һ�����ű�
//	}
//	file.close();
//
//	// ��ȡ��ѯ����
//	file.open("ExpQuery.txt", ios::in);
//	static int query[1000][5] = { 0 };// ������ѯ���5��docId,ȫ����ȡ
//	string line;
//	int count = 0;
//
//	while (getline(file, line)) {// ��ȡһ��
//		stringstream ss; // �ַ���������
//		ss << line; // ������һ��
//		int i = 0;
//		int temp;
//		ss >> temp;
//		while (!ss.eof()) {
//			query[count][i] = temp;
//			i++;
//			ss >> temp;
//		}
//		count++;// �ܲ�ѯ��
//	}
//
//	//------��ʼ��MPI------
//	//int provided;// ���߳�
//	//MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
//	//if (provided < MPI_THREAD_FUNNELED)// �ṩ�ĵȼ�����
//	//	MPI_Abort(MPI_COMM_WORLD, 1);
//
//	//Ԥ����
//	//preprocessing(*invertedLists, 2000);
//	//bitMapProcessing(*invertedLists, 2000);
//	MPI_Init(&argc, &argv);
//	int rank, size;
//	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//	MPI_Comm_size(MPI_COMM_WORLD, &size);
//
//	//------��------
//	long long head, tail, freq;
//
//	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
//	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time
//
//
//	int length = floor(count / size);// ÿ�����̴����length
//	int start = length * rank;
//	int end = rank != size - 1 ? length * (rank + 1) : count;// ����ĩβ
//
////#pragma omp parallel for num_threads(NUM_THREADS)
//	for (int i = start; i < end; i++) {// count����ѯ
//		int num = 0;// query��ѯ����
//		for (int j = 0; j < 5; j++) {
//			if (query[i][j] != 0) {
//				num++;
//			}
//		}
//		int* list = new int[num];// Ҫ�����list
//		for (int j = 0; j < num; j++) {
//			list[j] = query[i][j];
//		}
//		sorted(list, *(invertedLists), num);// ����������
//		InvertedIndex res = ADP_omp(list, *invertedLists, num);
//	}
//
//	//MPI_Barrier(MPI_COMM_WORLD);// ����ͬ��
//	//// ������ͨ��������
//	//int temp = 0;
//	//if (rank == 0) {
//	//	for (int i = 1; i < size; i++)
//	//		MPI_Recv(&temp, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//	//}
//	//else {
//	//	MPI_Send(&temp, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
//	//}
//
//	// ��һ���������
//	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
//	printf("%f\n", (tail - head) * 1000.0 / freq);
//
//
//	MPI_Finalize();
//}