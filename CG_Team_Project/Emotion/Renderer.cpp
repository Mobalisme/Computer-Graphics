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

    if (g_floorTex) std::cout << "[SYSTEM] Floor texture loaded.\n";
    else std::cout << "[WARNING] Floor texture NOT found.\n";

    // 비디오 프레임 로드
    g_videoFrames.clear();
    std::cout << "[SYSTEM] Loading video frames...\n";
    for (int i = 1; i <= 120; ++i) {
        std::string path = "Video/" + std::to_string(i) + ".jpg";
        GLuint t = LoadTextureFromFile(path.c_str());

        // 비디오는 반복(REPEAT) 대신 늘이기(CLAMP)가 좋음
        if (t != 0) {
            glBindTexture(GL_TEXTURE_2D, t);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            g_videoFrames.push_back(t);
        }
    }
}

void DrawWallBlock(float gx, float gz) {
    glPushMatrix(); // 현재 위치 저장

    // g_confusionTimer는 Global.h에 선언되어 있어야 합니다.
    if (g_confusionTimer > 0) {
        float time = glutGet(GLUT_ELAPSED_TIME) * 0.005f;
        // 사인 함수(sin)를 이용해 크기를 키웠다 줄였다 반복
        float s = 1.0f + sin(time + gx * 0.5f) * 0.3f;

        // 벽의 중심을 기준으로 크기 조절 (안 그러면 벽이 밀려남)
        glTranslatef(gx * CELL_SIZE + CELL_SIZE / 2, 0, gz * CELL_SIZE + CELL_SIZE / 2);
        glScalef(s, 1.0f, s);
        glTranslatef(-(gx * CELL_SIZE + CELL_SIZE / 2), 0, -(gz * CELL_SIZE + CELL_SIZE / 2));
    }

    float x0 = gx * CELL_SIZE, x1 = (gx + 1) * CELL_SIZE;
    float z0 = gz * CELL_SIZE, z1 = (gz + 1) * CELL_SIZE;
    glBindTexture(GL_TEXTURE_2D, g_wallTex); glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);  glTexCoord2f(0, 0); glVertex3f(x0, 0, z0); glTexCoord2f(1, 0); glVertex3f(x1, 0, z0); glTexCoord2f(1, 1); glVertex3f(x1, WALL_HEIGHT, z0); glTexCoord2f(0, 1); glVertex3f(x0, WALL_HEIGHT, z0);
    glNormal3f(0, 0, -1); glTexCoord2f(0, 0); glVertex3f(x1, 0, z1); glTexCoord2f(1, 0); glVertex3f(x0, 0, z1); glTexCoord2f(1, 1); glVertex3f(x0, WALL_HEIGHT, z1); glTexCoord2f(0, 1); glVertex3f(x1, WALL_HEIGHT, z1);
    glNormal3f(-1, 0, 0); glTexCoord2f(0, 0); glVertex3f(x0, 0, z1); glTexCoord2f(1, 0); glVertex3f(x0, 0, z0); glTexCoord2f(1, 1); glVertex3f(x0, WALL_HEIGHT, z0); glTexCoord2f(0, 1); glVertex3f(x0, WALL_HEIGHT, z1);
    glNormal3f(1, 0, 0);  glTexCoord2f(0, 0); glVertex3f(x1, 0, z0); glTexCoord2f(1, 0); glVertex3f(x1, 0, z1); glTexCoord2f(1, 1); glVertex3f(x1, WALL_HEIGHT, z1); glTexCoord2f(0, 1); glVertex3f(x1, WALL_HEIGHT, z0);
    glEnd(); glBindTexture(GL_TEXTURE_2D, 0);

    glPopMatrix(); // ★ [추가된 부분] 위치 복구
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
            if (obj.type != 0) continue;
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
    if (frameIdx >= (int)g_videoFrames.size()) frameIdx = (int)g_videoFrames.size() - 1;
    if (frameIdx < 0) frameIdx = 0;
    if (!g_videoFrames.empty()) glBindTexture(GL_TEXTURE_2D, g_videoFrames[frameIdx]);
    else glBindTexture(GL_TEXTURE_2D, g_wallTex);

    glPushMatrix(); glTranslatef(0.0f, 25.0f, 45.0f);
    float flk = 0.95f + (rand() % 5) * 0.01f; glColor3f(flk, flk, flk);
    glBegin(GL_QUADS); glTexCoord2f(0, 1); glVertex3f(-24, -13.5, 0); glTexCoord2f(1, 1); glVertex3f(24, -13.5, 0); glTexCoord2f(1, 0); glVertex3f(24, 13.5, 0); glTexCoord2f(0, 0); glVertex3f(-24, 13.5, 0); glEnd(); glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    float seaY = -1.0f;
    if (g_cutsceneTime > 12.5f) seaY += (g_cutsceneTime - 12.5f) * (g_cutsceneTime - 12.5f) * 6.0f;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.15f, 0.4f, 0.85f);
    for (float z = -100; z < 100; z += 2.5f) {
        glBegin(GL_QUAD_STRIP);
        for (float x = -100; x <= 100; x += 2.5f) {
            glVertex3f(x, seaY + GetWaveHeight(x, z, g_cutsceneTime), z);
            glVertex3f(x, seaY + GetWaveHeight(x, z + 2.5f, g_cutsceneTime), z + 2.5f);
        }
        glEnd();
    }
    glDisable(GL_BLEND);

    if (seaY < 5.0f) {
        glPushMatrix();
        float bY = seaY + GetWaveHeight(0, 5.0f, g_cutsceneTime);
        glTranslatef(0, bY + 1.0f, 5.0f);
        glRotatef(cosf(g_cutsceneTime * 0.8f) * 4.0f, 1, 0, 0); glRotatef(sinf(g_cutsceneTime * 0.6f) * 2.0f, 0, 0, 1); glRotatef(180, 0, 1, 0);
        glScalef(2.5f, 2.5f, 2.5f);
        glColor3f(0.45f, 0.25f, 0.1f); glBegin(GL_TRIANGLES);
        glVertex3f(-1, 0.5, -1.5); glVertex3f(1, 0.5, -1.5); glVertex3f(0, 0, 1.5);
        glVertex3f(-1, 0.5, -1.5); glVertex3f(0, 0, 1.5); glVertex3f(0, 0, -1);
        glVertex3f(1, 0.5, -1.5); glVertex3f(0, 0, -1); glVertex3f(0, 0, 1.5);
        glColor3f(0.6f, 0.45f, 0.25f); glVertex3f(-1, 0.5, -1.5); glVertex3f(0, 0.5, 1.8); glVertex3f(1, 0.5, -1.5); glEnd();
        glColor3f(0.3f, 0.2f, 0.1f); glLineWidth(6.0f); glBegin(GL_LINES); glVertex3f(0, 0.5, 0); glVertex3f(0, 3.5, 0); glEnd();
        glColor4f(0.95f, 0.95f, 0.95f, 0.9f); glEnable(GL_BLEND); glBegin(GL_TRIANGLES); glVertex3f(0, 3.5, 0); glVertex3f(0, 1.0, 0); glVertex3f(1.5, 1.2, 0.3); glEnd(); glDisable(GL_BLEND);
        glPopMatrix();
    }
}