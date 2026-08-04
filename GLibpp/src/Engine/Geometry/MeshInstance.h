#pragma once

#include "ResourceHandles.h"
#include "Mtx4.h"
#include "Color.h"

// instance = odkaz na mesh + "zapeceny" lokalni transform + vzhled
// world transform se predava per-draw, protoze ho pocita logika kazdy tick (fyzika auta)
// instance je "mesh + vzhled", ne "vyskyt ve svete" - jedna instance se smi kreslit vicekrat
// (napr. 4 kola = 1 instance kreslena 4x s ruznymi world maticemi)
// color je docasny zastupce - pozdeji se nahradi MaterialHandle bez zmeny volajicich
struct MeshInstance {
    MeshHandle mesh = MESH_HANDLE_INVALID;
    Mtx4 localTransform = Mtx4::Identity();
    Color color = Color::Grayscale(0.3f);
    bool wireframe = false;
};
