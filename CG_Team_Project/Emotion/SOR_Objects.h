// SOR_Objects.h
#pragma once

#include <vector>

// SOR 모델의 한 점 (위치 + 색상/알파)
struct ModelPoint
{
    float x, y, z;      // 위치
    float r, g, b, a;   // 색 + 알파(투명도)
};

// SOR 모델 하나: 링(세로) × 각도(가로) 격자
struct SORModel
{
    // geometry[row][col]
    std::vector<std::vector<ModelPoint> > geometry;
};

// 미로 위에 배치되는 SOR 오브젝트(감정 하나)
struct GameObject
{
    int   modelIndex;    // g_loadedModels 인덱스
    int   mazeX;         // 미로 격자 X
    int   mazeY;         // 미로 격자 Y

    float scale;         // 전체 크기

    // 회전
    float baseAngle;      // 시작 각도(도)
    float rotationSpeed;  // 프레임당 증가 각도(도)
    float currentAngle;   // 누적 각도

    // 위아래 떠다니기
    float baseAltitude;   // 기본 높이
    float floatSpeed;     // 위상 변화 속도
    float floatRange;     // 진폭
    float floatPhase;     // 현재 위상(라디안)

    // 습득 여부
    bool  collected;      // true 면 이미 먹은 감정
};

// 전역 컨테이너 선언 (실제 정의는 SOR_Objects.cpp 안에 있음)
extern std::vector<SORModel>   g_loadedModels;
extern std::vector<GameObject> g_worldObjects;

// SOR_Test01.cpp에서 만든 모델 파일 로드
// 성공: 모델 인덱스(0,1,2,...) / 실패: -1
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
