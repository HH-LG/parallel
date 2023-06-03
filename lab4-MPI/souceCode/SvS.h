#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include<omp.h>
#include<mpi.h>
#include"util.h"
#include"Adp.h"
using namespace std;

// svs实现
InvertedIndex SVS(int* queryList, vector<InvertedIndex>& index, int num)
{
	InvertedIndex s = index[queryList[0]];// 取最短的列表
	int count = 0;
	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		count = 0;// s从头往后遍历一遍
		int t = 0;
		// s列表中的每个元素都拿出来比较
		for (int j = 0; j < s.docIdList.size(); j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			for (; t < index[queryList[i]].docIdList.size(); t++) {
				// 遍历i列表中所有元素
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
					break;
			}
			if (isFind)
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.docIdList.size())
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
	}
	return s;
}

InvertedIndex SVS_MPI(int* queryList, vector<InvertedIndex>& index, int num, int rank, int size) {
	InvertedIndex s = index[queryList[0]];// 取最短的列表
	int initialLength = s.length;

	// 获取工作范围
	int begin = rank * (initialLength / size), end = min((rank + 1) * (initialLength / size), initialLength);

	// 与剩余列表求交
	for (int i = 1; i < num; i++)
	{
		int t = 0;

		// 预处理加速大概率事件
		t = rank * (index[queryList[i]].length / size);
		for (int j = 2 * rank; j > 0; j--) {
			if (s.docIdList[begin] < index[queryList[i]].docIdList[t])
				t -= (index[queryList[i]].length / size) / 2;
			else break;
		}

		// s列表中的每个元素都拿出来比较
		for (int j = begin; j < end; j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			for (; t < index[queryList[i]].length; t++) {
				// 遍历i列表中所有元素
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
					break;
			}
			if (!isFind)// 没找到合适的，该位置置为0
			{
				s.docIdList[j] = INT_MAX;
			}
		}
	}

	InvertedIndex result;
	// 用0号进程做收集
	if (rank == 0) {
		// 把该部分求交有效的拿出来->为了通信
		for (int i = begin; i < end; i++) {
			if (s.docIdList[i] != INT_MAX) {
				result.docIdList.push_back(s.docIdList[i]);
				result.length++;
			}
		}

		for (int i = 1; i < size; i++) {
			int count;
			MPI_Recv(&count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			unsigned int* temp;// 转成int数组发送
			temp = new unsigned int[count];

			MPI_Recv(temp, count, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			for (int j = 0; j < initialLength / size; j++) {
				result.docIdList.push_back(temp[j]);
			}
			delete[]temp;
		}
		result.length = result.docIdList.size();
	}
	// 非0号线程则把vector发送过去
	else {
		unsigned int* temp;// 转成int数组发送
		temp = new unsigned[initialLength / size];
		int count = 0;
		for (int i = begin; i < end; i++) {
			if (s.docIdList[i] != INT_MAX) {
				temp[count++] = s.docIdList[i];
			}
		}
		MPI_Send(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);// 先发一个长度过去
		MPI_Send(temp, count, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD);// 再把数据发过去
		delete[]temp;
	}


	return s;
}

InvertedIndex SVS_MPI2(int* queryList, vector<InvertedIndex>& index, int num, int rank, int size) {
	// 取最短的列表
	int initialLength = index[queryList[0]].length;
	InvertedIndex s;// 取工作范围1/4
	int start = (initialLength / size) * rank, end = min((initialLength / size) * (rank + 1), initialLength);
	s.docIdList.assign(index[queryList[0]].docIdList.begin() + start, index[queryList[0]].docIdList.begin() + end);
	s.length = s.docIdList.size();

	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		int count = 0;// s从头往后遍历一遍
		int t = 0;

		if (s.length == 0)break;
		// 预处理加速大概率事件
		t = rank * (index[queryList[i]].length / size);
		for (int j = 2 * rank; j > 0; j--) {
			if (s.docIdList[0] < index[queryList[i]].docIdList[t])
				t -= (index[queryList[i]].length / size) / 2;
			else break;
		}

		// s列表中的每个元素都拿出来比较
		for (int j = 0; j < s.length; j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			for (; t < index[queryList[i]].length; t++) {
				// 遍历i列表中所有元素
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
					break;
			}
			if (isFind)// 覆盖
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}

	// 进程通信
	// 用0号进程做收集
	//if (rank == 0) {
	//	for (int i = 1; i < size; i++) {
	//		int count;
	//		MPI_Recv(&count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	//		unsigned int* temp;// 转成int数组发送
	//		temp = new unsigned int[count];
	//		// 接受数据
	//		MPI_Recv(temp, count, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	//		s.docIdList.insert(s.docIdList.end(), temp, temp + count);
	//		delete[]temp;
	//	}
	//	s.length = s.docIdList.size();
	//}
	//// 非0号线程则把vector发送过去
	//else {
	//	unsigned int* temp;// 转成int数组发送
	//	temp = new unsigned[s.length];
	//	copy(s.docIdList.begin(), s.docIdList.end(), temp);
	//	MPI_Send(&s.length, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);// 先发一个长度过去
	//	MPI_Send(temp, s.length, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD);// 再把数据发过去
	//	delete[]temp;
	//}

	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0) {
		unsigned int* temp;// 转成int数组发送
		//int maxLen = max(initialLength / size, initialLength - initialLength / size * (size - 1));
		temp = new unsigned int[initialLength];// 节省空间实测比开个大的还慢？
		for (int i = 1; i < size; i++) {
			// 接受数据
			MPI_Recv(temp, initialLength, MPI_UNSIGNED, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			int len = temp[0];
			s.docIdList.insert(s.docIdList.end(), temp + 1, temp + 1 + len);
		}
		delete[]temp;
		s.length = s.docIdList.size();
	}
	// 非0号线程则把vector发送过去
	else {
		unsigned int* temp;// 转成int数组发送
		temp = new unsigned[s.length + 1];
		temp[0] = s.length;
		copy(s.docIdList.begin(), s.docIdList.end(), temp + 1);
		// 直接一起发过去
		MPI_Send(temp, s.length + 1, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD);// 再把数据发过去
		delete[]temp;
	}
	return s;
}

InvertedIndex SVS_MPI_Precis(int* queryList, vector<InvertedIndex>& index, int num, int rank, int size) {
	//每个进程的最短倒排链表的开始位置和结束位置
	int initialLength = index[queryList[0]].docIdList.size();
	InvertedIndex s;// 取工作范围1/4
	int start = (initialLength / size) * rank,
		end = rank != size - 1 ? (initialLength / size) * (rank + 1) : initialLength;// 末尾特殊处理
	s.docIdList.assign(index[queryList[0]].docIdList.begin() + start, index[queryList[0]].docIdList.begin() + end);
	s.length = s.docIdList.size();

	// 构造数据结构保存其余链表求交范围
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理,关键字赋值
		list[i].key = queryList[i];
	//找到剩余倒排链表的起始位置和结束位置
	int* from = new int[num - 1];
	int* to = new int[num - 1];
	for (int i = 1; i < num; i++) {
		//找到自己的开始位置
		from[i - 1] = find1stGreaterEqual(index[list[i].key].docIdList, index[list[0].key].docIdList[start]);
		list[i].cursor = from[i - 1];

	}
	if (rank != size - 1) {
		//最后一个进程不需要知道自己何时结束，因为它直接扫描到最后
		MPI_Recv(to, num - 1, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);//从下一个进程接受该进程的起始位置
		for (int i = 1; i < num; i++) {
			list[i].end = to[i - 1];//下一个进程的开始正是这个进程的结束位置
		}
	}
	else {
		for (int i = 1; i < num; i++) {
			//最后一个进程做到底
			list[i].end = index[list[i].key].docIdList.size();
		}
	}
	if (rank != 0) //第一个进程不用发送信息，其余发送给前一个进程
	{
		MPI_Send(from, num - 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD);
	}


	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		int count = 0;// s从头往后遍历一遍

		// s列表中的每个元素都拿出来比较
		for (int j = 0; j < s.length; j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			for (; list[i].cursor < list[i].end; list[i].cursor++) {
				// 遍历i列表中所有元素
				if (s.docIdList[j] == index[queryList[i]].docIdList[list[i].cursor]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[list[i].cursor])// 升序排列
					break;
			}
			if (isFind)// 覆盖
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// 最后才做删除
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

InvertedIndex SVS_MPI_Commun(int* queryList, vector<InvertedIndex>& index, int num, int rank, int size) {
	// 取最短的列表
	int initialLength = index[queryList[0]].length;
	InvertedIndex s;// 取工作范围1/4
	int start = (initialLength / size) * rank, end = min((initialLength / size) * (rank + 1), initialLength);
	s.docIdList.assign(index[queryList[0]].docIdList.begin() + start, index[queryList[0]].docIdList.begin() + end);
	s.length = s.docIdList.size();

	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		int count = 0;// s从头往后遍历一遍
		int t = 0;

		if (s.length == 0)break;
		// 预处理加速大概率事件
		t = rank * (index[queryList[i]].length / size);
		for (int j = 2 * rank; j > 0; j--) {
			if (s.docIdList[0] < index[queryList[i]].docIdList[t])
				t -= (index[queryList[i]].length / size) / 2;
			else break;
		}

		// s列表中的每个元素都拿出来比较
		for (int j = 0; j < s.length; j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			for (; t < index[queryList[i]].length; t++) {
				// 遍历i列表中所有元素
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
					break;
			}
			if (isFind)// 覆盖
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}

	// 进程通信
	vector<int> recvCounts(size); // 存储每个进程发送的数据数量
	int totalCount = 0; // 所有进程发送的总数据数量
	vector<int> displacements(size, 0); // 接收缓冲区中每个进程数据的位移量

	// 计算本进程的数据数量
	int count = s.length; // 根据实际情况获取本进程的数据数量

	// 收集每个进程的数据数量到根进程
	MPI_Gather(&count, 1, MPI_INT, recvCounts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

	// 计算总数据数量
	if (rank == 0) {
		totalCount = recvCounts[0];
		// 计算位移量
		for (int i = 1; i < size; i++) {
			totalCount += recvCounts[i];
			displacements[i] = displacements[i - 1] + recvCounts[i - 1];
		}
	}
	vector<unsigned int> receivedData(totalCount); // 存储接收到的所有数据

	// 发送本进程的数据给进程0
	MPI_Gatherv(s.docIdList.data(), count, MPI_UNSIGNED, receivedData.data(), recvCounts.data(), displacements.data(), MPI_UNSIGNED, 0, MPI_COMM_WORLD);

	return s;
}

InvertedIndex SVS_MPI_PreCom(int* queryList, vector<InvertedIndex>& index, int num, int rank, int size) {
	//每个进程的最短倒排链表的开始位置和结束位置
	int initialLength = index[queryList[0]].docIdList.size();
	InvertedIndex s;// 取工作范围1/4
	int start = (initialLength / size) * rank,
		end = rank != size - 1 ? (initialLength / size) * (rank + 1) : initialLength;// 末尾特殊处理
	s.docIdList.assign(index[queryList[0]].docIdList.begin() + start, index[queryList[0]].docIdList.begin() + end);
	s.length = s.docIdList.size();

	// 构造数据结构保存其余链表求交范围
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// 预处理,关键字赋值
		list[i].key = queryList[i];
	//找到剩余倒排链表的起始位置和结束位置
	int* from = new int[num - 1];
	int* to = new int[num - 1];
	for (int i = 1; i < num; i++) {
		//找到自己的开始位置
		from[i - 1] = find1stGreaterEqual(index[list[i].key].docIdList, index[list[0].key].docIdList[start]);
		list[i].cursor = from[i - 1];

	}
	if (rank != size - 1) {
		//最后一个进程不需要知道自己何时结束，因为它直接扫描到最后
		MPI_Recv(to, num - 1, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);//从下一个进程接受该进程的起始位置
		for (int i = 1; i < num; i++) {
			list[i].end = to[i - 1];//下一个进程的开始正是这个进程的结束位置
		}
	}
	else {
		for (int i = 1; i < num; i++) {
			//最后一个进程做到底
			list[i].end = index[list[i].key].docIdList.size();
		}
	}
	if (rank != 0) {
		//第一个进程不用发送信息，其余发送给前一个进程
		MPI_Send(from, num - 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD);
	}


	// 与剩余列表求交
	for (int i = 1; i < num; i++) {
		int count = 0;// s从头往后遍历一遍

		// s列表中的每个元素都拿出来比较
		for (int j = 0; j < s.length; j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			for (; list[i].cursor < list[i].end; list[i].cursor++) {
				// 遍历i列表中所有元素
				if (s.docIdList[j] == index[queryList[i]].docIdList[list[i].cursor]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[list[i].cursor])// 升序排列
					break;
			}
			if (isFind)// 覆盖
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// 最后才做删除
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}

	// 进程通信
	vector<int> recvCounts(size); // 存储每个进程发送的数据数量
	int totalCount = 0; // 所有进程发送的总数据数量
	vector<int> displacements(size, 0); // 接收缓冲区中每个进程数据的位移量

	// 计算本进程的数据数量
	int count = s.length; // 根据实际情况获取本进程的数据数量

	// 收集每个进程的数据数量到根进程
	MPI_Gather(&count, 1, MPI_INT, recvCounts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

	// 计算总数据数量
	if (rank == 0) {
		totalCount = recvCounts[0];
		// 计算位移量
		for (int i = 1; i < size; i++) {
			totalCount += recvCounts[i];
			displacements[i] = displacements[i - 1] + recvCounts[i - 1];
		}
	}
	vector<unsigned int> receivedData(totalCount); // 存储接收到的所有数据

	// 发送本进程的数据给进程0
	MPI_Gatherv(s.docIdList.data(), count, MPI_UNSIGNED, receivedData.data(), recvCounts.data(), displacements.data(), MPI_UNSIGNED, 0, MPI_COMM_WORLD);


	return s;
}

InvertedIndex SVS_omp(int* queryList, vector<InvertedIndex>& index, int num)
{
	InvertedIndex s = index[queryList[0]];// 取最短的列表
	int count = 0;
	// 与剩余列表求交
#pragma omp parallel num_threads(NUM_THREADS),shared(count)
	for (int i = 1; i < num; i++) {
		count = 0;// s从头往后遍历一遍
		int t = 0;
		// s列表中的每个元素都拿出来比较
#pragma omp for
		for (int j = 0; j < s.docIdList.size(); j++) {// 所有元素都得访问一遍
			bool isFind = false;// 标志，判断当前count位是否能求交

			for (; t < index[queryList[i]].docIdList.size(); t++) {
				// 遍历i列表中所有元素
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// 升序排列
					break;
			}
#pragma omp critical
			if (isFind)
				s.docIdList[count++] = s.docIdList[j];
		}
#pragma omp single
		if (count < s.docIdList.size())
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
	}
	return s;
}