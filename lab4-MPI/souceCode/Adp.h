#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include<omp.h>
using namespace std;
#define NUM_THREADS 2
class InvertedIndex {// 倒排索引结构
public:
	int length = 0;
	vector<unsigned int> docIdList;
};

// adp实现
class QueryItem {
public:
	int cursor;// 当前读到哪了
	int end;// 倒排索引总长度
	int key;// 关键字值
};
bool operator<(const QueryItem& q1, const QueryItem& q2) {// 选剩余元素最少的元素
	return (q1.end - q1.cursor) < (q2.end - q2.cursor);
}
InvertedIndex ADP(int* queryList, vector<InvertedIndex>& index, int num)
{
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].end = index[queryList[i]].docIdList.size();
	}
	for (int i = list[0].cursor; i < list[0].end; i++) {// 最短的列表非空
		bool isFind = true;
		unsigned int e = index[list[0].key].docIdList[i];
		for (int s = 1; s != num && isFind == true; s++) {
			isFind = false;
			while (list[s].cursor < list[s].end) {// 检查s列表
				if (e == index[list[s].key].docIdList[list[s].cursor]) {
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])
					break;
				list[s].cursor++;// 当前访问过，且没找到合适的，往后移
			}
			// 下一个链表
		}
		// 当前元素已被访问过
		if (isFind)
			S.docIdList.push_back(e);
		//sort(list, list + num);// 重排，将未探查元素少的列表前移
	}
	return S;
}

InvertedIndex ADP_MPI(int* queryList, vector<InvertedIndex>& index, int num, int rank, int proNum)
{
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理
	{
		list[i].key = queryList[i];
		list[i].end = index[queryList[i]].docIdList.size();
		list[i].cursor = 0;
	}

	list[0].cursor = list[0].end / proNum * rank;
	int maxIndex = rank != proNum - 1 ? list[0].end / proNum * (rank + 1) : list[0].end;
	for (int i = list[0].cursor; i < maxIndex; i++) // 最短的列表非空
	{
		bool isFind = true;
		unsigned int e = index[list[0].key].docIdList[i];
		for (int s = 1; s != num && isFind == true; s++)
		{
			isFind = false;
			while (list[s].cursor < list[s].end)
			{// 检查s列表
				if (e == index[list[s].key].docIdList[list[s].cursor])
				{
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])
					break;
				list[s].cursor++;// 当前访问过，且没找到合适的，往后移
			}
			// 下一个链表
		}
		// 当前元素已被访问过
		if (isFind)
			S.docIdList.push_back(e);
		//sort(list, list + num);// 重排，将未探查元素少的列表前移
	}
	InvertedIndex result;
	// 用0号进程做收集
	if (rank == 0)
	{
		for (int i = 1; i < proNum; i++)
		{
			int count;
			MPI_Recv(&count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			unsigned int* temp;// 转成数组发送
			temp = new unsigned int[count];
			MPI_Recv(temp, count, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			S.docIdList.insert(S.docIdList.end(), temp, temp + count);
			delete[]temp;
		}
	}
	// 非0号线程则把vector发送过去
	else {
		int count = S.docIdList.size();
		MPI_Send(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);// 先发一个长度过去
		//复制数据到数组发送
		unsigned int* temp;
		temp = new unsigned[S.docIdList.size()];
		copy(S.docIdList.begin(), S.docIdList.end(), temp);
		MPI_Send(temp, count, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD);// 再把数据发过去
		delete[]temp;
	}
	return S;
}

InvertedIndex ADP_MPI2(int* queryList, vector<InvertedIndex>& index, int num, int rank, int proNum)
{
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理
	{
		list[i].key = queryList[i];
		list[i].end = index[queryList[i]].docIdList.size();
		list[i].cursor = 0;
	}
	list[0].cursor = list[0].end / proNum * rank;
	int maxIndex = rank != proNum - 1 ? list[0].end / proNum * (rank + 1) : list[0].end;
	//求所有进程可能的最大长度
	unsigned int maxLen = max(list[0].end / proNum, list[0].end - list[0].end / proNum * (proNum - 1));
	for (int i = list[0].cursor; i < maxIndex; i++) // 最短的列表非空
	{
		bool isFind = true;
		unsigned int e = index[list[0].key].docIdList[i];
		for (int s = 1; s != num && isFind == true; s++)
		{
			isFind = false;
			while (list[s].cursor < list[s].end)
			{// 检查s列表
				if (e == index[list[s].key].docIdList[list[s].cursor])
				{
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])
					break;
				list[s].cursor++;// 当前访问过，且没找到合适的，往后移
			}
			// 下一个链表
		}
		// 当前元素已被访问过
		if (isFind)
			S.docIdList.push_back(e);
	}
	InvertedIndex result;
	// 用0号进程做收集
	if (rank == 0)
	{
		for (int i = 1; i < proNum; i++)
		{
			int count;
			MPI_Recv(&count, 1, MPI_INT, i, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			unsigned int* temp;// 转成数组发送
			temp = new unsigned int[count];
			MPI_Recv(temp, count, MPI_UNSIGNED, i, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			S.docIdList.insert(S.docIdList.end(), temp, temp + count);
			delete[]temp;
		}
	}
	// 非0号线程则把vector发送过去
	else {
		int count = S.docIdList.size();
		MPI_Send(&count, 1, MPI_INT, 0, 6, MPI_COMM_WORLD);// 先发一个长度过去
		//复制数据到数组发送
		unsigned int* temp;
		temp = new unsigned[S.docIdList.size()];
		copy(S.docIdList.begin(), S.docIdList.end(), temp);
		MPI_Send(temp, count, MPI_UNSIGNED, 0, 7, MPI_COMM_WORLD);// 再把数据发过去
		delete[]temp;
	}

	return S;
}

InvertedIndex ADP_MPI_Precis(int* queryList, vector<InvertedIndex>& index, int num, int rank, int proNum)
{
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理,关键字赋值
		list[i].key = queryList[i];

	//每个进程的第一个倒排链表的开始位置和结束位置
	int list0Len = index[list[0].key].docIdList.size();
	list[0].cursor = list0Len / proNum * rank;
	list[0].end = rank != proNum - 1 ? list0Len / proNum * (rank + 1) : list0Len;

	//找到剩余倒排链表的起始位置和结束位置
	int* start = new int[num - 1];
	int* end = new int[num - 1];
	for (int i = 1; i < num; i++)
	{
		//找到自己的开始位置
		start[i - 1] = find1stGreaterEqual(index[list[i].key].docIdList, index[list[0].key].docIdList[list[0].cursor]);
		list[i].cursor = start[i - 1];
	}
	if (rank != proNum - 1)//最后一个进程不需要知道自己何时结束，因为它直接扫描到最后
	{
		MPI_Recv(end, num - 1, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);//从下一个进程接受该进程的起始位置
		for (int i = 1; i < num; i++)
		{
			list[i].end = end[i - 1];//下一个进程的开始正是这个进程的结束位置
		}
	}
	else
	{
		for (int i = 1; i < num; i++)//最后一个进程做到底
		{
			list[i].end = index[list[i].key].docIdList.size();
		}
	}
	if (rank != 0) //第一个进程不用发送信息，其余发送给前一个进程
	{
		MPI_Send(start, num - 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD);
	}


	for (int i = list[0].cursor; i < list[0].end; i++) // 在自己的工作范围查找
	{
		bool isFind = true;
		unsigned int e = index[list[0].key].docIdList[i];
		for (int s = 1; s != num && isFind == true; s++)
		{
			isFind = false;
			while (list[s].cursor < list[s].end)
			{// 检查s列表
				if (e == index[list[s].key].docIdList[list[s].cursor])
				{
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])
					break;
				list[s].cursor++;// 当前访问过，且没找到合适的，往后移
			}
			// 下一个链表
		}
		// 当前元素已被访问过
		if (isFind)
			S.docIdList.push_back(e);
		//sort(list, list + num);// 重排，将未探查元素少的列表前移
	}
	InvertedIndex result;
	// 用0号进程做收集
	if (rank == 0)
	{
		for (int i = 1; i < proNum; i++)
		{
			int count;
			MPI_Recv(&count, 1, MPI_INT, i, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			unsigned int* temp;// 转成数组发送
			temp = new unsigned int[count];
			MPI_Recv(temp, count, MPI_UNSIGNED, i, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			S.docIdList.insert(S.docIdList.end(), temp, temp + count);
			delete[]temp;
		}
	}
	// 非0号线程则把vector发送过去
	else {
		int count = S.docIdList.size();
		MPI_Send(&count, 1, MPI_INT, 0, 6, MPI_COMM_WORLD);// 先发一个长度过去
		//复制数据到数组发送
		unsigned int* temp;
		temp = new unsigned[S.docIdList.size()];
		copy(S.docIdList.begin(), S.docIdList.end(), temp);
		MPI_Send(temp, count, MPI_UNSIGNED, 0, 7, MPI_COMM_WORLD);// 再把数据发过去
		delete[]temp;
	}
	return S;
}

InvertedIndex ADP_omp(int* queryList, vector<InvertedIndex>& index, int num)
{
	InvertedIndex S;
#pragma omp parallel num_threads(NUM_THREADS)
	{
		QueryItem* list = new QueryItem[num]();
		for (int i = 0; i < num; i++)// 预处理
		{
			list[i].cursor = 0;
			list[i].key = queryList[i];
			list[i].end = index[queryList[i]].docIdList.size();
		}

#pragma omp for 
		for (int i = list[0].cursor; i < list[0].end; i++) {// 最短的列表非空
			bool isFind = true;
			unsigned int e = index[list[0].key].docIdList[i];
			for (int s = 1; s != num && isFind == true; s++) {
				isFind = false;
				while (list[s].cursor < list[s].end) {// 检查s列表
					if (e == index[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < index[list[s].key].docIdList[list[s].cursor])
						break;
					list[s].cursor++;// 当前访问过，且没找到合适的，往后移
				}
				// 下一个链表
			}
#pragma omp critical
			// 当前元素已被访问过
			if (isFind)
				S.docIdList.push_back(e);
			//sort(list, list + num);// 重排，将未探查元素少的列表前移
		}
	}
	return S;
}

