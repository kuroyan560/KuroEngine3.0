#pragma once
#include"PostEffect.h"
class ComputePipeline;
class ConstantBuffer;
class TextureBuffer;
class RenderTarget;

class GaussianBlur : public PostEffect
{
private:
	static const int NUM_WEIGHTS = 8;
	static std::shared_ptr<ComputePipeline>X_BLUR_PIPELINE;	//横ブラー
	static std::shared_ptr<ComputePipeline>Y_BLUR_PIPELINE;	//縦ブラー
	static std::shared_ptr<ComputePipeline>FINAL_PIPELINE;		//各ブラーテクスチャ合成
	static void GeneratePipeline();

private:
	//重みテーブル
	float weights[NUM_WEIGHTS];

	//重みテーブルの定数バッファ
	std::shared_ptr<ConstantBuffer>weightConstBuff;
	//テクスチャ情報の定数バッファ
	std::shared_ptr<ConstantBuffer>texInfoConstBuff;

	//横ブラーの結果
	std::shared_ptr<TextureBuffer>xBlurResult;

	//縦ブラーの結果
	std::shared_ptr<TextureBuffer>yBlurResult;

	//最終結果
	std::shared_ptr<TextureBuffer>finalResult;

public:
	//ソース画像 & 結果描画先設定、ぼかし強さ
	GaussianBlur(const Vec2<int>& Size, const DXGI_FORMAT& Format, const float& BlurPower = 8.0f);
	//ボケ具合
	void SetBlurPower(const float& BlurPower);
	//実行
	void Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const std::shared_ptr<TextureBuffer>& SourceTex)override;

	//結果のテクスチャ取得
	std::shared_ptr<TextureBuffer>& GetResultTex() { return finalResult; }

	//ウィンドウサイズで結果を描画
	void DrawResult(const AlphaBlendMode& AlphaBlend);
};