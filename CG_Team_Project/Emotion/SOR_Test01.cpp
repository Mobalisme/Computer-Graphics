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

// ================== 데이터 구조 ==================

struct Color {
    float r, g, b, a; // [NEW] Alpha(투명도) 추가
};

struct Point2D {
    float x, y;
    int sides;
    Color color;
};

struct Point3D {
    float x, y, z;
    float r, g, b, a; // [NEW] 3D 점에도 Alpha 추가
};

// ================== 전역 변수 ==================

std::vector<Point2D> g_profile;
std::vector<std::vector<Point3D>> g_surface;

bool g_is3DMode = false;
int  g_selectedPointIdx = -1;
int  g_currentSides = 0;
Color g_currentColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // 기본값: 흰색 불투명 (Alpha=1.0)

float g_camDist = 12.0f;
float g_camRotX = 20.0f;
float g_camRotY = 0.0f;
int   g_prevMouseX = 0;
int   g_prevMouseY = 0;

int g_winWidth = 1000;
int g_winHeight = 800;
const float SCALE_FACTOR = 0.01f;
const float SELECT_RADIUS = 15.0f;

// ================== 파일 저장 (Export: RGBA) ==================

void ExportModel(const char* filename) {
    if (g_surface.empty()) return;
    std::ofstream fout(filename);
    if (!fout.is_open()) return;

    int rows = g_surface.size();
    int cols = g_surface[0].size();

    fout << rows << " " << cols << std::endl;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            Point3D p = g_surface[i][j];
            // [NEW] x y z r g b a 순서로 저장
            fout << p.x << " " << p.y << " " << p.z << " "
                << p.r << " " << p.g << " " << p.b << " " << p.a << std::endl;
        }
    }
    fout.close();
    std::cout << "모델 저장 완료 (RGBA): " << filename << std::endl;
}

// ================== 수학/로직 ==================

void ScreenToWorld(int x, int y, float& wx, float& wy) {
    wx = (float)x - (g_winWidth / 2.0f);
    wy = (g_winHeight / 2.0f) - (float)y;
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

            // [NEW] 색상 및 투명도 전달
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

// ================== UI 그리기 (RGBA 게이지) ==================

void DrawString(float x, float y, std::string str, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : str) glutBitmapCharacter(font, c);
}

// 바 그리기 헬퍼 (배경, 게이지, 테두리)
void DrawBar(float x, float y, float w, float h, float val, float r, float g, float b, bool isAlpha = false) {
    // 배경
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS); glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h); glEnd();

    // 게이지
    if (isAlpha) glColor3f(val, val, val); // 투명도는 흰색 막대로 표시
    else glColor3f(r, g, b);

    glBegin(GL_QUADS); glVertex2f(x, y); glVertex2f(x + w * val, y); glVertex2f(x + w * val, y + h); glVertex2f(x, y + h); glEnd();

    // 테두리
    glColor3f(0.6f, 0.6f, 0.6f); glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP); glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h); glEnd();
}

void DrawUI() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, g_winWidth, g_winHeight, 0);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    // 배경 패널
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.1f, 0.1f, 0.1f, 0.9f);
    glBegin(GL_QUADS);
    glVertex2f(10, 10); glVertex2f(350, 10);
    glVertex2f(350, 420); glVertex2f(10, 420); // 높이 확장
    glEnd();
    glDisable(GL_BLEND);

    int y = 40; int h = 25;
    glColor3f(1, 1, 1);

    if (g_is3DMode) {
        glColor3f(0, 1, 1); DrawString(20, y, "[ 3D VIEW MODE ]");
        glColor3f(0.8f, 0.8f, 0.8f);
        DrawString(20, y += h * 2, "Space: Return to Edit");
        DrawString(20, y += h, "Drag : Rotate");
        glColor3f(1.0f, 0.5f, 0.5f);
        DrawString(20, y += h * 2, ">> Press 'E' to Export (RGBA)");
    }
    else {
        glColor3f(1, 0.8f, 0); DrawString(20, y, "[ 2D EDIT MODE ]");

        std::string shape = (g_currentSides == 0) ? "Circle" : std::to_string(g_currentSides) + "-Gon";
        glColor3f(0.9f, 0.9f, 0.9f);
        DrawString(20, y += h, "Shape: " + shape);

        // --- 색상 미리보기 ---
        y += h;
        // 투명도 확인을 위해 체크무늬 배경을 먼저 그림 (선택 사항)
        glColor3f(0.5f, 0.5f, 0.5f);
        glRectf(20, y, 340, y + 30);

        // 실제 색상 (투명도 적용)
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(g_currentColor.r, g_currentColor.g, g_currentColor.b, g_currentColor.a);
        glRectf(20, y, 340, y + 30);
        glDisable(GL_BLEND);
        y += 40;

        // --- RGBA 슬라이더 ---
        float bw = 200.0f, bh = 15.0f, bx = 60.0f;

        DrawString(20, y + 12, "R"); DrawBar(bx, y, bw, bh, g_currentColor.r, 1, 0, 0); y += 25;
        DrawString(20, y + 12, "G"); DrawBar(bx, y, bw, bh, g_currentColor.g, 0, 1, 0); y += 25;
        DrawString(20, y + 12, "B"); DrawBar(bx, y, bw, bh, g_currentColor.b, 0, 0, 1); y += 25;

        // [NEW] Alpha 슬라이더
        DrawString(20, y + 12, "A"); DrawBar(bx, y, bw, bh, g_currentColor.a, 1, 1, 1, true); y += 25;

        // 수치 표시
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2)
            << "R:" << g_currentColor.r << " G:" << g_currentColor.g
            << " B:" << g_currentColor.b << " A:" << g_currentColor.a;
        glColor3f(0.8f, 0.8f, 0.8f);
        DrawString(20, y, ss.str(), GLUT_BITMAP_HELVETICA_12);

        // 조작 설명
        glColor3f(0.6f, 0.6f, 0.6f);
        y += h;
        DrawString(20, y += h, "Color: R, G, B, A(Alpha)");
        DrawString(20, y += h, "(Shift + Key to Decrease)");
        DrawString(20, y += h, "Space: 3D / E: Export");
    }

    glEnable(GL_DEPTH_TEST);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void Draw2DProfile() {
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glColor3f(0.3f, 1.0f, 0.3f); glVertex2f(0, -g_winHeight); glVertex2f(0, g_winHeight);
    glColor3f(0.3f, 0.3f, 0.3f); glVertex2f(-g_winWidth, 0); glVertex2f(g_winWidth, 0);
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

    // [NEW] 투명도 렌더링 설정
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 투명한 물체 렌더링 시 깊이 버퍼 쓰기를 끄는 것이 일반적이지만,
    // SOR 모델은 겹치는 부분이 적어 단순히 켜두어도 괜찮습니다.
    // 만약 뒷면이 안 보인다면 아래 줄의 주석을 해제하세요.
    // glDepthMask(GL_FALSE); 

    for (size_t i = 0; i < g_surface.size() - 1; ++i) {
        glBegin(GL_QUAD_STRIP);
        for (size_t j = 0; j < g_surface[i].size(); ++j) {
            Point3D p1 = g_surface[i][j];
            Point3D p2 = g_surface[i + 1][j];

            float len = sqrt(p1.x * p1.x + p1.z * p1.z);
            if (len > 0) glNormal3f(p1.x / len, 0, p1.z / len);

            // [NEW] 4f로 변경하여 Alpha 적용
            glColor4f(p1.r, p1.g, p1.b, p1.a);
            glVertex3f(p1.x, p1.y, p1.z);

            glColor4f(p2.r, p2.g, p2.b, p2.a);
            glVertex3f(p2.x, p2.y, p2.z);
        }
        glEnd();
    }

    // glDepthMask(GL_TRUE); // 복구
    glDisable(GL_BLEND);
}

// ================== 입력 콜백 ==================

void Display() {
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    if (g_is3DMode) gluPerspective(45, (float)g_winWidth / g_winHeight, 0.1, 100);
    else gluOrtho2D(-g_winWidth / 2, g_winWidth / 2, -g_winHeight / 2, g_winHeight / 2);

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
    glutSwapBuffers();
}

void Mouse(int button, int state, int x, int y) {
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

void Motion(int x, int y) {
    if (g_is3DMode) {
        g_camRotY += (x - g_prevMouseX) * 0.5f;
        g_camRotX += (y - g_prevMouseY) * 0.5f;
        g_prevMouseX = x; g_prevMouseY = y;
    }
    else if (g_selectedPointIdx != -1) {
        float wx, wy; ScreenToWorld(x, y, wx, wy);
        if (wx >= 0) {
            g_profile[g_selectedPointIdx].x = wx;
            g_profile[g_selectedPointIdx].y = wy;
        }
    }
    glutPostRedisplay();
}

void AdjustColor(float& channel, float delta) {
    channel += delta;
    if (channel > 1.0f) channel = 1.0f;
    if (channel < 0.0f) channel = 0.0f;
}

void Keyboard(unsigned char key, int x, int y) {
    float step = 0.02f;

    // RGB 조절
    if (key == 'r') AdjustColor(g_currentColor.r, step);
    else if (key == 'R') AdjustColor(g_currentColor.r, -step);
    else if (key == 'g') AdjustColor(g_currentColor.g, step);
    else if (key == 'G') AdjustColor(g_currentColor.g, -step);
    else if (key == 'b') AdjustColor(g_currentColor.b, step);
    else if (key == 'B') AdjustColor(g_currentColor.b, -step);

    // [NEW] 투명도(Alpha) 조절 (a: 증가, A: 감소)
    else if (key == 'a') AdjustColor(g_currentColor.a, step); // 더 불투명하게
    else if (key == 'A') AdjustColor(g_currentColor.a, -step); // 더 투명하게

    else if (key >= '0' && key <= '9') {
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
        if (g_is3DMode) ExportModel("model_data.txt");
    }
    else if (key == 127 || key == 8) {
        if (!g_is3DMode && g_selectedPointIdx != -1) {
            g_profile.erase(g_profile.begin() + g_selectedPointIdx);
            g_selectedPointIdx = -1;
        }
    }
    else if (key == 27) exit(0);

    if (g_selectedPointIdx != -1) {
        g_profile[g_selectedPointIdx].color = g_currentColor;
    }

    glutPostRedisplay();
}

void Reshape(int w, int h) {
    g_winWidth = w; g_winHeight = h;
    glViewport(0, 0, w, h);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // RGBA 모드
    glutInitWindowSize(1000, 800);
    glutCreateWindow("Fine Control RGBA Modeler");

    glEnable(GL_DEPTH_TEST);
    // [NEW] 메인 루프 시작 전 블렌딩 허용
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glutDisplayFunc(Display);
    glutMouseFunc(Mouse);
    glutMotionFunc(Motion);
    glutKeyboardFunc(Keyboard);
    glutReshapeFunc(Reshape);

    glutMainLoop();
    return 0;
}