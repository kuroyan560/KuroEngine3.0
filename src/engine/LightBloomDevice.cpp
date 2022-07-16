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
	m_pipeline = D3D12App::Instance()->GenerateComputePipeline(cs, rootParams, { WrappedSampler(false,false) });
}

LightBloomDevice::LightBloomDevice()
{
	//パイプライン生成
	if (!m_pipeline)GeneratePipeline();

	//ウィンドウサイズ取得
	const auto winSize = WinApp::Instance()->GetExpandWinSize().Int();

	//エミッシブマップのフォーマット

	//エミッシブマップ
	m_emissiveMap = D3D12App::Instance()->GenerateRenderTarget(D3D12App::Instance()->GetBackBuffFormat(), Color(0, 0, 0, 0), winSize, L"LightBloom - EmissiveMap");
	m_emissiveMapDepth = D3D12App::Instance()->GenerateDepthStencil(winSize);
	m_emissiveMapFiltered = D3D12App::Instance()->GenerateTextureBuffer(winSize, DXGI_FORMAT_R32G32B32A32_FLOAT, "LightBloom - EmissiveMap - Filtered");

	//定数バッファ生成
	m_constBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(LightBloomConfig), 1, &m_config, "LgihtBloom - Config - ConstantBuffer");

	//ガウシアンブラー
	m_gaussianBlur = std::make_shared<GaussianBlur>(winSize, DXGI_FORMAT_R32G32B32A32_FLOAT);
}

void LightBloomDevice::SetRenderTargets()
{
	KuroEngine::Instance().Graphics().SetRenderTargets({ m_emissiveMap }, m_emissiveMapDepth);
}

void LightBloomDevice::Draw(const bool& Clear)
{
	//エミッシブマップをしきい値などに応じて加工
	KuroEngine::Instance().Graphics().SetComputePipeline(m_pipeline);
	static const int DIV = 16;
	Vec3<int>threadNum = { m_emissiveMap->GetGraphSize().x / DIV,m_emissiveMap->GetGraphSize().y / DIV,1 };
	KuroEngine::Instance().Graphics().Dispatch(threadNum, { m_constBuff,m_emissiveMap,m_emissiveMapFiltered }, { CBV,SRV,UAV });

	//エミッシブマップにブラーをかける
	m_gaussianBlur->Register(m_emissiveMapFiltered);

	//結果を描画
	m_gaussianBlur->DrawResult(AlphaBlendMode_Add);

	//レンダーターゲットのクリア
	if (Clear)
	{
		KuroEngine::Instance().Graphics().ClearRenderTarget(m_emissiveMap);
		KuroEngine::Instance().Graphics().ClearDepthStencil(m_emissiveMapDepth);
	}
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
