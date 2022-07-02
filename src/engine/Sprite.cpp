#include "Sprite.h"
#include"KuroEngine.h"

std::shared_ptr<GraphicsPipeline>Sprite::PIPELINE[AlphaBlendModeNum];
std::shared_ptr<TextureBuffer>Sprite::DEFAULT_TEX;

Sprite::Sprite(const std::shared_ptr<TextureBuffer>& Texture, const char* Name) : mesh(Name)
{
	if (!PIPELINE[0])
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		PIPELINE_OPTION.depthTest = false;

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/engine/Sprite.hlsl", "VSmain", "vs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/engine/Sprite.hlsl", "PSmain", "ps_5_0");

		//インプットレイアウト
		static std::vector<InputLayoutParam>INPUT_LAYOUT =
		{
			InputLayoutParam("POSITION",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("TEXCOORD",DXGI_FORMAT_R32G32_FLOAT)
		};

		//ルートパラメータ
		static std::vector<RootParam>ROOT_PARAMETER =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"平行投影行列定数バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"テクスチャシェーダーリソース"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カラー & 行列定数バッファ")
		};

		//レンダーターゲット描画先情報
		for (int i = 0; i < AlphaBlendModeNum; ++i)
		{
			std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), (AlphaBlendMode)i) };
			//パイプライン生成
			PIPELINE[i] = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, INPUT_LAYOUT, ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(true, false) });
		}

		//白テクスチャ
		DEFAULT_TEX = D3D12App::Instance()->GenerateTextureBuffer(Color(1.0f, 1.0f, 1.0f, 1.0f));
	}

	//デフォルトのテクスチャバッファ
	texBuff = DEFAULT_TEX;

	//テクスチャセット
	SetTexture(Texture);

	//行列初期化
	constData.mat = transform.GetMat();

	//定数バッファ生成
	constBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(constData), 1, &constData, Name);
}

void Sprite::SetTexture(const std::shared_ptr<TextureBuffer>& Texture)
{
	if (Texture == nullptr)return;
	texBuff = Texture;
}

void Sprite::SetColor(const Color& Color)
{
	if (constData.color == Color)return;
	constData.color = Color;
	constBuff->Mapping(&constData);
}

void Sprite::Draw(const AlphaBlendMode& BlendMode)
{
	KuroEngine::Instance().Graphics().SetGraphicsPipeline(PIPELINE[(int)BlendMode]);

	if (transform.GetDirty())
	{
		constData.mat = transform.GetMat();
		constBuff->Mapping(&constData);
	}

	mesh.Render({
		KuroEngine::Instance().GetParallelMatProjBuff(),	//平行投影行列
		texBuff,			//テクスチャリソース
		constBuff },//カラー & ワールド行列
		{ CBV,SRV,CBV });
}