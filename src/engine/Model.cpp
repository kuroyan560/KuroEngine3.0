#include "Model.h"

std::vector<CollisionTriangleArray> Model::GetCollisionTriangleArray()
{
    std::vector<CollisionTriangleArray>triangleArray;

    for (auto& m : meshes)
    {
        triangleArray.emplace_back(m.GetCollisionTriangles());
    }

    return triangleArray;
}
