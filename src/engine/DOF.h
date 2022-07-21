#pragma once
#include<memory>
class RenderTarget;
class ComputePipeline;
class ConstantBuffer;
class GaussianBlur;
class TextureBuffer;

//DOF設定情報
struct DOFConfig
{
	//手前
	float m_nearPint = 0.0f;
	//奥（手前～奥までボケない）
	float m_farPint  = 10.0f;
	//手前以降、奥以降からボケ最大になるまでの距離
	float m_pintLength = 20.0f;
};

class DOF
{
private:
	static std::shared_ptr<ComputePipeline>s_pipeline;
	void GeneratePipeline();

private:
	//DOF設定情報
	DOFConfig m_config;
	std::shared_ptr<ConstantBuffer>m_configBuff;

	//ガウシアンブラー
	std::shared_ptr<GaussianBlur>m_gaussianBlur;

	//深度マップをもとに生成した透過ボケ画像の格納先
	std::shared_ptr<TextureBuffer>m_processedTex;

public:
	DOF();
	void SetPintConfig(float FrontPint = 0.0f, float BackPint = 10.0f, float PintLength = 20.0f);
	void Draw(std::weak_ptr<RenderTarget>Src, std::weak_ptr<RenderTarget>DepthMap);
};

