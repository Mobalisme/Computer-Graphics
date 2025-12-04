#pragma once

#include <vector>

// SOR 모델의 한 점 (위치 + 색상/알파)
struct ModelPoint
{
    float x, y, z;       // 위치
    float r, g, b, a;    // 색상 + 알파
};

// SOR 모델 하나: rows x cols 형태의 격자
struct SORModel
{
    // geometry[row][col]
    std::vector<std::vector<ModelPoint>> geometry;
};

// 미로 위에 배치되는 SOR 오브젝트(= 감정 구슬)
struct GameObject
{
    int   modelIndex;    // g_loadedModels에서의 인덱스
    int   mazeX;         // 미로 격자 X (0 ~ MAZE_W-1)
    int   mazeY;         // 미로 격자 Y (0 ~ MAZE_H-1)
    float scale;         // 전체 스케일

    // 회전
    float baseAngle;      // 기본 각도(도 단위)
    float rotationSpeed;  // 매 프레임 증가 각도(도 단위)
    float currentAngle;   // 현재까지 누적 회전

    // 위아래 떠다니는 움직임
    float baseAltitude;   // 기본 높이
    float floatSpeed;     // 위상 변화 속도
    float floatRange;     // 위아래 진폭
    float floatPhase;     // 현재 위상(라디안)
};

// 전역 컨테이너 (Emotion.cpp에서 필요하면 직접 접근 가능)
extern std::vector<SORModel>   g_loadedModels;
extern std::vector<GameObject> g_worldObjects;

// ================== 인터페이스 ==================

// SOR_Test01.cpp에서 ExportModel로 만든 파일을 읽어서 g_loadedModels에 등록
// 성공: 모델 인덱스(0,1,2,...) 반환 / 실패: -1
int LoadAndRegisterModel(const char* filename);

// 미로 격자 좌표 기준으로 오브젝트 하나 추가
void AddObjectGrid(
    int modelIdx,
    int gridX, int gridY,
    float height,
    float scale,
    float initAngle,
    float rotSpeed,
    float floatSpeed,
    float floatRange);

// 게임 시작 시 한 번 호출해서 SOR 오브젝트(=감정 구슬) 배치
void InitGameObjects();

// 3D 씬에서 SOR 오브젝트 전부 렌더링
// cellSize: Emotion.cpp에서 쓰는 셀 크기 그대로 넘겨주면 됨
void DrawSORObjects(float cellSize);
