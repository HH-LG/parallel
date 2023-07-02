#include<windows.h>
#include<iostream>
#include<vector>
#include<omp.h>
#include <algorithm>
#include<sstream>
#include<math.h>
#include"bitOp.h"
#include"dgap.h"
#include"pfd.h"
#define LIST_NUM 2000
#define NUM_THREADS 4
using namespace std;

// void dgapDeOpt(const vector<unsigned>& compressedLists, vector<vector<unsigned>>& result)
//{
//	int idx = 0;
//	int listIndex[LIST_NUM] = { 0 };
//	int bitNum;
//
//	//��ȡÿ������ĳ��Ⱥ�ÿ��docID���õ�bit��
//	for (int i = 0; i < LIST_NUM; i++)
//	{
//		// ǰ32λ�ǳ���
//		int len = readBitData(compressedLists, idx, 32);
//
//		//6λ���õ�Bit��
//		bitNum = (int)readBitData(compressedLists, idx + 32, 6);
//
//		// ��һ�����ȵ�λ��Ϊ+=32 + 6 + len*bitNum,ͬʱ����
//		idx += 32 + 6 + len * bitNum;
//		if (i != LIST_NUM - 1)
//			listIndex[i + 1] = idx;
//	}
//
//#pragma omp parallel num_threads(NUM_THREADS)
//#pragma omp for 
//	for (int i = 0; i < LIST_NUM; i++)
//	{
//		vector<unsigned> curList;
//		dgapDecompress(compressedLists, curList, listIndex[i]);
//#pragma omp critical
//		result.push_back(curList);
//	}
//}

//int main(int argc, char* argv[])
//{
//	//---------------------------------------��ȡ---------------------------------------
//	vector<vector<unsigned>> invertedLists;//��ȡ������
//	vector<vector<unsigned>> decompressed;//��ѹ�������Ӧ�õ���invertedLists
//
//	//��ʱ
//	long long head, tail, freq;
//	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
//	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time
//
//	//getIndex(invertedLists);//��ȡ��������
//
//	//��ʱ��ֹ
//	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
//	cout << "Read(orignal) Time:" << (tail - head) * 1000.0 / freq << "ms" << endl;
//
//	vector<unsigned> compressedRes;//ѹ�����
//	vector<unsigned> compressed; //��������ѹ������
//	vector<unsigned> curList;//��ǰ��ѹ������
//
//	//---------------------------------------ѹ��---------------------------------------------
//	int idx = 0;
//	//for (int i = 0; i < invertedLists.size(); i++) // ��ÿ���������ѹ�����浽һ��vector<unsigned> compressedRes��
//	//    dgapCompress(invertedLists[i], compressedRes, idx, idx);
//	//
//	//vectorToBin(compressedRes, "compress.bin");
//
//	//---------------------------------------��ѹ---------------------------------------------
//	//��ʱ
//	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
//	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time
//
//	//�ӵ�0��bit��ʼ�����ݣ���ѹ
//	int curIdx = 0;
//	binToVector("compress.bin", compressed);//��ȡ��compressed�У�Ӧ��compressed����compressedRes
//
//	dgapDecompressAll(compressed, decompressed);
//	//dgapDeOpt(compressed, decompressed);
//
//	//��ʱ��ֹ
//	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
//	cout << "Decompresss Time:" << (tail - head) * 1000.0 / freq << "ms" << endl;
//	return 0;
//}

int main()
{
	//---------------------------------------��ȡ---------------------------------------
	vector<vector<unsigned>> invertedLists;//��ȡ������
	vector<vector<unsigned>> decompressed;//��ѹ�������Ӧ�õ���invertedLists

	//��ʱ
	long long head, tail, freq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time

	getIndex(invertedLists);//��ȡ��������

	//��ʱ��ֹ
	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
	cout << "Read(orignal) Time:" << (tail - head) * 1000.0 / freq << "ms" << endl;

	vector<unsigned> compressedRes;//ѹ�����
	vector<unsigned> compressed; //��������ѹ������
	vector<unsigned> curList;//��ǰ��ѹ������

	//---------------------------------------ѹ��---------------------------------------------
	int idx = 0;
	for (int i = 0; i < invertedLists.size(); i++) // ��ÿ���������ѹ�����浽һ��vector<unsigned> compressedRes��
		pfdCompress(invertedLists[i], compressedRes, idx);
	/*for (int i = 0; i < 2000; i++)
	{
		pfdCompress(invertedLists[i], compressedRes, idx);
		idx = 0;
		vector<unsigned> vec_u_decompressed = pfdDecompress(compressedRes, idx);

		if (invertedLists[i].size() != vec_u_decompressed.size())
		{
			cout << "error size" << endl;
			return 0;
		}
		for (int j = 0; j < invertedLists[i].size(); j++)
		{
			if ((invertedLists[i])[j] != vec_u_decompressed[j])
			{
				cout << "error val" << endl;
			}
		}
		compressedRes.clear();
	}*/
	vectorToBin(compressedRes, "compress.bin");

	////---------------------------------------��ѹ---------------------------------------------
	////��ʱ
	//QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	//QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time

	////�ӵ�0��bit��ʼ�����ݣ���ѹ
	//int curIdx = 0;
	//binToVector("compress.bin", compressed);//��ȡ��compressed�У�Ӧ��compressed����compressedRes

	//dgapDecompressAll(compressed, decompressed);
	////dgapDeOpt(compressed, decompressed);

	////��ʱ��ֹ
	//QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
	//cout << "Decompresss Time:" << (tail - head) * 1000.0 / freq << "ms" << endl;
	return 0;
}
