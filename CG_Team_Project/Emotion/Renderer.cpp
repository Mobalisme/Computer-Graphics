#include "Global.h"
#include "SOR_Objects.h"
#include <iostream>
#include <string>
#include <cmath>

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
    // stage 0~3: Textures/wall0~3.png 우선
    std::string path = "Textures/wall" + std::to_string(stage) + ".png";
    GLuint newTex = LoadTextureFromFile(path.c_str());

    if (newTex == 0)
    {
        path = "Textures/wall" + std::to_string(stage) + ".jpg";
        newTex = LoadTextureFromFile(path.c_str());
    }

    // stage 0 기본 대체
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

    // (선택) 영상 프레임 로드: Video/1.jpg ...
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
    glTexCoord2f(0, 0);         glVertex3f(0, 0, 0);
    glTexCoord2f(MAZE_W, 0);    glVertex3f(w, 0, 0);
    glTexCoord2f(MAZE_W, MAZE_H); glVertex3f(w, 0, h);
    glTexCoord2f(0, MAZE_H);    glVertex3f(0, 0, h);
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

    // 벽
    glColor3f(1, 1, 1);
    for (int z = 0; z < MAZE_H; ++z)
        for (int x = 0; x < MAZE_W; ++x)
            if (maze[z][x] == WALL)
                glRectf(ox + x * cw, oy + z * ch, ox + (x + 1) * cw, oy + (z + 1) * ch);

    // 감정 오브젝트 마커(감정별 색)
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

    // 플레이어
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

// ---------------- 슬픔(밤/바다) ----------------
struct Star { float x, y, z; float b; };
static std::vector<Star> g_stars;

void InitStars()
{
    g_stars.clear();
    for (int i = 0; i < 200; ++i)
    {
        Star s;
        s.x = -50.0f + (rand() % 100);
        s.y = 10.0f + (rand() % 40);
        s.z = -50.0f + (rand() % 100);
        s.b = 0.4f + (rand() % 60) / 100.0f;
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
    glTranslatef(0, 6.0f, -8.0f);
    glScalef(8.0f, 4.5f, 1.0f);

    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-1, -1, 0);
    glTexCoord2f(1, 0); glVertex3f(1, -1, 0);
    glTexCoord2f(1, 1); glVertex3f(1, 1, 0);
    glTexCoord2f(0, 1); glVertex3f(-1, 1, 0);
    glEnd();

    glPopMatrix();
}

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
        glColor3f(s.b, s.b, s.b);
        glVertex3f(s.x, s.y, s.z);
    }
    glEnd();

    // 바다(단순)
    glColor3f(0.03f, 0.08f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(-80, 0, -80);
    glVertex3f(80, 0, -80);
    glVertex3f(80, 0, 80);
    glVertex3f(-80, 0, 80);
    glEnd();

    // (선택) 영상 패널
    DrawVideoIfAny();

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

    // 풀 바닥
    glColor3f(0.25f, 0.75f, 0.25f);
    glBegin(GL_QUADS);
    glVertex3f(-40, 0, -40);
    glVertex3f(40, 0, -40);
    glVertex3f(40, 0, 40);
    glVertex3f(-40, 0, 40);
    glEnd();

    // 꽃 점묘
    for (auto& f : g_flowers)
    {
        glPushMatrix();
        glTranslatef(f.x, 0.05f, f.z);
        glScalef(f.scale, f.scale, f.scale);
        glColor3f(f.r, f.g, f.b);
        glutSolidSphere(1.0, 8, 8);
        glPopMatrix();
    }

    // (선택) 영상 패널
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
