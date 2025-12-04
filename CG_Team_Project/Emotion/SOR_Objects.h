// SOR_Objects.h
#pragma once

#include <vector>

// SOR 모델의 한 점 (위치 + 색상/알파)
struct ModelPoint
{
    float x, y, z;
    float r, g, b, a;
};

// SOR 모델 하나: 세로(링) × 가로(각도) 격자
struct SORModel
{
    std::vector<std::vector<ModelPoint> > geometry;
};

// 미로 위에 배치되는 SOR 오브젝트(감정 하나)
struct GameObject
{
    int   modelIndex;    // g_loadedModels 인덱스
    int   mazeX;         // 미로 격자 X
    int   mazeY;         // 미로 격자 Y

    float scale;         // 크기

    float baseAngle;      // 시작 각도(도)
    float rotationSpeed;  // 프레임당 증가 각도(도)
    float currentAngle;   // 누적 각도(도)

    float baseAltitude;   // 기본 높이
    float floatSpeed;     // 위상 변화 속도
    float floatRange;     // 진폭
    float floatPhase;     // 현재 위상(라디안)

    bool  collected;      // 이미 습득했는지
};

// 전역 컨테이너(정의는 SOR_Objects.cpp 안에 있음)
extern std::vector<SORModel>   g_loadedModels;
extern std::vector<GameObject> g_worldObjects;

// SOR_Test01.cpp 에서 만든 model_data.txt 같은 파일 로드
//   성공: 모델 인덱스(0,1,2,...) / 실패: -1
int LoadAndRegisterModel(const char* filename);

// 미로 격자 좌표 기준으로 오브젝트 하나 추가
void AddObjectGrid(
    int   modelIdx,
    int   gridX, int gridY,
    float height,
    float scale,
    float initAngle,
    float rotSpeed,
    float floatSpeed,
    float floatRange);

// 3D 씬에서 SOR 오브젝트 전부 렌더링
// cellSize: Emotion.cpp에서 쓰는 셀 크기
void DrawSORObjects(float cellSize);
