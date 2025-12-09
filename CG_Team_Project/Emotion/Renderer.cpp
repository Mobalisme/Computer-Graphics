#include "Global.h"
#include "SOR_Objects.h"

#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <random>
#include <ctime>
#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// --------------------------------------------------------
// Star
// --------------------------------------------------------
struct Star { float x, y, z; float brightness; };
std::vector<Star> g_stars;

// --------------------------------------------------------
// Shader Utils
// --------------------------------------------------------
static std::string LoadTextFile(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static GLuint CompileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, 1024, nullptr, log);
        std::cout << "[SHADER ERROR] " << log << "\n";
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint CreateProgramFromFiles(const char* vsPath, const char* fsPath) {
    std::string vs = LoadTextFile(vsPath);
    std::string fs = LoadTextFile(fsPath);
    if (vs.empty() || fs.empty()) {
        std::cout << "[SHADER ERROR] Cannot open shader files.\n";
        return 0;
    }

    GLuint v = CompileShader(GL_VERTEX_SHADER, vs.c_str());
    GLuint f = CompileShader(GL_FRAGMENT_SHADER, fs.c_str());
    if (!v || !f) return 0;

    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);

    glDeleteShader(v);
    glDeleteShader(f);

    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, 1024, nullptr, log);
        std::cout << "[PROGRAM ERROR] " << log << "\n";
        glDeleteProgram(p);
        return 0;
    }

    return p;
}

// --------------------------------------------------------
// Texture loader
// --------------------------------------------------------
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
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

// --------------------------------------------------------
// Confusion vertex distortion
// --------------------------------------------------------
void VertexConfusion(float x, float y, float z) {
    if (g_confusionTimer > 0.0f) {
        float distortion = sinf(y * 0.5f + g_cutsceneTime * 3.0f) * 0.2f;
        float wave = cosf(x * 0.2f + z * 0.2f + g_cutsceneTime * 5.0f) * 0.1f;
        x += distortion + wave;
        z += distortion - wave;
    }
    glVertex3f(x, y, z);
}

// --------------------------------------------------------
// Wall texture stage switch
// --------------------------------------------------------
void ChangeWallTexture(int stage) {
    std::string path = "Textures/wall" + std::to_string(stage) + ".png";
    GLuint newTex = LoadTextureFromFile(path.c_str());

    if (newTex == 0) {
        path = "Textures/wall" + std::to_string(stage) + ".jpg";
        newTex = LoadTextureFromFile(path.c_str());
    }

    if (newTex == 0 && stage == 0) {
        std::cout << "[SYSTEM] 'wall0' not found. Falling back to default 'Wall.png'\n";
        newTex = LoadTextureFromFile("Textures/Wall.png");
        if (newTex == 0) newTex = LoadTextureFromFile("Wall.png");
        if (newTex == 0) newTex = LoadTextureFromFile("wall.png");
    }

    if (newTex != 0) {
        if (g_wallTex != 0) glDeleteTextures(1, &g_wallTex);
        g_wallTex = newTex;
        std::cout << "[SYSTEM] Wall texture set to Stage " << stage << "\n";
    }
    else {
        std::cout << "[WARNING] Failed to load wall texture for Stage " << stage << "\n";
    }
}

// --------------------------------------------------------
// Lava shader init
// --------------------------------------------------------
static void InitLavaShader() {
    if (g_lavaProgram) return;

    g_lavaProgram = CreateProgramFromFiles("Shaders/lava.vert", "Shaders/lava.frag");
    if (g_lavaProgram) std::cout << "[SYSTEM] Lava shader ready.\n";
    else std::cout << "[WARNING] Lava shader failed.\n";
}

// --------------------------------------------------------
// LoadTextures
// --------------------------------------------------------
void LoadTextures() {
    ChangeWallTexture(0);

    // 1) 기본 바닥
    g_floorTex = LoadTextureFromFile("Textures/floor.jpeg");
    if (g_floorTex == 0) g_floorTex = LoadTextureFromFile("floor.jpeg");
    if (g_floorTex == 0) g_floorTex = LoadTextureFromFile("Textures/floor.jpg");
    if (g_floorTex == 0) g_floorTex = LoadTextureFromFile("floor.jpg");

    // 2) Ocean
    g_oceanTex1 = LoadTextureFromFile("Textures/Ocean 01.jpg");
    if (g_oceanTex1 == 0) g_oceanTex1 = LoadTextureFromFile("Textures/Ocean 01.png");
    if (g_oceanTex1 == 0) g_oceanTex1 = LoadTextureFromFile("Ocean 01.jpg");
    if (g_oceanTex1 == 0) g_oceanTex1 = LoadTextureFromFile("Ocean 01.png");

    g_oceanTex2 = LoadTextureFromFile("Textures/Ocean 02.jpg");
    if (g_oceanTex2 == 0) g_oceanTex2 = LoadTextureFromFile("Textures/Ocean 02.png");
    if (g_oceanTex2 == 0) g_oceanTex2 = LoadTextureFromFile("Ocean 02.jpg");
    if (g_oceanTex2 == 0) g_oceanTex2 = LoadTextureFromFile("Ocean 02.png");

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

    // 3) Lava 텍스처
    g_lavaTex1 = LoadTextureFromFile("Textures/Lava01.jpg");
    if (!g_lavaTex1) g_lavaTex1 = LoadTextureFromFile("Textures/Lava01.png");

    g_lavaTex2 = LoadTextureFromFile("Textures/Lava02.jpg");
    if (!g_lavaTex2) g_lavaTex2 = LoadTextureFromFile("Textures/Lava02.png");

    if (g_lavaTex1) {
        glBindTexture(GL_TEXTURE_2D, g_lavaTex1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    if (g_lavaTex2) {
        glBindTexture(GL_TEXTURE_2D, g_lavaTex2);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // 4) 비디오
    g_videoSad.clear();
    g_videoJoy.clear();

    for (int i = 1; i <= 80; ++i) {
        std::string path = "Video/Sad/" + std::to_string(i) + ".jpg";
        GLuint t = LoadTextureFromFile(path.c_str());
        if (t != 0) {
            glBindTexture(GL_TEXTURE_2D, t);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            g_videoSad.push_back(t);
        }
    }

    for (int i = 201; i <= 280; ++i) {
        std::string path = "Video/Joy/" + std::to_string(i) + ".jpg";
        GLuint t = LoadTextureFromFile(path.c_str());
        if (t != 0) {
            glBindTexture(GL_TEXTURE_2D, t);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            g_videoJoy.push_back(t);
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    // 5) Lava shader
    InitLavaShader();
}

// --------------------------------------------------------
// Wall block
// --------------------------------------------------------
void DrawWallBlock(float gx, float gz) {
    float x0 = gx * CELL_SIZE, x1 = (gx + 1) * CELL_SIZE;
    float z0 = gz * CELL_SIZE, z1 = (gz + 1) * CELL_SIZE;

    glBindTexture(GL_TEXTURE_2D, g_wallTex);

    if (g_confusionTimer > 0) glColor3f(0.8f, 1.0f, 0.8f);
    else glColor3f(1, 1, 1);

    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); VertexConfusion(x0, 0, z0);
    glTexCoord2f(1, 0); VertexConfusion(x1, 0, z0);
    glTexCoord2f(1, 1); VertexConfusion(x1, WALL_HEIGHT, z0);
    glTexCoord2f(0, 1); VertexConfusion(x0, WALL_HEIGHT, z0);

    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); VertexConfusion(x1, 0, z1);
    glTexCoord2f(1, 0); VertexConfusion(x0, 0, z1);
    glTexCoord2f(1, 1); VertexConfusion(x0, WALL_HEIGHT, z1);
    glTexCoord2f(0, 1); VertexConfusion(x1, WALL_HEIGHT, z1);

    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 0); VertexConfusion(x0, 0, z1);
    glTexCoord2f(1, 0); VertexConfusion(x0, 0, z0);
    glTexCoord2f(1, 1); VertexConfusion(x0, WALL_HEIGHT, z0);
    glTexCoord2f(0, 1); VertexConfusion(x0, WALL_HEIGHT, z1);

    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); VertexConfusion(x1, 0, z0);
    glTexCoord2f(1, 0); VertexConfusion(x1, 0, z1);
    glTexCoord2f(1, 1); VertexConfusion(x1, WALL_HEIGHT, z1);
    glTexCoord2f(0, 1); VertexConfusion(x1, WALL_HEIGHT, z0);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glColor3f(1, 1, 1);
}

// --------------------------------------------------------
// 기본 미로 바닥
// --------------------------------------------------------
void DrawFloor() {
    float w = MAZE_W * CELL_SIZE, h = MAZE_H * CELL_SIZE;
    glBindTexture(GL_TEXTURE_2D, g_floorTex);
    glColor3f(1, 1, 1);

    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0);            glVertex3f(0, 0, 0);
    glTexCoord2f(MAZE_W, 0);       glVertex3f(w, 0, 0);
    glTexCoord2f(MAZE_W, MAZE_H);  glVertex3f(w, 0, h);
    glTexCoord2f(0, MAZE_H);       glVertex3f(0, 0, h);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
}

// --------------------------------------------------------
// 분노 바닥(셰이더)
// --------------------------------------------------------
void DrawAngerFloorWithShader(float t) {
    if (!g_lavaProgram || !g_lavaTex1 || !g_lavaTex2) {
        DrawFloor();
        return;
    }

    glUseProgram(g_lavaProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_lavaTex1);
    glUniform1i(glGetUniformLocation(g_lavaProgram, "uLava1"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_lavaTex2);
    glUniform1i(glGetUniformLocation(g_lavaProgram, "uLava2"), 1);

    glUniform1f(glGetUniformLocation(g_lavaProgram, "uTime"), t);

    // 분노 씬 전용 바닥
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-100, 0, -100);
    glTexCoord2f(1, 0); glVertex3f(100, 0, -100);
    glTexCoord2f(1, 1); glVertex3f(100, 0, 100);
    glTexCoord2f(0, 1); glVertex3f(-100, 0, 100);
    glEnd();

    glUseProgram(0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// --------------------------------------------------------
// Maze 3D (플레이 상태)
// --------------------------------------------------------
void DrawMaze3D() {
    DrawFloor();
    for (int z = 0; z < MAZE_H; ++z) {
        for (int x = 0; x < MAZE_W; ++x) {
            if (maze[z][x] == WALL) DrawWallBlock((float)x, (float)z);
        }
    }
}

// --------------------------------------------------------
// Stars
// --------------------------------------------------------
void InitStars() {
    g_stars.clear();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> pos(-80.0f, 80.0f);
    std::uniform_real_distribution<float> h(10.0f, 60.0f);
    std::uniform_real_distribution<float> b(0.5f, 1.0f);

    for (int i = 0; i < 400; ++i)
        g_stars.push_back({ pos(gen), h(gen), pos(gen), b(gen) });
}

// --------------------------------------------------------
// Wave height
// --------------------------------------------------------
float GetWaveHeight(float x, float z, float t) {
    return sinf(x * 0.5f + t) * 0.5f
        + sinf(z * 0.3f + t * 0.8f) * 0.5f
        + sinf((x + z) * 0.2f + t * 1.5f) * 0.2f;
}

// --------------------------------------------------------
// DrawAngerScene (Main.cpp 분기용)
// --------------------------------------------------------
void DrawAngerScene() {
    glClearColor(0.05f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)winWidth / winHeight, 0.1, 300.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float dx = cos(camYaw) * cos(camPitch);
    float dy = sin(camPitch);
    float dz = sin(camYaw) * cos(camPitch);

    gluLookAt(camX, camY, camZ,
        camX + dx, camY + dy, camZ + dz,
        0, 1, 0);

    glDisable(GL_LIGHTING);
    DrawAngerFloorWithShader(g_cutsceneTime);
    glEnable(GL_LIGHTING);
}

// --------------------------------------------------------
// DrawMiniMap (네 기존 코드 유지)
// --------------------------------------------------------
void DrawMiniMap() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, winWidth, 0, winHeight);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_TEXTURE_2D); glDisable(GL_LIGHTING);

    float ms = 200.0f, ox = winWidth - ms - 10, oy = winHeight - ms - 10;
    float cw = ms / MAZE_W, ch = ms / MAZE_H;

    glColor4f(0, 0, 0, 0.6f);
    glRectf(ox - 2, oy - 2, ox + ms + 2, oy + ms + 2);

    glColor3f(1, 1, 1);
    for (int z = 0; z < MAZE_H; ++z) for (int x = 0; x < MAZE_W; ++x)
        if (maze[z][x] == WALL)
            glRectf(ox + x * cw, oy + z * ch, ox + (x + 1) * cw, oy + (z + 1) * ch);

    glColor3f(1, 1, 0);
    for (const auto& obj : g_worldObjects) {
        if (!obj.collected) {
            float cx = ox + (obj.mazeX + 0.5f) * cw;
            float cy = oy + (obj.mazeY + 0.5f) * ch;
            glRectf(cx - 2, cy - 2, cx + 2, cy + 2);
        }
    }

    glColor3f(1, 0, 0);
    float px = ox + (camX / (MAZE_W * CELL_SIZE)) * ms;
    float py = oy + (camZ / (MAZE_H * CELL_SIZE)) * ms;

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(px, py);
    for (int i = 0; i <= 16; ++i)
        glVertex2f(px + cos(i / 16.0f * 6.28f) * 4, py + sin(i / 16.0f * 6.28f) * 4);
    glEnd();

    glEnable(GL_DEPTH_TEST); glEnable(GL_TEXTURE_2D);
    if (g_gameState == STATE_PLAYING) glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// --------------------------------------------------------
// DrawSkyCylinder / DrawSunsetClouds / DrawJoyScene
// DrawNightScene / DrawEndingCredits
// 이 아래는 네 기존 코드 그대로 유지해서 붙이면 됨
// --------------------------------------------------------
