#pragma once
#include"Vec.h"
#include<memory>
#include<string>
#include"D3D12Data.h"

template<typename VertexType>
class Mesh;

class SpriteMesh
{
public:
	//スプライト専用頂点クラス
	class Vertex
	{
	public:
		Vec2<float>pos;
		Vec2<float>uv;

		static std::vector<InputLayoutParam>GetInputLayout()
		{
			static std::vector<InputLayoutParam>INPUT_LAYOUT =
			{
				InputLayoutParam("POSITION",DXGI_FORMAT_R32G32_FLOAT),
				InputLayoutParam("TEXCOORD",DXGI_FORMAT_R32G32_FLOAT)
			};
			return INPUT_LAYOUT;
		}
	};
private:

	//頂点インデックス
	enum { LB, LT, RB, RT, IDX_NUM };

	//メッシュ情報
	std::shared_ptr<Mesh<Vertex>>mesh;

	//トリミングインデックス
	enum { TOP, BOTTOM, LEFT, RIGHT, TRIM_IDX_NUM };
	//各辺のトリミング率
	float trim[TRIM_IDX_NUM];

	//基準点
	Vec2<float>anchorPoint = { 0,0 };
	//サイズ
	Vec2<float>size = { 64,64 };
	//反転フラグ
	Vec2<bool>flip = { false,false };

	//内部パラメータZバッファ
	float zLayer = 0.0f;

	//透過するか
	bool trans = false;

	bool dirty = true;

public:
	SpriteMesh(const char* Name = nullptr);

	//セッタ
	void SetUv(const float& Top = 0.0f, const float& Buttom = 1.0f, const float& Left = 0.0f, const float& Right = 1.0f);
	void SetAnchorPoint(const Vec2<float>&AnchorPoint) {
		if (anchorPoint == AnchorPoint)return;
		anchorPoint = AnchorPoint;
		dirty = true;
	}
	void SetSize(const Vec2<float>&Size) {
		if (size == Size)return;
		size = Size;
		dirty = true;
	}
	void SetFlip(const Vec2<bool>&Flip) {
		if (flip == Flip)return;
		flip = Flip;
		dirty = true;
	}
	void SetZLayer(const float& ZLayer){
		zLayer = ZLayer;
	}
	void SetTransFlg(const bool& TransFlg){
		trans = TransFlg;
	}

	//画像上の座標比率（0.0f ~ 1.0f）を指定してトリミング
	void Trimming(const float& Top = 0.0f, const float& Bottom = 1.0f, const float& Left = 0.0f, const float& Right = 1.0f) {
		if (Top < 0 || 1.0f < Top)assert(0);
		if (Bottom < 0 || 1.0f < Bottom)assert(0);
		if (Left < 0 || 1.0f < Left)assert(0);
		if (Right < 0 || 1.0f < Right)assert(0);

		if (Bottom < Top)assert(0);
		if (Right < Left)assert(0);

		trim[TOP] = Top;
		trim[BOTTOM] = Bottom;
		trim[LEFT] = Left;
		trim[RIGHT] = Right;

		dirty = true;
	}

	void Render(const std::vector<std::shared_ptr<DescriptorData>>& DescDatas, const std::vector<DESC_HANDLE_TYPE>& DescHandleTypes, const int& InstanceNum = 1);
};