#include "Mesh.h"
#include "Mathematics.h"
#include <algorithm> // std::min, std::max

namespace GLibpp::Geometry {

const std::vector<Vec4>& Mesh::getVertexBuffer() const
{
    return vertexBuffer;
}

const std::vector<uint32_t>& Mesh::getIndexBuffer() const
{
    return indexBuffer;
}

const std::vector<float>& Mesh::getUVBuffer() const
{
    return uvBuffer;
}

}
