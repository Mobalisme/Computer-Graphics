#include "SOR_Objects.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <GL/freeglut.h>

std::vector<SORModel>   g_loadedModels;
std::vector<GameObject> g_worldObjects;

// [핵심] 단면도(x, y) 데이터를 읽어서 3D 회전체(SOR)로 변환하는 함수
int LoadAndRegisterModel(const char* filename)
{
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        std::cout << "[ERROR] 파일을 열 수 없습니다: " << filename << std::endl;
        return -1;
    }

    int numPoints = 0;
    fin >> numPoints; // 점의 개수 읽기
    if (numPoints <= 0) {
        std::cout << "[ERROR] 점 개수 오류: " << filename << std::endl;
        return -1;
    }

    struct Point2D { float r, y; }; // r: 중심축 거리, y: 높이
    std::vector<Point2D> profile;

    // 1. 2D 단면도 읽기
    for (int i = 0; i < numPoints; ++i) {
        Point2D p;
        fin >> p.r >> p.y;
        profile.push_back(p);
    }
    fin.close();

    // 2. 360도 회전시켜 3D 모델 생성 (36등분 = 10도씩)
    SORModel model;
    int slices = 36;
    model.geometry.resize(numPoints);

    for (int i = 0; i < numPoints; ++i) {
        model.geometry[i].resize(slices + 1);
        for (int j = 0; j <= slices; ++j) {
            float theta = (float)j / slices * 2.0f * 3.14159265f; // 각도

            ModelPoint p;
            // 원통 좌표계 -> 직교 좌표계 변환 (x = r*cos, z = r*sin)
            p.x = profile[i].r * cosf(theta);
            p.y = profile[i].y;
            p.z = profile[i].r * sinf(theta);

            // 색상과 알파값은 그릴 때 결정하거나 기본값 설정 (흰색)
            p.r = 1.0f; p.g = 1.0f; p.b = 1.0f; p.a = 1.0f;

            model.geometry[i][j] = p;
        }
    }

    g_loadedModels.push_back(model);
    std::cout << "[SUCCESS] 모델 로드 완료: " << filename << " (ID: " << g_loadedModels.size() - 1 << ")" << std::endl;
    return (int)g_loadedModels.size() - 1;
}

// 오브젝트 추가 (색상 기능 추가됨!)
void AddObjectGrid(int modelIdx, int gridX, int gridY, float height, float scale, int type)
{
    if (modelIdx < 0 && type != 99) return;

    GameObject obj;
    obj.modelIndex = modelIdx;
    obj.mazeX = gridX;
    obj.mazeY = gridY;
    obj.scale = scale;
    obj.baseAltitude = height;

    // [자동 설정] 함수 인자에서 뺐으므로 여기서 직접 값을 정해줍니다.
    obj.baseAngle = 0.0f;
    obj.currentAngle = 0.0f;
    obj.floatPhase = (float)(rand() % 100) * 0.1f;
    obj.collected = false;
    obj.type = type;

    // 타입에 따라 움직임 속도 자동 조절
    if (type == 99) { // 골렘은 안 움직임
        obj.rotationSpeed = 0.0f;
        obj.floatSpeed = 0.0f;
        obj.floatRange = 0.0f;
    }
    else if (type != 0) { // 함정은 좀 더 빨리 돔
        obj.rotationSpeed = 5.0f;
        obj.floatSpeed = 0.05f;
        obj.floatRange = 0.2f;
    }
    else { // 일반 아이템
        obj.rotationSpeed = 1.5f;
        obj.floatSpeed = 0.02f;
        obj.floatRange = 0.3f;
    }

    // 색상 설정
    if (type == 0) { // 메인 감정
        if (modelIdx % 3 == 0) { obj.r = 0.2f; obj.g = 0.5f; obj.b = 1.0f; } // 파랑
        else if (modelIdx % 3 == 1) { obj.r = 1.0f; obj.g = 0.2f; obj.b = 0.2f; } // 빨강
        else { obj.r = 1.0f; obj.g = 1.0f; obj.b = 0.0f; } // 노랑
    }
    else if (type == 1) { obj.r = 0.6f; obj.g = 0.0f; obj.b = 0.8f; } // 보라 (좌절)
    else if (type == 2) { obj.r = 0.2f; obj.g = 0.8f; obj.b = 0.2f; } // 초록 (혼란)
    else if (type == 3) { obj.r = 0.3f; obj.g = 0.3f; obj.b = 0.3f; } // 검정 (외로움)
    else { obj.r = 1.0f; obj.g = 1.0f; obj.b = 1.0f; }

    g_worldObjects.push_back(obj);
}

// 렌더링 (단일 모델)
static void DrawSingleModel(const SORModel& model, float r, float g, float b)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (size_t i = 0; i < model.geometry.size() - 1; ++i) {
        glBegin(GL_TRIANGLE_STRIP);
        for (size_t j = 0; j < model.geometry[i].size(); ++j) {
            const auto& p0 = model.geometry[i][j];
            const auto& p1 = model.geometry[i + 1][j];

            // 법선 벡터 (중심에서 바깥쪽으로)
            glNormal3f(p0.x, 0.0f, p0.z);

            // 여기서 색상을 적용합니다!
            glColor4f(r, g, b, 1.0f); // 투명도 1.0
            glVertex3f(p0.x, p0.y, p0.z);

            glNormal3f(p1.x, 0.0f, p1.z);
            glColor4f(r, g, b, 1.0f);
            glVertex3f(p1.x, p1.y, p1.z);
        }
        glEnd();
    }
    glDisable(GL_BLEND);
}

// 전체 오브젝트 및 그림자 그리기
void DrawSORObjects(float cellSize)
{
    extern GLfloat g_shadowMatrix[16]; // 그림자 행렬

    glDisable(GL_TEXTURE_2D);

    for (auto& obj : g_worldObjects) {
        if (obj.collected) continue;
        if (obj.modelIndex < 0 || obj.modelIndex >= (int)g_loadedModels.size()) continue;

        obj.floatPhase += obj.floatSpeed;
        float floatOffset = std::sin(obj.floatPhase) * obj.floatRange;
        obj.currentAngle += obj.rotationSpeed;

        float wx = (obj.mazeX + 0.5f) * cellSize;
        float wy = obj.baseAltitude + floatOffset;
        float wz = (obj.mazeY + 0.5f) * cellSize;

        // 1. 본체 그리기 (색상 적용)
        glEnable(GL_LIGHTING);
        glPushMatrix();
        glTranslatef(wx, wy, wz);
        glRotatef(obj.baseAngle + obj.currentAngle, 0, 1, 0);
        glScalef(obj.scale, obj.scale, obj.scale);

        // 색상 전달 (obj.r, obj.g, obj.b)
        DrawSingleModel(g_loadedModels[obj.modelIndex], obj.r, obj.g, obj.b);
        glPopMatrix();

        // 2. 그림자 그리기
        glDisable(GL_LIGHTING);
        glPushMatrix();
        glTranslatef(0.0f, 0.01f, 0.0f);
        glMultMatrixf(g_shadowMatrix);
        glTranslatef(wx, wy, wz);
        glRotatef(obj.baseAngle + obj.currentAngle, 0, 1, 0);
        glScalef(obj.scale, obj.scale, obj.scale);

        // 그림자는 검은색으로
        DrawSingleModel(g_loadedModels[obj.modelIndex], 0.0f, 0.0f, 0.0f);
        glPopMatrix();
        glEnable(GL_LIGHTING);
    }
    glEnable(GL_TEXTURE_2D);
}