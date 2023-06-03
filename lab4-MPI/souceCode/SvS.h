#pragma once
#include<string>
#include<vector>
#include<algorithm>
#include<omp.h>
#include<mpi.h>
#include"util.h"
#include"Adp.h"
using namespace std;

// svsʵ��
InvertedIndex SVS(int* queryList, vector<InvertedIndex>& index, int num)
{
	InvertedIndex s = index[queryList[0]];// ȡ��̵��б�
	int count = 0;
	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		count = 0;// s��ͷ�������һ��
		int t = 0;
		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (int j = 0; j < s.docIdList.size(); j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			for (; t < index[queryList[i]].docIdList.size(); t++) {
				// ����i�б�������Ԫ��
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��������
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
	InvertedIndex s = index[queryList[0]];// ȡ��̵��б�
	int initialLength = s.length;

	// ��ȡ������Χ
	int begin = rank * (initialLength / size), end = min((rank + 1) * (initialLength / size), initialLength);

	// ��ʣ���б���
	for (int i = 1; i < num; i++)
	{
		int t = 0;

		// Ԥ������ٴ�����¼�
		t = rank * (index[queryList[i]].length / size);
		for (int j = 2 * rank; j > 0; j--) {
			if (s.docIdList[begin] < index[queryList[i]].docIdList[t])
				t -= (index[queryList[i]].length / size) / 2;
			else break;
		}

		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (int j = begin; j < end; j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			for (; t < index[queryList[i]].length; t++) {
				// ����i�б�������Ԫ��
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��������
					break;
			}
			if (!isFind)// û�ҵ����ʵģ���λ����Ϊ0
			{
				s.docIdList[j] = INT_MAX;
			}
		}
	}

	InvertedIndex result;
	// ��0�Ž������ռ�
	if (rank == 0) {
		// �Ѹò�������Ч���ó���->Ϊ��ͨ��
		for (int i = begin; i < end; i++) {
			if (s.docIdList[i] != INT_MAX) {
				result.docIdList.push_back(s.docIdList[i]);
				result.length++;
			}
		}

		for (int i = 1; i < size; i++) {
			int count;
			MPI_Recv(&count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			unsigned int* temp;// ת��int���鷢��
			temp = new unsigned int[count];

			MPI_Recv(temp, count, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			for (int j = 0; j < initialLength / size; j++) {
				result.docIdList.push_back(temp[j]);
			}
			delete[]temp;
		}
		result.length = result.docIdList.size();
	}
	// ��0���߳����vector���͹�ȥ
	else {
		unsigned int* temp;// ת��int���鷢��
		temp = new unsigned[initialLength / size];
		int count = 0;
		for (int i = begin; i < end; i++) {
			if (s.docIdList[i] != INT_MAX) {
				temp[count++] = s.docIdList[i];
			}
		}
		MPI_Send(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);// �ȷ�һ�����ȹ�ȥ
		MPI_Send(temp, count, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD);// �ٰ����ݷ���ȥ
		delete[]temp;
	}


	return s;
}

InvertedIndex SVS_MPI2(int* queryList, vector<InvertedIndex>& index, int num, int rank, int size) {
	// ȡ��̵��б�
	int initialLength = index[queryList[0]].length;
	InvertedIndex s;// ȡ������Χ1/4
	int start = (initialLength / size) * rank, end = min((initialLength / size) * (rank + 1), initialLength);
	s.docIdList.assign(index[queryList[0]].docIdList.begin() + start, index[queryList[0]].docIdList.begin() + end);
	s.length = s.docIdList.size();

	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		int count = 0;// s��ͷ�������һ��
		int t = 0;

		if (s.length == 0)break;
		// Ԥ������ٴ�����¼�
		t = rank * (index[queryList[i]].length / size);
		for (int j = 2 * rank; j > 0; j--) {
			if (s.docIdList[0] < index[queryList[i]].docIdList[t])
				t -= (index[queryList[i]].length / size) / 2;
			else break;
		}

		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (int j = 0; j < s.length; j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			for (; t < index[queryList[i]].length; t++) {
				// ����i�б�������Ԫ��
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��������
					break;
			}
			if (isFind)// ����
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// ������ɾ��
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}

	// ����ͨ��
	// ��0�Ž������ռ�
	//if (rank == 0) {
	//	for (int i = 1; i < size; i++) {
	//		int count;
	//		MPI_Recv(&count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	//		unsigned int* temp;// ת��int���鷢��
	//		temp = new unsigned int[count];
	//		// ��������
	//		MPI_Recv(temp, count, MPI_UNSIGNED, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	//		s.docIdList.insert(s.docIdList.end(), temp, temp + count);
	//		delete[]temp;
	//	}
	//	s.length = s.docIdList.size();
	//}
	//// ��0���߳����vector���͹�ȥ
	//else {
	//	unsigned int* temp;// ת��int���鷢��
	//	temp = new unsigned[s.length];
	//	copy(s.docIdList.begin(), s.docIdList.end(), temp);
	//	MPI_Send(&s.length, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);// �ȷ�һ�����ȹ�ȥ
	//	MPI_Send(temp, s.length, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD);// �ٰ����ݷ���ȥ
	//	delete[]temp;
	//}

	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0) {
		unsigned int* temp;// ת��int���鷢��
		//int maxLen = max(initialLength / size, initialLength - initialLength / size * (size - 1));
		temp = new unsigned int[initialLength];// ��ʡ�ռ�ʵ��ȿ�����Ļ�����
		for (int i = 1; i < size; i++) {
			// ��������
			MPI_Recv(temp, initialLength, MPI_UNSIGNED, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			int len = temp[0];
			s.docIdList.insert(s.docIdList.end(), temp + 1, temp + 1 + len);
		}
		delete[]temp;
		s.length = s.docIdList.size();
	}
	// ��0���߳����vector���͹�ȥ
	else {
		unsigned int* temp;// ת��int���鷢��
		temp = new unsigned[s.length + 1];
		temp[0] = s.length;
		copy(s.docIdList.begin(), s.docIdList.end(), temp + 1);
		// ֱ��һ�𷢹�ȥ
		MPI_Send(temp, s.length + 1, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD);// �ٰ����ݷ���ȥ
		delete[]temp;
	}
	return s;
}

InvertedIndex SVS_MPI_Precis(int* queryList, vector<InvertedIndex>& index, int num, int rank, int size) {
	//ÿ�����̵���̵�������Ŀ�ʼλ�úͽ���λ��
	int initialLength = index[queryList[0]].docIdList.size();
	InvertedIndex s;// ȡ������Χ1/4
	int start = (initialLength / size) * rank,
		end = rank != size - 1 ? (initialLength / size) * (rank + 1) : initialLength;// ĩβ���⴦��
	s.docIdList.assign(index[queryList[0]].docIdList.begin() + start, index[queryList[0]].docIdList.begin() + end);
	s.length = s.docIdList.size();

	// �������ݽṹ�������������󽻷�Χ
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// Ԥ����,�ؼ��ָ�ֵ
		list[i].key = queryList[i];
	//�ҵ�ʣ�൹���������ʼλ�úͽ���λ��
	int* from = new int[num - 1];
	int* to = new int[num - 1];
	for (int i = 1; i < num; i++) {
		//�ҵ��Լ��Ŀ�ʼλ��
		from[i - 1] = find1stGreaterEqual(index[list[i].key].docIdList, index[list[0].key].docIdList[start]);
		list[i].cursor = from[i - 1];

	}
	if (rank != size - 1) {
		//���һ�����̲���Ҫ֪���Լ���ʱ��������Ϊ��ֱ��ɨ�赽���
		MPI_Recv(to, num - 1, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);//����һ�����̽��ܸý��̵���ʼλ��
		for (int i = 1; i < num; i++) {
			list[i].end = to[i - 1];//��һ�����̵Ŀ�ʼ����������̵Ľ���λ��
		}
	}
	else {
		for (int i = 1; i < num; i++) {
			//���һ������������
			list[i].end = index[list[i].key].docIdList.size();
		}
	}
	if (rank != 0) //��һ�����̲��÷�����Ϣ�����෢�͸�ǰһ������
	{
		MPI_Send(from, num - 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD);
	}


	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		int count = 0;// s��ͷ�������һ��

		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (int j = 0; j < s.length; j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			for (; list[i].cursor < list[i].end; list[i].cursor++) {
				// ����i�б�������Ԫ��
				if (s.docIdList[j] == index[queryList[i]].docIdList[list[i].cursor]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[list[i].cursor])// ��������
					break;
			}
			if (isFind)// ����
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// ������ɾ��
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

InvertedIndex SVS_MPI_Commun(int* queryList, vector<InvertedIndex>& index, int num, int rank, int size) {
	// ȡ��̵��б�
	int initialLength = index[queryList[0]].length;
	InvertedIndex s;// ȡ������Χ1/4
	int start = (initialLength / size) * rank, end = min((initialLength / size) * (rank + 1), initialLength);
	s.docIdList.assign(index[queryList[0]].docIdList.begin() + start, index[queryList[0]].docIdList.begin() + end);
	s.length = s.docIdList.size();

	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		int count = 0;// s��ͷ�������һ��
		int t = 0;

		if (s.length == 0)break;
		// Ԥ������ٴ�����¼�
		t = rank * (index[queryList[i]].length / size);
		for (int j = 2 * rank; j > 0; j--) {
			if (s.docIdList[0] < index[queryList[i]].docIdList[t])
				t -= (index[queryList[i]].length / size) / 2;
			else break;
		}

		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (int j = 0; j < s.length; j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			for (; t < index[queryList[i]].length; t++) {
				// ����i�б�������Ԫ��
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��������
					break;
			}
			if (isFind)// ����
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// ������ɾ��
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}

	// ����ͨ��
	vector<int> recvCounts(size); // �洢ÿ�����̷��͵���������
	int totalCount = 0; // ���н��̷��͵�����������
	vector<int> displacements(size, 0); // ���ջ�������ÿ���������ݵ�λ����

	// ���㱾���̵���������
	int count = s.length; // ����ʵ�������ȡ�����̵���������

	// �ռ�ÿ�����̵�����������������
	MPI_Gather(&count, 1, MPI_INT, recvCounts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

	// ��������������
	if (rank == 0) {
		totalCount = recvCounts[0];
		// ����λ����
		for (int i = 1; i < size; i++) {
			totalCount += recvCounts[i];
			displacements[i] = displacements[i - 1] + recvCounts[i - 1];
		}
	}
	vector<unsigned int> receivedData(totalCount); // �洢���յ�����������

	// ���ͱ����̵����ݸ�����0
	MPI_Gatherv(s.docIdList.data(), count, MPI_UNSIGNED, receivedData.data(), recvCounts.data(), displacements.data(), MPI_UNSIGNED, 0, MPI_COMM_WORLD);

	return s;
}

InvertedIndex SVS_MPI_PreCom(int* queryList, vector<InvertedIndex>& index, int num, int rank, int size) {
	//ÿ�����̵���̵�������Ŀ�ʼλ�úͽ���λ��
	int initialLength = index[queryList[0]].docIdList.size();
	InvertedIndex s;// ȡ������Χ1/4
	int start = (initialLength / size) * rank,
		end = rank != size - 1 ? (initialLength / size) * (rank + 1) : initialLength;// ĩβ���⴦��
	s.docIdList.assign(index[queryList[0]].docIdList.begin() + start, index[queryList[0]].docIdList.begin() + end);
	s.length = s.docIdList.size();

	// �������ݽṹ�������������󽻷�Χ
	QueryItem* list = new QueryItem[num]();
	for (int i = 0; i < num; i++)// Ԥ����,�ؼ��ָ�ֵ
		list[i].key = queryList[i];
	//�ҵ�ʣ�൹���������ʼλ�úͽ���λ��
	int* from = new int[num - 1];
	int* to = new int[num - 1];
	for (int i = 1; i < num; i++) {
		//�ҵ��Լ��Ŀ�ʼλ��
		from[i - 1] = find1stGreaterEqual(index[list[i].key].docIdList, index[list[0].key].docIdList[start]);
		list[i].cursor = from[i - 1];

	}
	if (rank != size - 1) {
		//���һ�����̲���Ҫ֪���Լ���ʱ��������Ϊ��ֱ��ɨ�赽���
		MPI_Recv(to, num - 1, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);//����һ�����̽��ܸý��̵���ʼλ��
		for (int i = 1; i < num; i++) {
			list[i].end = to[i - 1];//��һ�����̵Ŀ�ʼ����������̵Ľ���λ��
		}
	}
	else {
		for (int i = 1; i < num; i++) {
			//���һ������������
			list[i].end = index[list[i].key].docIdList.size();
		}
	}
	if (rank != 0) {
		//��һ�����̲��÷�����Ϣ�����෢�͸�ǰһ������
		MPI_Send(from, num - 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD);
	}


	// ��ʣ���б���
	for (int i = 1; i < num; i++) {
		int count = 0;// s��ͷ�������һ��

		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
		for (int j = 0; j < s.length; j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			for (; list[i].cursor < list[i].end; list[i].cursor++) {
				// ����i�б�������Ԫ��
				if (s.docIdList[j] == index[queryList[i]].docIdList[list[i].cursor]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[list[i].cursor])// ��������
					break;
			}
			if (isFind)// ����
				s.docIdList[count++] = s.docIdList[j];
		}
		if (count < s.length)// ������ɾ��
			s.docIdList.erase(s.docIdList.begin() + count, s.docIdList.end());
		s.length = count;
	}

	// ����ͨ��
	vector<int> recvCounts(size); // �洢ÿ�����̷��͵���������
	int totalCount = 0; // ���н��̷��͵�����������
	vector<int> displacements(size, 0); // ���ջ�������ÿ���������ݵ�λ����

	// ���㱾���̵���������
	int count = s.length; // ����ʵ�������ȡ�����̵���������

	// �ռ�ÿ�����̵�����������������
	MPI_Gather(&count, 1, MPI_INT, recvCounts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

	// ��������������
	if (rank == 0) {
		totalCount = recvCounts[0];
		// ����λ����
		for (int i = 1; i < size; i++) {
			totalCount += recvCounts[i];
			displacements[i] = displacements[i - 1] + recvCounts[i - 1];
		}
	}
	vector<unsigned int> receivedData(totalCount); // �洢���յ�����������

	// ���ͱ����̵����ݸ�����0
	MPI_Gatherv(s.docIdList.data(), count, MPI_UNSIGNED, receivedData.data(), recvCounts.data(), displacements.data(), MPI_UNSIGNED, 0, MPI_COMM_WORLD);


	return s;
}

InvertedIndex SVS_omp(int* queryList, vector<InvertedIndex>& index, int num)
{
	InvertedIndex s = index[queryList[0]];// ȡ��̵��б�
	int count = 0;
	// ��ʣ���б���
#pragma omp parallel num_threads(NUM_THREADS),shared(count)
	for (int i = 1; i < num; i++) {
		count = 0;// s��ͷ�������һ��
		int t = 0;
		// s�б��е�ÿ��Ԫ�ض��ó����Ƚ�
#pragma omp for
		for (int j = 0; j < s.docIdList.size(); j++) {// ����Ԫ�ض��÷���һ��
			bool isFind = false;// ��־���жϵ�ǰcountλ�Ƿ�����

			for (; t < index[queryList[i]].docIdList.size(); t++) {
				// ����i�б�������Ԫ��
				if (s.docIdList[j] == index[queryList[i]].docIdList[t]) {
					isFind = true;
					break;
				}
				else if (s.docIdList[j] < index[queryList[i]].docIdList[t])// ��������
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