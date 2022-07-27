#include "LightBloomDevice.h"
#include"KuroEngine.h"
#include"GaussianBlur.h"
#include"DrawFunc2D.h"

std::shared_ptr<ComputePipeline>LightBloomDevice::s_pipeline;

void LightBloomDevice::GeneratePipeline()
{
	//シェーダー
	auto cs = D3D12App::Instance()->CompileShader("resource/engine/LightBloom.hlsl", "CSmain", "cs_6_4");

	//ルートパラメータ
	std::vector<RootParam>rootParams =
	{
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"フラクタルパーリンノイズ生成情報"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"エミッシブマップ"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,"加工後エミッシブマップ"),
	};

	//パイプライン生成
	s_pipeline = D3D12App::Instance()->GenerateComputePipeline(cs, rootParams, { WrappedSampler(false,false) });
}

LightBloomDevice::LightBloomDevice()
{
	//パイプライン生成
	if (!s_pipeline)GeneratePipeline();

	//ウィンドウサイズ取得
	const auto winSize = WinApp::Instance()->GetExpandWinSize().Int();

	//エミッシブマップ
	m_processedEmissiveMap = D3D12App::Instance()->GenerateTextureBuffer(winSize, DXGI_FORMAT_R32G32B32A32_FLOAT, "LightBloom - EmissiveMap - Processed");

	//定数バッファ生成
	m_constBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(LightBloomConfig), 1, &m_config, "LgihtBloom - Config - ConstantBuffer");

	//ガウシアンブラー
	m_gaussianBlur = std::make_shared<GaussianBlur>(winSize, DXGI_FORMAT_R32G32B32A32_FLOAT);
}

void LightBloomDevice::Draw(std::weak_ptr<RenderTarget>EmissiveMap, std::weak_ptr<RenderTarget>Target)
{
	auto emissiveMap = EmissiveMap.lock();

	//エミッシブマップをしきい値などに応じて加工
	KuroEngine::Instance()->Graphics().SetComputePipeline(s_pipeline);
	static const int DIV = 32;
	Vec3<int>threadNum = { emissiveMap->GetGraphSize().x / DIV,emissiveMap->GetGraphSize().y / DIV,1 };
	KuroEngine::Instance()->Graphics().Dispatch(threadNum, { m_constBuff,emissiveMap,m_processedEmissiveMap }, { CBV,SRV,UAV });

	//エミッシブマップにブラーをかける
	m_gaussianBlur->Register(m_processedEmissiveMap);

	KuroEngine::Instance()->Graphics().SetRenderTargets({ Target.lock() });

	//結果を描画
	m_gaussianBlur->DrawResult(AlphaBlendMode_Add);
}

void LightBloomDevice::SetOutputColorRate(const Vec3<float>& RGBrate)
{
	m_config.m_outputColorRate = RGBrate;
	m_constBuff->Mapping(&m_config);
}

void LightBloomDevice::SetBrightThreshold(const float& Threshold)
{
	m_config.m_brightThreshold = Threshold;
	m_constBuff->Mapping(&m_config);
}

void LightBloomDevice::SetBlurPower(const float& BlurPower)
{
	m_gaussianBlur->SetBlurPower(BlurPower);
}
