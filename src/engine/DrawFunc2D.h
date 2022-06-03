#pragma once
#include"D3D12Data.h"
#include"Vec.h"
#include"Color.h"

static class DrawFunc2D
{
	//DrawLine
	static int DRAW_LINE_COUNT;	//呼ばれた回数

	//DrawBox
	static int DRAW_BOX_COUNT;

	//DrawCircle
	static int DRAW_CIRCLE_COUNT;

	//DrawExtendGraph
	static int DRAW_EXTEND_GRAPH_COUNT;

	//DrawRotaGraph
	static int DRAW_ROTA_GRAPH_COUNT;

public:
	//呼び出しカウントリセット
	static void CountReset()
	{
		DRAW_LINE_COUNT = 0;
		DRAW_BOX_COUNT = 0;
		DRAW_CIRCLE_COUNT = 0;
		DRAW_EXTEND_GRAPH_COUNT = 0;
		DRAW_ROTA_GRAPH_COUNT = 0;
	}

	/// <summary>
	/// 直線の描画
	/// </summary>
	/// <param name="FromPos">起点座標</param>
	/// <param name="ToPos">終点座標</param>
	/// <param name="Color">色</param>
	/// <param name="BlendMode">ブレンドモード</param>
	static void DrawLine2D(const Vec2<float>& FromPos, const Vec2<float>& ToPos, const Color& LineColor, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans);

	/// <summary>
	/// 直線の描画（画像）
	/// </summary>
	/// <param name="FromPos">起点座標</param>
	/// <param name="ToPos">終点座標</param>
	/// <param name="Tex">テクスチャ</param>
	/// <param name="Thickness">線の太さ</param>
	/// <param name="BlendMode">ブレンドモード</param>
	/// <param name="Miror">反転フラグ</param>
	static void DrawLine2DGraph(const Vec2<float>& FromPos, const Vec2<float>& ToPos, const std::shared_ptr<TextureBuffer>& Tex, const int& Thickness, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans, const Vec2<bool>& Mirror = { false,false });

	/// <summary>
	/// 2D四角形の描画
	/// </summary>
	/// <param name="LeftUpPos">左上座標</param>
	/// <param name="RightBottomPos">右下座標</param>
	/// <param name="Color">色</param>
	/// <param name="FillFlg">中を塗りつぶすか</param>
	/// <param name="BlendMode">ブレンドモード</param>
	static void DrawBox2D(const Vec2<float>& LeftUpPos, const Vec2<float>& RightBottomPos, const Color& BoxColor, const bool& FillFlg = false, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans);

	/// <summary>
	/// ２D円の描画
	/// </summary>
	/// <param name="Center">中心座標</param>
	/// <param name="Radius">半径</param>
	/// <param name="Color">色</param>
	/// <param name="FillFlg">中を塗りつぶすか</param>
	/// <param name="LineThickness">中を塗りつぶさない場合の線の太さ</param>
	/// <param name="BlendMode">ブレンドモード</param>
	static void DrawCircle2D(const Vec2<float>& Center, const float& Radius, const Color& CircleColor, const bool& FillFlg = false, const int& LineThickness = 1, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans);

	/// <summary>
	/// 画像描画
	/// </summary>
	/// <param name="LeftUpPos">矩形の左上座標</param>
	/// <param name="Tex">テクスチャ</param>
	/// <param name="BlendMode">ブレンドモード</param>
	static void DrawGraph(const Vec2<float>& LeftUpPos, const std::shared_ptr<TextureBuffer>& Tex, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans, const Vec2<bool>& Miror = { false,false });

	/// <summary>
	/// 拡大縮小描画
	/// </summary>
	/// <param name="LeftUpPos">矩形の左上座標</param>
	/// <param name="RightBottomPos">矩形の右上座標</param>
	/// <param name="Tex">テクスチャ</param>
	/// <param name="BlendMode">ブレンドモード</param>
	static void DrawExtendGraph2D(const Vec2<float>& LeftUpPos, const Vec2<float>& RightBottomPos,
		const std::shared_ptr<TextureBuffer>& Tex, const AlphaBlendMode& BlendMode = AlphaBlendMode_None, const Vec2<bool>& Miror = { false,false });

	/// <summary>
	/// ２D画像回転描画
	/// </summary>
	/// <param name="Center">中心座標</param>
	/// <param name="ExtRate">拡大率</param>
	/// <param name="Radian">回転角度</param>
	/// <param name="Tex">テクスチャ</param>
	/// <param name="BlendMode">ブレンドモード</param>
	/// <param name="LRTurn">左右反転フラグ</param>
	static void DrawRotaGraph2D(const Vec2<float>& Center, const Vec2<float>& ExtRate, const float& Radian,
		const std::shared_ptr<TextureBuffer>& Tex, const Vec2<float>& RotaCenterUV = { 0.5f,0.5f },
		const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans, const Vec2<bool>& Mirror = { false,false });
};