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
	static const int s_particleNum = 10000;
	static const int s_threadDiv = 32;

	//ブロック個体情報構造体
	struct Particle
	{
		Color m_color;
		Vec3<float>m_vel = { 0,1,0 };
		float m_life = 0;
		Vec3<float>m_pos = { 0,0,0 };
		int m_lifeSpan = 60;
	};

	//ブロックの各個体情報（GPU）
	std::shared_ptr<RWStructuredBuffer>m_particleBuff;

	//グラフィックスパイプライン
	std::shared_ptr<GraphicsPipeline>m_gPipeline;

	//Indirect機構
	std::shared_ptr<IndirectDevice>m_indirectDev;

	//頂点バッファ
	std::shared_ptr<VertexBuffer>m_vertBuff;

	//設定
	struct Config
	{
		//カメラのバッファアドレス
		//D3D12_GPU_VIRTUAL_ADDRESS camAddress;
		//パーティクル最大数
		unsigned int m_maxParticleNum = s_particleNum;
	}m_config;
	std::shared_ptr<ConstantBuffer>m_configBuffer;

	//更新用パイプライン
	std::shared_ptr<ComputePipeline>m_cUpdatePipeline;


	//死亡パーティクルのコマンド
	std::shared_ptr<IndirectCommandBuffer>m_commandBuffer;
	
	//稼働中パーティクルのコマンド
	std::shared_ptr<IndirectCommandBuffer>m_aliveCommandBuffer;

	bool m_invalidCommandBuffer = true;

public:
	HitParticle();
	void Init(Camera& Cam);
	void Update();
	void Draw(Camera& Cam);

	void Emit(int Num, Vec3<float>Pos);
};

