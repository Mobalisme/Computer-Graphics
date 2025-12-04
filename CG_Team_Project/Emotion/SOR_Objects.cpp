// SOR_Objects.cpp
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
            ModelPoint p;
            // 파일 포맷: x y z r g b a
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
    int idx = (int)g_loadedModels.size() - 1;

    std::cout << "[SOR] 모델 로드 완료: " << filename
        << " (rows=" << rows << ", cols=" << cols
        << ", index=" << idx << ")\n";

    return idx;
}

// ================== 오브젝트 배치 ==================

void AddObjectGrid(
    int   modelIdx,
    int   gridX, int gridY,
    float height,
    float scale,
    float initAngle,
    float rotSpeed,
    float floatSpeed,
    float floatRange)
{
    if (modelIdx < 0 || modelIdx >= (int)g_loadedModels.size())
        return;

    GameObject obj;
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
    obj.floatPhase = (float)(std::rand() % 100) * 0.1f; // 랜덤 위상

    obj.collected = false;

    g_worldObjects.push_back(obj);
}

// ================== 렌더링 ==================

static void DrawSingleModel(const SORModel& model)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int rows = (int)model.geometry.size();
    if (rows < 2)
    {
        glDisable(GL_BLEND);
        return;
    }

    for (int i = 0; i < rows - 1; ++i)
    {
        const std::vector<ModelPoint>& ring0 = model.geometry[i];
        const std::vector<ModelPoint>& ring1 = model.geometry[i + 1];

        int cols = (int)ring0.size();
        if (cols == 0 || (int)ring1.size() != cols)
            continue;

        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j < cols; ++j)
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

    for (int i = 0; i < (int)g_worldObjects.size(); ++i)
    {
        GameObject& obj = g_worldObjects[i];
        if (obj.collected)
            continue;

        if (obj.modelIndex < 0 ||
            obj.modelIndex >= (int)g_loadedModels.size())
            continue;

        glPushMatrix();

        // 위아래 떠다니는 오프셋
        obj.floatPhase += obj.floatSpeed;
        float floatOffset = (float)std::sin(obj.floatPhase) * obj.floatRange;

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

    glEnable(GL_TEXTURE_2D);
}
