#include "Global.h"
#include "SOR_Objects.h" // 오브젝트 그리기용
#include <iostream>
#include <string>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Star { float x, y, z; float brightness; };
std::vector<Star> g_stars;

GLuint LoadTextureFromFile(const char* filename) {
    int w, h, c;
    unsigned char* d = stbi_load(filename, &w, &h, &c, 0);
    if (!d) return 0;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    GLenum f = (c == 4) ? GL_RGBA : (c == 3 ? GL_RGB : GL_RED);
    glTexImage2D(GL_TEXTURE_2D, 0, f, w, h, 0, f, GL_UNSIGNED_BYTE, d);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(d);
    return tex;
}

void VertexConfusion(float x, float y, float z) {
    if (g_confusionTimer > 0.0f) {
        // 시간이 지남에 따라 물결치듯 흔들림
        float distortion = sinf(y * 0.5f + g_cutsceneTime * 3.0f) * 0.2f;
        float wave = cosf(x * 0.2f + z * 0.2f + g_cutsceneTime * 5.0f) * 0.1f;

        // 벽이 숨을 쉬는 것처럼 왜곡됨
        x += distortion + wave;
        z += distortion - wave;
    }
    glVertex3f(x, y, z);
}

void ChangeWallTexture(int stage) {
    // 1. 먼저 wall0.png / wall0.jpg 등 단계별 파일을 찾습니다.
    std::string path = "Textures/wall" + std::to_string(stage) + ".png";
    GLuint newTex = LoadTextureFromFile(path.c_str());

    if (newTex == 0) {
        // png가 없으면 jpg 시도
        path = "Textures/wall" + std::to_string(stage) + ".jpg";
        newTex = LoadTextureFromFile(path.c_str());
    }

    // 2. [핵심] 만약 파일을 못 찾았는데, 그게 '처음 시작(0단계)'이라면?
    // -> 기본 Wall.png를 대신 로드합니다. (회색 벽 방지)
    if (newTex == 0 && stage == 0) {
        std::cout << "[SYSTEM] 'wall0' not found. Falling back to default 'Wall.png'\n";

        newTex = LoadTextureFromFile("Textures/Wall.png");
        if (newTex == 0) newTex = LoadTextureFromFile("Wall.png");
        if (newTex == 0) newTex = LoadTextureFromFile("wall.png");
    }

    // 3. 텍스처 적용
    if (newTex != 0) {
        if (g_wallTex != 0) glDeleteTextures(1, &g_wallTex); // 기존 메모리 정리
        g_wallTex = newTex;
        std::cout << "[SYSTEM] Wall texture set to Stage " << stage << "\n";
    }
    else {
        std::cout << "[WARNING] Failed to load wall texture for Stage " << stage << "\n";
    }
}


void LoadTextures() {

    ChangeWallTexture(0);

    // 바닥 로드
    g_floorTex = LoadTextureFromFile("Textures/floor.jpeg");
    if (g_floorTex == 0) g_floorTex = LoadTextureFromFile("floor.jpeg");
    if (g_floorTex == 0) g_floorTex = LoadTextureFromFile("Textures/floor.jpg");
    if (g_floorTex == 0) g_floorTex = LoadTextureFromFile("floor.jpg");

    // ===== [NEW] Ocean textures for Sadness sea =====
    g_oceanTex1 = LoadTextureFromFile("Textures/Ocean 01.jpg");
    if (g_oceanTex1 == 0) g_oceanTex1 = LoadTextureFromFile("Textures/Ocean 01.png");
    if (g_oceanTex1 == 0) g_oceanTex1 = LoadTextureFromFile("Ocean 01.jpg");

    g_oceanTex2 = LoadTextureFromFile("Textures/Ocean 02.jpg");
    if (g_oceanTex2 == 0) g_oceanTex2 = LoadTextureFromFile("Textures/Ocean 02.png");
    if (g_oceanTex2 == 0) g_oceanTex2 = LoadTextureFromFile("Ocean 02.jpg");

    if (g_oceanTex1) {
        glBindTexture(GL_TEXTURE_2D, g_oceanTex1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    if (g_oceanTex2) {
        glBindTexture(GL_TEXTURE_2D, g_oceanTex2);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // repeat 설정 (파도 스크롤용)
    if (g_oceanTex1) {
        glBindTexture(GL_TEXTURE_2D, g_oceanTex1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    if (g_oceanTex2) {
        glBindTexture(GL_TEXTURE_2D, g_oceanTex2);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    if (g_floorTex) std::cout << "[SYSTEM] Floor texture loaded.\n";
    else std::cout << "[WARNING] Floor texture NOT found.\n";

    // ==========================================
    // [수정됨] 비디오 프레임 분리 로드
    // ==========================================
    g_videoSad.clear();
    g_videoJoy.clear();

    // 1. 슬픔 영상 로드 (1.jpg ~ 120.jpg)
    // 위치: Video/Sad/ 폴더
    std::cout << "[SYSTEM] Loading SAD video frames (1~120)...\n";
    for (int i = 1; i <= 80; ++i) {
        std::string path = "Video/Sad/" + std::to_string(i) + ".jpg";
        GLuint t = LoadTextureFromFile(path.c_str());

        if (t != 0) {
            glBindTexture(GL_TEXTURE_2D, t);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

            // 슬픔 벡터에 저장
            g_videoSad.push_back(t);
        }
    }

    // 2. 기쁨 영상 로드 (201.jpg ~ 280.jpg)
    // 위치: Video/Joy/ 폴더
    std::cout << "[SYSTEM] Loading JOY video frames (201~280)...\n";
    for (int i = 201; i <= 280; ++i) {
        std::string path = "Video/Joy/" + std::to_string(i) + ".jpg";
        GLuint t = LoadTextureFromFile(path.c_str());

        if (t != 0) {
            glBindTexture(GL_TEXTURE_2D, t);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

            // 기쁨 벡터에 저장
            g_videoJoy.push_back(t);
        }
    }
}

void DrawWallBlock(float gx, float gz) {
    float x0 = gx * CELL_SIZE, x1 = (gx + 1) * CELL_SIZE;
    float z0 = gz * CELL_SIZE, z1 = (gz + 1) * CELL_SIZE;

    glBindTexture(GL_TEXTURE_2D, g_wallTex);

    // 혼란 상태면 색상을 약간 붉게/초록으로 섞어줌
    if (g_confusionTimer > 0) glColor3f(0.8f, 1.0f, 0.8f);
    else glColor3f(1, 1, 1);

    glBegin(GL_QUADS);
    // 앞면 (Normal 생략 가능하지만 유지)
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); VertexConfusion(x0, 0, z0);
    glTexCoord2f(1, 0); VertexConfusion(x1, 0, z0);
    glTexCoord2f(1, 1); VertexConfusion(x1, WALL_HEIGHT, z0);
    glTexCoord2f(0, 1); VertexConfusion(x0, WALL_HEIGHT, z0);

    // 뒷면
    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); VertexConfusion(x1, 0, z1);
    glTexCoord2f(1, 0); VertexConfusion(x0, 0, z1);
    glTexCoord2f(1, 1); VertexConfusion(x0, WALL_HEIGHT, z1);
    glTexCoord2f(0, 1); VertexConfusion(x1, WALL_HEIGHT, z1);

    // 왼쪽
    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 0); VertexConfusion(x0, 0, z1);
    glTexCoord2f(1, 0); VertexConfusion(x0, 0, z0);
    glTexCoord2f(1, 1); VertexConfusion(x0, WALL_HEIGHT, z0);
    glTexCoord2f(0, 1); VertexConfusion(x0, WALL_HEIGHT, z1);

    // 오른쪽
    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); VertexConfusion(x1, 0, z0);
    glTexCoord2f(1, 0); VertexConfusion(x1, 0, z1);
    glTexCoord2f(1, 1); VertexConfusion(x1, WALL_HEIGHT, z1);
    glTexCoord2f(0, 1); VertexConfusion(x1, WALL_HEIGHT, z0);

    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DrawFloor() {
    float w = MAZE_W * CELL_SIZE, h = MAZE_H * CELL_SIZE;
    glBindTexture(GL_TEXTURE_2D, g_floorTex); glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0);       glVertex3f(0, 0, 0);
    glTexCoord2f(MAZE_W, 0);  glVertex3f(w, 0, 0);
    glTexCoord2f(MAZE_W, MAZE_H); glVertex3f(w, 0, h);
    glTexCoord2f(0, MAZE_H);  glVertex3f(0, 0, h);
    glEnd(); glBindTexture(GL_TEXTURE_2D, 0);
}

void DrawMaze3D() {
    DrawFloor();
    for (int z = 0; z < MAZE_H; ++z) for (int x = 0; x < MAZE_W; ++x)
        if (maze[z][x] == WALL) DrawWallBlock((float)x, (float)z);
}

void DrawMiniMap() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, winWidth, 0, winHeight);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_TEXTURE_2D); glDisable(GL_LIGHTING);

    float ms = 200.0f, ox = winWidth - ms - 10, oy = winHeight - ms - 10;
    float cw = ms / MAZE_W, ch = ms / MAZE_H;

    glColor4f(0, 0, 0, 0.6f); glRectf(ox - 2, oy - 2, ox + ms + 2, oy + ms + 2);
    glColor3f(1, 1, 1);
    for (int z = 0; z < MAZE_H; ++z) for (int x = 0; x < MAZE_W; ++x)
        if (maze[z][x] == WALL) glRectf(ox + x * cw, oy + z * ch, ox + (x + 1) * cw, oy + (z + 1) * ch);

    glColor3f(1, 1, 0);
    for (const auto& obj : g_worldObjects) {
        if (!obj.collected) {
            float cx = ox + (obj.mazeX + 0.5f) * cw, cy = oy + (obj.mazeY + 0.5f) * ch;
            glRectf(cx - 2, cy - 2, cx + 2, cy + 2);
        }
    }

    glColor3f(1, 0, 0);
    float px = ox + (camX / (MAZE_W * CELL_SIZE)) * ms, py = oy + (camZ / (MAZE_H * CELL_SIZE)) * ms;
    glBegin(GL_TRIANGLE_FAN); glVertex2f(px, py);
    for (int i = 0; i <= 16; ++i) glVertex2f(px + cos(i / 16.0f * 6.28f) * 4, py + sin(i / 16.0f * 6.28f) * 4);
    glEnd();

    glEnable(GL_DEPTH_TEST); glEnable(GL_TEXTURE_2D);
    if (g_gameState == STATE_PLAYING) glEnable(GL_LIGHTING);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

// ================== [NEW] 기쁨 씬 업그레이드 (고퀄리티 버전) ==================

void DrawSkyCylinder() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST); // 배경이므로 가장 뒤에 그림

    glPushMatrix();
    // 하늘을 카메라 위치에 고정 (따라다니게 만듦 -> 무한한 배경 효과)
    glTranslatef(camX, camY, camZ);

    float radius = 200.0f; // 매우 큰 반지름
    float topY = 150.0f;
    float botY = -100.0f;
    int slices = 32;

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; ++i) {
        float theta = (float)i / slices * 2.0f * 3.141592f;
        float x = cosf(theta) * radius;
        float z = sinf(theta) * radius;

        // 위쪽: 짙은 보라색 (밤하늘)
        glColor3f(0.1f, 0.05f, 0.2f);
        glVertex3f(x, topY, z);

        // 아래쪽: 불타는 오렌지색 (노을 지평선)
        glColor3f(0.9f, 0.4f, 0.2f);
        glVertex3f(x, botY, z);
    }
    glEnd();

    // 태양 그리기 (하늘과 함께 회전하도록 여기에 포함)
    glPushMatrix();
    // Z축 뒤쪽(-180)에 태양 배치
    glTranslatef(0.0f, 10.0f, -180.0f);
    glColor3f(1.0f, 0.6f, 0.1f);
    glutSolidSphere(15.0, 30, 30); // 태양 크기 확대
    glPopMatrix();

    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}


// 2. 노을빛 받은 구름 (분홍/보라빛)
void DrawSunsetClouds() {
    glDisable(GL_LIGHTING);
    glColor3f(0.8f, 0.5f, 0.5f); // 분홍빛 구름

    float move = g_cutsceneTime * 1.5f;

    for (int i = 0; i < 10; ++i) {
        glPushMatrix();
        float cx = -80.0f + (float)((i * 25 + (int)move) % 160);
        float cy = 20.0f + (i % 3) * 4.0f;
        float cz = -50.0f + (i * 15) % 80;

        glTranslatef(cx, cy, cz);
        glScalef(2.5f, 0.8f, 1.2f);

        glutSolidSphere(4.0, 12, 12);
        glTranslatef(2.5f, 0.2f, 0.5f); glutSolidSphere(3.0, 12, 12);
        glTranslatef(-4.0f, -0.1f, -0.5f); glutSolidSphere(2.8, 12, 12);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

// 3. 언덕 지형 (색감을 저녁에 맞춰 어둡고 따뜻하게 변경)
void DrawHillSunset() {
    glDisable(GL_TEXTURE_2D);

    int steps = 60;
    float range = 60.0f; // 범위 더 넓게
    float stepSize = range * 2.0f / steps;

    for (int z = 0; z < steps; ++z) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int x = 0; x <= steps; ++x) {
            float px = -range + x * stepSize;
            float pz = -range + z * stepSize;
            float pzNext = -range + (z + 1) * stepSize;

            float h1 = 8.0f * exp(-(px * px + pz * pz) / 400.0f); // 언덕 더 완만하게
            float h2 = 8.0f * exp(-(px * px + pzNext * pzNext) / 400.0f);

            float c1 = 0.2f + h1 * 0.03f;
            float c2 = 0.2f + h2 * 0.03f;

            glNormal3f(px, h1, pz);
            glColor3f(0.3f, 0.25f + c1, 0.1f);
            glVertex3f(px, h1, pz);

            glNormal3f(px, h2, pzNext);
            glColor3f(0.3f, 0.25f + c2, 0.1f);
            glVertex3f(px, h2, pzNext);
        }
        glEnd();
    }
}

void DrawJoyScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 카메라 셋팅
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0, (float)winWidth / winHeight, 0.1, 500.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    float dx = cos(camYaw) * cos(camPitch);
    float dy = sin(camPitch);
    float dz = sin(camYaw) * cos(camPitch);
    gluLookAt(camX, camY, camZ,
        camX + dx, camY + dy, camZ + dz,
        0.0f, 1.0f, 0.0f);

    // 1. 하늘
    DrawSkyCylinder();

    // 2. 조명 & 안개
    glEnable(GL_FOG);
    GLfloat fogColor[] = { 0.9f, 0.5f, 0.3f, 1.0f };
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.01f);

    GLfloat sunPos[] = { 0.0f, 20.0f, -100.0f, 1.0f };
    GLfloat sunColor[] = { 1.0f, 0.8f, 0.6f, 1.0f };
    glEnable(GL_LIGHTING);
    glLightfv(GL_LIGHT0, GL_POSITION, sunPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunColor);

    // 3. [영상 스크린] 카메라 정면 공중에 배치
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glEnable(GL_TEXTURE_2D);

    int frameIdx = (int)(g_cutsceneTime * 10);
    if (frameIdx >= (int)g_videoJoy.size()) frameIdx = (int)g_videoJoy.size() - 1;
    if (frameIdx < 0) frameIdx = 0;

    if (!g_videoJoy.empty()) glBindTexture(GL_TEXTURE_2D, g_videoJoy[frameIdx]);
    else glBindTexture(GL_TEXTURE_2D, g_wallTex);

    glPushMatrix();
    // [위치 수정] 
    // Z = -5.0f : 카메라(Z=15)보다 앞쪽 
    // Y = 10.0f : 눈높이(Y=5)보다 위쪽 (하늘에 뜬 느낌)
    glTranslatef(0.0f, 10.0f, -5.0f);
    glColor3f(1.0f, 1.0f, 1.0f);

    // [크기] 적당한 크기 유지 (16:9 비율)
    float hw = 8.0f;   // 폭
    float hh = 4.5f;   // 높이

    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);

    // [상하반전 유지]
    glTexCoord2f(0, 1); glVertex3f(-hw, -hh, 0); // 좌하단
    glTexCoord2f(1, 1); glVertex3f(hw, -hh, 0); // 우하단
    glTexCoord2f(1, 0); glVertex3f(hw, hh, 0); // 우상단
    glTexCoord2f(0, 0); glVertex3f(-hw, hh, 0); // 좌상단
    glEnd();
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);

    // 4. [수정] 언덕 제거 -> 평평한 바닥 깔기
    // DrawHillSunset(); // 기존 언덕 삭제

    // 대신 어두운 평지(바닥)를 그려서 지평선을 만듦
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.15f, 0.1f, 0.05f); // 어두운 갈색 땅
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    float groundSize = 200.0f;
    glVertex3f(-groundSize, 0.0f, -groundSize);
    glVertex3f(groundSize, 0.0f, -groundSize);
    glVertex3f(groundSize, 0.0f, groundSize);
    glVertex3f(-groundSize, 0.0f, groundSize);
    glEnd();

    // 5. 구름
    DrawSunsetClouds();

    glDisable(GL_FOG);
}
void InitStars() {
    g_stars.clear();
    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_real_distribution<float> pos(-80.0f, 80.0f), h(10.0f, 60.0f), b(0.5f, 1.0f);
    for (int i = 0; i < 400; ++i) g_stars.push_back({ pos(gen), h(gen), pos(gen), b(gen) });
}

float GetWaveHeight(float x, float z, float t) {
    return sinf(x * 0.5f + t) * 0.5f + sinf(z * 0.3f + t * 0.8f) * 0.5f + sinf((x + z) * 0.2f + t * 1.5f) * 0.2f;
}

void DrawNightScene() {
    glClearColor(0.01f, 0.01f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0, (float)winWidth / winHeight, 0.1, 300.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    float dx = cos(camYaw) * cos(camPitch), dy = sin(camPitch), dz = sin(camYaw) * cos(camPitch);
    float waveY = sinf(g_cutsceneTime * 1.2f) * 0.15f;
    gluLookAt(camX, camY + waveY, camZ, camX + dx, camY + dy + waveY, camZ + dz, 0, 1, 0);

    // [NEW] Sadness sea fog (컷신 전용)
    glEnable(GL_FOG);
    GLfloat fogColor[] = { 0.02f, 0.03f, 0.06f, 1.0f }; // 바다/밤 톤에 맞춘 아주 어두운 청흑
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.012f);  // 0.008~0.02 사이에서 취향 조절

    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    glPointSize(2.0f); glBegin(GL_POINTS);
    for (const auto& s : g_stars) {
        float tw = 0.8f + 0.2f * sinf(g_cutsceneTime * 5.0f + s.x);
        glColor3f(s.brightness * tw, s.brightness * tw, s.brightness * tw);
        glVertex3f(s.x, s.y, s.z);
    }
    glEnd();

    glEnable(GL_TEXTURE_2D);
    int frameIdx = (int)(g_cutsceneTime * 10);
    if (frameIdx >= (int)g_videoSad.size()) frameIdx = (int)g_videoSad.size() - 1;
    if (frameIdx < 0) frameIdx = 0;

    if (!g_videoSad.empty()) glBindTexture(GL_TEXTURE_2D, g_videoSad[frameIdx]);
    else glBindTexture(GL_TEXTURE_2D, g_wallTex); // 영상 없으면 벽 텍스처로 대체

    glPushMatrix(); glTranslatef(0.0f, 25.0f, 45.0f);
    float flk = 0.95f + (rand() % 5) * 0.01f; glColor3f(flk, flk, flk);
    glBegin(GL_QUADS); glTexCoord2f(0, 1); glVertex3f(-24, -13.5, 0); glTexCoord2f(1, 1); glVertex3f(24, -13.5, 0); glTexCoord2f(1, 0); glVertex3f(24, 13.5, 0); glTexCoord2f(0, 0); glVertex3f(-24, 13.5, 0); glEnd(); glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    float seaY = -1.0f;
    if (g_cutsceneTime > 12.5f)
        seaY += (g_cutsceneTime - 12.5f) * (g_cutsceneTime - 12.5f) * 6.0f;

    const float step = 2.5f;
    const float range = 100.0f;
    const float uvScale = 0.08f; // 값이 작을수록 텍스처가 크게 보임

    float t = g_cutsceneTime;

    // 1패스 : Ocean 01
    if (g_oceanTex1) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_oceanTex1);
        glDisable(GL_BLEND);
        glColor4f(0.0f, 0.15f, 0.4f, 0.85f);

        float uOff1 = t * 0.05f;
        float vOff1 = t * 0.03f;

        for (float z = -range; z < range; z += step) {
            glBegin(GL_QUAD_STRIP);
            for (float x = -range; x <= range; x += step) {
                float u = (x * uvScale) + uOff1;
                float v = (z * uvScale) + vOff1;
                glTexCoord2f(u, v);
                glVertex3f(x, seaY + GetWaveHeight(x, z, t), z);

                float u2 = (x * uvScale) + uOff1;
                float v2 = ((z + step) * uvScale) + vOff1;
                glTexCoord2f(u2, v2);
                glVertex3f(x, seaY + GetWaveHeight(x, z + step, t), z + step);
            }
            glEnd();
        }
     glDisable(GL_FOG);
    }

    // 2패스 : Ocean 02 크로스페이드 오버레이
    if (g_oceanTex2) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_oceanTex2);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float alpha = 0.25f + 0.25f * (0.5f + 0.5f * sinf(t * 1.3f));
        glColor4f(1, 1, 1, alpha);

        float uOff2 = -t * 0.07f;
        float vOff2 = t * 0.04f;

        for (float z = -range; z < range; z += step) {
            glBegin(GL_QUAD_STRIP);
            for (float x = -range; x <= range; x += step) {
                float u = (x * uvScale) + uOff2;
                float v = (z * uvScale) + vOff2;
                glTexCoord2f(u, v);
                glVertex3f(x, seaY + GetWaveHeight(x, z, t), z);

                float u2 = (x * uvScale) + uOff2;
                float v2 = ((z + step) * uvScale) + vOff2;
                glTexCoord2f(u2, v2);
                glVertex3f(x, seaY + GetWaveHeight(x, z + step, t), z + step);
            }
            glEnd();
        }

        glDisable(GL_BLEND);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);



}

void InitFlowers() {}
void DrawFlowers() {}
void DrawClouds() {}

// ================== [NEW] 엔딩 크레딧 구현 ==================

struct FireParticle {
    float x, y;      // 위치
    float vx, vy;    // 속도
    float r, g, b;   // 색상
    float life;      // 수명 (1.0 -> 0.0)
    float size;      // 크기
};

std::vector<FireParticle> g_fireworks; // 폭죽 파티클 관리

// 텍스트 출력 도우미 함수
void DrawString(float x, float y, void* font, const char* string) {
    const char* c;
    glRasterPos2f(x, y);
    for (c = string; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}

// 텍스트 폭 계산
int GetTextWidth(void* font, const char* string) {
    int width = 0;
    for (const char* c = string; *c != '\0'; c++) {
        width += glutBitmapWidth(font, *c);
    }
    return width;
}

// [핵심] 폭죽 업데이트 및 그리기 함수
void UpdateAndDrawFireworks() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 빛이 겹치면 밝아지게

    // 1. 폭죽 생성 (랜덤 확률)
    // 프레임마다 일정 확률로 새로운 폭죽 발사
    if (rand() % 20 == 0) {
        float cx = (float)(rand() % winWidth);     // 랜덤 X 위치
        float cy = (float)(rand() % winHeight);    // 랜덤 Y 위치

        // 색상 랜덤 (빨강, 초록, 파랑, 노랑, 보라 등)
        float r = (rand() % 10) * 0.1f;
        float g = (rand() % 10) * 0.1f;
        float b = (rand() % 10) * 0.1f;

        // 하나의 폭죽 당 파티클 50개 생성
        for (int i = 0; i < 50; ++i) {
            FireParticle p;
            p.x = cx;
            p.y = cy;

            // 원형으로 퍼지도록 속도 계산
            float angle = (rand() % 360) * 3.141592f / 180.0f;
            float speed = (rand() % 50 + 50) * 0.1f; // 속도 랜덤

            p.vx = cos(angle) * speed;
            p.vy = sin(angle) * speed;

            p.r = r; p.g = g; p.b = b;
            p.life = 1.0f;
            p.size = (rand() % 3 + 2.0f); // 크기 2~4

            g_fireworks.push_back(p);
        }
    }

    // 2. 파티클 이동 및 그리기
    glPointSize(3.0f);
    glBegin(GL_POINTS);

    // iterator를 사용하여 순회하며 삭제 가능하게 함
    for (auto it = g_fireworks.begin(); it != g_fireworks.end(); ) {
        // 물리 적용
        it->x += it->vx;
        it->y += it->vy;
        it->vy -= 0.15f; // 중력 적용 (아래로 떨어짐)
        it->life -= 0.015f; // 수명 감소 (서서히 사라짐)

        // 그리기
        if (it->life > 0.0f) {
            glColor4f(it->r, it->g, it->b, it->life); // 투명도 적용
            glVertex2f(it->x, it->y);
            ++it;
        }
        else {
            // 수명 다한 파티클 삭제
            it = g_fireworks.erase(it);
        }
    }
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 블렌딩 모드 복구
}

void DrawEndingCredits() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // 배경: 검은색
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2D 모드 설정
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, winWidth, 0, winHeight);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_TEXTURE_2D);

    // [추가됨] 폭죽 터트리기 (글자 뒤에 보이도록 먼저 그림)
    UpdateAndDrawFireworks();

    // ----------------------------------------------------
    // 크레딧 텍스트 (기존 내용 유지)
    struct CreditLine {
        std::string text;
        void* font;
        float space;
        float r, g, b;
    };

    std::vector<CreditLine> lines = {
        { "Emotion.exe", GLUT_BITMAP_TIMES_ROMAN_24, 60, 1.0f, 1.0f, 0.0f },
        { "A 3D Maze Project", GLUT_BITMAP_HELVETICA_18, 100, 0.8f, 0.8f, 0.8f },

        { "DEVELOPED BY", GLUT_BITMAP_HELVETICA_18, 40, 0.5f, 0.5f, 1.0f },
        { "Team_REDBULL", GLUT_BITMAP_TIMES_ROMAN_24, 80, 1.0f, 1.0f, 1.0f },

        { "PROGRAMMING", GLUT_BITMAP_HELVETICA_18, 40, 0.5f, 0.5f, 1.0f },
        { "Maze Algorithm & OpenGL Engine", GLUT_BITMAP_HELVETICA_18, 30, 1.0f, 1.0f, 1.0f },
        { "Physics & Camera System", GLUT_BITMAP_HELVETICA_18, 80, 1.0f, 1.0f, 1.0f },

        { "ART & DESIGN", GLUT_BITMAP_HELVETICA_18, 40, 0.5f, 0.5f, 1.0f },
        { "3D Modeling (SOR)", GLUT_BITMAP_HELVETICA_18, 30, 1.0f, 1.0f, 1.0f },
        { "Texture & Environment", GLUT_BITMAP_HELVETICA_18, 80, 1.0f, 1.0f, 1.0f },

        { "Producer", GLUT_BITMAP_HELVETICA_18, 40, 0.5f, 0.5f, 1.0f },
        { "20225155_moon", GLUT_BITMAP_HELVETICA_18, 30, 1.0f, 1.0f, 1.0f },

        { "20224284_park", GLUT_BITMAP_HELVETICA_18, 30, 1.0f, 1.0f, 1.0f },

        { "20224195_sung", GLUT_BITMAP_HELVETICA_18, 30, 1.0f, 1.0f, 1.0f },

        { "Thank You For Playing", GLUT_BITMAP_TIMES_ROMAN_24, 0, 1.0f, 0.5f, 0.0f }
    };

    float scrollSpeed = 60.0f;
    float startY = -50.0f + (g_cutsceneTime * scrollSpeed);
    float currentY = startY;

    for (const auto& line : lines) {
        if (currentY > -50 && currentY < winHeight + 50) {
            int w = GetTextWidth(line.font, line.text.c_str());
            float x = (winWidth - w) / 2.0f;
            glColor3f(line.r, line.g, line.b);
            DrawString(x, currentY, line.font, line.text.c_str());
        }
        currentY -= line.space;
    }

    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}