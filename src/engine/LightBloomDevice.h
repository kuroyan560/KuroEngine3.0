#pragma once
#include"Vec.h"
#include<memory>
#include<vector>
class ComputePipeline;
class RenderTarget;
class DepthStencil;
class TextureBuffer;
class GaussianBlur;
class ConstantBuffer;
class ModelObject;

struct LightBloomConfig
{
	//出力するカラーに乗算
	Vec3<float>outputColorRate = { 1,1,1 };
	//明るさのしきい値
	float brightThreshold = 0.0f;
};

class LightBloomDevice
{
private:
	std::shared_ptr<ComputePipeline>PIPELINE;
	void GeneratePipeline();

private:
	//設定
	LightBloomConfig config;

	//エミッシブマップ
	std::shared_ptr<RenderTarget>emissiveMap;
	std::shared_ptr<DepthStencil>emissiveMapDepth;
	std::shared_ptr<TextureBuffer>emissiveMapFiltered;
	//設定情報送信用
	std::shared_ptr<ConstantBuffer>constBuff;
	//ガウシアンブラー
	std::shared_ptr<GaussianBlur>gaussianBlur;

public:
	LightBloomDevice();

	//エミッシブマップに書き込むためにレンダーターゲットをセット
	void SetRenderTargets();
	//レンダーターゲットに加算して描画（Clear：描画後レンダーターゲットをクリアするか）
	void Draw(const bool& Clear = true);


	//セッタ
	void SetOutputColorRate(const Vec3<float>& RGBrate = { 1,1,1 });
	void SetBrightThreshold(const float& Threshold = 0.0f);
	void SetBlurPower(const float& BlurPower = 8.0f);
};

