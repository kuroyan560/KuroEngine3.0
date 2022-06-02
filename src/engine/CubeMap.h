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
	
private:
	std::string name;
	std::array<Surface, SURFACE_NUM>surfaces;	//描画に使用
	float sideLength = 10.0f;	//辺の長さ
	std::shared_ptr<TextureBuffer>cubeMap;	//ライティングで参照する（ディメンションがTEXTURE_CUBE）

	void ResetMeshVertices();

public:
	/// <summary>
	///コンストラクタ
	/// </summary>
	/// <param name="Name">キューブマップ名</param>
	/// <param name="SideLength">辺の長さ</param>
	/// <returns></returns>
	CubeMap(const std::string& Name, const float& SideLength = 100.0f);

	//指定面にテクスチャをアタッチ
	void AttachTex(const SURFACE_TYPE& Surface, const std::shared_ptr<TextureBuffer>& Tex)
	{
		surfaces[Surface].tex = Tex;
	}

	//ライティングに用いるテクスチャをアタッチ
	void AttachCubeMapTex(const std::shared_ptr<TextureBuffer>& CubeMapTex)
	{
		cubeMap = CubeMapTex;
	}

	//辺の長さを設定
	void SetSideLength(const float& Length);

	//描画
	void Draw(Camera& Cam);

	const std::shared_ptr<TextureBuffer>& GetCubeMapTex() { return cubeMap; }
};

