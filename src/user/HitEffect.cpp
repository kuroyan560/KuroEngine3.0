#include "HitEffect.h"
#include"KuroEngine.h"
#include"DrawFunc2D.h"
#include"NoiseGenerator.h"

std::array<HitEffect, HitEffect::MAX_NUM>HitEffect::INSTANCES;
static const Vec2<int>IMG_SIZE = { 512,512 };

void HitEffect::Generate(const Vec2<float>& Pos)
{
	for (auto& e : INSTANCES)
	{
		if (e.isActive)continue;

		e.isActive = 1;
		e.pos = Pos;
		e.scale = 0.0f;
		e.rotate = KuroFunc::GetRand(Angle::ROUND());
		e.alpha = 1.0f;
		e.lifeTimer = 0.0f;
		e.lifeSpan = 15;
		e.blur = 34.158f;
		e.uvRadiusOffset = 0.0f;
		e.circleThickness = 0.125f;
		e.circleRadius = 0.18f;
		break;
	}
}

void HitEffect::Init()
{
	for (auto& e : INSTANCES)e.isActive = 0;
}

void HitEffect::Update()
{
	for (auto& e : INSTANCES)
	{
		if (!e.isActive)continue;

		float rate = e.lifeTimer;
		//e.circleRadius = KuroMath::Ease(In, Circ, rate, 0.192f, 0.186f);
		e.circleThickness = KuroMath::Ease(Out, Exp, rate, 0.125f, 0.212f);
		e.uvRadiusOffset = KuroMath::Ease(Out, Exp, rate, 0.0f, 0.05f);
		e.scale = KuroMath::Ease(Out, Exp, rate, 0.0f, 1.0f);
		e.alpha = KuroMath::Ease(In, Circ, rate, 1.0f, 0.0f);

		//寿命
		e.lifeTimer += 1.0f / e.lifeSpan;
		if (1.0f <= e.lifeTimer)e.isActive = false;
	}
}

void HitEffect::Draw(std::shared_ptr<TextureBuffer>& Noise, std::shared_ptr<TextureBuffer>& Noise2)
{
	static std::shared_ptr<GraphicsPipeline>PIPELINE;
	static std::shared_ptr<TextureBuffer>DISPLACEMENT_NOISE_TEX;
	static std::shared_ptr<TextureBuffer>ALPHA_NOISE_TEX;
	static std::shared_ptr<VertexBuffer>VERTEX_BUFF;
	if (!PIPELINE)
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		PIPELINE_OPTION.depthWriteMask = false;

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/user/HitEffect.hlsl", "VSmain", "vs_5_0");
		SHADERS.gs = D3D12App::Instance()->CompileShader("resource/user/HitEffect.hlsl", "GSmain", "gs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/user/HitEffect.hlsl", "PSmain", "ps_5_0");

		//インプットレイアウト
		static std::vector<InputLayoutParam>INPUT_LAYOUT =
		{
			InputLayoutParam("ALIVE",DXGI_FORMAT_R8_SINT),
			InputLayoutParam("POSITION",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("SCALE",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("ROTATE",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("ALPHA",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("LIFE_TIMER",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("LIFE_SPAN",DXGI_FORMAT_R32_SINT),
			InputLayoutParam("BLUR",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("UV_RADIUS_OFFSET",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("CIRCLE_THICKNESS",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("CIRCLE_RADIUS",DXGI_FORMAT_R32_FLOAT),
		};

		//ルートパラメータ
		static std::vector<RootParam>ROOT_PARAMETER =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"平行投影行列定数バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ディスプレイスメント用パーリンノイズテクスチャ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"アルファ用パーリンノイズテクスチャ"),
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_Trans) };
		//パイプライン生成
		PIPELINE = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, INPUT_LAYOUT, ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(true, true) });

		//パーリンノイズ
		DISPLACEMENT_NOISE_TEX = NoiseGenerator::PerlinNoise(IMG_SIZE, 12, 6, 1.647f, 0.775f);
		ALPHA_NOISE_TEX = NoiseGenerator::PerlinNoise(IMG_SIZE, 7, 2, 0.79f, 0.5f);

		//頂点バッファ生成
		VERTEX_BUFF = D3D12App::Instance()->GenerateVertexBuffer(sizeof(HitEffect), MAX_NUM, nullptr, "HitEffect - VertexBuffer");
	}

	//頂点バッファデータ転送
	VERTEX_BUFF->Mapping(INSTANCES.data());

	//パイプラインセット
	KuroEngine::Instance().Graphics().SetGraphicsPipeline(PIPELINE);

	KuroEngine::Instance().Graphics().ObjectRender(
		VERTEX_BUFF,
		{
			KuroEngine::Instance().GetParallelMatProjBuff(),
			Noise,
			//DISPLACEMENT_NOISE_TEX,
			Noise2,
			//ALPHA_NOISE_TEX
		},
		{ CBV,SRV,SRV },
		0.0f, true
	);
}
