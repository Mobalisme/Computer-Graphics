// Emotion.cpp

#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <queue>
#include <cmath>

#include <GL/freeglut.h>
#include <GL/glu.h>

// stb_image: 이 cpp 한 군데에서만 IMPLEMENTATION 정의
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ================== 미로 설정 ==================

const int MAZE_W = 25;   // 반드시 홀수
const int MAZE_H = 25;   // 반드시 홀수

const int CELL_W = (MAZE_W - 1) / 2; // 논리적 셀 개수 (가로)
const int CELL_H = (MAZE_H - 1) / 2; // 논리적 셀 개수 (세로)

const int WALL = 1;
const int PATH = 0;

int  maze[MAZE_H][MAZE_W];
bool visitedCell[CELL_H][CELL_W];

std::mt19937 rng;

// ================== 카메라 / 윈도우 설정 ==================

int winWidth = 800;
int winHeight = 800;

// 플레이어 위치 (1,1) 셀 중앙 기준
float cellSize = 1.0f;
float wallHeight = 1.6f;

float camX = 1.5f;   // x = (1 + 0.5) * cellSize
float camY = 0.8f;   // 눈 높이
float camZ = 1.5f;

float camYaw = 0.0f;   // 좌우 회전 (라디안)
float camPitch = 0.0f;   // 상하 회전 (라디안)

const float MOVE_SPEED = 2.0f;     // m/s
const float TURN_SPEED_MOUSE = 0.0025f;  // 마우스 감도

bool keyDown[256] = { false };

// 마우스 래핑용
bool warpInProgress = false;

// ================== 텍스처 ==================

GLuint g_wallTex = 0;

// ================== 감정 구슬 ==================

struct Orb {
    float x, y, z;
    float radius;
    int   emotionId; // 0:기쁨, 1:슬픔, 2:버럭, 3:까칠, 4:소심
    bool  collected;
};

std::vector<Orb> g_orbs;

// 입구 / 출구 셀 (PATH 셀의 좌표)
const int START_X = 1;
const int START_Z = 1;
const int EXIT_X = MAZE_W - 2;
const int EXIT_Z = MAZE_H - 2;

// ---------- 랜덤 / 초기화 ----------

void InitRandom()
{
    std::random_device rd;
    rng.seed(rd());   // 매번 다른 미로
    // rng.seed(1234); // 같은 미로를 보고 싶으면 이렇게 고정
}

int RandInt(int a, int b)
{
    std::uniform_int_distribution<int> dist(a, b);
    return dist(rng);
}

void InitMazeData()
{
    // 전부 벽으로 채우기
    for (int y = 0; y < MAZE_H; ++y)
    {
        for (int x = 0; x < MAZE_W; ++x)
        {
            maze[y][x] = WALL;
        }
    }

    // 셀 방문 배열 초기화
    for (int cy = 0; cy < CELL_H; ++cy)
    {
        for (int cx = 0; cx < CELL_W; ++cx)
        {
            visitedCell[cy][cx] = false;
        }
    }
}

// 논리 셀 → 실제 좌표 PATH 열기
void OpenCell(int cx, int cy)
{
    int x = 2 * cx + 1; // 홀수
    int y = 2 * cy + 1; // 홀수
    maze[y][x] = PATH;
}

// ---------- DFS 미로 생성 ----------

struct Dir { int dx, dy; };

const Dir dirs4[4] = {
    { 0, -1 }, // 위
    { 0,  1 }, // 아래
    { -1, 0 }, // 왼
    { 1,  0 }  // 오른
};

void ShuffleDirs(std::vector<int>& order)
{
    std::shuffle(order.begin(), order.end(), rng);
}

void GenerateMazeDFS(int cx, int cy)
{
    visitedCell[cy][cx] = true;
    OpenCell(cx, cy);

    std::vector<int> order = { 0, 1, 2, 3 };
    ShuffleDirs(order);

    for (int i = 0; i < 4; ++i)
    {
        int d = order[i];
        int ncx = cx + dirs4[d].dx;
        int ncy = cy + dirs4[d].dy;

        if (ncx < 0 || ncx >= CELL_W || ncy < 0 || ncy >= CELL_H)
            continue;

        if (!visitedCell[ncy][ncx])
        {
            int x1 = 2 * cx + 1;
            int y1 = 2 * cy + 1;
            int x2 = 2 * ncx + 1;
            int y2 = 2 * ncy + 1;

            int wx = (x1 + x2) / 2;
            int wy = (y1 + y2) / 2;

            maze[wy][wx] = PATH;

            GenerateMazeDFS(ncx, ncy);
        }
    }
}

// ---------- 입구 / 출구 ----------

void AddEntranceExit()
{
    // 입구: 왼쪽 벽 (시작 셀 (0,0)의 왼쪽)
    maze[1][0] = PATH;                        // (x=0, y=1)

    // 출구: 오른쪽 벽 (마지막 셀 오른쪽)
    maze[MAZE_H - 2][MAZE_W - 1] = PATH;      // (x=MAZE_W-1, y=MAZE_H-2)
}

void GenerateMaze()
{
    InitRandom();
    InitMazeData();

    int startCx = 0;
    int startCy = 0;
    GenerateMazeDFS(startCx, startCy);

    AddEntranceExit();
}

// ================== 텍스처 로드 ==================

// 작업 폴더가 어디냐에 따라 Textures/wall.png, ../Textures/wall.png 둘 다 시도
bool LoadWallTexture()
{
    const char* candidates[] = {
        "Textures/wall.png",     // 작업 폴더가 프로젝트 루트일 때
        "../Textures/wall.png",  // 작업 폴더가 Debug/ 또는 Release/일 때
        "wall.png"               // exe 옆에 둘 경우
    };

    int w = 0, h = 0, channels = 0;
    unsigned char* data = nullptr;
    const char* usedPath = nullptr;

    for (int i = 0; i < 3; ++i)
    {
        data = stbi_load(candidates[i], &w, &h, &channels, 0);
        if (data)
        {
            usedPath = candidates[i];
            break;
        }
    }

    if (!data)
    {
        std::cerr << "Failed to load wall texture. Tried:\n"
            << "  Textures/wall.png\n"
            << "  ../Textures/wall.png\n"
            << "  wall.png\n";
        return false;
    }

    GLenum format;
    if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;
    else
    {
        stbi_image_free(data);
        std::cerr << "Unsupported channel count in wall texture\n";
        return false;
    }

    glGenTextures(1, &g_wallTex);
    glBindTexture(GL_TEXTURE_2D, g_wallTex);

    glTexImage2D(
        GL_TEXTURE_2D, 0, format,
        w, h, 0, format, GL_UNSIGNED_BYTE, data
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    std::cout << "Loaded wall texture from: " << usedPath
        << " (" << w << "x" << h << ", ch=" << channels << ")\n";
    return true;
}

// ================== 유틸 / 충돌 ==================

bool IsWallCell(int gx, int gz)
{
    if (gx < 0 || gx >= MAZE_W || gz < 0 || gz >= MAZE_H)
        return true; // 바깥은 벽 취급
    return (maze[gz][gx] == WALL);
}

bool IsBlocked(float x, float z)
{
    float radius = 0.2f;

    int left = int((x - radius) / cellSize);
    int right = int((x + radius) / cellSize);
    int top = int((z - radius) / cellSize);
    int bottom = int((z + radius) / cellSize);

    if (IsWallCell(left, top) ||
        IsWallCell(right, top) ||
        IsWallCell(left, bottom) ||
        IsWallCell(right, bottom))
        return true;

    return false;
}

// ================== 입구→출구 경로(BFS) & 구슬 배치 ==================

struct Node { int x, z; };

std::vector<Node> ComputePathEntranceToExit()
{
    const int INF = 1e9;
    static int  dist[MAZE_H][MAZE_W];
    static Node prevNode[MAZE_H][MAZE_W];

    for (int z = 0; z < MAZE_H; ++z)
    {
        for (int x = 0; x < MAZE_W; ++x)
        {
            dist[z][x] = INF;
            prevNode[z][x] = { -1, -1 };
        }
    }

    std::queue<Node> q;
    q.push({ START_X, START_Z });
    dist[START_Z][START_X] = 0;

    const int dx[4] = { 1, -1, 0, 0 };
    const int dz[4] = { 0, 0, 1, -1 };

    while (!q.empty())
    {
        Node cur = q.front(); q.pop();
        if (cur.x == EXIT_X && cur.z == EXIT_Z) break;

        for (int dir = 0; dir < 4; ++dir)
        {
            int nx = cur.x + dx[dir];
            int nz = cur.z + dz[dir];

            if (nx < 0 || nx >= MAZE_W || nz < 0 || nz >= MAZE_H) continue;
            if (maze[nz][nx] == WALL) continue; // 벽이면 못감

            if (dist[nz][nx] == INF)
            {
                dist[nz][nx] = dist[cur.z][cur.x] + 1;
                prevNode[nz][nx] = cur;
                q.push({ nx, nz });
            }
        }
    }

    std::vector<Node> path;
    Node cur = { EXIT_X, EXIT_Z };
    if (dist[cur.z][cur.x] == INF)
    {
        return path;
    }

    while (!(cur.x == START_X && cur.z == START_Z))
    {
        path.push_back(cur);
        cur = prevNode[cur.z][cur.x];
    }
    path.push_back({ START_X, START_Z });

    std::reverse(path.begin(), path.end()); // 입구 -> 출구 순서
    return path;
}

void PlaceEmotionOrbs()
{
    g_orbs.clear();

    std::vector<Node> path = ComputePathEntranceToExit();
    if (path.size() < 7) return; // 너무 짧으면 안 놓음

    const int numOrbs = 5; // Joy, Sadness, Anger, Disgust, Fear

    for (int i = 0; i < numOrbs; ++i)
    {
        float t = (i + 1.0f) / (numOrbs + 1.0f);  // 경로 6등분, 가운데 5지점
        int idx = int(t * float(path.size() - 1));

        int gx = path[idx].x;
        int gz = path[idx].z;

        float worldX = (gx + 0.5f) * cellSize;
        float worldZ = (gz + 0.5f) * cellSize;
        float worldY = 0.8f;       // 눈높이 정도

        Orb orb;
        orb.x = worldX;
        orb.y = worldY;
        orb.z = worldZ;
        orb.radius = 0.3f;
        orb.emotionId = i;   // 0~4 : Joy, Sadness, Anger, Disgust, Fear
        orb.collected = false;

        g_orbs.push_back(orb);
    }
}

// ================== 색 / 렌더링 ==================

void GetEmotionColor(int emotionId, float& r, float& g, float& b)
{
    switch (emotionId)
    {
    case 0: // Joy
        r = 1.0f; g = 0.95f; b = 0.2f;  break;  // 노란빛
    case 1: // Sadness
        r = 0.3f; g = 0.5f;  b = 1.0f;  break;  // 파란빛
    case 2: // Anger
        r = 1.0f; g = 0.2f;  b = 0.1f;  break;  // 빨간빛
    case 3: // Disgust
        r = 0.2f; g = 0.9f;  b = 0.3f;  break;  // 초록빛
    case 4: // Fear
        r = 0.8f; g = 0.5f;  b = 1.0f;  break;  // 보라빛
    default:
        r = g = b = 1.0f;               break;
    }
}

void SetEmotionColor(int emotionId)
{
    float r, g, b;
    GetEmotionColor(emotionId, r, g, b);
    glColor3f(r, g, b);
}

void DrawWallBlock(float gx, float gz)
{
    float x0 = gx * cellSize;
    float x1 = (gx + 1) * cellSize;
    float z0 = gz * cellSize;
    float z1 = (gz + 1) * cellSize;

    float y0 = 0.0f;
    float y1 = wallHeight;

    float repeat = 1.0f;

    glBindTexture(GL_TEXTURE_2D, g_wallTex);
    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);

    // 앞면 (z = z0)
    glTexCoord2f(0.0f, 0.0f);      glVertex3f(x0, y0, z0);
    glTexCoord2f(repeat, 0.0f);    glVertex3f(x1, y0, z0);
    glTexCoord2f(repeat, repeat);  glVertex3f(x1, y1, z0);
    glTexCoord2f(0.0f, repeat);    glVertex3f(x0, y1, z0);

    // 뒷면 (z = z1)
    glTexCoord2f(0.0f, 0.0f);      glVertex3f(x1, y0, z1);
    glTexCoord2f(repeat, 0.0f);    glVertex3f(x0, y0, z1);
    glTexCoord2f(repeat, repeat);  glVertex3f(x0, y1, z1);
    glTexCoord2f(0.0f, repeat);    glVertex3f(x1, y1, z1);

    // 왼쪽 면 (x = x0)
    glTexCoord2f(0.0f, 0.0f);      glVertex3f(x0, y0, z1);
    glTexCoord2f(repeat, 0.0f);    glVertex3f(x0, y0, z0);
    glTexCoord2f(repeat, repeat);  glVertex3f(x0, y1, z0);
    glTexCoord2f(0.0f, repeat);    glVertex3f(x0, y1, z1);

    // 오른쪽 면 (x = x1)
    glTexCoord2f(0.0f, 0.0f);      glVertex3f(x1, y0, z0);
    glTexCoord2f(repeat, 0.0f);    glVertex3f(x1, y0, z1);
    glTexCoord2f(repeat, repeat);  glVertex3f(x1, y1, z1);
    glTexCoord2f(0.0f, repeat);    glVertex3f(x1, y1, z0);

    glEnd();
}

void DrawFloor()
{
    float w = MAZE_W * cellSize;
    float h = MAZE_H * cellSize;

    glBindTexture(GL_TEXTURE_2D, 0);
    glColor3f(0.2f, 0.2f, 0.2f);

    glBegin(GL_QUADS);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(w, 0.0f, 0.0f);
    glVertex3f(w, 0.0f, h);
    glVertex3f(0.0f, 0.0f, h);
    glEnd();
}

void DrawEmotionOrb(const Orb& orb)
{
    glPushMatrix();
    glTranslatef(orb.x, orb.y, orb.z);

    float r, g, b;
    GetEmotionColor(orb.emotionId, r, g, b);

    // 중심 구
    glColor3f(r, g, b);
    glutSolidSphere(orb.radius, 24, 24);

    // 발광용 외곽 구 (additive blending)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    float glowRadius = orb.radius * 1.6f;
    glColor4f(r, g, b, 0.18f);
    glutSolidSphere(glowRadius, 24, 24);
    glDisable(GL_BLEND);

    glPopMatrix();
}

void DrawMaze3D()
{
    DrawFloor();

    // 벽
    for (int z = 0; z < MAZE_H; ++z)
    {
        for (int x = 0; x < MAZE_W; ++x)
        {
            if (maze[z][x] == WALL)
            {
                DrawWallBlock((float)x, (float)z);
            }
        }
    }

    // 감정 구슬 (텍스처 끄고 단색으로)
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    for (const auto& orb : g_orbs)
    {
        if (!orb.collected)
            DrawEmotionOrb(orb);
    }
    glEnable(GL_TEXTURE_2D);
}

// 오른쪽 위 미니맵(간단 2D)
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

    float mapSize = 200.0f;
    float margin = 10.0f;
    float ox = winWidth - mapSize - margin;
    float oy = winHeight - mapSize - margin;

    float cellW = mapSize / MAZE_W;
    float cellH = mapSize / MAZE_H;

    // 배경
    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(ox - 2, oy - 2);
    glVertex2f(ox + mapSize + 2, oy - 2);
    glVertex2f(ox + mapSize + 2, oy + mapSize + 2);
    glVertex2f(ox - 2, oy + mapSize + 2);
    glEnd();

    // 미로 (벽)
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    for (int z = 0; z < MAZE_H; ++z)
    {
        for (int x = 0; x < MAZE_W; ++x)
        {
            if (maze[z][x] == WALL)
            {
                float x0 = ox + x * cellW;
                float y0 = oy + z * cellH;
                float x1 = x0 + cellW;
                float y1 = y0 + cellH;

                glVertex2f(x0, y0);
                glVertex2f(x1, y0);
                glVertex2f(x1, y1);
                glVertex2f(x0, y1);
            }
        }
    }
    glEnd();

    // 감정 구슬 위치 표시
    for (const auto& orb : g_orbs)
    {
        if (orb.collected) continue;

        float opx = orb.x / (MAZE_W * cellSize); // 0~1
        float opz = orb.z / (MAZE_H * cellSize); // 0~1

        float dotX = ox + opx * mapSize;
        float dotY = oy + opz * mapSize;

        SetEmotionColor(orb.emotionId);
        float r = 3.0f;

        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(dotX, dotY);
        for (int i = 0; i <= 12; ++i)
        {
            float ang = (float)i / 12.0f * 2.0f * 3.1415926f;
            glVertex2f(dotX + cosf(ang) * r, dotY + sinf(ang) * r);
        }
        glEnd();
    }

    // 플레이어 위치 표시 (빨간 점)
    float px = camX / (MAZE_W * cellSize);
    float pz = camZ / (MAZE_H * cellSize);

    float dotX = ox + px * mapSize;
    float dotY = oy + pz * mapSize;

    glColor3f(1.0f, 0.0f, 0.0f);
    float r = 4.0f;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(dotX, dotY);
    for (int i = 0; i <= 16; ++i)
    {
        float ang = (float)i / 16.0f * 2.0f * 3.1415926f;
        glVertex2f(dotX + cosf(ang) * r, dotY + sinf(ang) * r);
    }
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ================== 구슬 습득 처리 ==================

void TryCollectOrb()
{
    const float COLLECT_DIST = 0.7f; // 카메라와 구슬 중심 사이 거리 기준

    for (auto& orb : g_orbs)
    {
        if (orb.collected) continue;

        float dx = camX - orb.x;
        float dy = camY - orb.y;
        float dz = camZ - orb.z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (dist < COLLECT_DIST)
        {
            orb.collected = true;
            std::cout << "Collected orb (emotion " << orb.emotionId << ")\n";
            break; // 한 번에 하나만
        }
    }
}

// ================== 카메라 / 입력 ==================

void UpdateCamera(float dt)
{
    float forwardX = cosf(camYaw);
    float forwardZ = sinf(camYaw);

    float rightX = -forwardZ;
    float rightZ = forwardX;

    float moveX = 0.0f;
    float moveZ = 0.0f;

    if (keyDown['w'] || keyDown['W'])
    {
        moveX += forwardX;
        moveZ += forwardZ;
    }
    if (keyDown['s'] || keyDown['S'])
    {
        moveX -= forwardX;
        moveZ -= forwardZ;
    }
    if (keyDown['a'] || keyDown['A'])
    {
        moveX -= rightX;
        moveZ -= rightZ;
    }
    if (keyDown['d'] || keyDown['D'])
    {
        moveX += rightX;
        moveZ += rightZ;
    }

    float len = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (len > 0.0001f)
    {
        moveX /= len;
        moveZ /= len;

        float dx = moveX * MOVE_SPEED * dt;
        float dz = moveZ * MOVE_SPEED * dt;

        float newX = camX + dx;
        float newZ = camZ + dz;

        // 충돌 체크 (축 분리)
        if (!IsBlocked(newX, camZ))
            camX = newX;
        if (!IsBlocked(camX, newZ))
            camZ = newZ;
    }
}

void KeyboardDown(unsigned char key, int x, int y)
{
    if (key == 27) // ESC
    {
        std::exit(0);
    }
    else if (key == ' ') // 스페이스바로 구슬 습득 시도
    {
        TryCollectOrb();
        return;
    }

    keyDown[key] = true;
}

void KeyboardUp(unsigned char key, int x, int y)
{
    keyDown[key] = false;
}

void MouseMotionCallback(int x, int y)
{
    if (warpInProgress)
    {
        warpInProgress = false;
        return;
    }

    int cx = winWidth / 2;
    int cy = winHeight / 2;

    int dx = x - cx;
    int dy = y - cy;

    camYaw += dx * TURN_SPEED_MOUSE;
    camPitch -= dy * TURN_SPEED_MOUSE;

    // 피치 제한
    const float limit = 1.2f; // 약 +-70도
    if (camPitch > limit) camPitch = limit;
    if (camPitch < -limit) camPitch = -limit;

    warpInProgress = true;
    glutWarpPointer(cx, cy);
}

void ReshapeCallback(int w, int h)
{
    winWidth = (w > 1) ? w : 1;
    winHeight = (h > 1) ? h : 1;
    glViewport(0, 0, winWidth, winHeight);
}

// ================== 디스플레이 / 타이머 ==================

int lastTime = 0;

void DisplayCallback()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = (float)winWidth / (float)winHeight;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float dirX = cosf(camYaw) * cosf(camPitch);
    float dirY = sinf(camPitch);
    float dirZ = sinf(camYaw) * cosf(camPitch);

    gluLookAt(
        camX, camY, camZ,
        camX + dirX, camY + dirY, camZ + dirZ,
        0.0f, 1.0f, 0.0f
    );

    glEnable(GL_TEXTURE_2D);
    DrawMaze3D();

    // 2D 미니맵
    DrawMiniMap();

    glutSwapBuffers();
}

void IdleCallback()
{
    int   now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTime) / 1000.0f;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.1f) dt = 0.1f;

    lastTime = now;

    UpdateCamera(dt);

    glutPostRedisplay();
}

// ================== main ==================

int main(int argc, char** argv)
{
    GenerateMaze();       // 1) 미로 생성
    PlaceEmotionOrbs();   // 2) 입구→출구 경로 위에 감정 구슬 배치

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("Emotion Maze");

    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    if (!LoadWallTexture())
    {
        std::cerr << "벽 텍스처 로드 실패. 텍스처 없이 실행합니다.\n";
    }

    glutSetCursor(GLUT_CURSOR_NONE);
    warpInProgress = true;
    glutWarpPointer(winWidth / 2, winHeight / 2);

    glutDisplayFunc(DisplayCallback);
    glutReshapeFunc(ReshapeCallback);
    glutKeyboardFunc(KeyboardDown);
    glutKeyboardUpFunc(KeyboardUp);
    glutPassiveMotionFunc(MouseMotionCallback);
    glutIdleFunc(IdleCallback);

    lastTime = glutGet(GLUT_ELAPSED_TIME);

    glutMainLoop();
    return 0;
}
