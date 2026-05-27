#pragma once

#include "Mesh.h"

class MeshModifier {

public:
	static void Subdivide(Mesh& msh)
	{

        if (msh.indexBuffer.empty() || msh.vertexBuffer.empty())
            return;

        std::unordered_map<uint64_t, uint32_t> midpointCache;
        midpointCache.reserve(msh.indexBuffer.size());
        auto midpoint = [&](uint32_t a, uint32_t b) -> uint32_t
            {
                uint64_t key = (uint64_t)std::min(a, b) << 32 | std::max(a, b);

                auto it = midpointCache.find(key);
                if (it != midpointCache.end())
                    return it->second;

                Vec4 va = msh.vertexBuffer[a];
                Vec4 vb = msh.vertexBuffer[b];

                Vec4 m(
                    (va.x + vb.x) * 0.5f,
                    (va.y + vb.y) * 0.5f,
                    (va.z + vb.z) * 0.5f,
                    1.0f
                );

                msh.vertexBuffer.push_back(m);
                uint32_t idx = msh.vertexBuffer.size() - 1;
                midpointCache[key] = idx;
                return idx;
            };

        std::vector<uint32_t> newIdx;
        newIdx.reserve(msh.indexBuffer.size() * 4);

        for (size_t i = 0; i < msh.indexBuffer.size(); i += 3)
        {
            uint32_t a = msh.indexBuffer[i];
            uint32_t b = msh.indexBuffer[i + 1];
            uint32_t c = msh.indexBuffer[i + 2];

            uint32_t ab = midpoint(a, b);
            uint32_t bc = midpoint(b, c);
            uint32_t ca = midpoint(c, a);

            newIdx.insert(newIdx.end(), {
                a, ab, ca,
                b, bc, ab,
                c, ca, bc,
                ab, bc, ca
                });
        }

        msh.indexBuffer.swap(newIdx);
	}



};