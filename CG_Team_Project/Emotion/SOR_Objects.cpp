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
void AddObjectGrid(int modelIdx, int gridX, int gridY, float height, float scale,
    float initAngle, float rotSpeed, float floatSpeed, float floatRange, int type)
{
    if (modelIdx < 0 || modelIdx >= (int)g_loadedModels.size()) return;

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
    obj.floatPhase = (float)(std::rand() % 100) * 0.1f;
    obj.collected = false;
    obj.type = type;

    // [중요] 모델 인덱스에 따라 색상 자동 지정
    // 0번(슬픔): 파란색, 1번(분노): 빨간색, 2번(행복): 노란/핑크색
    if (type == 0) { // 기존 감정 오브젝트 (0, 1, 2번 모델)
        if (modelIdx % 3 == 0) { obj.r = 0.2f; obj.g = 0.5f; obj.b = 1.0f; } // 파랑
        else if (modelIdx % 3 == 1) { obj.r = 1.0f; obj.g = 0.2f; obj.b = 0.2f; } // 빨강
        else { obj.r = 1.0f; obj.g = 1.0f; obj.b = 0.0f; } // 노랑
    }
    else if (type == 1) { // 🟣 좌절 (보라색)
        obj.r = 0.6f; obj.g = 0.0f; obj.b = 0.8f;
    }
    else if (type == 2) { // 🟢 혼란 (초록색)
        obj.r = 0.2f; obj.g = 0.8f; obj.b = 0.2f;
    }
    else if (type == 3) { // ⚫ 외로움 (검은색/회색)
        obj.r = 0.3f; obj.g = 0.3f; obj.b = 0.3f;
    }

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