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
//	//读取每个链表的长度和每个docID所用的bit数
//	for (int i = 0; i < LIST_NUM; i++)
//	{
//		// 前32位是长度
//		int len = readBitData(compressedLists, idx, 32);
//
//		//6位是用的Bit数
//		bitNum = (int)readBitData(compressedLists, idx + 32, 6);
//
//		// 下一个长度的位置为+=32 + 6 + len*bitNum,同时保存
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
//	//---------------------------------------读取---------------------------------------
//	vector<vector<unsigned>> invertedLists;//读取的链表
//	vector<vector<unsigned>> decompressed;//解压后的链表应该等于invertedLists
//
//	//计时
//	long long head, tail, freq;
//	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
//	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time
//
//	//getIndex(invertedLists);//读取倒排链表
//
//	//计时终止
//	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
//	cout << "Read(orignal) Time:" << (tail - head) * 1000.0 / freq << "ms" << endl;
//
//	vector<unsigned> compressedRes;//压缩结果
//	vector<unsigned> compressed; //读出来的压缩链表
//	vector<unsigned> curList;//当前解压的链表
//
//	//---------------------------------------压缩---------------------------------------------
//	int idx = 0;
//	//for (int i = 0; i < invertedLists.size(); i++) // 对每个链表进行压缩，存到一个vector<unsigned> compressedRes中
//	//    dgapCompress(invertedLists[i], compressedRes, idx, idx);
//	//
//	//vectorToBin(compressedRes, "compress.bin");
//
//	//---------------------------------------解压---------------------------------------------
//	//计时
//	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
//	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time
//
//	//从第0个bit开始读数据，解压
//	int curIdx = 0;
//	binToVector("compress.bin", compressed);//读取到compressed中，应该compressed等于compressedRes
//
//	dgapDecompressAll(compressed, decompressed);
//	//dgapDeOpt(compressed, decompressed);
//
//	//计时终止
//	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
//	cout << "Decompresss Time:" << (tail - head) * 1000.0 / freq << "ms" << endl;
//	return 0;
//}

int main()
{
	//---------------------------------------读取---------------------------------------
	vector<vector<unsigned>> invertedLists;//读取的链表
	vector<vector<unsigned>> decompressed;//解压后的链表应该等于invertedLists

	//计时
	long long head, tail, freq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time

	getIndex(invertedLists);//读取倒排链表

	//计时终止
	QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
	cout << "Read(orignal) Time:" << (tail - head) * 1000.0 / freq << "ms" << endl;

	vector<unsigned> compressedRes;//压缩结果
	vector<unsigned> compressed; //读出来的压缩链表
	vector<unsigned> curList;//当前解压的链表

	//---------------------------------------压缩---------------------------------------------
	int idx = 0;
	for (int i = 0; i < invertedLists.size(); i++) // 对每个链表进行压缩，存到一个vector<unsigned> compressedRes中
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

	////---------------------------------------解压---------------------------------------------
	////计时
	//QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	//QueryPerformanceCounter((LARGE_INTEGER*)&head);// Start Time

	////从第0个bit开始读数据，解压
	//int curIdx = 0;
	//binToVector("compress.bin", compressed);//读取到compressed中，应该compressed等于compressedRes

	//dgapDecompressAll(compressed, decompressed);
	////dgapDeOpt(compressed, decompressed);

	////计时终止
	//QueryPerformanceCounter((LARGE_INTEGER*)&tail);// End Time
	//cout << "Decompresss Time:" << (tail - head) * 1000.0 / freq << "ms" << endl;
	return 0;
}
