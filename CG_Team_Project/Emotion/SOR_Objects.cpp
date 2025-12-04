#include "SOR_Objects.h"

#include <fstream>
#include <iostream>
#include <cmath>
#include <cstdlib>

#include <GL/freeglut.h>

// 전역 컨테이너 실제 정의
std::vector<SORModel>   g_loadedModels;
std::vector<GameObject> g_worldObjects;

// ================== SOR 모델 로딩 ==================

int LoadAndRegisterModel(const char* filename)
{
    std::ifstream fin(filename);
    if (!fin.is_open())
    {
        std::cout << "[SOR] 파일을 열 수 없습니다: " << filename << "\n";
        return -1;
    }

    int rows = 0;
    int cols = 0;
    fin >> rows >> cols;
    if (!fin || rows <= 0 || cols <= 0)
    {
        std::cout << "[SOR] 잘못된 rows/cols: " << filename << "\n";
        return -1;
    }

    SORModel model;
    model.geometry.resize(rows);

    for (int i = 0; i < rows; ++i)
    {
        model.geometry[i].resize(cols);
        for (int j = 0; j < cols; ++j)
        {
            ModelPoint p{};
            // SOR_Test01.cpp Export 포맷: x y z r g b a
            fin >> p.x >> p.y >> p.z >> p.r >> p.g >> p.b >> p.a;
            if (!fin)
            {
                std::cout << "[SOR] 데이터 읽기 실패: " << filename << "\n";
                return -1;
            }
            model.geometry[i][j] = p;
        }
    }

    g_loadedModels.push_back(model);
    int idx = static_cast<int>(g_loadedModels.size()) - 1;
    std::cout << "[SOR] 모델 로드 완료: " << filename
        << " (rows=" << rows << ", cols=" << cols
        << ", index=" << idx << ")\n";
    return idx;
}

// ================== 오브젝트 배치 ==================

void AddObjectGrid(
    int modelIdx,
    int gridX, int gridY,
    float height,
    float scale,
    float initAngle,
    float rotSpeed,
    float floatSpeed,
    float floatRange)
{
    if (modelIdx < 0 || modelIdx >= static_cast<int>(g_loadedModels.size()))
        return;

    GameObject obj{};
    obj.modelIndex = modelIdx;
    obj.mazeX = gridX;
    obj.mazeY = gridY;
    obj.scale = scale;

    obj.baseAngle = initAngle;
    obj.rotationSpeed = rotSpeed;
    obj.currentAngle = 0.0f;

    obj.baseAltitude = height;
    obj.floatSpeed = floatSpeed;
    obj.floatRange = floatRange;

    obj.floatPhase = static_cast<float>(std::rand() % 100) * 0.1f;

    g_worldObjects.push_back(obj);
}

// 게임 시작 시 한 번 호출해서 감정(SOR 오브젝트) 배치
void InitGameObjects()
{
    // 경로는 Export 한 위치에 맞게 바꿔도 됨
    // 예: "Models/model_data.txt"
    int modelA = LoadAndRegisterModel("model_data.txt");
    if (modelA == -1)
    {
        // 로드 실패 시: 감정 오브젝트 없이 진행
        return;
    }

    // ====== 예시 배치들 ======
    // 필요하면 마음대로 수정해서 실제 감정 위치/개수 맞추면 됨

    // 입구 근처
    AddObjectGrid(
        modelA,
        1, 1,           // gridX, gridY
        2.0f,           // height
        0.5f,           // scale
        0.0f,           // baseAngle
        1.5f,           // rotationSpeed
        0.08f,          // floatSpeed
        0.25f           // floatRange
    );

    // 중간 지점
    AddObjectGrid(
        modelA,
        10, 10,
        2.5f,
        0.6f,
        30.0f,
        0.8f,
        0.05f,
        0.4f
    );

    // 출구 근처
    AddObjectGrid(
        modelA,
        23, 23,
        3.0f,
        0.7f,
        -20.0f,
        1.0f,
        0.06f,
        0.3f
    );
}

// ================== 렌더링 ==================

static void DrawSingleModel(const SORModel& model)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const std::size_t rows = model.geometry.size();
    if (rows < 2)
    {
        glDisable(GL_BLEND);
        return;
    }

    for (std::size_t i = 0; i + 1 < rows; ++i)
    {
        const auto& ring0 = model.geometry[i];
        const auto& ring1 = model.geometry[i + 1];

        const std::size_t cols = ring0.size();
        if (cols == 0 || ring1.size() != cols)
            continue;

        glBegin(GL_TRIANGLE_STRIP);
        for (std::size_t j = 0; j < cols; ++j)
        {
            const ModelPoint& p0 = ring0[j];
            const ModelPoint& p1 = ring1[j];

            glColor4f(p0.r, p0.g, p0.b, p0.a);
            glVertex3f(p0.x, p0.y, p0.z);

            glColor4f(p1.r, p1.g, p1.b, p1.a);
            glVertex3f(p1.x, p1.y, p1.z);
        }
        glEnd();
    }

    glDisable(GL_BLEND);
}

void DrawSORObjects(float cellSize)
{
    if (g_worldObjects.empty())
        return;

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_COLOR_MATERIAL);

    for (auto& obj : g_worldObjects)
    {
        if (obj.modelIndex < 0 ||
            obj.modelIndex >= static_cast<int>(g_loadedModels.size()))
            continue;

        glPushMatrix();

        // 위아래 떠다니는 오프셋
        obj.floatPhase += obj.floatSpeed;
        float floatOffset = std::sin(obj.floatPhase) * obj.floatRange;

        float worldX = (obj.mazeX + 0.5f) * cellSize;
        float worldY = obj.baseAltitude + floatOffset;
        float worldZ = (obj.mazeY + 0.5f) * cellSize;

        glTranslatef(worldX, worldY, worldZ);

        // 회전
        obj.currentAngle += obj.rotationSpeed;
        glRotatef(obj.baseAngle + obj.currentAngle, 0.0f, 1.0f, 0.0f);

        // 스케일
        glScalef(obj.scale, obj.scale, obj.scale);

        DrawSingleModel(g_loadedModels[obj.modelIndex]);

        glPopMatrix();
    }

    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_TEXTURE_2D);
}
