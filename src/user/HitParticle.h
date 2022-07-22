#pragma once
#include"Vec.h"
#include"Color.h"
#include<array>
#include<memory>
#include"D3D12Data.h"
class Camera;

class HitParticle
{
	//ブロック数
	static const int s_particleNum = 30000;
	static const int s_threadDiv = 32;
	static const int GetThreadNumX();

	//ブロック個体情報構造体
	struct Particle
	{
		Color m_color;
		float m_scale = 1.0f;
		Vec3<float>m_vel = { 0,1,0 };
		Vec3<float>m_pos = { 0,0,0 };
		int m_life;
		int m_lifeSpan;
		int pad[3];
	};

	//ブロックの各個体情報（CPU）
	std::array<Particle, s_particleNum>m_particleDataArray;
	//ブロックの各個体情報（GPU）
	std::shared_ptr<StructuredBuffer>m_particleBuff;

	//グラフィックスパイプライン
	std::shared_ptr<GraphicsPipeline>m_gPipeline;

	//Indirect機構
	std::shared_ptr<IndirectDevice>m_indirectDev;

	//頂点バッファ
	std::shared_ptr<VertexBuffer>m_vertBuff;

	//設定
	struct Emitter
	{
		//パーティクル最大数
		unsigned int m_maxParticleNum = s_particleNum;
		//生成するパーティクル数
		unsigned int m_generateNum;
	}m_config;
	std::shared_ptr<ConstantBuffer>m_configBuffer;

	//初期化用パイプライン
	std::shared_ptr<ComputePipeline>m_cInitPipeline;
	//生成用パイプライン
	std::shared_ptr<ComputePipeline>m_cGeneratePipeline;
	//更新用パイプライン
	std::shared_ptr<ComputePipeline>m_cUpdatePipeline;


	//死亡パーティクルのコマンド
	std::shared_ptr<DescriptorData>m_deadComBuffer;
	//std::shared_ptr<GPUResource>m_deadComCounterBuffer;
	
	//稼働中パーティクルのコマンド
	std::shared_ptr<DescriptorData>m_aliveComBuffer;
	std::shared_ptr<GPUResource>m_aliveComCounterBuffer;

	bool m_invalidCommandBuffer = true;
	void GenerateCommandBuffers(const size_t& CommandSize);

public:
	HitParticle();
	void Init(Camera& Cam);
	void Update();
	void Draw(Camera& Cam);

	void Emit(Vec3<int>EmitPos);
};

