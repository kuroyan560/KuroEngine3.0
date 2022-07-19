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
	static const int s_blockNum = 30000;

	//ブロック個体情報構造体
	struct Block
	{
		Color m_color;
		float m_scale = 1.0f;
		Vec3<float>m_vel = { 0,1,0 };
		Vec3<float>m_offset = { 0,0,0 };
		float m_pad;
	};

	//ブロックの各個体情報（CPU）
	std::array<Block, s_blockNum>m_blockDataArray;
	//ブロックの各個体情報（GPU）
	std::shared_ptr<StructuredBuffer>m_blockBuff;

	//コマンドバッファ
	std::shared_ptr<DescriptorData>m_commandBuffer;

	//グラフィックスパイプライン
	std::shared_ptr<GraphicsPipeline>m_gPipeline;

	//Indirect機構
	std::shared_ptr<IndirectDevice>m_indirectDev;

	//頂点バッファ
	std::shared_ptr<VertexBuffer>m_vertBuff;

	//カリングフラグ
	bool m_enableCulling = true;
	//bool m_enableCulling = false;
	//カリング設定
	struct CallingConfig
	{
		float cullOffset = 0.25f;
		unsigned int commandCount = s_blockNum;
	}m_callingConfig;
	std::shared_ptr<ConstantBuffer>m_callingConfigBuffer;
	//カリング用パイプライン
	std::shared_ptr<ComputePipeline>m_cPipeline;
	//カウンターバッファ
	std::shared_ptr<GPUResource>m_counterBuffer;
	//カリング処理済コマンドバッファ
	std::shared_ptr<DescriptorData>m_processedCommandBuffer;

	bool m_invalidCommandBuffer = true;
	void GenerateCommandBuffers(const size_t& CommandSize);

public:
	IndirectSample();
	void Init(Camera& Cam);
	//void Update(bool EnableCulling);
	void Update(float CullingOffset);
	void Draw(Camera& Cam);
};

