// Emotion.cpp

#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdlib>

#include <GL/freeglut.h>
#include <GL/glu.h>

// stb_image: 이 cpp 한 군데에서만 IMPLEMENTATION 정의
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "SOR_Objects.h"

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

// 셀 크기 및 벽 높이
float cellSize = 1.0f;
float wallHeight = 1.6f;

// 플레이어 카메라 위치 (1,1셀 중앙에서 시작)
float camX = 1.5f;
float camY = 0.8f;
float camZ = 1.5f;

float camYaw = 0.0f;   // 좌우 회전 (라디안)
float camPitch = 0.0f;   // 상하 회전 (라디안)

const float MOVE_SPEED = 2.0f;     // m/s
const float TURN_SPEED_MOUSE = 0.0025f;  // 마우스 회전 속도

bool keyDown[256] = { false };

// 마우스 래핑용
bool warpInProgress = false;

// ================== 텍스처 ==================

GLuint g_wallTex = 0;

// ================== 랜덤 / 초기화 ==================

void InitRandom()
{
    std::random_device rd;
    rng.seed(rd());   // 매번 다른 미로
    // rng.seed(1234); // 같은 미로 보고 싶으면 고정
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

// ================== DFS 미로 생성 ==================

struct Dir { int dx, dy; };

const Dir dirs4[4] = {
    { 0, -1 }, // 위
    { 0,  1 }, // 아래
    { -1, 0 }, // 왼
    { 1,  0 }  // 오
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

// ================== 입구 / 출구 ==================

void AddEntranceExit()
{
    // 입구: 왼쪽 벽
    maze[1][0] = PATH;                        // (x=0, y=1)
    // 출구: 오른쪽 벽
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

// 작업 디렉터리가 어디냐에 따라 몇 가지 후보 경로를 시도
bool LoadWallTexture()
{
    const char* candidates[] = {
        "Textures/wall.png",     // 작업 디렉터리가 프로젝트 루트일 때
        "../Textures/wall.png",  // 작업 디렉터리가 Debug/ 또는 Release/일 때
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

    glTexImage2D(
        GL_TEXTURE_2D, 0, format,
        w, h, 0, format, GL_UNSIGNED_BYTE, data
    );

    // mipmap 없이 간단한 필터 사용
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

// ================== 렌더링 ==================

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
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(x0, y0, z0);
    glTexCoord2f(repeat, 0.0f);   glVertex3f(x1, y0, z0);
    glTexCoord2f(repeat, repeat);    glVertex3f(x1, y1, z0);
    glTexCoord2f(0.0f, repeat);     glVertex3f(x0, y1, z0);

    // 뒷면 (z = z1)
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(x1, y0, z1);
    glTexCoord2f(repeat, 0.0f);   glVertex3f(x0, y0, z1);
    glTexCoord2f(repeat, repeat);    glVertex3f(x0, y1, z1);
    glTexCoord2f(0.0f, repeat);     glVertex3f(x1, y1, z1);

    // 왼쪽 면 (x = x0)
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(x0, y0, z1);
    glTexCoord2f(repeat, 0.0f);   glVertex3f(x0, y0, z0);
    glTexCoord2f(repeat, repeat);    glVertex3f(x0, y1, z0);
    glTexCoord2f(0.0f, repeat);     glVertex3f(x0, y1, z1);

    // 오른쪽 면 (x = x1)
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
    {
        for (int x = 0; x < MAZE_W; ++x)
        {
            if (maze[z][x] == WALL)
            {
                DrawWallBlock((float)x, (float)z);
            }
        }
    }

    // SOR 오브젝트(감정)는 DisplayCallback에서 따로 렌더링
}

// 오른쪽 위 미니맵
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

    // 미로 벽
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

    // 플레이어 위치
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
        glVertex2f(dotX + std::cos(ang) * r, dotY + std::sin(ang) * r);
    }
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ================== 카메라 / 입력 ==================

void UpdateCamera(float dt)
{
    float forwardX = std::cos(camYaw);
    float forwardZ = std::sin(camYaw);

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

        if (!IsBlocked(newX, camZ))
            camX = newX;
        if (!IsBlocked(camX, newZ))
            camZ = newZ;
    }
}

void KeyboardDown(unsigned char key, int, int)
{
    if (key == 27) // ESC
        std::exit(0);

    keyDown[key] = true;
}

void KeyboardUp(unsigned char key, int, int)
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

    const float limit = 1.2f; // 약 ±70도
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

    float dirX = std::cos(camYaw) * std::cos(camPitch);
    float dirY = std::sin(camPitch);
    float dirZ = std::sin(camYaw) * std::cos(camPitch);

    gluLookAt(
        camX, camY, camZ,
        camX + dirX, camY + dirY, camZ + dirZ,
        0.0f, 1.0f, 0.0f
    );

    glEnable(GL_TEXTURE_2D);
    DrawMaze3D();

    // SOR 오브젝트(= 감정들) 렌더링
    DrawSORObjects(cellSize);

    // 2D 미니맵
    DrawMiniMap();

    glutSwapBuffers();
}

void IdleCallback()
{
    int  now = glutGet(GLUT_ELAPSED_TIME);
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
    GenerateMaze();       // 미로 생성

    // SOR 오브젝트용 랜덤 위상 초기화
    std::srand((unsigned int)std::time(nullptr));
    InitGameObjects();    // SOR 모델 로드 + 감정 오브젝트 배치

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

    // 마우스 커서 숨기기 + 중앙으로 고정
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
