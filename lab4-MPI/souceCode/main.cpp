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


// 重载比较符，以长度排序各个索引
bool operator<(const InvertedIndex& i1, const InvertedIndex& i2) {
	return i1.length < i2.length;
}


// 把倒排列表按长度排序
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
	// 读取二进制文件
	fstream file;
	file.open("ExpIndex.bin", ios::binary | ios::in);
	if (!file.is_open()) {
		printf("Wrong in opening file!\n");
		return 0;

	}
	static vector<InvertedIndex>* invertedLists = new vector<InvertedIndex>();
	for (int i = 0; i < 2000; i++)		//总共读取2000个倒排链表
	{
		InvertedIndex* t = new InvertedIndex();				//一个倒排链表
		file.read((char*)&t->length, sizeof(t->length));
		for (int j = 0; j < t->length; j++)
		{
			unsigned int docId;			//文件id
			file.read((char*)&docId, sizeof(docId));
			t->docIdList.push_back(docId);//加入至倒排表
		}
		sort(t->docIdList.begin(), t->docIdList.end());//对文档编号排序
		invertedLists->push_back(*t);		//加入一个倒排表
	}
	file.close();

	// 读取查询数据
	file.open("ExpQuery.txt", ios::in);
	static int query[1000][5] = { 0 };// 单个查询最多5个docId,全部读取
	string line;
	int count = 0;

	while (getline(file, line)) {// 读取一行
		stringstream ss; // 字符串输入流
		ss << line; // 传入这一行
		int i = 0;
		int temp;
		ss >> temp;
		while (!ss.eof()) {
			query[count][i] = temp;
			i++;
			ss >> temp;
		}
		count++;// 总查询数
	}


	//预处理
	//preprocessing(*invertedLists, 2000);
	//------初始化MPI------ 

	int requr = MPI_THREAD_MULTIPLE, provd;
	MPI_Init(&argc, &argv);

	int rank, proNum;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &proNum);

	//------求交------
	long long head, tail, freq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time

	//int length = floor(count / proNum);// 每个进程处理的length
	//int start = length * rank;
	//int end = min(length * (rank + 1), count);// 考虑末尾

	//preprocessing(*invertedLists, 2000);
	for (int i = 0; i < count; i++) {// count个查询
		int num = 0;// query查询项数
		for (int j = 0; j < 5; j++) {
			if (query[i][j] != 0) {
				num++;
			}
		}
		int* list = new int[num];// 要传入的list
		for (int j = 0; j < num; j++) {
			list[j] = query[i][j];
		}
		sorted(list, *(invertedLists), num);// 按长度排序
		
		InvertedIndex res = SVS(list, *invertedLists, num);
		//printf("%d\n", i);
	}

	//MPI_Barrier(MPI_COMM_WORLD);// 进程同步

	// 让进程输出
	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
	printf("%f\n", (tail - head) * 1000.0 / freq);

	MPI_Finalize();
}

//int main(int argc, char* argv[])
//{
//	// 读取二进制文件
//	fstream file;
//	file.open("ExpIndex.bin", ios::binary | ios::in);
//	if (!file.is_open()) {
//		printf("Wrong in opening file!\n");
//		return 0;
//
//	}
//	static vector<InvertedIndex>* invertedLists = new vector<InvertedIndex>();
//	for (int i = 0; i < 2000; i++)		//总共读取2000个倒排链表
//	{
//		InvertedIndex* t = new InvertedIndex();				//一个倒排链表
//		file.read((char*)&t->length, sizeof(t->length));
//		for (int j = 0; j < t->length; j++)
//		{
//			unsigned int docId;			//文件id
//			file.read((char*)&docId, sizeof(docId));
//			t->docIdList.push_back(docId);//加入至倒排表
//		}
//		sort(t->docIdList.begin(), t->docIdList.end());//对文档编号排序
//		invertedLists->push_back(*t);		//加入一个倒排表
//	}
//	file.close();
//
//	// 读取查询数据
//	file.open("ExpQuery.txt", ios::in);
//	static int query[1000][5] = { 0 };// 单个查询最多5个docId,全部读取
//	string line;
//	int count = 0;
//
//	while (getline(file, line)) {// 读取一行
//		stringstream ss; // 字符串输入流
//		ss << line; // 传入这一行
//		int i = 0;
//		int temp;
//		ss >> temp;
//		while (!ss.eof()) {
//			query[count][i] = temp;
//			i++;
//			ss >> temp;
//		}
//		count++;// 总查询数
//	}
//
//	//------初始化MPI------
//	//int provided;// 多线程
//	//MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
//	//if (provided < MPI_THREAD_FUNNELED)// 提供的等级不够
//	//	MPI_Abort(MPI_COMM_WORLD, 1);
//
//	//预处理
//	//preprocessing(*invertedLists, 2000);
//	//bitMapProcessing(*invertedLists, 2000);
//	MPI_Init(&argc, &argv);
//	int rank, size;
//	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//	MPI_Comm_size(MPI_COMM_WORLD, &size);
//
//	//------求交------
//	long long head, tail, freq;
//
//	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
//	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time
//
//
//	int length = floor(count / size);// 每个进程处理的length
//	int start = length * rank;
//	int end = rank != size - 1 ? length * (rank + 1) : count;// 考虑末尾
//
////#pragma omp parallel for num_threads(NUM_THREADS)
//	for (int i = start; i < end; i++) {// count个查询
//		int num = 0;// query查询项数
//		for (int j = 0; j < 5; j++) {
//			if (query[i][j] != 0) {
//				num++;
//			}
//		}
//		int* list = new int[num];// 要传入的list
//		for (int j = 0; j < num; j++) {
//			list[j] = query[i][j];
//		}
//		sorted(list, *(invertedLists), num);// 按长度排序
//		InvertedIndex res = ADP_omp(list, *invertedLists, num);
//	}
//
//	//MPI_Barrier(MPI_COMM_WORLD);// 进程同步
//	//// 用阻塞通信试试呢
//	//int temp = 0;
//	//if (rank == 0) {
//	//	for (int i = 1; i < size; i++)
//	//		MPI_Recv(&temp, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//	//}
//	//else {
//	//	MPI_Send(&temp, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
//	//}
//
//	// 让一个进程输出
//	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
//	printf("%f\n", (tail - head) * 1000.0 / freq);
//
//
//	MPI_Finalize();
//}