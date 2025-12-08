#pragma once
#include <vector>

struct ModelPoint
{
    float x, y, z;
    float r, g, b, a;
};

struct SORModel
{
    std::vector<std::vector<ModelPoint>> geometry;
};

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
    int   emotionId = -1;
};

extern std::vector<SORModel>    g_loadedModels;
extern std::vector<GameObject> g_worldObjects;

int  LoadAndRegisterModel(const char* filename);

void AddObjectGrid(
    int   modelIdx,
    int   gridX, int gridY,
    float height,
    float scale,
    float initAngle,
    float rotSpeed,
    float floatSpeed,
    float floatRange,
    int   emotionId);

void DrawSORObjects(float cellSize);
void ClearSORObjects();
