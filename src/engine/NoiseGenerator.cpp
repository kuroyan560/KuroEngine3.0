#include "NoiseGenerator.h"
#include<vector>
#include"KuroFunc.h"
#include"D3D12App.h"

float NoiseGenerator::Wavelet(float t)
{
	//return 1 - 3 * pow(t, 2) + 2 * pow(abs(t), 3);
	return 1 - (6 * pow(abs(t), 5) - 15 * pow(t, 4) + 10 * pow(abs(t), 3));
}

std::shared_ptr<TextureBuffer>NoiseGenerator::PerlinNoise(const Vec2<int>& Size, const int& Split)
{
	//最大分割数
	static const int SPLIT_MAX = 256;
	assert(0 < Split && Split <= SPLIT_MAX);

	//生成した数（画像名にIDとして付与する）
	static int ID = 0;

	//コンピュートパイプライン
	static std::shared_ptr<ComputePipeline>PIPELINE;

	//定数バッファ
	static std::shared_ptr<ConstantBuffer>CONST_BUFF;
	//定数バッファ用データ
	struct ConstData
	{
		Vec2<float>rectLength;
		int split;
		ConstData(const Vec2<float>& RectLength, const int& Split) :rectLength(RectLength), split(Split) {}
	};

	//構造体バッファ
	static std::shared_ptr<StructuredBuffer>STRUCTURED_BUFF;

	if (!PIPELINE)
	{
		//シェーダ
		auto cs = D3D12App::Instance()->CompileShader("resource/engine/PerlinNoise.hlsl", "CSmain", "cs_5_0");
		//ルートパラメータ
		std::vector<RootParam>rootParams =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"パーリンノイズ生成情報"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"勾配ベクトル配列"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,"パーリンノイズテクスチャ"),
		};
		//パイプライン生成
		PIPELINE = D3D12App::Instance()->GenerateComputePipeline(cs, rootParams, { WrappedSampler(false, false) });

		//定数バッファ生成
		CONST_BUFF = D3D12App::Instance()->GenerateConstantBuffer(sizeof(ConstData), 1, nullptr, "PerlinNoise - ConstantBuffer");

		//構造体バッファ生成
		STRUCTURED_BUFF = D3D12App::Instance()->GenerateStructuredBuffer(sizeof(Vec2<float>), pow(SPLIT_MAX + 1, 2), nullptr, "PerlinNoise - StructuredBuffer");
	}

	//定数バッファにデータ転送
	ConstData constData(Size.Float() / Split, Split);
	CONST_BUFF->Mapping(&constData);

	//分割後の各頂点の勾配ベクトル格納先
	Vec2<float>grad[(SPLIT_MAX + 1) * (SPLIT_MAX + 1)];
	for (int y = 0; y <= Split; ++y)
	{
		for (int x = 0; x <= Split; ++x)
		{
			int idx = y * (Split + 1) + x;
			//ランダムな勾配ベクトル
			grad[idx].x = KuroFunc::GetRand(1.0f) * KuroFunc::GetRandPlusMinus();
			grad[idx].y = KuroFunc::GetRand(1.0f) * KuroFunc::GetRandPlusMinus();
		}
	}
	//構造化バッファに転送
	STRUCTURED_BUFF->Mapping(grad);

	//描き込み先用テクスチャバッファ生成
	auto result = D3D12App::Instance()->GenerateTextureBuffer(Size, DXGI_FORMAT_R32G32B32A32_FLOAT, ("PerlinNoise - " + std::to_string(ID++)).c_str());

	//コマンドリスト取得
	auto cmdList = D3D12App::Instance()->GetCmdList();

	//ディスクリプタヒープセット
	D3D12App::Instance()->SetDescHeaps();

	//パイプラインセット
	PIPELINE->SetPipeline(cmdList);

	//定数バッファセット
	CONST_BUFF->SetComputeDescriptorBuffer(cmdList, CBV, 0);

	//構造体バッファセット
	STRUCTURED_BUFF->SetComputeDescriptorBuffer(cmdList, SRV, 1);

	//描き込み先テクスチャバッファセット
	result->SetComputeDescriptorBuffer(cmdList, UAV, 2);

	//実行
	static const int THREAD_NUM = 8;
	const Vec2<UINT>thread =
	{
		static_cast<UINT>(Size.x / THREAD_NUM),
		static_cast<UINT>(Size.y / THREAD_NUM)
	};
	//cmdList->Dispatch(thread.x, thread.y, 1);
	cmdList->Dispatch(Size.x, Size.y, 1);

	//描き込んだ画像のリソースバリア変更
	result->ChangeBarrier(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	
	return result;
}
