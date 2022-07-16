#pragma once
#include<memory>
class GraphicsPipeline;
class ConstantBuffer;
class LightManager;
class TextureBuffer;

#include"Color.h"
#include"KuroMath.h"
#include"SpriteMesh.h"
#include"Transform2D.h"

class Sprite_Shadow
{
private:
	static std::shared_ptr<GraphicsPipeline>s_transPipeline;
	//視点座標用の定数バッファ
	static std::shared_ptr<ConstantBuffer>s_EyePosBuff;
	//白のベタ塗りテクスチャ
	static std::shared_ptr<TextureBuffer>s_defaultTex;
	// (0,0,-1) のベタ塗りノーマルマップ
	static std::shared_ptr<TextureBuffer>s_defaultNormalMap;
	//黒のベタ塗りテクスチャ
	static std::shared_ptr<TextureBuffer>s_defaultEmissiveMap;

public:
	static void SetEyePos(Vec3<float> EyePos);

private:
	//定数バッファ送信用データ
	struct ConstantData
	{
		Matrix m_mat;
		Color m_color;
		float z = 0.0f;
		float m_diffuse = 1.0f;	//ディヒューズの影響度
		float m_specular = 1.0f;	//スペキュラーの影響度
		float m_lim = 1.0f;	//リムライトの影響度
	}m_constData;

	//定数バッファ
	std::shared_ptr<ConstantBuffer>m_constBuff;

	//テクスチャバッファ
	std::shared_ptr<TextureBuffer>m_texBuff;
	//ノーマルマップ
	std::shared_ptr<TextureBuffer>m_normalMap;
	//エミッシブマップ
	std::shared_ptr<TextureBuffer>m_emissiveMap;

public:
	//トランスフォーム
	Transform2D m_transform;
	//メッシュ（頂点情報）
	SpriteMesh m_mesh;

	//テクスチャ、ノーマルマップ、スプライト名
	Sprite_Shadow(const std::shared_ptr<TextureBuffer>& Texture = nullptr, const std::shared_ptr<TextureBuffer>& NormalMap = nullptr,
		const std::shared_ptr<TextureBuffer>& EmissiveMap = nullptr, const char* Name = nullptr);

	//テクスチャとノーマルマップセット
	void SetTexture(const std::shared_ptr<TextureBuffer>& Texture, const std::shared_ptr<TextureBuffer>& NormalMap, const std::shared_ptr<TextureBuffer>& EmissiveMap);

	//色セット（描画の際にこの値が乗算される）
	void SetColor(const Color& Color);

	//陰影をつける際に使う深度値
	void SetDepth(const float& Depth);

	//ライトの影響度設定
	void SetDiffuseAffect(const float& Diffuse);
	void SetSpecularAffect(const float& Specular);
	void SetLimAffect(const float& Lim);

	//描画
	void Draw(LightManager& LigManager);
};

