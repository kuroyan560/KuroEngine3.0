#pragma once
#include"Vec.h"
#include<string>
#include<array>
#include"Mesh.h"
#include<memory>
#include<vector>
class TextureBuffer;
class Camera;
class LightManager;
class ModelObject;

class CubeMap
{
protected:
	static std::shared_ptr<TextureBuffer>DEFAULT_CUBE_MAP_TEX;
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
	static const std::array<std::string, SURFACE_NUM>SURFACE_NAME_TAG;
	
protected:
	CubeMap();
	std::shared_ptr<TextureBuffer>cubeMap;	//ライティングで参照する（ディメンションがTEXTURE_CUBE）

public:

	const std::shared_ptr<TextureBuffer>& GetCubeMapTex() { return cubeMap; }
	void CopyCubeMap(std::shared_ptr<CubeMap>Src);
};

//静的キューブマップ
class StaticallyCubeMap : public CubeMap
{
private:
	struct Vertex
	{
		Vec3<float>pos;
		Vec2<float>uv;
	};
	struct Surface
	{
		Mesh<Vertex>mesh;
		std::shared_ptr<TextureBuffer>tex;
	};

public:
	static std::shared_ptr<StaticallyCubeMap>&GetDefaultCubeMap();

private:
	std::string name;
	float sideLength = 10.0f;	//辺の長さ
	std::array<Surface, SURFACE_NUM>surfaces;	//描画に使用

	void ResetMeshVertices();

public:
	StaticallyCubeMap(const std::string& Name, const float& SideLength = 100.0f);

	//辺の長さを設定
	void SetSideLength(const float& Length);

	//指定面に描画に用いるテクスチャをアタッチ
	void AttachTex(const SURFACE_TYPE& Surface, const std::shared_ptr<TextureBuffer>& Tex)
	{
		surfaces[Surface].tex = Tex;
	}
	void AttachTex(const std::string& Dir, const std::string& Extention);

	//描画に用いるテクスチャ取得
	std::shared_ptr<TextureBuffer>GetTex(const SURFACE_TYPE& Surface)
	{
		return surfaces[Surface].tex;
	}

	//ライティングに用いるテクスチャをアタッチ
	void AttachCubeMapTex(const std::shared_ptr<TextureBuffer>& CubeMapTex)
	{
		cubeMap = CubeMapTex;
	}

	//描画
	void Draw(Camera& Cam);

};

//動的キューブマップ
class DynamicCubeMap : public CubeMap
{
private:
	static int ID;
	static std::shared_ptr<ConstantBuffer>VIEW_PROJ_MATRICIES;

private:
	std::shared_ptr<RenderTarget>cubeRenderTarget;
	std::shared_ptr<DepthStencil>cubeDepth;

public:
	DynamicCubeMap(const int& CubeMapEdge = 512);
	void Clear();
	void DrawToCubeMap(LightManager& LigManager, const std::vector<std::weak_ptr<ModelObject>>&ModelObject);
};