#pragma once
#include"Vec.h"
#include<string>
#include<array>
#include"Mesh.h"
#include<memory>
#include<vector>
#include"Transform.h"
class TextureBuffer;
class Camera;
class LightManager;
class ModelObject;
class ConstantBuffer;

class CubeMap
{
protected:
	static std::shared_ptr<TextureBuffer>s_defaultCubeMapTex;
public:
	enum SURFACE_TYPE
	{
		FRONT,	//+Z
		PZ = FRONT,
		BACK,	//-Z
		NZ = BACK,
		RIGHT,	//+X
		PX = RIGHT,
		LEFT,		//-X
		NX = LEFT, 
		UP,		//+Y
		PY = UP,
		DOWN,	//-Y
		NY = DOWN,
		SURFACE_NUM
	};

	//メッシュ名に付与するタグ
	static const std::array<std::string, SURFACE_NUM>s_surfaceNameTag;
	
protected:
	CubeMap();
	std::shared_ptr<TextureBuffer>m_cubeMap;	//ライティングで参照する（ディメンションがTEXTURE_CUBE）

public:

	const std::shared_ptr<TextureBuffer>& GetCubeMapTex() { return m_cubeMap; }
	void CopyCubeMap(std::shared_ptr<CubeMap>Src);
};

//静的キューブマップ
class StaticallyCubeMap : public CubeMap
{
private:
	struct Vertex
	{
		Vec3<float>m_pos;
		Vec2<float>m_uv;
	};
	struct Surface
	{
		Mesh<Vertex>m_mesh;
		std::shared_ptr<TextureBuffer>m_tex;
	};

public:
	static std::shared_ptr<StaticallyCubeMap>&GetDefaultCubeMap();

private:
	std::string m_name;
	float m_sideLength = 10.0f;	//辺の長さ
	std::array<Surface, SURFACE_NUM>m_surfaces;	//描画に使用
	std::shared_ptr<ConstantBuffer>m_transformBuff;

	void ResetMeshVertices();

public:
	Transform m_transform;

	StaticallyCubeMap(const std::string& Name, const float& SideLength = 100.0f);

	//辺の長さを設定
	void SetSideLength(const float& Length);

	//指定面に描画に用いるテクスチャをアタッチ
	void AttachTex(SURFACE_TYPE Surface, const std::shared_ptr<TextureBuffer>& Tex)
	{
		//YとXは逆
		if (Surface == PY)Surface = NY;
		else if (Surface == NY)Surface = PY;
		else if (Surface == PX)Surface = NX;
		else if (Surface == NX)Surface = PX;
		m_surfaces[Surface].m_tex = Tex;
	}
	void AttachTex(const std::string& Dir, const std::string& Extention);

	//描画に用いるテクスチャ取得
	std::shared_ptr<TextureBuffer>GetTex(const SURFACE_TYPE& Surface)
	{
		return m_surfaces[Surface].m_tex;
	}

	//ライティングに用いるテクスチャをアタッチ
	void AttachCubeMapTex(const std::shared_ptr<TextureBuffer>& CubeMapTex)
	{
		m_cubeMap = CubeMapTex;
	}

	//描画
	void Draw(Camera& Cam);

};

//動的キューブマップ
class DynamicCubeMap : public CubeMap
{
private:
	static int s_id;
	static std::shared_ptr<ConstantBuffer>s_viewProjMat;

private:
	std::shared_ptr<RenderTarget>m_cubeRenderTarget;
	std::shared_ptr<DepthStencil>m_cubeDepth;

public:
	DynamicCubeMap(const int& CubeMapEdge = 512);
	void Clear();
	void DrawToCubeMap(LightManager& LigManager, const std::vector<std::weak_ptr<ModelObject>>&ModelObject);
};