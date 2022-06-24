#pragma once
#include<vector>

template<typename T>
struct KeyFrame
{
	int frame;
	T value;
};

template<typename T>
struct Animation
{
	int startFrame;
	int endFrame;
	std::vector<KeyFrame<T>>keyFrames;
};