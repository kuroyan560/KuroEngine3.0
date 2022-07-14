#pragma once
#include"Vec.h"
#include"Color.h"
#include<array>
#include<memory>
#include"D3D12Data.h"
class Camera;

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
		float pad[53];
	};
	//ブロックの各個体情報（CPU）
	std::array<Block, BLOCK_NUM>m_blockDataArray;
	//ブロックの各個体情報（GPU）
	std::shared_ptr<StructuredBuffer>m_blockBuff;

	//コマンドバッファ
	//std::shared_ptr<StructuredBuffer>m_commandBuffer;
	//コマンドバッファ（生）
	Microsoft::WRL::ComPtr<ID3D12Resource1>m_commandBuffer;
	//コマンドバッファの更新用受け皿（Mappingが出来ないため）
	Microsoft::WRL::ComPtr<ID3D12Resource1>m_uploadCommandBuffer;

	//グラフィックスパイプライン
	std::shared_ptr<GraphicsPipeline>m_gPipeline;

	//Indirect機構
	std::shared_ptr<IndirectDevice>m_indirectDev;

	//頂点バッファ
	std::shared_ptr<VertexBuffer>m_vertBuff;

	void GenerateCommandBuffer(std::array<IndirectCommand, BLOCK_NUM>&UploadCommands);

public:
	IndirectSample();
	void Init(Camera& Cam);
	void Update();
	void Draw();
};

