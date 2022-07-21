#pragma once
#include"Vec.h"
#include<memory>
#include<vector>
class ComputePipeline;
class RenderTarget;
class TextureBuffer;
class GaussianBlur;
class ConstantBuffer;
class ModelObject;

struct LightBloomConfig
{
	//出力するカラーに乗算
	Vec3<float>m_outputColorRate = { 1,1,1 };
	//明るさのしきい値
	float m_brightThreshold = 0.0f;
};

class LightBloomDevice
{
private:
	static std::shared_ptr<ComputePipeline>s_pipeline;
	void GeneratePipeline();

private:
	//設定
	LightBloomConfig m_config;

	//エミッシブマップ
	std::shared_ptr<TextureBuffer>m_processedEmissiveMap;
	//設定情報送信用
	std::shared_ptr<ConstantBuffer>m_constBuff;
	//ガウシアンブラー
	std::shared_ptr<GaussianBlur>m_gaussianBlur;

public:
	LightBloomDevice();

	//レンダーターゲットに加算して描画
	void Draw(std::weak_ptr<RenderTarget>EmissiveMap, std::weak_ptr<RenderTarget>Target);


	//セッタ
	void SetOutputColorRate(const Vec3<float>& RGBrate = { 1,1,1 });
	void SetBrightThreshold(const float& Threshold = 0.0f);
	void SetBlurPower(const float& BlurPower = 8.0f);
};

