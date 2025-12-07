#pragma once

#include <vector>

// SOR 모델의 한 점 (RGBA 포함)
struct ModelPoint
{
    float x, y, z;
    float r, g, b, a;
};

// SOR 모델
struct SORModel
{
    std::vector<std::vector<ModelPoint>> geometry; // [row][col]
};

// 미로 안에 배치되는 SOR 오브젝트
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

    // 감정 식별용
    int   emotionId = -1;
};

// 전역 컨테이너 (정의는 cpp)
extern std::vector<SORModel>    g_loadedModels;
extern std::vector<GameObject> g_worldObjects;

// 모델 로드 후 인덱스 반환
int  LoadAndRegisterModel(const char* filename);

// 미로 격자 좌표에 오브젝트 추가
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

// SOR 오브젝트 렌더
void DrawSORObjects(float cellSize);

// 전역 오브젝트 초기화(선택)
void ClearSORObjects();
