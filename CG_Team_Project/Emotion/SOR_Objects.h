#pragma once
#include <vector>

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

    // [추가된 부분] 오브젝트별 색상
    float r, g, b;
    // ★ [추가] 0:일반, 1:좌절(느림), 2:혼란(벽), 3:외로움(어둠)
    int type;
};

// 전역 변수 공유 (다른 파일에서도 쓸 수 있게)
extern std::vector<SORModel>   g_loadedModels;
extern std::vector<GameObject> g_worldObjects;

// 함수 선언
int LoadAndRegisterModel(const char* filename);

// [수정] 기존 선언은 지우고, 이 한 줄만 남기시면 됩니다!
// (맨 끝에 int type = 0 덕분에, type을 안 넣으면 자동으로 0(일반)으로 인식됩니다)
void AddObjectGrid(int modelIdx, int gridX, int gridY, float height, float scale,
    float initAngle, float rotSpeed, float floatSpeed, float floatRange, int type = 0);

void DrawSORObjects(float cellSize);