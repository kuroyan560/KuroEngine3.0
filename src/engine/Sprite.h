#pragma once
#include<memory>
class GraphicsPipeline;
class ConstantBuffer;

#include"Color.h"
#include"KuroMath.h"
#include"SpriteMesh.h"
#include"Transform2D.h"


class Sprite
{
private:
	static std::shared_ptr<GraphicsPipeline>s_pipeline[AlphaBlendModeNum];
	//白のベタ塗りテクスチャ
	static std::shared_ptr<TextureBuffer>s_defaultTex;

private:
	//定数バッファ送信用データ
	struct ConstantData
	{
		Matrix m_mat;
		Color m_color;
	}m_constData;

	//定数バッファ
	std::shared_ptr<ConstantBuffer>m_constBuff;

	//テクスチャバッファ
	std::shared_ptr<TextureBuffer>m_texBuff;

public:
	//トランスフォーム
	Transform2D m_transform;
	//メッシュ（頂点情報）
	SpriteMesh m_mesh;

	//テクスチャ、スプライト名
	Sprite(const std::shared_ptr<TextureBuffer>& Texture = nullptr, const char* Name = nullptr);

	//テクスチャセット
	void SetTexture(const std::shared_ptr<TextureBuffer>& Texture);

	//色セット（描画の際にこの値が乗算される）
	void SetColor(const Color& Color);
	
	//描画
	void Draw(const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans);
};