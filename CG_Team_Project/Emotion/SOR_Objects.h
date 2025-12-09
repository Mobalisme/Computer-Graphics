#pragma once
#include <vector>
#include <GL/freeglut.h>

// 데이터 구조체
struct ModelPoint {
    float x, y, z;
    float r, g, b, a;
};

struct SORModel {
    std::vector<std::vector<ModelPoint>> geometry;
};

struct GameObject {
    int modelIndex;
    int mazeX, mazeY;
    float scale;

    float baseAngle;
    float rotationSpeed;
    float currentAngle;

    float baseAltitude;
    float floatSpeed;
    float floatRange;
    float floatPhase;

    bool collected;

    int type;
    // [추가된 부분] 오브젝트별 색상
    float r, g, b;
};

// 전역 변수 공유 (다른 파일에서도 쓸 수 있게)
extern std::vector<SORModel>   g_loadedModels;
extern std::vector<GameObject> g_worldObjects;

// 함수 선언
int LoadAndRegisterModel(const char* filename);

void AddObjectGrid(int modelIdx, int gridX, int gridY, float height, float scale, int type = 0);

void DrawSORObjects(float cellSize);