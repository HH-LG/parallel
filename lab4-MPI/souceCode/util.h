#pragma once
#include <iostream>
#include <vector>

//返回第一个大于等于目标元素的下标
int find1stGreaterEqual(const std::vector<unsigned int>& arr, unsigned int target) {
    int left = 0;
    int right = arr.size();

    while (left < right) 
    {
        int mid = left + (right - left) / 2;
        if (arr[mid] < target) 
        {
            left = mid + 1;
        }
        else 
            right = mid;
    }
    return left;
}
