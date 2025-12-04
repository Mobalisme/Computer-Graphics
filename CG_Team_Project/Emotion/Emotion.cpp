// Emotion.cpp

#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <random>
#include <fstream>
#include <sstream>
#include <string>

#include <GL/freeglut.h>
#include <GL/glu.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "SOR_Objects.h"
#include "Model.h"

// ================== 미로 설정 ==================

const int MAZE_W = 25;   // 반드시 홀수
const int MAZE_H = 25;   // 반드시 홀수

const int CELL_W = (MAZE_W - 1) / 2;
const int CELL_H = (MAZE_H - 1) / 2;

const int WALL = 1;
const int PATH = 0;

int  maze[MAZE_H][MAZE_W];
bool visitedCell[CELL_H][CELL_W];

std::mt19937 rng;

struct Dir { int dx, dy; };
const Dir dirs4[4] = {
    { 0, -1 }, { 0,  1 }, { -1, 0 }, { 1,  0 }
};

// ================== 카메라 / 윈도우 설정 ==================

int winWidth = 800;
int winHeight = 800;

float cellSize = 1.0f;
float wallHeight = 1.6f;

// 플레이어 위치 (1,1 셀 중심)
float camX = 1.5f;
float camY = 0.8f;
float camZ = 1.5f;

float camYaw = 0.0f;   // 좌우 회전 (라디안)
float camPitch = 0.0f;   // 상하 회전 (라디안)

const float MOVE_SPEED = 2.0f;  // m/s
const float TURN_SPEED_KEY = 1.5f;  // 방향키 회전 속도 (rad/s)

// 점프
float camVelY = 0.0f;
bool  isOnGround = true;
const float BASE_CAM_Y = 0.8f;
const float GRAVITY = -9.8f;
const float JUMP_SPEED = 4.5f;

// WASD 키 상태
bool keyDown[256] = { false };

// 방향키 상태
enum { ARROW_LEFT = 0, ARROW_RIGHT, ARROW_UP, ARROW_DOWN };
bool arrowDown[4] = { false, false, false, false };

// ================== 배(보트) 관련 ==================

// 배를 둘 미로 격자 위치
const int BOAT_GRID_X = 5;
const int BOAT_GRID_Z = 5;

float g_boatPosX = 0.0f;
float g_boatPosY = 0.0f;
float g_boatPosZ = 0.0f;
float g_boatRadius = 2.0f;
bool  g_boatPlaced = false;

int   g_collectedCount = 0; // 감정 습득 개수
bool  g_onBoat = false;

// ================== 텍스처 ==================

GLuint g_wallTex = 0;

// ================== 시간 ==================

int lastTime = 0;

// ================== 랜덤 / 미로 생성 ==================

void InitRandom()
{
    std::random_device rd;
    rng.seed(rd());
}

void InitMazeData()
{
    for (int y = 0; y < MAZE_H; ++y)
        for (int x = 0; x < MAZE_W; ++x)
            maze[y][x] = WALL;

    for (int cy = 0; cy < CELL_H; ++cy)
        for (int cx = 0; cx < CELL_W; ++cx)
            visitedCell[cy][cx] = false;
}

void OpenCell(int cx, int cy)
{
    int x = 2 * cx + 1;
    int y = 2 * cy + 1;
    maze[y][x] = PATH;
}

void ShuffleDirs(std::vector<int>& order)
{
    std::shuffle(order.begin(), order.end(), rng);
}

void GenerateMazeDFS(int cx, int cy)
{
    visitedCell[cy][cx] = true;
    OpenCell(cx, cy);

    std::vector<int> order;
    order.push_back(0);
    order.push_back(1);
    order.push_back(2);
    order.push_back(3);
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

void AddEntranceExit()
{
    maze[1][0] = PATH;                               // 입구
    maze[MAZE_H - 2][MAZE_W - 1] = PATH;             // 출구
}

// 배 주변을 PATH로 비워주기 (벽이랑 겹치지 않게)
void ClearBoatArea()
{
    for (int dz = -1; dz <= 1; ++dz)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            int x = BOAT_GRID_X + dx;
            int z = BOAT_GRID_Z + dz;

            if (x > 0 && x < MAZE_W - 1 &&
                z > 0 && z < MAZE_H - 1)
            {
                maze[z][x] = PATH;
            }
        }
    }
}

void GenerateMaze()
{
    InitRandom();
    InitMazeData();
    GenerateMazeDFS(0, 0);
    AddEntranceExit();
    ClearBoatArea();
}

// ================== 배 위치 초기화 ==================

void InitBoatPosition()
{
    g_boatPosX = (BOAT_GRID_X + 0.5f) * cellSize;
    g_boatPosZ = (BOAT_GRID_Z + 0.5f) * cellSize;
    g_boatPosY = 0.0f;

    g_boatRadius = 2.0f;   // 배 OBJ 크기에 맞게 필요하면 조절
    g_boatPlaced = true;
}

// ================== 텍스처 로드 ==================

bool LoadWallTexture()
{
    const char* candidates[3] = {
        "Textures/wall.png",
        "../Textures/wall.png",
        "wall.png"
    };

    int   w = 0, h = 0, channels = 0;
    unsigned char* data = 0;
    const char* usedPath = 0;

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
        std::cerr << "Failed to load wall texture.\n";
        return false;
    }

    GLenum format;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;
    else
    {
        stbi_image_free(data);
        std::cerr << "Unsupported channel count in wall texture\n";
        return false;
    }

    glGenTextures(1, &g_wallTex);
    glBindTexture(GL_TEXTURE_2D, g_wallTex);

    glTexImage2D(GL_TEXTURE_2D, 0, format,
        w, h, 0, format, GL_UNSIGNED_BYTE, data);

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
        return true;
    return (maze[gz][gx] == WALL);
}

bool IsBlocked(float x, float z)
{
    float radius = 0.2f;

    int left = (int)((x - radius) / cellSize);
    int right = (int)((x + radius) / cellSize);
    int top = (int)((z - radius) / cellSize);
    int bottom = (int)((z + radius) / cellSize);

    if (IsWallCell(left, top) ||
        IsWallCell(right, top) ||
        IsWallCell(left, bottom) ||
        IsWallCell(right, bottom))
        return true;

    // 배 충돌 (배 위에 올라간 상태가 아니라면 배 안으로 못 들어감)
    if (g_boatPlaced && !g_onBoat)
    {
        float dx = x - g_boatPosX;
        float dz = z - g_boatPosZ;
        float dist2 = dx * dx + dz * dz;

        if (dist2 < g_boatRadius * g_boatRadius)
            return true;
    }

    return false;
}

// ================== 렌더링: 벽/바닥 ==================

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

    // 앞면
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(x0, y0, z0);
    glTexCoord2f(repeat, 0.0f);   glVertex3f(x1, y0, z0);
    glTexCoord2f(repeat, repeat);    glVertex3f(x1, y1, z0);
    glTexCoord2f(0.0f, repeat);     glVertex3f(x0, y1, z0);

    // 뒷면
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(x1, y0, z1);
    glTexCoord2f(repeat, 0.0f);   glVertex3f(x0, y0, z1);
    glTexCoord2f(repeat, repeat);    glVertex3f(x0, y1, z1);
    glTexCoord2f(0.0f, repeat);     glVertex3f(x1, y1, z1);

    // 왼쪽
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(x0, y0, z1);
    glTexCoord2f(repeat, 0.0f);   glVertex3f(x0, y0, z0);
    glTexCoord2f(repeat, repeat);    glVertex3f(x0, y1, z0);
    glTexCoord2f(0.0f, repeat);     glVertex3f(x0, y1, z1);

    // 오른쪽
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(x1, y0, z0);
    glTexCoord2f(repeat, 0.0f);   glVertex3f(x1, y0, z1);
    glTexCoord2f(repeat, repeat);    glVertex3f(x1, y1, z1);
    glTexCoord2f(0.0f, repeat);     glVertex3f(x1, y1, z0);

    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
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

void DrawMaze3D()
{
    DrawFloor();

    for (int z = 0; z < MAZE_H; ++z)
        for (int x = 0; x < MAZE_W; ++x)
            if (maze[z][x] == WALL)
                DrawWallBlock((float)x, (float)z);
}

// ================== 미니맵 ==================

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

    // 벽
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

    // SOR 오브젝트 (노란 사각형)
    glColor3f(1.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    for (int i = 0; i < (int)g_worldObjects.size(); ++i)
    {
        const GameObject& obj = g_worldObjects[i];
        if (obj.collected)
            continue;

        float gx = obj.mazeX + 0.5f;
        float gz = obj.mazeY + 0.5f;

        float cx = ox + gx * cellW;
        float cy = oy + gz * cellH;

        float half = (cellW < cellH ? cellW : cellH) * 0.25f;

        float x0 = cx - half;
        float y0 = cy - half;
        float x1 = cx + half;
        float y1 = cy + half;

        glVertex2f(x0, y0);
        glVertex2f(x1, y0);
        glVertex2f(x1, y1);
        glVertex2f(x0, y1);
    }
    glEnd();

    // 플레이어 위치 (빨간 점)
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
        glVertex2f(dotX + (float)std::cos(ang) * r,
            dotY + (float)std::sin(ang) * r);
    }
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ================== 점프 / 카메라 이동 ==================

void UpdateJump(float dt)
{
    camVelY += GRAVITY * dt;
    camY += camVelY * dt;

    if (camY <= BASE_CAM_Y)
    {
        camY = BASE_CAM_Y;
        camVelY = 0.0f;
        isOnGround = true;
    }
}

void UpdateCamera(float dt)
{
    // 이동(WASD)
    float forwardX = (float)std::cos(camYaw);
    float forwardZ = (float)std::sin(camYaw);

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

    float len = (float)std::sqrt(moveX * moveX + moveZ * moveZ);
    if (len > 0.0001f)
    {
        moveX /= len;
        moveZ /= len;

        float dx = moveX * MOVE_SPEED * dt;
        float dz = moveZ * MOVE_SPEED * dt;

        float newX = camX + dx;
        float newZ = camZ + dz;

        if (!IsBlocked(newX, camZ))
            camX = newX;
        if (!IsBlocked(camX, newZ))
            camZ = newZ;
    }

    // 회전(방향키)
    if (arrowDown[ARROW_LEFT])
        camYaw -= TURN_SPEED_KEY * dt;
    if (arrowDown[ARROW_RIGHT])
        camYaw += TURN_SPEED_KEY * dt;
    if (arrowDown[ARROW_UP])
        camPitch += TURN_SPEED_KEY * dt;
    if (arrowDown[ARROW_DOWN])
        camPitch -= TURN_SPEED_KEY * dt;

    const float limit = 1.2f;
    if (camPitch > limit) camPitch = limit;
    if (camPitch < -limit) camPitch = -limit;

    // 배 위에 있는지 여부
    if (g_boatPlaced)
    {
        float dx = camX - g_boatPosX;
        float dz = camZ - g_boatPosZ;
        float dist2 = dx * dx + dz * dz;

        if (dist2 < g_boatRadius * g_boatRadius)
            g_onBoat = true;
        else
            g_onBoat = false;
    }
}

// ================== 감정 오브젝트 습득 ==================

void TryCollectObject()
{
    float px = camX;
    float pz = camZ;

    float radius = 1.0f;
    float radius2 = radius * radius;

    int bestIndex = -1;

    for (int i = 0; i < (int)g_worldObjects.size(); ++i)
    {
        GameObject& obj = g_worldObjects[i];
        if (obj.collected)
            continue;

        float ox = (obj.mazeX + 0.5f) * cellSize;
        float oz = (obj.mazeY + 0.5f) * cellSize;

        float dx = ox - px;
        float dz = oz - pz;
        float dist2 = dx * dx + dz * dz;

        if (dist2 <= radius2)
        {
            radius2 = dist2;
            bestIndex = i;
        }
    }

    if (bestIndex != -1)
    {
        g_worldObjects[bestIndex].collected = true;
        std::cout << "감정을 습득했습니다! index = "
            << bestIndex << std::endl;

        g_collectedCount++;

        // 첫 번째 감정을 습득했을 때 배 근처로 이동
        if (g_collectedCount == 1 && g_boatPlaced)
        {
            camX = g_boatPosX;
            camZ = g_boatPosZ - 1.0f;  // 배 뒤쪽 약간
            camY = BASE_CAM_Y;
            camVelY = 0.0f;
            isOnGround = true;

            g_onBoat = true;
            std::cout << "첫 번째 감정을 얻어서 배 근처로 이동했습니다.\n";
        }
    }
    else
    {
        std::cout << "주변에 습득할 감정 오브젝트가 없습니다.\n";
    }
}

// ================== 입력 콜백 ==================

void KeyboardDown(unsigned char key, int, int)
{
    if (key == 27) // ESC
    {
        std::exit(0);
    }
    else if (key == ' ')
    {
        if (isOnGround)
        {
            camVelY = JUMP_SPEED;
            isOnGround = false;
        }
    }
    else if (key == 'r' || key == 'R')
    {
        TryCollectObject();
    }
    else
    {
        keyDown[key] = true;
    }
}

void KeyboardUp(unsigned char key, int, int)
{
    keyDown[key] = false;
}

void SpecialDown(int key, int, int)
{
    if (key == GLUT_KEY_LEFT)  arrowDown[ARROW_LEFT] = true;
    if (key == GLUT_KEY_RIGHT) arrowDown[ARROW_RIGHT] = true;
    if (key == GLUT_KEY_UP)    arrowDown[ARROW_UP] = true;
    if (key == GLUT_KEY_DOWN)  arrowDown[ARROW_DOWN] = true;
}

void SpecialUp(int key, int, int)
{
    if (key == GLUT_KEY_LEFT)  arrowDown[ARROW_LEFT] = false;
    if (key == GLUT_KEY_RIGHT) arrowDown[ARROW_RIGHT] = false;
    if (key == GLUT_KEY_UP)    arrowDown[ARROW_UP] = false;
    if (key == GLUT_KEY_DOWN)  arrowDown[ARROW_DOWN] = false;
}

void ReshapeCallback(int w, int h)
{
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;

    winWidth = w;
    winHeight = h;

    glViewport(0, 0, winWidth, winHeight);
}

// ================== BFS: 입구→출구 경로 ==================

struct Cell { int x; int z; };

bool BuildPathEntranceToExit(std::vector<Cell>& outPath)
{
    int startX = 1;
    int startZ = 1;
    int endX = MAZE_W - 2;
    int endZ = MAZE_H - 2;

    bool visited[MAZE_H][MAZE_W];
    int  prevX[MAZE_H][MAZE_W];
    int  prevZ[MAZE_H][MAZE_W];

    for (int z = 0; z < MAZE_H; ++z)
    {
        for (int x = 0; x < MAZE_W; ++x)
        {
            visited[z][x] = false;
            prevX[z][x] = -1;
            prevZ[z][x] = -1;
        }
    }

    std::queue<Cell> q;
    q.push(Cell{ startX, startZ });
    visited[startZ][startX] = true;

    bool found = false;

    while (!q.empty())
    {
        Cell c = q.front();
        q.pop();

        if (c.x == endX && c.z == endZ)
        {
            found = true;
            break;
        }

        for (int i = 0; i < 4; ++i)
        {
            int nx = c.x + dirs4[i].dx;
            int nz = c.z + dirs4[i].dy;

            if (nx < 0 || nx >= MAZE_W || nz < 0 || nz >= MAZE_H)
                continue;

            if (visited[nz][nx])
                continue;

            if (maze[nz][nx] != PATH)
                continue;

            visited[nz][nx] = true;
            prevX[nz][nx] = c.x;
            prevZ[nz][nx] = c.z;

            q.push(Cell{ nx, nz });
        }
    }

    if (!found)
        return false;

    int cx = endX;
    int cz = endZ;
    outPath.clear();

    while (!(cx == startX && cz == startZ))
    {
        Cell c; c.x = cx; c.z = cz;
        outPath.push_back(c);

        int px = prevX[cz][cx];
        int pz = prevZ[cz][cx];

        cx = px;
        cz = pz;
    }

    outPath.push_back(Cell{ startX, startZ });
    std::reverse(outPath.begin(), outPath.end());
    return true;
}

// ================== 감정(SOR) 경로 위에 배치 ==================

void InitEmotionObjects()
{
    int modelIndex = LoadAndRegisterModel("model_data.txt");
    if (modelIndex < 0)
    {
        std::cout << "[Emotion] SOR 모델 로드 실패.\n";
        return;
    }

    std::vector<Cell> path;
    if (!BuildPathEntranceToExit(path))
    {
        std::cout << "[Emotion] 미로 경로를 찾지 못했습니다.\n";
        return;
    }

    const int EMOTION_COUNT = 5;
    if ((int)path.size() < EMOTION_COUNT + 2)
    {
        std::cout << "[Emotion] 경로가 너무 짧습니다.\n";
        return;
    }

    for (int i = 0; i < EMOTION_COUNT; ++i)
    {
        float t = (float)(i + 1) / (float)(EMOTION_COUNT + 1);
        int idx = (int)(t * (path.size() - 1));
        if (idx < 0) idx = 0;
        if (idx >= (int)path.size()) idx = (int)path.size() - 1;

        Cell c = path[idx];

        float height = 2.0f;
        float scale = 0.6f;
        float baseAngle = 0.0f;
        float rotSpeed = 1.0f + 0.3f * i;
        float floatSpeed = 0.05f + 0.01f * i;
        float floatRange = 0.3f;

        AddObjectGrid(
            modelIndex,
            c.x, c.z,
            height,
            scale,
            baseAngle,
            rotSpeed,
            floatSpeed,
            floatRange
        );
    }

    std::cout << "[Emotion] 감정 오브젝트 " << EMOTION_COUNT
        << "개 경로 위에 배치 완료.\n";
}

// ================== 배(마야 모델) 렌더링 ==================

void DrawBoat()
{
    if (g_boatVertices.empty() || !g_boatPlaced)
        return;

    DrawOBJModel(g_boatVertices,
        g_boatPosX, g_boatPosY, g_boatPosZ,
        0.5f); // 배 스케일
}

// ================== 디스플레이 / 타이머 ==================

void DisplayCallback()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = (float)winWidth / (float)winHeight;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float dirX = (float)std::cos(camYaw) * (float)std::cos(camPitch);
    float dirY = (float)std::sin(camPitch);
    float dirZ = (float)std::sin(camYaw) * (float)std::cos(camPitch);

    gluLookAt(
        camX, camY, camZ,
        camX + dirX, camY + dirY, camZ + dirZ,
        0.0f, 1.0f, 0.0f
    );

    glEnable(GL_TEXTURE_2D);
    DrawMaze3D();

    DrawSORObjects(cellSize);   // 감정 SOR 오브젝트
    DrawBoat();                 // 배 모델

    DrawMiniMap();

    glutSwapBuffers();
}

void IdleCallback()
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTime) / 1000.0f;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.1f) dt = 0.1f;

    lastTime = now;

    UpdateCamera(dt);
    UpdateJump(dt);

    glutPostRedisplay();
}

// ================== main ==================

int main(int argc, char** argv)
{
    GenerateMaze();
    std::srand((unsigned int)std::time(0));
    InitEmotionObjects();
    InitBoatPosition();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("Emotion Maze with SOR + Maya Boat");

    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    if (!LoadWallTexture())
        std::cerr << "벽 텍스처 로드 실패. 텍스처 없이 실행합니다.\n";

    // 마야에서 Export한 OBJ(배) 모델 로드
    if (!LoadOBJ_Simple("Models/Boad.obj", g_boatVertices))
        std::cout << "마야 배 모델 로드 실패.\n";

    glutDisplayFunc(DisplayCallback);
    glutReshapeFunc(ReshapeCallback);
    glutKeyboardFunc(KeyboardDown);
    glutKeyboardUpFunc(KeyboardUp);
    glutSpecialFunc(SpecialDown);
    glutSpecialUpFunc(SpecialUp);
    glutIdleFunc(IdleCallback);

    lastTime = glutGet(GLUT_ELAPSED_TIME);

    glutMainLoop();
    return 0;
}
