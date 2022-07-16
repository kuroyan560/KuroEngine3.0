#pragma once
#include<cfloat>
//最小値と最大値を格納するための構造体
struct ValueMinMax
{
	float m_min;
	float m_max;

	//中央の値を求める
	float GetCenter()const
	{
		return (m_max - m_min) * 0.5f + m_min;
	}

	//デフォルト引数：比較時に必ず通るように
	void Set(const float& Min = FLT_MAX, const float& Max = 0.0f)
	{
		m_min = Min;
		m_max = Max;
	}

	//minとmaxの大小関係の異常を確認
	bool Invalid()const
	{
		return m_max < m_min;
	}
	operator bool()const { return !Invalid(); }
};

