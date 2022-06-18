#pragma once
#include<cfloat>
//最小値と最大値を格納するための構造体
struct ValueMinMax
{
	float min;
	float max;

	//中央の値を求める
	float GetCenter()const
	{
		return (max - min) * 0.5f + min;
	}

	//デフォルト引数：比較時に必ず通るように
	void Set(const float& Min = FLT_MAX, const float& Max = 0.0f)
	{
		min = Min;
		max = Max;
	}

	//minとmaxの大小関係の異常を確認
	bool Invalid()const
	{
		return max < min;
	}
	operator bool()const { return !Invalid(); }
};

