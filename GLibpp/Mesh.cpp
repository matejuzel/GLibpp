
#include "Mesh.h"
#include <iostream>

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

