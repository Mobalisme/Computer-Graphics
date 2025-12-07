#pragma once

#include <vector>

// SOR    (RGBA )
struct ModelPoint
{
    float x, y, z;
    float r, g, b, a;
};

// SOR 
struct SORModel
{
    std::vector<std::vector<ModelPoint>> geometry; // [row][col]
};

// ̷ ȿ ġǴ SOR Ʈ
struct GameObject
{
    int   modelIndex = -1;
    int   mazeX = 0;
    int   mazeY = 0;

    float scale = 1.0f;

    float baseAngle = 0.0f;
    float rotationSpeed = 0.0f;
    float currentAngle = 0.0f;

    float baseAltitude = 0.0f;
    float floatSpeed = 0.0f;
    float floatRange = 0.0f;
    float floatPhase = 0.0f;

    bool  collected = false;

    //  ĺ
    int   emotionId = -1;
};

//  ̳ (Ǵ cpp)
extern std::vector<SORModel>    g_loadedModels;
extern std::vector<GameObject> g_worldObjects;

//  ε  ε ȯ
int  LoadAndRegisterModel(const char* filename);

// ̷  ǥ Ʈ ߰
void AddObjectGrid(
    int   modelIdx,
    int   gridX, int gridY,
    float height,
    float scale,
    float initAngle,
    float rotSpeed,
    float floatSpeed,
    float floatRange,
    int   emotionId = -1);

// SOR Ʈ 
void DrawSORObjects(float cellSize);

//  Ʈ ʱȭ()
void ClearSORObjects();
