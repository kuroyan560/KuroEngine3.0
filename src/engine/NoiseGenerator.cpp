#include "NoiseGenerator.h"
#include<vector>
#include"KuroFunc.h"
#include"D3D12App.h"

int NoiseGenerator::PERLIN_NOISE_ID_2D = 0;

void NoiseGenerator::PerlinNoise2D(std::shared_ptr<TextureBuffer> DestTex, const Vec2<int>& Split, const int& Contrast, const int& Octaves, const float& Frequency, const float& Persistance)
{
	//最大分割数
	static const int SPLIT_MAX = 256;
	assert(0 < Split.x && Split.x <= SPLIT_MAX && 0 < Split.y && Split.y <= SPLIT_MAX);

	//コンピュートパイプライン
	static std::shared_ptr<ComputePipeline>PIPELINE;

	//定数バッファ
	static std::vector<std::shared_ptr<ConstantBuffer>>CONST_BUFF;
	//定数バッファ用データ
	struct ConstData
	{
		Vec2<float>rectLength;
		Vec2<int> split;
		int contrast;
		int octaveNum;
		float frequency;
		float persistance;
		ConstData(const Vec2<float>& RectLength, const Vec2<int>& Split, const int& Contrast, const int& Octaves, const float& Frequency, const float& Persistance)
			:rectLength(RectLength), split(Split), contrast(Contrast), octaveNum(Octaves), frequency(Frequency), persistance(Persistance) {}
	};

	//構造体バッファ
	static std::vector<std::shared_ptr<StructuredBuffer>>STRUCTURED_BUFF;

	if (!PIPELINE)
	{
		//シェーダ
		auto cs = D3D12App::Instance()->CompileShader("resource/engine/PerlinNoise2D.hlsl", "CSmain", "cs_5_0");
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
	if (CONST_BUFF.size() < PERLIN_NOISE_ID_2D + 1)
	{
		CONST_BUFF.emplace_back(D3D12App::Instance()->GenerateConstantBuffer(sizeof(ConstData), 1, nullptr, ("PerlinNoise2D - ConstantBuffer - " + std::to_string(PERLIN_NOISE_ID_2D)).c_str()));
	}

	//構造体バッファ生成
	if (STRUCTURED_BUFF.size() < PERLIN_NOISE_ID_2D + 1)
	{
		STRUCTURED_BUFF.emplace_back(D3D12App::Instance()->GenerateStructuredBuffer(sizeof(Vec2<float>), pow(SPLIT_MAX + 1, 2), nullptr, ("PerlinNoise2D - StructuredBuffer" + std::to_string(PERLIN_NOISE_ID_2D)).c_str()));
	}

	//定数バッファにデータ転送
	ConstData constData(DestTex->GetGraphSize().Float() / Split.Float(), Split, Contrast, Octaves, Frequency, Persistance);
	CONST_BUFF[PERLIN_NOISE_ID_2D]->Mapping(&constData);

	//分割後の各頂点の勾配ベクトル格納先
	Vec2<float>grad[(SPLIT_MAX + 1) * (SPLIT_MAX + 1)];
	for (int y = 0; y <= Split.y; ++y)
	{
		for (int x = 0; x <= Split.x; ++x)
		{
			int idx = y * (Split.x + 1) + x;
			//ランダムな勾配ベクトル
			grad[idx].x = KuroFunc::GetRand(1.0f) * KuroFunc::GetRandPlusMinus();
			grad[idx].y = KuroFunc::GetRand(1.0f) * KuroFunc::GetRandPlusMinus();

			if (x == Split.x)grad[idx] = grad[y * (Split.x + 1)];
			if (y == Split.y)grad[idx] = grad[x];
		}
	}
	//構造化バッファに転送
	STRUCTURED_BUFF[PERLIN_NOISE_ID_2D]->Mapping(grad);

	//コマンドリスト取得
	auto cmdList = D3D12App::Instance()->GetCmdList();

	//ディスクリプタヒープセット
	D3D12App::Instance()->SetDescHeaps();

	//パイプラインセット
	PIPELINE->SetPipeline(cmdList);

	//定数バッファセット
	CONST_BUFF[PERLIN_NOISE_ID_2D]->SetComputeDescriptorBuffer(cmdList, CBV, 0);

	//構造体バッファセット
	STRUCTURED_BUFF[PERLIN_NOISE_ID_2D]->SetComputeDescriptorBuffer(cmdList, SRV, 1);

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

	PERLIN_NOISE_ID_2D++;
}

std::shared_ptr<TextureBuffer> NoiseGenerator::PerlinNoise2D(const std::string& Name, const Vec2<int>& Size, const Vec2<int>& Split, const int& Contrast, const int& Octaves, const float& Frequency, const float& Persistance, const DXGI_FORMAT& Format)
{
	//描き込み先用テクスチャバッファ生成
	auto result = D3D12App::Instance()->GenerateTextureBuffer(Size, Format, Name.c_str());
	PerlinNoise2D(result, Split, Contrast, Octaves, Frequency, Persistance);
	return result;
}