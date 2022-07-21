#include "ModelMesh.h"
#include"Collision.h"

void ModelMesh::Smoothing()
{
	mesh->vertBuff.reset();

	std::vector<std::vector<unsigned int>>sameVertexIndices;
	std::vector<char>flg;
	flg.resize(mesh->vertices.size());
	std::fill(flg.begin(), flg.end(), 0);
	for (int i = 0; i < mesh->vertices.size(); ++i)
	{
		if (flg[i] != 0)continue;
		flg[i] = 1;
		sameVertexIndices.emplace_back();
		sameVertexIndices.back().emplace_back(i);
		for (int j = i + 1; j < mesh->vertices.size(); ++j)
		{
			if (mesh->vertices[i].pos == mesh->vertices[j].pos)
			{
				sameVertexIndices.back().emplace_back(j);
				flg[j] = 1;
			}
		}
	}
	for (auto sameVertex : sameVertexIndices)
	{
		Vec3<float>norm(0, 0, 0);
		for (auto vIdx : sameVertex)
		{
			norm += mesh->vertices[vIdx].normal;
		}
		norm /= static_cast<float>(sameVertex.size());
		for (auto vIdx : sameVertex)
		{
			mesh->vertices[vIdx].normal = norm;
		}
	}

	mesh->CreateBuff();
}

void ModelMesh::BuildTangentAndBiNormal()
{
	auto numPolygon = mesh->indices.size() / 3;
	for (auto polyNo = 0; polyNo < numPolygon; ++polyNo)
	{
		const auto no = polyNo * 3;
		auto vertNo_0 = mesh->indices[no];
		auto vertNo_1 = mesh->indices[no + 1];
		auto vertNo_2 = mesh->indices[no + 2];

		auto& vert_0 = mesh->vertices[vertNo_0];
		auto& vert_1 = mesh->vertices[vertNo_1];
		auto& vert_2 = mesh->vertices[vertNo_2];

		Vec3<float>cp0[] = {
			{ vert_0.pos.x, vert_0.uv.x, vert_0.uv.y},
			{ vert_0.pos.y, vert_0.uv.x, vert_0.uv.y},
			{ vert_0.pos.z, vert_0.uv.x, vert_0.uv.y}
		};

		Vec3<float> cp1[] = {
			{ vert_1.pos.x, vert_1.uv.x, vert_1.uv.y},
			{ vert_1.pos.y, vert_1.uv.x, vert_1.uv.y},
			{ vert_1.pos.z, vert_1.uv.x, vert_1.uv.y}
		};

		Vec3<float> cp2[] = {
			{ vert_2.pos.x, vert_2.uv.x, vert_2.uv.y},
			{ vert_2.pos.y, vert_2.uv.x, vert_2.uv.y},
			{ vert_2.pos.z, vert_2.uv.x, vert_2.uv.y}
		};

		// 平面パラメータからUV軸座標算出する。
		Vec3<float> tangent, binormal;
		for (int i = 0; i < 3; ++i) {
			auto V1 = cp1[i] - cp0[i];
			auto V2 = cp2[i] - cp1[i];
			auto ABC = V1.Cross(V2);

			if (ABC.x == 0.0f) {
				tangent[i] = 0.0f;
				binormal[i] = 0.0f;
			}
			else {
				tangent[i] = -ABC.y / ABC.x;
				binormal[i] = -ABC.z / ABC.x;
			}
		}

		tangent.Normalize();
		binormal.Normalize();

		vert_0.tangent += tangent;
		vert_1.tangent += tangent;
		vert_2.tangent += tangent;

		vert_0.binormal += binormal;
		vert_1.binormal += binormal;
		vert_2.binormal += binormal;
	}

	//法線、接ベクトル、従ベクトルを平均化する。
	for (auto& vert : mesh->vertices) {
		vert.tangent.Normalize();
		vert.binormal.Normalize();
	}
}

std::vector<CollisionTriangle> ModelMesh::GetCollisionTriangles()
{
	std::vector<CollisionTriangle> triangles;

	for (int idx = 0; idx < mesh->indices.size(); idx += 3)
	{
		triangles.emplace_back();
		auto& tri = triangles.back();
		tri.m_p0 = mesh->vertices[mesh->indices[idx]].pos;
		tri.m_p1 = mesh->vertices[mesh->indices[idx + 1]].pos;
		tri.m_p2 = mesh->vertices[mesh->indices[idx + 2]].pos;
		tri.CalculateNormal();
	}

	return triangles;
}

Vec3<ValueMinMax> ModelMesh::GetPosMinMax()const
{
	Vec3<ValueMinMax>result;
	result.x.Set();
	result.y.Set();
	result.z.Set();

	for (const auto& v : mesh->vertices)
	{
		if (result.x.m_max < v.pos.x)result.x.m_max = v.pos.x;
		if (v.pos.x < result.x.m_min)result.x.m_min = v.pos.x;
		if (result.y.m_max < v.pos.y)result.y.m_max = v.pos.y;
		if (v.pos.y < result.y.m_min)result.y.m_min = v.pos.y;
		if (result.z.m_max < v.pos.z)result.z.m_max = v.pos.z;
		if (v.pos.z < result.z.m_min)result.z.m_min = v.pos.z;
	}
	return result;
}
