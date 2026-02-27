
#include "Mesh.h"
#include "math/Mtx4.h"
#include "Triangle.h"
#include "Material.h"
#include "AABB.h"
#include <iostream>
#include <algorithm>

Mesh& Mesh::addTriangle(const Triangle& triangle)
{
	tris.push_back(triangle);
	return *this;
}

Mesh& Mesh::addTriangle(const Vertex& a, const Vertex& b, const Vertex& c)
{
	tris.push_back(Triangle(a, b, c));
	return *this;
}

Mesh& Mesh::addTriangle(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz)
{
	Vertex v = Vertex();

	tris.push_back(Triangle(
		Vertex(ax, ay, az),
		Vertex(ax, ay, az),
		Vertex(ax, ay, az)
	));

	return *this;
}

Mesh& Mesh::addQuad(const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d)
{
	addTriangle(a, b, c);
	addTriangle(a, c, d);
    return *this;
}

Mesh& Mesh::addCube(float s)
{
    // Polovina velikosti (krychle je od -s/2 do +s/2)
    float h = s * 0.5f;

    // 8 vrcholù krychle
    Vertex v000(-h, -h, -h);
    Vertex v001(-h, -h, h);
    Vertex v010(-h, h, -h);
    Vertex v011(-h, h, h);

    Vertex v100(h, -h, -h);
    Vertex v101(h, -h, h);
    Vertex v110(h, h, -h);
    Vertex v111(h, h, h);

    // Pøední strana (Z+)
    addQuad(v001, v101, v111, v011);

    // Zadní strana (Z-)
    addQuad(v100, v000, v010, v110);

    // Levá strana (X-)
    addQuad(v000, v001, v011, v010);

    // Pravá strana (X+)
    addQuad(v101, v100, v110, v111);

    // Horní strana (Y+)
    addQuad(v010, v011, v111, v110);

    // Dolní strana (Y-)
    addQuad(v000, v100, v101, v001);

    return *this;
}

Mesh& Mesh::applyTransformation()
{

    // normaly se spravne transformuji invertovanou a nasledne transponovanou matici n' = (M^-1)^T * n
    Mtx4 inverseTransposedMatrix = this->transformation.inverted().transposed();

    for (Triangle& t : tris)
    {
        // A
        t.a.pos = transformation * t.a.pos;
        t.a.normal = inverseTransposedMatrix * t.a.normal; // pokud chceš transformovat i normály

        // B
        t.b.pos = transformation * t.b.pos;
        t.b.normal = inverseTransposedMatrix * t.b.normal;

        // C
        t.c.pos = transformation * t.c.pos;
        t.c.normal = inverseTransposedMatrix * t.c.normal;
    }

    // Po aplikaci mùžeš transformaci resetovat na identitu
    transformation = Mtx4::identity();

    return *this;
}

Mesh& Mesh::transform(const Mtx4& m)
{
	this->transformation *= m;

    std::cout << m.toStringDetail() << std::endl;

    return *this;
}

AABB Mesh::computeAABB() const
{
    AABB box;

    // Inicializace extrémními hodnotami
    box.min = Vec4(std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity(),
        1.0f);

    box.max = Vec4(-std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        1.0f);

    for (const Triangle& t : tris)
    {
        const Vertex* verts[3] = { &t.a, &t.b, &t.c };

        for (int i = 0; i < 3; i++)
        {
            const Vec4& p = verts[i]->pos;

            if (p.x < box.min.x) box.min.x = p.x;
            if (p.y < box.min.y) box.min.y = p.y;
            if (p.z < box.min.z) box.min.z = p.z;

            if (p.x > box.max.x) box.max.x = p.x;
            if (p.y > box.max.y) box.max.y = p.y;
            if (p.z > box.max.z) box.max.z = p.z;
        }
    }

    return box;
}

Mtx4 Mesh::computeViewMatrixToFillScreen(float verticalFovRadians, float aspect) const
{
    // 1) AABB v lokálním prostoru
    AABB localBox = computeAABB();

    // 2) AABB pøevedeme do world space pomocí model matice
    AABB worldBox = transformAABB(localBox, transformation);

    // 3) støed a extents
    Vec4 center = (worldBox.min + worldBox.max) * 0.5f;

    // 4) vzdálenost kamery
    float distance = computeCameraDistanceForAABB(worldBox, verticalFovRadians, aspect);

    // 5) kamera se pohybuje jen po ose Z (do mínusu)
    Vec4 eye(center.x, center.y, center.z + distance, 1.0f);
    Vec4 target = center;
    Vec4 up(0, 1, 0, 0);

    return Mtx4::lookAt(eye, target, up);
}



AABB Mesh::transformAABB(const AABB& box, const Mtx4& m)
{
    
    AABB out;
    
    Vec4 corners[8] = {
        { box.min.x, box.min.y, box.min.z, 1 },
        { box.min.x, box.min.y, box.max.z, 1 },
        { box.min.x, box.max.y, box.min.z, 1 },
        { box.min.x, box.max.y, box.max.z, 1 },
        { box.max.x, box.min.y, box.min.z, 1 },
        { box.max.x, box.min.y, box.max.z, 1 },
        { box.max.x, box.max.y, box.min.z, 1 },
        { box.max.x, box.max.y, box.max.z, 1 }
    };

    
    out.min = Vec4(+INFINITY, +INFINITY, +INFINITY, 1);
    out.max = Vec4(-INFINITY, -INFINITY, -INFINITY, 1);

    for (int i = 0; i < 8; i++) {
        Vec4 p = m * corners[i];
        out.min.x = std::min(out.min.x, p.x);
        out.min.y = std::min(out.min.y, p.y);
        out.min.z = std::min(out.min.z, p.z);

        out.max.x = std::max(out.max.x, p.x);
        out.max.y = std::max(out.max.y, p.y);
        out.max.z = std::max(out.max.z, p.z);
    }
    return out;
}

float Mesh::computeCameraDistanceForAABB(const AABB& box, float verticalFovRadians, float aspect)
{
    Vec4 center = (box.min + box.max) * 0.5f;
    Vec4 extents = (box.max - box.min) * 0.5f;

    float halfHeight = extents.y;
    float halfWidth = extents.x;

    // vertikální omezení
    float z_v = halfHeight / std::tan(verticalFovRadians * 0.5f);

    // horizontální omezení – spoèítáme efektivní horizontální FOV
    float tanHalfVert = std::tan(verticalFovRadians * 0.5f);
    float tanHalfHoriz = tanHalfVert * aspect;
    float fovHoriz = 2.0f * std::atan(tanHalfHoriz);
    float z_h = halfWidth / std::tan(fovHoriz * 0.5f);

    return std::max(z_v, z_h);
}

Mesh& Mesh::addNet(int divisions) {

	// Zajistit alespoò 1 dìlení
	if (divisions < 1) {
		divisions = 1;
	}

	const float step = 1.0f / static_cast<float>(divisions);

	for (int i = 0; i < divisions; ++i) {
		const float x0 = -0.5f + i * step;
		const float x1 = x0 + step;

		for (int j = 0; j < divisions; ++j) {
			const float z0 = -0.5f + j * step;
			const float z1 = z0 + step;

			// Vytvoøení ètyø vrcholù ètverce na rovinì y = 0
			Vertex a{ x0, 0.0f, z0 };
			Vertex b{ x1, 0.0f, z0 };
			Vertex c{ x1, 0.0f, z1 };
			Vertex d{ x0, 0.0f, z1 };

			// Pøidat ètverec jako dvì trojúhelníky
			addQuad(a, b, c, d);
		}
	}

	return *this;
}


