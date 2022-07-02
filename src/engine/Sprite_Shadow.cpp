#include "Sprite_Shadow.h"
#include"KuroEngine.h"
#include"LightManager.h"

std::shared_ptr<GraphicsPipeline>Sprite_Shadow::PIPELINE_TRANS;
std::shared_ptr<ConstantBuffer>Sprite_Shadow::EYE_POS_BUFF;
std::shared_ptr<TextureBuffer>Sprite_Shadow::DEFAULT_TEX;
std::shared_ptr<TextureBuffer>Sprite_Shadow::DEFAULT_NORMAL_MAP;
std::shared_ptr<TextureBuffer>Sprite_Shadow::DEFAULT_EMISSIVE_MAP;

void Sprite_Shadow::SetEyePos(Vec3<float> EyePos)
{
	EYE_POS_BUFF->Mapping(&EyePos);
}

Sprite_Shadow::Sprite_Shadow(const std::shared_ptr<TextureBuffer>& Texture, const std::shared_ptr<TextureBuffer>& NormalMap, const std::shared_ptr<TextureBuffer>& EmissiveMap, const char* Name)
{
	//静的メンバの初期化
	if (!PIPELINE_TRANS)
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		PIPELINE_OPTION.depthTest = false;

		//インプットレイアウト
		static std::vector<InputLayoutParam>INPUT_LAYOUT =
		{
			InputLayoutParam("POSITION",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("TEXCOORD",DXGI_FORMAT_R32G32_FLOAT)
		};

		//レンダーターゲット描画先情報
		static std::vector<RenderTargetInfo>RENDER_TARGET_INFO =
		{
			//バックバッファのフォーマット、アルファブレンド
			RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(),AlphaBlendMode_Trans),
			//エミッシブ
			RenderTargetInfo(DXGI_FORMAT_R32G32B32A32_FLOAT,AlphaBlendMode_Trans)
		};

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/engine/Sprite_Shadow.hlsl", "VSmain", "vs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/engine/Sprite_Shadow.hlsl", "PSmain", "ps_5_0");

		//ルートパラメータ
		static std::vector<RootParam>ROOT_PARAMETER =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"平行投影行列定数バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カラー & 行列定数 & スプライトのZ設定値バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"視点座標情報"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"アクティブ中のライト数バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ディレクションライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ポイントライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"スポットライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"天球ライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"テクスチャシェーダーリソース"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ノーマルマップシェーダーリソース"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"エミッシブマップシェーダーリソース"),
		};

		//パイプライン生成
		PIPELINE_TRANS = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, INPUT_LAYOUT, ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(false, true) });

		//視点座標
		Vec3<float>INIT_EYE_POS = { WinApp::Instance()->GetExpandWinCenter().x,WinApp::Instance()->GetExpandWinCenter().y,-5.0f };
		EYE_POS_BUFF = D3D12App::Instance()->GenerateConstantBuffer(sizeof(Vec3<float>), 1, &INIT_EYE_POS, "Sprite_Shadow - EYE_POS");

		//白テクスチャ
		DEFAULT_TEX = D3D12App::Instance()->GenerateTextureBuffer(Color(1.0f, 1.0f, 1.0f, 1.0f));

		// (0,0,-1) ノーマルマップ
		DEFAULT_NORMAL_MAP = D3D12App::Instance()->GenerateTextureBuffer(Color(0.5f, 0.5f, 1.0f, 1.0f));

		//黒テクスチャ
		DEFAULT_EMISSIVE_MAP = D3D12App::Instance()->GenerateTextureBuffer(Color(0.0f, 0.0f, 0.0f, 1.0f));
	}

	//デフォルトのテクスチャバッファ
	texBuff = DEFAULT_TEX;
	normalMap = DEFAULT_NORMAL_MAP;
	emissiveMap = DEFAULT_EMISSIVE_MAP;
	
	//テクスチャセット
	SetTexture(Texture, NormalMap, EmissiveMap);

	//行列初期化
	constData.mat = transform.GetMat();

	//定数バッファ生成
	constBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(constData), 1, &constData, Name);
}

void Sprite_Shadow::SetTexture(const std::shared_ptr<TextureBuffer>& Texture, const std::shared_ptr<TextureBuffer>& NormalMap, const std::shared_ptr<TextureBuffer>& EmissiveMap)
{
	if (Texture != nullptr)
	{
		texBuff = Texture;
	}
	if (NormalMap != nullptr)
	{
		normalMap = NormalMap;
	}
	if (EmissiveMap != nullptr)
	{
		emissiveMap = EmissiveMap;
	}
}

void Sprite_Shadow::SetColor(const Color& Color)
{
	if (constData.color == Color)return;
	constData.color = Color;
	constBuff->Mapping(&constData);
}

void Sprite_Shadow::SetDepth(const float& Depth)
{
	if (constData.z == Depth)return;
	constData.z = Depth;
	constBuff->Mapping(&constData);
}

void Sprite_Shadow::SetDiffuseAffect(const float& Diffuse)
{
	if (constData.diffuse == Diffuse)return;
	constData.diffuse = Diffuse;
	constBuff->Mapping(&constData);
}

void Sprite_Shadow::SetSpecularAffect(const float& Specular)
{
	if (constData.specular == Specular)return;
	constData.specular = Specular;
	constBuff->Mapping(&constData);
}

void Sprite_Shadow::SetLimAffect(const float& Lim)
{
	if (constData.lim == Lim)return;
	constData.lim = Lim;
	constBuff->Mapping(&constData);
}

void Sprite_Shadow::Draw(LightManager& LigManager)
{
	KuroEngine::Instance().Graphics().SetGraphicsPipeline(PIPELINE_TRANS);

	if (transform.GetDirty())
	{
		constData.mat = transform.GetMat();
		constBuff->Mapping(&constData);
	}

	mesh.Render({
		KuroEngine::Instance().GetParallelMatProjBuff(),	//平行投影行列
		constBuff,	//カラー & ワールド行列
		EYE_POS_BUFF,	//視点座標
		LigManager.GetLigNumInfo(),	//アクティブ中のライト数
		LigManager.GetLigInfo(Light::DIRECTION),	//ディレクションライト
		LigManager.GetLigInfo(Light::POINT),	//ポイントライト
		LigManager.GetLigInfo(Light::SPOT),	//スポットライト
		LigManager.GetLigInfo(Light::HEMISPHERE),	//天球ライト
		texBuff,			//テクスチャ
		normalMap, 	//ノーマルマップ
		emissiveMap,	//エミッシブマップ
		},
		{ CBV,CBV,CBV,CBV,SRV,SRV,SRV,SRV,SRV,SRV,SRV });
}
