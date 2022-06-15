#pragma once
#include"Vec.h"
#include<string>
#include<memory>
#include<array>

class ConstantBuffer;
class TextureBuffer;
enum MATERIAL_TEX_TYPE { COLOR_TEX, EMISSIVE_TEX, NORMAL_TEX, BASE_COLOR_TEX, METALNESS_TEX, ROUGHNESS_TEX, MATERIAL_TEX_TYPE_NUM };

class Material
{
private:
	bool invalid = true;
public:
	std::string name = "DefaultMaterial";
	struct Lambert
	{
		Vec3<float> ambient = { 0.2f,0.2f,0.2f };	//環境光(全方向から当たる光)
		float ambientFactor = 1.0f;			//強度(weight)
		Vec3<float> diffuse = { 1.0f,1.0f,1.0f };	//拡散反射光(モデルが本来持っている色味)
		float diffuseFactor = 1.0f;
		Vec3<float> emissive = { 0,0,0 };		//放射光(モデル自身が放つ光、暗い環境で光る)
		float emissiveFactor = 1.0f;
		float transparent = 0.0f;				//透過度
	};
	struct Phong
	{
		Vec3<float> specular = { 1,1,1 };	//スペキュラ(モデルの艶)
		float specularFactor = 1.0f;
		float shininess = 0.0f;					//光沢
		float reflection = 0.0f;					//反射(値が大きいと鏡のようになる)
		float pad;
	};
	struct PBR
	{
		Vec3<float>baseColor = { 0.0f,0.0f,0.0f };
		//float subsurface = 0.0f;
		float metalness = 0.0f;
		float specular = 0.5f;
		//Vec3<float> specularColor = { 1.0f,1.0f,1.0f };
		float roughness = 0.0f;
		//float specularAnisotropy = 0.0f;
		//float sheen = 0.0f;
		//Vec3<float> sheenColor = { 1.0f,1.0f,1.0f };
		//float coarRoughness = 0.0f;
		//float coat = 0.0f;
	};
	struct ConstData
	{
		Lambert lambert;
		Phong phong;
		PBR pbr;
	}constData;
	std::shared_ptr<ConstantBuffer>buff;

	std::array<std::shared_ptr<TextureBuffer>, MATERIAL_TEX_TYPE_NUM> texBuff;

	Material();
	void CreateBuff();
	void Mapping();
};