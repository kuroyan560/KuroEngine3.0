#include "Material.h"
#include"D3D12App.h"

Material::Material()
{
	const auto BLACK = D3D12App::Instance()->GenerateTextureBuffer(Color(0.0f, 0.0f, 0.0f, 0.0f));
	static std::shared_ptr<TextureBuffer> DEFAULT[MATERIAL_TEX_TYPE_NUM] =
	{
		//D3D12App::Instance()->GenerateTextureBuffer(Color(1.0f, 0.0f, 1.0f, 1.0f)),		//ベタ塗り色(Diffuse)
		D3D12App::Instance()->GenerateTextureBuffer(Color(0.0f, 0.0f, 0.0f, 1.0f)),		//ベタ塗り色(Diffuse)
		D3D12App::Instance()->GenerateTextureBuffer(Color(0.0f, 0.0f, 0.0f, 1.0f)),		//発光
		D3D12App::Instance()->GenerateTextureBuffer(Color(0.5f, 0.5f, 1.0f, 1.0f)),	//法線
		BLACK,		//ベースカラー(PBR)
		BLACK,		//金属度(PBR)
		BLACK,		//粗さ(PBR)
	};
	for (int i = 0; i < MATERIAL_TEX_TYPE_NUM; ++i)
	{
		texBuff[i] = DEFAULT[i];
	}
}

void Material::CreateBuff()
{
	//既に生成済み
	if (!invalid)return;
	buff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(ConstData), 1, &constData, name.c_str());
	
	invalid = false;
}

void Material::Mapping()
{
	buff->Mapping(&constData);
}
