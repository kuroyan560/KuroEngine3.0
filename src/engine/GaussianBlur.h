#pragma once
#include<memory>
#include<wrl.h>
#include"D3D12Data.h"

class GaussianBlur
{
private:
	static const int s_weightNum = 8;
	static std::shared_ptr<ComputePipeline>s_xBlurPipeline;	//横ブラー
	static std::shared_ptr<ComputePipeline>s_yBlurPipeline;	//縦ブラー
	static std::shared_ptr<ComputePipeline>s_finalPipeline;		//各ブラーテクスチャ合成
	static void GeneratePipeline();

private:
	//重みテーブル
	float m_weights[s_weightNum];

	//重みテーブルの定数バッファ
	std::shared_ptr<ConstantBuffer>m_weightConstBuff;
	//テクスチャ情報の定数バッファ
	std::shared_ptr<ConstantBuffer>m_texInfoConstBuff;

	//横ブラーの結果
	std::shared_ptr<TextureBuffer>m_xBlurResult;

	//縦ブラーの結果
	std::shared_ptr<TextureBuffer>m_yBlurResult;

	//最終結果
	std::shared_ptr<TextureBuffer>m_finalResult;

public:
	//ソース画像 & 結果描画先設定、ぼかし強さ
	GaussianBlur(const Vec2<int>& Size, const DXGI_FORMAT& Format, const float& BlurPower = 8.0f);
	//ボケ具合
	void SetBlurPower(const float& BlurPower);
	//即時実行
	void Execute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& CmdList, const std::shared_ptr<TextureBuffer>& SourceTex);
	//グラフィックスマネージャに登録
	void Register(const std::shared_ptr<TextureBuffer>& SourceTex);

	//結果のテクスチャ取得
	std::shared_ptr<TextureBuffer>& GetResultTex() { return m_finalResult; }

	//ウィンドウサイズで結果を描画
	void DrawResult(const AlphaBlendMode& AlphaBlend);
};