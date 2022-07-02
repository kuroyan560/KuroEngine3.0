#include "LightBloomDevice.h"
#include"KuroEngine.h"
#include"GaussianBlur.h"
#include"DrawFunc2D.h"

void LightBloomDevice::GeneratePipeline()
{
	//シェーダー
	auto cs = D3D12App::Instance()->CompileShader("resource/engine/LightBloom.hlsl", "CSmain", "cs_5_0");

	//ルートパラメータ
	std::vector<RootParam>rootParams =
	{
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"フラクタルパーリンノイズ生成情報"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"エミッシブマップ"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,"加工後エミッシブマップ"),
	};

	//パイプライン生成
	PIPELINE = D3D12App::Instance()->GenerateComputePipeline(cs, rootParams, { WrappedSampler(false,false) });
}

LightBloomDevice::LightBloomDevice()
{
	//パイプライン生成
	if (!PIPELINE)GeneratePipeline();

	//ウィンドウサイズ取得
	const auto winSize = WinApp::Instance()->GetExpandWinSize().Int();

	//エミッシブマップのフォーマット

	//エミッシブマップ
	emissiveMap = D3D12App::Instance()->GenerateRenderTarget(D3D12App::Instance()->GetBackBuffFormat(), Color(0, 0, 0, 0), winSize, L"LightBloom - EmissiveMap");
	emissiveMapDepth = D3D12App::Instance()->GenerateDepthStencil(winSize);
	emissiveMapFiltered = D3D12App::Instance()->GenerateTextureBuffer(winSize, DXGI_FORMAT_R32G32B32A32_FLOAT, "LightBloom - EmissiveMap - Filtered");

	//定数バッファ生成
	constBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(LightBloomConfig), 1, &config, "LgihtBloom - Config - ConstantBuffer");

	//ガウシアンブラー
	gaussianBlur = std::make_shared<GaussianBlur>(winSize, DXGI_FORMAT_R32G32B32A32_FLOAT);
}

void LightBloomDevice::SetRenderTargets()
{
	KuroEngine::Instance().Graphics().SetRenderTargets({ emissiveMap }, emissiveMapDepth);
}

void LightBloomDevice::Draw(const bool& Clear)
{
	//エミッシブマップをしきい値などに応じて加工
	KuroEngine::Instance().Graphics().SetComputePipeline(PIPELINE);
	static const int DIV = 16;
	Vec3<int>threadNum = { emissiveMap->GetGraphSize().x / DIV,emissiveMap->GetGraphSize().y / DIV,1 };
	KuroEngine::Instance().Graphics().Dispatch(threadNum, { constBuff,emissiveMap,emissiveMapFiltered }, { CBV,SRV,UAV });

	//エミッシブマップにブラーをかける
	gaussianBlur->Register(emissiveMapFiltered);

	//結果を描画
	gaussianBlur->DrawResult(AlphaBlendMode_Add);

	//レンダーターゲットのクリア
	if (Clear)
	{
		KuroEngine::Instance().Graphics().ClearRenderTarget(emissiveMap);
		KuroEngine::Instance().Graphics().ClearDepthStencil(emissiveMapDepth);
	}
}

void LightBloomDevice::SetOutputColorRate(const Vec3<float>& RGBrate)
{
	config.outputColorRate = RGBrate;
	constBuff->Mapping(&config);
}

void LightBloomDevice::SetBrightThreshold(const float& Threshold)
{
	config.brightThreshold = Threshold;
	constBuff->Mapping(&config);
}

void LightBloomDevice::SetBlurPower(const float& BlurPower)
{
	gaussianBlur->SetBlurPower(BlurPower);
}
