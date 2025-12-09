#include "Global.h" // Global.h 포함 필수
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <GL/freeglut.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 이름 충돌 방지를 위해 namespace 사용
namespace Modeler {

    // ================== 데이터 구조 ==================

    struct Color {
        float r, g, b, a;
    };

    struct Point2D {
        float x, y;
        int sides;
        Color color;
    };

    struct Point3D {
        float x, y, z;
        float r, g, b, a;
    };

    // ================== 전역 변수 (Modeler 내부용) ==================

    std::vector<Point2D> g_profile;
    std::vector<std::vector<Point3D>> g_surface;

    bool g_is3DMode = false;
    int  g_selectedPointIdx = -1;
    int  g_currentSides = 0;
    Color g_currentColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    float g_camDist = 12.0f;
    float g_camRotX = 20.0f;
    float g_camRotY = 0.0f;
    int   g_prevMouseX = 0;
    int   g_prevMouseY = 0;

    // Window 크기는 Global.h의 winWidth, winHeight를 사용하거나 내부 변수 사용
    // 여기서는 화면 좌표 변환을 위해 전역 변수 참조
    const float SCALE_FACTOR = 0.01f;
    const float SELECT_RADIUS = 15.0f;

    // ================== 수학/로직 ==================

    void ScreenToWorld(int x, int y, float& wx, float& wy) {
        wx = (float)x - (winWidth / 2.0f);
        wy = (winHeight / 2.0f) - (float)y;
    }

    float GetPolyScale(float angle, int sides) {
        if (sides < 3) return 1.0f;
        float sectorAngle = (2.0f * M_PI) / (float)sides;
        float offset = (sides == 4) ? (M_PI / 4.0f) : 0.0f;
        if (sides == 3) offset = M_PI / 6.0f;

        float t = angle + offset;
        while (t < 0) t += 2.0f * M_PI;
        float localAngle = fmod(t, sectorAngle) - (sectorAngle / 2.0f);
        return cosf(sectorAngle / 2.0f) / cosf(localAngle);
    }

    void GenerateSOR() {
        if (g_profile.empty()) return;
        g_surface.clear();

        int segments = 60;
        float angleStep = (2.0f * M_PI) / (float)segments;

        for (const auto& p : g_profile) {
            std::vector<Point3D> ring;
            float rawRadius = p.x * SCALE_FACTOR;
            float y = p.y * SCALE_FACTOR;

            for (int i = 0; i <= segments; ++i) {
                float angle = i * angleStep;
                float scale = GetPolyScale(angle, p.sides);

                float x = rawRadius * cosf(angle) * scale;
                float z = rawRadius * sinf(angle) * scale;

                ring.push_back({ x, y, z, p.color.r, p.color.g, p.color.b, p.color.a });
            }
            g_surface.push_back(ring);
        }
    }

    int FindPointNear(float wx, float wy) {
        for (int i = 0; i < g_profile.size(); ++i) {
            float dx = g_profile[i].x - wx;
            float dy = g_profile[i].y - wy;
            if (sqrt(dx * dx + dy * dy) < SELECT_RADIUS) return i;
        }
        return -1;
    }

    // ================== 파일 저장 및 게임 시작 연동 ==================

    void SaveAndStartGame() {
        if (g_profile.empty()) return;

        // 게임용 데이터 저장 (user_model.txt)
        // 형식: 점개수 \n r h
        std::ofstream fout("user_model.txt");
        if (fout.is_open()) {
            fout << g_profile.size() << std::endl;
            for (const auto& p : g_profile) {
                // Modeler의 x는 반지름(r), y는 높이(h)
                // 화면 좌표(예: 100.0)를 게임 스케일(1.0)로 변환
                float r = std::abs(p.x) * SCALE_FACTOR;
                float h = p.y * SCALE_FACTOR;
                fout << r << " " << h << std::endl;
            }
            fout.close();
            std::cout << "[SYSTEM] 모델 저장 완료! 게임을 시작합니다.\n";
        }

        // 상태 전환: 게임 플레이로
        g_gameState = STATE_PLAYING;

        // 저장된 모델을 포함하여 게임 오브젝트 다시 로드
        InitEmotionObjects();
    }

    // ================== UI 그리기 ==================

    void DrawStringHelper(float x, float y, std::string str, void* font = GLUT_BITMAP_HELVETICA_18) {
        glRasterPos2f(x, y);
        for (char c : str) glutBitmapCharacter(font, c);
    }

    void DrawBar(float x, float y, float w, float h, float val, float r, float g, float b, bool isAlpha = false) {
        glColor3f(0.2f, 0.2f, 0.2f);
        glBegin(GL_QUADS); glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h); glEnd();

        if (isAlpha) glColor3f(val, val, val);
        else glColor3f(r, g, b);

        glBegin(GL_QUADS); glVertex2f(x, y); glVertex2f(x + w * val, y); glVertex2f(x + w * val, y + h); glVertex2f(x, y + h); glEnd();

        glColor3f(0.6f, 0.6f, 0.6f); glLineWidth(1.0f);
        glBegin(GL_LINE_LOOP); glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h); glEnd();
    }

    void DrawUI() {
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
        gluOrtho2D(0, winWidth, winHeight, 0); // Modeler 좌표계에 맞춤
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.1f, 0.1f, 0.1f, 0.9f);
        glBegin(GL_QUADS);
        glVertex2f(10, 10); glVertex2f(350, 10);
        glVertex2f(350, 420); glVertex2f(10, 420);
        glEnd();
        glDisable(GL_BLEND);

        int y = 40; int h = 25;
        glColor3f(1, 1, 1);

        if (g_is3DMode) {
            glColor3f(0, 1, 1); DrawStringHelper(20, y, "[ 3D PREVIEW ]");
            glColor3f(0.8f, 0.8f, 0.8f);
            DrawStringHelper(20, y += h * 2, "Space: Return to Edit");
            DrawStringHelper(20, y += h, "Drag : Rotate");
            glColor3f(1.0f, 0.5f, 0.5f);
            DrawStringHelper(20, y += h * 2, ">> Press 'E' to START GAME");
        }
        else {
            glColor3f(1, 0.8f, 0); DrawStringHelper(20, y, "[ MODELER MODE ]");

            std::string shape = (g_currentSides == 0) ? "Circle" : std::to_string(g_currentSides) + "-Gon";
            glColor3f(0.9f, 0.9f, 0.9f);
            DrawStringHelper(20, y += h, "Shape: " + shape);

            y += h;
            glColor3f(0.5f, 0.5f, 0.5f); glRectf(20, y, 340, y + 30);
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(g_currentColor.r, g_currentColor.g, g_currentColor.b, g_currentColor.a);
            glRectf(20, y, 340, y + 30);
            glDisable(GL_BLEND);
            y += 40;

            float bw = 200.0f, bh = 15.0f, bx = 60.0f;
            DrawStringHelper(20, y + 12, "R"); DrawBar(bx, y, bw, bh, g_currentColor.r, 1, 0, 0); y += 25;
            DrawStringHelper(20, y + 12, "G"); DrawBar(bx, y, bw, bh, g_currentColor.g, 0, 1, 0); y += 25;
            DrawStringHelper(20, y + 12, "B"); DrawBar(bx, y, bw, bh, g_currentColor.b, 0, 0, 1); y += 25;
            DrawStringHelper(20, y + 12, "A"); DrawBar(bx, y, bw, bh, g_currentColor.a, 1, 1, 1, true); y += 25;

            std::stringstream ss;
            ss << std::fixed << std::setprecision(2)
                << "R:" << g_currentColor.r << " G:" << g_currentColor.g
                << " B:" << g_currentColor.b << " A:" << g_currentColor.a;
            glColor3f(0.8f, 0.8f, 0.8f);
            DrawStringHelper(20, y, ss.str(), GLUT_BITMAP_HELVETICA_12);

            glColor3f(0.6f, 0.6f, 0.6f);
            y += h;
            DrawStringHelper(20, y += h, "Draw Profile on Right");
            DrawStringHelper(20, y += h, "Space: 3D / E: Start Game");
        }

        glEnable(GL_DEPTH_TEST);
        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    }

    void Draw2DProfile() {
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glColor3f(0.3f, 1.0f, 0.3f); glVertex2f(0, -winHeight); glVertex2f(0, winHeight);
        glColor3f(0.3f, 0.3f, 0.3f); glVertex2f(-winWidth, 0); glVertex2f(winWidth, 0);
        glEnd();

        if (g_profile.empty()) return;

        glLineWidth(2.0f);
        glColor3f(0.6f, 0.6f, 0.6f);
        glBegin(GL_LINE_STRIP);
        for (const auto& p : g_profile) glVertex2f(p.x, p.y);
        glEnd();

        glPointSize(12.0f);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_POINTS);
        for (int i = 0; i < g_profile.size(); ++i) {
            glColor4f(g_profile[i].color.r, g_profile[i].color.g, g_profile[i].color.b, g_profile[i].color.a);
            glVertex2f(g_profile[i].x, g_profile[i].y);
        }
        glEnd();
        glDisable(GL_BLEND);

        if (g_selectedPointIdx != -1) {
            Point2D p = g_profile[g_selectedPointIdx];
            glColor3f(1, 1, 1); glPointSize(16.0f);
            glBegin(GL_POINTS); glVertex2f(p.x, p.y); glEnd();
        }
    }

    void Draw3D() {
        if (g_surface.empty()) return;

        glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_COLOR_MATERIAL);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for (size_t i = 0; i < g_surface.size() - 1; ++i) {
            glBegin(GL_QUAD_STRIP);
            for (size_t j = 0; j < g_surface[i].size(); ++j) {
                Point3D p1 = g_surface[i][j];
                Point3D p2 = g_surface[i + 1][j];

                float len = sqrt(p1.x * p1.x + p1.z * p1.z);
                if (len > 0) glNormal3f(p1.x / len, 0, p1.z / len);

                glColor4f(p1.r, p1.g, p1.b, p1.a);
                glVertex3f(p1.x, p1.y, p1.z);

                glColor4f(p2.r, p2.g, p2.b, p2.a);
                glVertex3f(p2.x, p2.y, p2.z);
            }
            glEnd();
        }
        glDisable(GL_BLEND);
    }
} // namespace Modeler

using namespace Modeler;

// ================== Main.cpp에서 호출할 외부 함수 ==================

void InitModeler() {
    g_profile.clear();
    g_surface.clear();
    g_is3DMode = false;
    std::cout << "[SYSTEM] 모델러 초기화 완료.\n";
}

void DrawModeler() {
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    if (g_is3DMode) gluPerspective(45, (float)winWidth / winHeight, 0.1, 100);
    else gluOrtho2D(-winWidth / 2, winWidth / 2, -winHeight / 2, winHeight / 2);

    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glClearColor(0.2f, 0.2f, 0.2f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (g_is3DMode) {
        glTranslatef(0, 0, -g_camDist);
        glRotatef(g_camRotX, 1, 0, 0);
        glRotatef(g_camRotY, 0, 1, 0);
        float lp[] = { 10, 50, 50, 1 }; glLightfv(GL_LIGHT0, GL_POSITION, lp);
        Draw3D();
    }
    else {
        Draw2DProfile();
    }
    DrawUI();
}

void HandleModelerMouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (g_is3DMode) {
            g_prevMouseX = x; g_prevMouseY = y;
        }
        else {
            float wx, wy; ScreenToWorld(x, y, wx, wy);
            int picked = FindPointNear(wx, wy);
            if (picked != -1) {
                g_selectedPointIdx = picked;
                g_currentSides = g_profile[picked].sides;
                g_currentColor = g_profile[picked].color;
            }
            else if (wx >= 0) {
                g_profile.push_back({ wx, wy, g_currentSides, g_currentColor });
                g_selectedPointIdx = g_profile.size() - 1;
            }
        }
    }
}

void HandleModelerMotion(int x, int y) {
    if (g_is3DMode) {
        g_camRotY += (x - g_prevMouseX) * 0.5f;
        g_camRotX += (y - g_prevMouseY) * 0.5f;
        g_prevMouseX = x; g_prevMouseY = y;
        glutPostRedisplay();
    }
    else if (g_selectedPointIdx != -1) {
        float wx, wy; ScreenToWorld(x, y, wx, wy);
        if (wx >= 0) {
            g_profile[g_selectedPointIdx].x = wx;
            g_profile[g_selectedPointIdx].y = wy;
        }
        glutPostRedisplay();
    }
}

void HandleModelerKeyboard(unsigned char key, int x, int y) {
    float step = 0.05f;

    if (key == 'r') Modeler::g_currentColor.r += step;
    else if (key == 'R') Modeler::g_currentColor.r -= step;
    else if (key == 'g') Modeler::g_currentColor.g += step;
    else if (key == 'G') Modeler::g_currentColor.g -= step;
    else if (key == 'b') Modeler::g_currentColor.b += step;
    else if (key == 'B') Modeler::g_currentColor.b -= step;
    else if (key == 'a') Modeler::g_currentColor.a += step;
    else if (key == 'A') Modeler::g_currentColor.a -= step;

    // 클램핑
    if (Modeler::g_currentColor.r > 1) Modeler::g_currentColor.r = 1; if (Modeler::g_currentColor.r < 0) Modeler::g_currentColor.r = 0;
    if (Modeler::g_currentColor.g > 1) Modeler::g_currentColor.g = 1; if (Modeler::g_currentColor.g < 0) Modeler::g_currentColor.g = 0;
    if (Modeler::g_currentColor.b > 1) Modeler::g_currentColor.b = 1; if (Modeler::g_currentColor.b < 0) Modeler::g_currentColor.b = 0;
    if (Modeler::g_currentColor.a > 1) Modeler::g_currentColor.a = 1; if (Modeler::g_currentColor.a < 0) Modeler::g_currentColor.a = 0;

    if (key >= '0' && key <= '9') {
        int sides = (key == '0') ? 0 : (key - '0');
        if (sides < 3 && sides != 0) sides = 0;
        g_currentSides = sides;
        if (g_selectedPointIdx != -1) g_profile[g_selectedPointIdx].sides = sides;
    }
    else if (key == ' ') {
        g_is3DMode = !g_is3DMode;
        g_selectedPointIdx = -1;
        if (g_is3DMode) GenerateSOR();
    }
    else if (key == 'e' || key == 'E') {
        if (g_is3DMode) SaveAndStartGame(); // 게임 시작 트리거
    }
    else if (key == 127 || key == 8) {
        if (!g_is3DMode && g_selectedPointIdx != -1) {
            g_profile.erase(g_profile.begin() + g_selectedPointIdx);
            g_selectedPointIdx = -1;
        }
    }

    if (g_selectedPointIdx != -1) {
        g_profile[g_selectedPointIdx].color = g_currentColor;
    }
}