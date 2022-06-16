#include "Model.h"

std::vector<CollisionTriangle> Model::GetCollisionTriangles()
{
    std::vector<CollisionTriangle>triangles;

    for (auto& m : meshes)
    {
        auto meshTriangles = m.GetCollisionTriangles();
        for (auto& tri : meshTriangles)
        {
            triangles.emplace_back(tri);
        }
    }

    return triangles;
}
