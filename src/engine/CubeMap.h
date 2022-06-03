#pragma once
#include"Vec.h"
#include<string>
#include<array>
#include"Mesh.h"
#include<memory>
class TextureBuffer;
class Camera;

class CubeMap
{
protected:
	static std::shared_ptr<TextureBuffer>DEFAULT_CUBE_MAP_TEX;
public:
	static const enum SURFACE_TYPE
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
	
protected:
	CubeMap();
	std::shared_ptr<TextureBuffer>cubeMap;	//ライティングで参照する（ディメンションがTEXTURE_CUBE）

public:

	const std::shared_ptr<TextureBuffer>& GetCubeMapTex() { return cubeMap; }
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

	//ライティングに用いるテクスチャをアタッチ
	void AttachCubeMapTex(const std::shared_ptr<TextureBuffer>& CubeMapTex)
	{
		cubeMap = CubeMapTex;
	}

	//描画
	void Draw(Camera& Cam);

};