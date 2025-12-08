#include "Global.h"
#include "SOR_Objects.h"

#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ---------------- 텍스처 로더 ----------------
static GLuint LoadTextureFromFile(const char* filename)
{
    int w, h, c;
    unsigned char* d = stbi_load(filename, &w, &h, &c, 0);
    if (!d) return 0;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    GLenum f = (c == 4) ? GL_RGBA : (c == 3 ? GL_RGB : GL_RED);
    glTexImage2D(GL_TEXTURE_2D, 0, f, w, h, 0, f, GL_UNSIGNED_BYTE, d);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(d);
    return tex;
}

void ChangeWallTexture(int stage)
{
    std::string path = "Textures/wall" + std::to_string(stage) + ".png";
    GLuint newTex = LoadTextureFromFile(path.c_str());

    if (newTex == 0)
    {
        path = "Textures/wall" + std::to_string(stage) + ".jpg";
        newTex = LoadTextureFromFile(path.c_str());
    }

    if (newTex == 0 && stage == 0)
    {
        newTex = LoadTextureFromFile("Textures/Wall.png");
        if (newTex == 0) newTex = LoadTextureFromFile("Wall.png");
        if (newTex == 0) newTex = LoadTextureFromFile("wall.png");
    }

    if (newTex != 0)
    {
        if (g_wallTex != 0) glDeleteTextures(1, &g_wallTex);
        g_wallTex = newTex;
        std::cout << "[SYSTEM] Wall texture stage " << stage << " applied.\n";
    }
    else
    {
        std::cout << "[WARNING] Wall texture for stage " << stage << " not found.\n";
    }
}

void LoadTextures()
{
    ChangeWallTexture(0);

    g_floorTex = LoadTextureFromFile("Textures/floor.png");
    if (g_floorTex == 0) g_floorTex = LoadTextureFromFile("Textures/floor.jpg");
    if (g_floorTex == 0) g_floorTex = LoadTextureFromFile("Textures/floor.jpeg");
    if (g_floorTex == 0) g_floorTex = LoadTextureFromFile("floor.jpg");
    if (g_floorTex == 0) g_floorTex = LoadTextureFromFile("floor.jpeg");

    // ★ 추가: 슬픔 맵 Ocean 텍스처 로드
    // Windows에서 확장자 숨김일 수 있으니 png/jpg/jpeg 순서로 시도
    g_sadWaterTex = LoadTextureFromFile("Textures/Ocean.png");
    if (g_sadWaterTex == 0) g_sadWaterTex = LoadTextureFromFile("Textures/Ocean.jpg");
    if (g_sadWaterTex == 0) g_sadWaterTex = LoadTextureFromFile("Textures/Ocean.jpeg");
    // 혹시 소문자/루트 경로 대비
    if (g_sadWaterTex == 0) g_sadWaterTex = LoadTextureFromFile("Textures/ocean.png");
    if (g_sadWaterTex == 0) g_sadWaterTex = LoadTextureFromFile("Textures/ocean.jpg");
    if (g_sadWaterTex == 0) g_sadWaterTex = LoadTextureFromFile("Textures/ocean.jpeg");

    if (g_sadWaterTex != 0)
    {
        glBindTexture(GL_TEXTURE_2D, g_sadWaterTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    else
    {
        std::cout << "[WARNING] Sad Ocean texture not found.\n";
    }

    // 영상 프레임 (Video/1.jpg ...)
    g_videoFrames.clear();
    for (int i = 1; i <= 120; ++i)
    {
        std::string vpath = "Video/" + std::to_string(i) + ".jpg";
        GLuint t = LoadTextureFromFile(vpath.c_str());
        if (t != 0)
        {
            glBindTexture(GL_TEXTURE_2D, t);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            g_videoFrames.push_back(t);
        }
    }
}

// ---------------- 미로 렌더 ----------------
static void DrawWallBlock(float gx, float gz)
{
    float x0 = gx * CELL_SIZE, x1 = (gx + 1) * CELL_SIZE;
    float z0 = gz * CELL_SIZE, z1 = (gz + 1) * CELL_SIZE;

    glBindTexture(GL_TEXTURE_2D, g_wallTex);
    glColor3f(1, 1, 1);

    glBegin(GL_QUADS);

    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(x0, 0, z0);
    glTexCoord2f(1, 0); glVertex3f(x1, 0, z0);
    glTexCoord2f(1, 1); glVertex3f(x1, WALL_HEIGHT, z0);
    glTexCoord2f(0, 1); glVertex3f(x0, WALL_HEIGHT, z0);

    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); glVertex3f(x1, 0, z1);
    glTexCoord2f(1, 0); glVertex3f(x0, 0, z1);
    glTexCoord2f(1, 1); glVertex3f(x0, WALL_HEIGHT, z1);
    glTexCoord2f(0, 1); glVertex3f(x1, WALL_HEIGHT, z1);

    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(x0, 0, z1);
    glTexCoord2f(1, 0); glVertex3f(x0, 0, z0);
    glTexCoord2f(1, 1); glVertex3f(x0, WALL_HEIGHT, z0);
    glTexCoord2f(0, 1); glVertex3f(x0, WALL_HEIGHT, z1);

    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(x1, 0, z0);
    glTexCoord2f(1, 0); glVertex3f(x1, 0, z1);
    glTexCoord2f(1, 1); glVertex3f(x1, WALL_HEIGHT, z1);
    glTexCoord2f(0, 1); glVertex3f(x1, WALL_HEIGHT, z0);

    glEnd();
}

static void DrawFloor()
{
    float w = MAZE_W * CELL_SIZE;
    float h = MAZE_H * CELL_SIZE;

    glBindTexture(GL_TEXTURE_2D, g_floorTex);
    glColor3f(1, 1, 1);

    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0);            glVertex3f(0, 0, 0);
    glTexCoord2f(MAZE_W, 0);       glVertex3f(w, 0, 0);
    glTexCoord2f(MAZE_W, MAZE_H);  glVertex3f(w, 0, h);
    glTexCoord2f(0, MAZE_H);       glVertex3f(0, 0, h);
    glEnd();
}

void DrawMaze3D()
{
    DrawFloor();
    for (int z = 0; z < MAZE_H; ++z)
        for (int x = 0; x < MAZE_W; ++x)
            if (maze[z][x] == WALL)
                DrawWallBlock((float)x, (float)z);
}

// ---------------- 미니맵 ----------------
void DrawMiniMap()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winWidth, 0, winHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    float ms = 200.0f;
    float ox = winWidth - ms - 10.0f;
    float oy = winHeight - ms - 10.0f;
    float cw = ms / MAZE_W;
    float ch = ms / MAZE_H;

    glColor4f(0, 0, 0, 0.6f);
    glRectf(ox - 2, oy - 2, ox + ms + 2, oy + ms + 2);

    glColor3f(1, 1, 1);
    for (int z = 0; z < MAZE_H; ++z)
        for (int x = 0; x < MAZE_W; ++x)
            if (maze[z][x] == WALL)
                glRectf(ox + x * cw, oy + z * ch, ox + (x + 1) * cw, oy + (z + 1) * ch);

    for (const auto& obj : g_worldObjects)
    {
        if (obj.collected) continue;

        if (obj.emotionId == EMO_SADNESS) glColor3f(0.4f, 0.7f, 1.0f);
        else if (obj.emotionId == EMO_ANGER) glColor3f(1.0f, 0.3f, 0.2f);
        else if (obj.emotionId == EMO_JOY) glColor3f(1.0f, 0.9f, 0.2f);
        else glColor3f(1.0f, 1.0f, 1.0f);

        float cx = ox + (obj.mazeX + 0.5f) * cw;
        float cy = oy + (obj.mazeY + 0.5f) * ch;
        glRectf(cx - 3, cy - 3, cx + 3, cy + 3);
    }

    glColor3f(1, 0, 0);
    float px = ox + (camX / (MAZE_W * CELL_SIZE)) * ms;
    float py = oy + (camZ / (MAZE_H * CELL_SIZE)) * ms;

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(px, py);
    for (int i = 0; i <= 16; ++i)
    {
        float a = i / 16.0f * 6.28318f;
        glVertex2f(px + cosf(a) * 4.0f, py + sinf(a) * 4.0f);
    }
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    if (g_gameState == STATE_PLAYING) glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ---------------- 슬픔(밤/별/수면/오로라) ----------------
struct Star { float x, y, z; float b; };
static std::vector<Star> g_stars;

void InitStars()
{
    g_stars.clear();
    for (int i = 0; i < 220; ++i)
    {
        Star s;
        s.x = -60.0f + (rand() % 120);
        s.y = 12.0f + (rand() % 45);
        s.z = -60.0f + (rand() % 120);
        s.b = 0.35f + (rand() % 65) / 100.0f;
        g_stars.push_back(s);
    }
}

static void DrawVideoIfAny()
{
    if (g_videoFrames.empty()) return;

    int idx = (int)(g_cutsceneTime * VIDEO_FPS) % (int)g_videoFrames.size();
    GLuint tex = g_videoFrames[idx];

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);

    glPushMatrix();
    glTranslatef(0.0f, 7.5f, -18.0f);
    glRotatef(5.0f, 1, 0, 0);
    glScalef(10.0f, 5.6f, 1.0f);

    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-1, -1, 0);
    glTexCoord2f(1, 0); glVertex3f(1, -1, 0);
    glTexCoord2f(1, 1); glVertex3f(1, 1, 0);
    glTexCoord2f(0, 1); glVertex3f(-1, 1, 0);
    glEnd();

    glPopMatrix();
}

// ★ 잔잔한 물결 + 은은한 별/달빛 반짝임 + Ocean 텍스처 적용
static void DrawSadShallowWater()
{
    const float startX = -25.0f;
    const float startZ = -8.0f;
    const float sizeX = 50.0f;
    const float sizeZ = 60.0f;

    const float waterBaseY = 0.02f;

    // 너무 넓게 일렁이지 않도록 해상도/패턴 완만화
    const int NX = 55;
    const int NZ = 55;

    float t = g_cutsceneTime;

    const bool useTex = (g_sadWaterTex != 0);

    if (useTex)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_sadWaterTex);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
    }

    glDisable(GL_LIGHTING);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 텍스처 타일링 스케일(원하면 1.0~4.0 사이로 조절)
    const float TILE_U = 2.0f;
    const float TILE_V = 2.0f;

    for (int iz = 0; iz < NZ; ++iz)
    {
        float z0 = startZ + sizeZ * (float)iz / (float)NZ;
        float z1 = startZ + sizeZ * (float)(iz + 1) / (float)NZ;

        float depth0 = (z0 - startZ) / sizeZ;
        float depth1 = (z1 - startZ) / sizeZ;

        float v0 = depth0 * TILE_V;
        float v1 = depth1 * TILE_V;

        // ---- 잔잔한 파동(속도/진폭 축소) ----
        auto waveY = [&](float xx, float zz)
            {
                float w1 = sinf(xx * 0.22f + t * 0.35f) * 0.010f;
                float w2 = sinf(zz * 0.20f + t * 0.30f) * 0.008f;
                float w3 = sinf((xx + zz) * 0.14f + t * 0.45f) * 0.006f;
                return w1 + w2 + w3;
            };

        // ---- 은은한 반짝임(과하지 않게) ----
        auto shimmer = [&](float xx, float zz, float depth)
            {
                float s = 0.5f + 0.5f * sinf(xx * 0.55f - zz * 0.45f + t * 0.8f);
                s = s * s;
                return 0.05f * s * (1.0f - depth * 0.8f);
            };

        auto baseColor = [&](float depth, float add)
            {
                float nr = 0.06f, ng = 0.14f, nb = 0.22f;
                float fr = 0.02f, fg = 0.06f, fb = 0.12f;

                float r = nr * (1.0f - depth) + fr * depth;
                float g = ng * (1.0f - depth) + fg * depth;
                float b = nb * (1.0f - depth) + fb * depth;

                r += add; g += add * 1.05f; b += add * 1.15f;

                if (r > 1) r = 1; if (g > 1) g = 1; if (b > 1) b = 1;

                // 텍스처와 곱해져도 너무 진해지지 않게 알파 유지
                glColor4f(r, g, b, 0.93f);
            };

        glBegin(GL_TRIANGLE_STRIP);

        for (int ix = 0; ix <= NX; ++ix)
        {
            float x = startX + sizeX * (float)ix / (float)NX;
            float depthU = (x - startX) / sizeX;
            float u = depthU * TILE_U;

            {
                float wy = waveY(x, z0);
                float add = shimmer(x, z0, depth0);
                baseColor(depth0, add);
                if (useTex) glTexCoord2f(u, v0);
                glVertex3f(x, waterBaseY + wy, z0);
            }
            {
                float wy = waveY(x, z1);
                float add = shimmer(x, z1, depth1);
                baseColor(depth1, add);
                if (useTex) glTexCoord2f(u, v1);
                glVertex3f(x, waterBaseY + wy, z1);
            }
        }

        glEnd();
    }

    glDisable(GL_BLEND);

    // 먼 바다 확장(어두운 톤) - 텍스처 없이 단순 톤 유지
    if (useTex) glDisable(GL_TEXTURE_2D);

    glColor3f(0.01f, 0.03f, 0.08f);
    glBegin(GL_QUADS);
    glVertex3f(-80, waterBaseY, 40);
    glVertex3f(80, waterBaseY, 40);
    glVertex3f(80, waterBaseY, 120);
    glVertex3f(-80, waterBaseY, 120);
    glEnd();

    if (g_gameState == STATE_PLAYING)
        glEnable(GL_LIGHTING);
}

// ★ 오로라 리본
static void DrawAurora()
{
    float t = g_cutsceneTime;

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 은은한 발광

    const int STEPS = 80;
    const float X0 = -45.0f;
    const float X1 = 45.0f;

    glBegin(GL_QUAD_STRIP);

    for (int i = 0; i <= STEPS; ++i)
    {
        float u = (float)i / (float)STEPS;
        float x = X0 + (X1 - X0) * u;

        float yTop = 24.0f + sinf(x * 0.08f + t * 0.15f) * 1.8f;
        float yBot = yTop - (4.0f + sinf(x * 0.05f + t * 0.10f) * 0.8f);

        float z = -35.0f + cosf(x * 0.06f + t * 0.08f) * 2.5f;

        float phase = 0.5f + 0.5f * sinf(x * 0.10f + t * 0.12f);

        // 녹청 ~ 보라 계열 혼합
        float r = 0.06f + 0.10f * (1.0f - phase);
        float g = 0.35f + 0.45f * phase;
        float b = 0.18f + 0.25f * (1.0f - phase);

        float aTop = 0.22f;
        float aBot = 0.04f;

        glColor4f(r, g, b, aTop);
        glVertex3f(x, yTop, z);

        glColor4f(r, g, b, aBot);
        glVertex3f(x, yBot, z);
    }

    glEnd();

    glDisable(GL_BLEND);
}

// ---------------- 슬픔 씬 메인 ----------------
void DrawNightScene()
{
    glClearColor(0.02f, 0.03f, 0.08f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)winWidth / winHeight, 0.1, 300.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float dx = cosf(camYaw) * cosf(camPitch);
    float dy = sinf(camPitch);
    float dz = sinf(camYaw) * cosf(camPitch);

    gluLookAt(camX, camY, camZ,
        camX + dx, camY + dy, camZ + dz,
        0, 1, 0);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    // 별
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (auto& s : g_stars)
    {
        // 과한 깜빡임 방지: 아주 잔잔한 트윙클
        float tw = 0.9f + 0.1f * sinf(g_cutsceneTime * 1.6f + s.x * 0.08f);
        float b = s.b * tw;
        glColor3f(b, b, b);
        glVertex3f(s.x, s.y, s.z);
    }
    glEnd();

    // 오로라(하늘에 먼저)
    DrawAurora();

    // 수평선 안개
    glEnable(GL_FOG);
    {
        GLfloat fogColor[4] = { 0.02f, 0.05f, 0.10f, 1.0f };
        glFogfv(GL_FOG_COLOR, fogColor);

        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, 14.0f);
        glFogf(GL_FOG_END, 75.0f);

        glHint(GL_FOG_HINT, GL_NICEST);
    }

    // 잔잔한 수면
    DrawSadShallowWater();

    // 영상 패널
    DrawVideoIfAny();

    glDisable(GL_FOG);

    glEnable(GL_LIGHTING);
}

// ---------------- 기쁨(초원) ----------------
void InitFlowers()
{
    g_flowers.clear();
    for (int i = 0; i < 120; ++i)
    {
        Flower f;
        f.x = -12.0f + (rand() % 240) / 10.0f;
        f.z = -12.0f + (rand() % 240) / 10.0f;
        f.y = 0.0f;
        f.r = 0.6f + (rand() % 40) / 100.0f;
        f.g = 0.6f + (rand() % 40) / 100.0f;
        f.b = 0.6f + (rand() % 40) / 100.0f;
        f.scale = 0.15f + (rand() % 20) / 100.0f;
        g_flowers.push_back(f);
    }
}

void DrawJoyScene()
{
    glClearColor(0.75f, 0.90f, 1.0f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)winWidth / winHeight, 0.1, 300.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float dx = cosf(camYaw) * cosf(camPitch);
    float dy = sinf(camPitch);
    float dz = sinf(camYaw) * cosf(camPitch);

    gluLookAt(camX, camY, camZ,
        camX + dx, camY + dy, camZ + dz,
        0, 1, 0);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    glColor3f(0.25f, 0.75f, 0.25f);
    glBegin(GL_QUADS);
    glVertex3f(-40, 0, -40);
    glVertex3f(40, 0, -40);
    glVertex3f(40, 0, 40);
    glVertex3f(-40, 0, 40);
    glEnd();

    for (auto& f : g_flowers)
    {
        glPushMatrix();
        glTranslatef(f.x, 0.05f, f.z);
        glScalef(f.scale, f.scale, f.scale);
        glColor3f(f.r, f.g, f.b);
        glutSolidSphere(1.0, 8, 8);
        glPopMatrix();
    }

    DrawVideoIfAny();

    glEnable(GL_LIGHTING);
}

// ---------------- 엔딩 크레딧 ----------------
static void DrawString(float x, float y, const char* s)
{
    glRasterPos2f(x, y);
    for (const char* p = s; *p; ++p)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
}

void DrawEndingCredits()
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winWidth, 0, winHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    float y = -100.0f + g_cutsceneTime * 40.0f;

    glColor3f(1, 1, 1);
    DrawString(80, y + 0, "Emotion.exe");
    DrawString(80, y + 40, "AI Robot Emotion Maze");
    DrawString(80, y + 100, "Team:");
    DrawString(80, y + 140, "- Member A");
    DrawString(80, y + 180, "- Member B");
    DrawString(80, y + 220, "- Member C");
    DrawString(80, y + 300, "Thanks for playing");

    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
