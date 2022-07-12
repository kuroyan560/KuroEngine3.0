#pragma once
#include"Vec.h"
#include"Color.h"
#include<array>
#include<memory>
class StructuredBuffer;
class IndirectDevice;
class GraphicsPipeline;
class Camera;
class VertexBuffer;

class IndirectSample
{
	//ブロック数
	static const int BLOCK_NUM = 1024;

	//ブロック個体情報構造体
	struct Block
	{
		float scale = 1.0f;
		Vec3<float>vel = { 0,1,0 };
		Vec3<float>offset = { 0,0,0 };
		Color color;
	};
	//ブロックの各個体情報（CPU）
	std::array<Block, BLOCK_NUM>blockDataArray;
	//ブロックの各個体情報（GPU）
	std::shared_ptr<StructuredBuffer>blockBuff;

	//コマンドバッファ
	std::shared_ptr<StructuredBuffer>commandBuffer;

	//グラフィックスパイプライン
	std::shared_ptr<GraphicsPipeline>gPipeline;

	//Indirect機構
	std::shared_ptr<IndirectDevice>indirectDev;

	//頂点バッファ
	std::shared_ptr<VertexBuffer>vertBuff;

public:
	IndirectSample();
	void Init(Camera& Cam);
	void Update();
	void Draw();
};

