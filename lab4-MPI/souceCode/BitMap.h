#pragma once
#include"Adp.h"
#include<string>
#include<list>
#include<vector>
#include<algorithm>
#include <mmintrin.h>   //mmx
#include <xmmintrin.h>  //sse
#include <emmintrin.h>  //sse2
#include <pmmintrin.h>  //sse3
#include <omp.h>
extern vector<InvertedIndex> invertedLists;
struct BitMap
{
	vector<int> bits;			//id��Ӧλ
	vector<int> firstIndex;		//һ������
	vector<int> secondIndex;	//��������
	BitMap(int size = 25205200)
	{
		//���ٿռ�
		this->bits.resize(size / 32 + 1);
		this->firstIndex.resize(size / 1024 + 1);
		this->secondIndex.resize(size / 32768 + 1);
	}

	void set(int x) //��x��Ӧ��������λ
	{
		//����λ������
		int index0 = x / 32; //��������������
		int index1 = index0 / 1024;
		int index2 = index1 / 1024;
		int offset0 = x % 32;
		int offset1 = offset0 / 32;
		int offset2 = offset1 / 32;

		this->bits[index0] |= (1 << offset0); //����,��λΪ1
		this->firstIndex[index1] |= (1 << offset1);
		this->secondIndex[index2] |= (1 << offset2);
	}
};
BitMap bitmapList[2000];//���ռ�
BitMap chosenList;
list<int> skipPointer;
void bitMapProcessing(vector<InvertedIndex>& invertedLists, int count)
{
	for (int i = 0; i < count; i++)
		for (int j = 0; j < invertedLists[i].docIdList.size(); j++)
			bitmapList[i].set(invertedLists[i].docIdList[j]);	//����id��Ӧbitmap
	//��������
	for (int i = 0; i < bitmapList[0].secondIndex.size(); i++)
		skipPointer.push_back(i);

}
InvertedIndex S_BITMAP(int* list, vector<InvertedIndex>& idx, int num)
{

	int len = bitmapList[list[0]].secondIndex.size();
	len = len / 4;

	for (int i = 0; i < 4; i++)
		copy(bitmapList[list[0]].secondIndex.begin() + len * i, bitmapList[list[0]].secondIndex.begin() + len * (i + 1), chosenList.secondIndex.begin() + len * i);
	len = bitmapList[list[0]].firstIndex.size();
	len = len / 4;

	for (int i = 0; i < 4; i++)
		copy(bitmapList[list[0]].firstIndex.begin() + len * i, bitmapList[list[0]].firstIndex.begin() + len * (i + 1), chosenList.firstIndex.begin() + len * i);
	len = bitmapList[list[0]].bits.size();
	len = len / 4;

	for (int i = 0; i < 4; i++)
		copy(bitmapList[list[0]].bits.begin() + len * i, bitmapList[list[0]].bits.begin() + len * (i + 1), chosenList.bits.begin() + len * i);

	//copy(bitmapList[list[0]].secondIndex.begin(), bitmapList[list[0]].secondIndex.end(), chosenList.secondIndex.begin());
	//copy(bitmapList[list[0]].firstIndex.begin(), bitmapList[list[0]].firstIndex.end(), chosenList.firstIndex.begin());
	//copy(bitmapList[list[0]].bits.begin(), bitmapList[list[0]].bits.end(), chosenList.bits.begin());

	//chosenList = bitmapList[list[0]];
	InvertedIndex res;
	for (int i = 1; i < num; i++)
	{
		for (int j = 0; j < chosenList.secondIndex.size(); j++)
		{
			chosenList.secondIndex[j] &= bitmapList[list[i]].secondIndex[j];//��λ��
			if (chosenList.secondIndex[j])
			{
				for (int t = j * 32; t < j * 32 + 32; t++)
				{
					chosenList.firstIndex[t] &= bitmapList[list[i]].firstIndex[t];//��λ��
					if (chosenList.firstIndex[t])
					{
						for (int l = t * 32; l < t * 32 + 32; l++)
						{
							chosenList.bits[l] &= bitmapList[list[i]].bits[l];//��λ��
							if (i == num - 1 && chosenList.bits[l])
							{
								int bit = chosenList.bits[l];//ȡ���ö�
								for (int m = 0; m < 32; m++)
								{
									if ((bit & 1) != 0)
										res.docIdList.push_back(32 * l + m);
									bit = bit >> 1;
								}
							}
						}
					}
				}
			}
		}
	}
	return res;
}
