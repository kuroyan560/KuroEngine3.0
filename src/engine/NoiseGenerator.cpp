#include "NoiseGenerator.h"
#include<vector>
#include"KuroFunc.h"
#include"D3D12App.h"

int NoiseGenerator::PERLIN_NOISE_ID = 0;

void NoiseGenerator::PerlinNoise(std::shared_ptr<TextureBuffer> DestTex, const int& Split, const int& Octaves, const float& Frequency, const float& Persistance)
{
	//最大分割数
	static const int SPLIT_MAX = 256;
	assert(0 < Split && Split <= SPLIT_MAX);

	//コンピュートパイプライン
	static std::shared_ptr<ComputePipeline>PIPELINE;

	//定数バッファ
	static std::vector<std::shared_ptr<ConstantBuffer>>CONST_BUFF;
	//定数バッファ用データ
	struct ConstData
	{
		Vec2<float>rectLength;
		int split;
		int octaveNum;
		float frequency;
		float persistance;
		ConstData(const Vec2<float>& RectLength, const int& Split, const int& Octaves, const float& Frequency, const float& Persistance)
			:rectLength(RectLength), split(Split), octaveNum(Octaves), frequency(Frequency), persistance(Persistance) {}
	};

	//構造体バッファ
	static std::vector<std::shared_ptr<StructuredBuffer>>STRUCTURED_BUFF;

	if (!PIPELINE)
	{
		//シェーダ
		auto cs = D3D12App::Instance()->CompileShader("resource/engine/PerlinNoise.hlsl", "CSmain", "cs_5_0");
		//ルートパラメータ
		std::vector<RootParam>rootParams =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"フラクタルパーリンノイズ生成情報"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"勾配ベクトル配列"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,"フラクタルパーリンノイズテクスチャ"),
		};
		//パイプライン生成
		PIPELINE = D3D12App::Instance()->GenerateComputePipeline(cs, rootParams, { WrappedSampler(false, false) });
	}

	//定数バッファ生成
	if (CONST_BUFF.size() < PERLIN_NOISE_ID + 1)
	{
		CONST_BUFF.emplace_back(D3D12App::Instance()->GenerateConstantBuffer(sizeof(ConstData), 1, nullptr, ("PerlinNoise - ConstantBuffer - " + std::to_string(PERLIN_NOISE_ID)).c_str()));
	}

	//構造体バッファ生成
	if (STRUCTURED_BUFF.size() < PERLIN_NOISE_ID + 1)
	{
		STRUCTURED_BUFF.emplace_back(D3D12App::Instance()->GenerateStructuredBuffer(sizeof(Vec2<float>), pow(SPLIT_MAX + 1, 2), nullptr, ("PerlinNoise - StructuredBuffer" + std::to_string(PERLIN_NOISE_ID)).c_str()));
	}

	//定数バッファにデータ転送
	ConstData constData(DestTex->GetGraphSize().Float() / Split, Split, Octaves, Frequency, Persistance);
	CONST_BUFF[PERLIN_NOISE_ID]->Mapping(&constData);

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

			if (x == Split)grad[idx] = grad[y * (Split + 1)];
			if (y == Split)grad[idx] = grad[x];
		}
	}
	//構造化バッファに転送
	STRUCTURED_BUFF[PERLIN_NOISE_ID]->Mapping(grad);

	//コマンドリスト取得
	auto cmdList = D3D12App::Instance()->GetCmdList();

	//ディスクリプタヒープセット
	D3D12App::Instance()->SetDescHeaps();

	//パイプラインセット
	PIPELINE->SetPipeline(cmdList);

	//定数バッファセット
	CONST_BUFF[PERLIN_NOISE_ID]->SetComputeDescriptorBuffer(cmdList, CBV, 0);

	//構造体バッファセット
	STRUCTURED_BUFF[PERLIN_NOISE_ID]->SetComputeDescriptorBuffer(cmdList, SRV, 1);

	//描き込み先テクスチャバッファセット
	DestTex->SetComputeDescriptorBuffer(cmdList, UAV, 2);


	//実行
	static const int THREAD_NUM = 16;
	const Vec2<UINT>thread =
	{
		static_cast<UINT>(DestTex->GetGraphSize().x / THREAD_NUM),
		static_cast<UINT>(DestTex->GetGraphSize().y / THREAD_NUM)
	};
	cmdList->Dispatch(thread.x, thread.y, 1);

	PERLIN_NOISE_ID++;
}

std::shared_ptr<TextureBuffer> NoiseGenerator::PerlinNoise(const Vec2<int>& Size, const int& Split, const int& Octaves, const float& Frequency, const float& Persistance)
{
	//描き込み先用テクスチャバッファ生成
	auto result = D3D12App::Instance()->GenerateTextureBuffer(Size, DXGI_FORMAT_R32G32B32A32_FLOAT, ("PerlinNoise - " + std::to_string(PERLIN_NOISE_ID)).c_str());
	PerlinNoise(result, Split, Octaves, Frequency, Persistance);
	return result;
}