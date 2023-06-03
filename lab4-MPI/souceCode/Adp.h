#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include<omp.h>
using namespace std;
#define NUM_THREADS 2
class InvertedIndex {// ���������ṹ
public:
	int length = 0;
	vector<unsigned int> docIdList;
};

// adpʵ��
class QueryItem {
public:
	int cursor;// ��ǰ��������
	int end;// ���������ܳ���
	int key;// �ؼ���ֵ
};
bool operator<(const QueryItem& q1, const QueryItem& q2) {// ѡʣ��Ԫ�����ٵ�Ԫ��
	return (q1.end - q1.cursor) < (q2.end - q2.cursor);
}
InvertedIndex ADP(int* queryList, vector<InvertedIndex>& index, int num)
{
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// Ԥ����
	{
		list[i].cursor = 0;
		list[i].key = queryList[i];
		list[i].end = index[queryList[i]].docIdList.size();
	}
	for (int i = list[0].cursor; i < list[0].end; i++) {// ��̵��б�ǿ�
		bool isFind = true;
		unsigned int e = index[list[0].key].docIdList[i];
		for (int s = 1; s != num && isFind == true; s++) {
			isFind = false;
			while (list[s].cursor < list[s].end) {// ���s�б�
				if (e == index[list[s].key].docIdList[list[s].cursor]) {
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])
					break;
				list[s].cursor++;// ��ǰ���ʹ�����û�ҵ����ʵģ�������
			}
			// ��һ������
		}
		// ��ǰԪ���ѱ����ʹ�
		if (isFind)
			S.docIdList.push_back(e);
		//sort(list, list + num);// ���ţ���δ̽��Ԫ���ٵ��б�ǰ��
	}
	return S;
}

InvertedIndex ADP_MPI(int* queryList, vector<InvertedIndex>& index, int num, int rank, int proNum)
{
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// Ԥ����
	{
		list[i].key = queryList[i];
		list[i].end = index[queryList[i]].docIdList.size();
		list[i].cursor = 0;
	}

	list[0].cursor = list[0].end / proNum * rank;
	int maxIndex = rank != proNum - 1 ? list[0].end / proNum * (rank + 1) : list[0].end;
	for (int i = list[0].cursor; i < maxIndex; i++) // ��̵��б�ǿ�
	{
		bool isFind = true;
		unsigned int e = index[list[0].key].docIdList[i];
		for (int s = 1; s != num && isFind == true; s++)
		{
			isFind = false;
			while (list[s].cursor < list[s].end)
			{// ���s�б�
				if (e == index[list[s].key].docIdList[list[s].cursor])
				{
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])
					break;
				list[s].cursor++;// ��ǰ���ʹ�����û�ҵ����ʵģ�������
			}
			// ��һ������
		}
		// ��ǰԪ���ѱ����ʹ�
		if (isFind)
			S.docIdList.push_back(e);
		//sort(list, list + num);// ���ţ���δ̽��Ԫ���ٵ��б�ǰ��
	}
	InvertedIndex result;
	// ��0�Ž������ռ�
	if (rank == 0)
	{
		for (int i = 1; i < proNum; i++)
		{
			int count;
			MPI_Recv(&count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			unsigned int* temp;// ת�����鷢��
			temp = new unsigned int[count];
			MPI_Recv(temp, count, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			S.docIdList.insert(S.docIdList.end(), temp, temp + count);
			delete[]temp;
		}
	}
	// ��0���߳����vector���͹�ȥ
	else {
		int count = S.docIdList.size();
		MPI_Send(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);// �ȷ�һ�����ȹ�ȥ
		//�������ݵ����鷢��
		unsigned int* temp;
		temp = new unsigned[S.docIdList.size()];
		copy(S.docIdList.begin(), S.docIdList.end(), temp);
		MPI_Send(temp, count, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD);// �ٰ����ݷ���ȥ
		delete[]temp;
	}
	return S;
}

InvertedIndex ADP_MPI2(int* queryList, vector<InvertedIndex>& index, int num, int rank, int proNum)
{
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// Ԥ����
	{
		list[i].key = queryList[i];
		list[i].end = index[queryList[i]].docIdList.size();
		list[i].cursor = 0;
	}
	list[0].cursor = list[0].end / proNum * rank;
	int maxIndex = rank != proNum - 1 ? list[0].end / proNum * (rank + 1) : list[0].end;
	//�����н��̿��ܵ���󳤶�
	unsigned int maxLen = max(list[0].end / proNum, list[0].end - list[0].end / proNum * (proNum - 1));
	for (int i = list[0].cursor; i < maxIndex; i++) // ��̵��б�ǿ�
	{
		bool isFind = true;
		unsigned int e = index[list[0].key].docIdList[i];
		for (int s = 1; s != num && isFind == true; s++)
		{
			isFind = false;
			while (list[s].cursor < list[s].end)
			{// ���s�б�
				if (e == index[list[s].key].docIdList[list[s].cursor])
				{
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])
					break;
				list[s].cursor++;// ��ǰ���ʹ�����û�ҵ����ʵģ�������
			}
			// ��һ������
		}
		// ��ǰԪ���ѱ����ʹ�
		if (isFind)
			S.docIdList.push_back(e);
	}
	InvertedIndex result;
	// ��0�Ž������ռ�
	if (rank == 0)
	{
		for (int i = 1; i < proNum; i++)
		{
			int count;
			MPI_Recv(&count, 1, MPI_INT, i, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			unsigned int* temp;// ת�����鷢��
			temp = new unsigned int[count];
			MPI_Recv(temp, count, MPI_UNSIGNED, i, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			S.docIdList.insert(S.docIdList.end(), temp, temp + count);
			delete[]temp;
		}
	}
	// ��0���߳����vector���͹�ȥ
	else {
		int count = S.docIdList.size();
		MPI_Send(&count, 1, MPI_INT, 0, 6, MPI_COMM_WORLD);// �ȷ�һ�����ȹ�ȥ
		//�������ݵ����鷢��
		unsigned int* temp;
		temp = new unsigned[S.docIdList.size()];
		copy(S.docIdList.begin(), S.docIdList.end(), temp);
		MPI_Send(temp, count, MPI_UNSIGNED, 0, 7, MPI_COMM_WORLD);// �ٰ����ݷ���ȥ
		delete[]temp;
	}

	return S;
}

InvertedIndex ADP_MPI_Precis(int* queryList, vector<InvertedIndex>& index, int num, int rank, int proNum)
{
	InvertedIndex S;
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// Ԥ����,�ؼ��ָ�ֵ
		list[i].key = queryList[i];

	//ÿ�����̵ĵ�һ����������Ŀ�ʼλ�úͽ���λ��
	int list0Len = index[list[0].key].docIdList.size();
	list[0].cursor = list0Len / proNum * rank;
	list[0].end = rank != proNum - 1 ? list0Len / proNum * (rank + 1) : list0Len;

	//�ҵ�ʣ�൹���������ʼλ�úͽ���λ��
	int* start = new int[num - 1];
	int* end = new int[num - 1];
	for (int i = 1; i < num; i++)
	{
		//�ҵ��Լ��Ŀ�ʼλ��
		start[i - 1] = find1stGreaterEqual(index[list[i].key].docIdList, index[list[0].key].docIdList[list[0].cursor]);
		list[i].cursor = start[i - 1];
	}
	if (rank != proNum - 1)//���һ�����̲���Ҫ֪���Լ���ʱ��������Ϊ��ֱ��ɨ�赽���
	{
		MPI_Recv(end, num - 1, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);//����һ�����̽��ܸý��̵���ʼλ��
		for (int i = 1; i < num; i++)
		{
			list[i].end = end[i - 1];//��һ�����̵Ŀ�ʼ����������̵Ľ���λ��
		}
	}
	else
	{
		for (int i = 1; i < num; i++)//���һ������������
		{
			list[i].end = index[list[i].key].docIdList.size();
		}
	}
	if (rank != 0) //��һ�����̲��÷�����Ϣ�����෢�͸�ǰһ������
	{
		MPI_Send(start, num - 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD);
	}


	for (int i = list[0].cursor; i < list[0].end; i++) // ���Լ��Ĺ�����Χ����
	{
		bool isFind = true;
		unsigned int e = index[list[0].key].docIdList[i];
		for (int s = 1; s != num && isFind == true; s++)
		{
			isFind = false;
			while (list[s].cursor < list[s].end)
			{// ���s�б�
				if (e == index[list[s].key].docIdList[list[s].cursor])
				{
					isFind = true;
					break;
				}
				else if (e < index[list[s].key].docIdList[list[s].cursor])
					break;
				list[s].cursor++;// ��ǰ���ʹ�����û�ҵ����ʵģ�������
			}
			// ��һ������
		}
		// ��ǰԪ���ѱ����ʹ�
		if (isFind)
			S.docIdList.push_back(e);
		//sort(list, list + num);// ���ţ���δ̽��Ԫ���ٵ��б�ǰ��
	}
	InvertedIndex result;
	// ��0�Ž������ռ�
	if (rank == 0)
	{
		for (int i = 1; i < proNum; i++)
		{
			int count;
			MPI_Recv(&count, 1, MPI_INT, i, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			unsigned int* temp;// ת�����鷢��
			temp = new unsigned int[count];
			MPI_Recv(temp, count, MPI_UNSIGNED, i, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			S.docIdList.insert(S.docIdList.end(), temp, temp + count);
			delete[]temp;
		}
	}
	// ��0���߳����vector���͹�ȥ
	else {
		int count = S.docIdList.size();
		MPI_Send(&count, 1, MPI_INT, 0, 6, MPI_COMM_WORLD);// �ȷ�һ�����ȹ�ȥ
		//�������ݵ����鷢��
		unsigned int* temp;
		temp = new unsigned[S.docIdList.size()];
		copy(S.docIdList.begin(), S.docIdList.end(), temp);
		MPI_Send(temp, count, MPI_UNSIGNED, 0, 7, MPI_COMM_WORLD);// �ٰ����ݷ���ȥ
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
		for (int i = 0; i < num; i++)// Ԥ����
		{
			list[i].cursor = 0;
			list[i].key = queryList[i];
			list[i].end = index[queryList[i]].docIdList.size();
		}

#pragma omp for 
		for (int i = list[0].cursor; i < list[0].end; i++) {// ��̵��б�ǿ�
			bool isFind = true;
			unsigned int e = index[list[0].key].docIdList[i];
			for (int s = 1; s != num && isFind == true; s++) {
				isFind = false;
				while (list[s].cursor < list[s].end) {// ���s�б�
					if (e == index[list[s].key].docIdList[list[s].cursor]) {
						isFind = true;
						break;
					}
					else if (e < index[list[s].key].docIdList[list[s].cursor])
						break;
					list[s].cursor++;// ��ǰ���ʹ�����û�ҵ����ʵģ�������
				}
				// ��һ������
			}
#pragma omp critical
			// ��ǰԪ���ѱ����ʹ�
			if (isFind)
				S.docIdList.push_back(e);
			//sort(list, list + num);// ���ţ���δ̽��Ԫ���ٵ��б�ǰ��
		}
	}
	return S;
}

