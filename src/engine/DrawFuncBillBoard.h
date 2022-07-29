#pragma once
#include"D3D12Data.h"
#include"Vec.h"
#include"Color.h"
class Camera;
class DrawFuncBillBoard
{
	//DrawBox
	static int s_drawBoxCount;

public:
	//呼び出しカウントリセット
	static void CountReset()
	{
		s_drawBoxCount = 0;
	}

	//四角描画
	static void Box(Camera& Cam, const Vec3<float>& Pos, const Vec2<float>& Size, const Color& BoxColor, const AlphaBlendMode& BlendMode = AlphaBlendMode_None);
};