#pragma once
#pragma once
#include<wrl.h>
#include<memory>

class ComputePipeline;
class ConstantBuffer;
class TextureBuffer;
class RenderTarget;

class RadialBlur
{
private:
	static std::shared_ptr<ComputePipeline>RADIAL_PIPELINE;
	static void GeneratePipeline();

private:
	//定数バッファ用データ
	struct ConstData
	{
		Vec2<int>texSize;
		float blurPower = 20.0f;
		Vec2<float>centerUV = { 0.5f,0.5f };
		float pad[3];
	}constData;

	//定数バッファ
	std::shared_ptr<ConstantBuffer>constBuff;

	//結果のテクスチャ
	std::shared_ptr<TextureBuffer>finalResult;

public:
	//ソース画像 & 結果描画先設定、ぼかし強さ
	RadialBlur(const Vec2<int>& Size, const DXGI_FORMAT& Format, const float& BlurPower = 8.0f, const Vec2<float>& CenterUV = { 0.5f,0.5f });
	//ボケ具合
	void SetBlurPower(const float& BlurPower);
	//ブラーの中心点UV
	void SetCenterUV(const Vec2<float>& CenterUV = { 0.5f,0.5f });
	
	//実行
	void Excute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& CmdList, const std::shared_ptr<TextureBuffer>& SourceTex);

	//結果のテクスチャ取得
	std::shared_ptr<TextureBuffer>& GetResultTex() { return finalResult; }
};