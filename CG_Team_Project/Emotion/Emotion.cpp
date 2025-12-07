// Emotion.cpp (정리 버전)

#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>

#include <GL/freeglut.h>
#include <GL/glu.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Config.h"
#include "GameState.h"
#include "SOR_Objects.h"
#include "AngerMap.h"

// ================== Maze Data ==================

const int CELL_W = (MAZE_W - 1) / 2;
const int CELL_H = (MAZE_H - 1) / 2;

int  maze[MAZE_H][MAZE_W];
bool visitedCell[CELL_H][CELL_W];

std::mt19937 rng;

struct Dir { int dx, dy; };
const Dir dirs4[4] = {
    { 0,-1 }, { 0,1 }, { -1,0 }, { 1,0 }
};

// ================== Window / Camera ==================

int   winWidth = WIN_W;
int   winHeight = WIN_H;

float cellSize = CELL_SIZE;
float wallHeight = WALL_HEIGHT;

// 플레이어 카메라
float camX = 1.5f;
float camY = BASE_CAM_Y;
float camZ = 1.5f;

float camYaw = 0.0f;
float camPitch = 0.0f;

// 키 상태
bool keyDown[256] = { false };
enum { ARROW_LEFT = 0, ARROW_RIGHT, ARROW_UP, ARROW_DOWN };
bool arrowDown[4] = { false, false, false, false };

// 시간
int lastTime = 0;

// ================== Texture ==================

GLuint g_wallTex = 0;

// 후보 경로 중 하나를 로드
static unsigned char* LoadImageTryCandidates(int& w, int& h, int& ch, const char** outUsedPath)
{
    for (const char* p : WALL_TEX_CANDIDATES)
    {
        unsigned char* data = stbi_load(p, &w, &h, &ch, 0);
        if (data)
        {
            if (outUsedPath) *outUsedPath = p;
            return data;
        }
    }
    if (outUsedPath) *outUsedPath = nullptr;
    return nullptr;
}

bool LoadWallTexture()
{
    int w, h, channels;
    const char* used = nullptr;

    unsigned char* data = LoadImageTryCandidates(w, h, channels, &used);
    if (!data)
    {
        std::cerr << "Failed to load wall texture candidates.\n";
        return false;
    }

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_LUMINANCE;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    glGenTextures(1, &g_wallTex);
    glBindTexture(GL_TEXTURE_2D, g_wallTex);

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);

    // mipmap을 만들지 않으므로 MIN_FILTER는 GL_LINEAR로
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    std::cout << "Loaded wall texture: " << (used ? used : "(unknown)")
        << " (" << w << "x" << h << ", ch=" << channels << ")\n";
    return true;
}

// ================== Maze Generation ==================

void InitRandom()
{
    std::random_device rd;
    rng.seed(rd());
    std::srand((unsigned int)std::time(nullptr));
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

    std::vector<int> order = { 0,1,2,3 };
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
    maze[1][0] = PATH;
    maze[MAZE_H - 2][MAZE_W - 1] = PATH;
}

void GenerateMaze()
{
    InitRandom();
    InitMazeData();

    GenerateMazeDFS(0, 0);
    AddEntranceExit();
}

// ================== BFS Path (for ordered emotion placement) ==================

struct Cell { int x, z; };

bool BuildPathEntranceToExit(std::vector<Cell>& outPath)
{
    int startX = 1, startZ = 1;
    int endX = MAZE_W - 2, endZ = MAZE_H - 2;

    bool visited[MAZE_H][MAZE_W] = {};
    int  prevX[MAZE_H][MAZE_W];
    int  prevZ[MAZE_H][MAZE_W];

    for (int z = 0; z < MAZE_H; ++z)
        for (int x = 0; x < MAZE_W; ++x)
        {
            prevX[z][x] = -1;
            prevZ[z][x] = -1;
        }

    std::queue<Cell> q;
    q.push({ startX, startZ });
    visited[startZ][startX] = true;

    bool found = false;

    while (!q.empty())
    {
        Cell c = q.front(); q.pop();
        if (c.x == endX && c.z == endZ)
        {
            found = true; break;
        }

        for (int i = 0; i < 4; ++i)
        {
            int nx = c.x + dirs4[i].dx;
            int nz = c.z + dirs4[i].dy;

            if (nx < 0 || nx >= MAZE_W || nz < 0 || nz >= MAZE_H)
                continue;
            if (visited[nz][nx]) continue;
            if (maze[nz][nx] == WALL) continue;

            visited[nz][nx] = true;
            prevX[nz][nx] = c.x;
            prevZ[nz][nx] = c.z;
            q.push({ nx, nz });
        }
    }

    if (!found) return false;

    // 역추적
    std::vector<Cell> rev;
    int cx = endX, cz = endZ;
    while (!(cx == startX && cz == startZ))
    {
        rev.push_back({ cx, cz });
        int px = prevX[cz][cx];
        int pz = prevZ[cz][cx];
        if (px < 0 || pz < 0) break;
        cx = px; cz = pz;
    }
    rev.push_back({ startX, startZ });

    std::reverse(rev.begin(), rev.end());
    outPath = rev;
    return true;
}

// ================== Place Emotion Objects ==================

void InitEmotionObjects()
{
    ClearSORObjects();

    // 모델 로드
    int modelIdx[EMO_COUNT];
    for (int i = 0; i < EMO_COUNT; ++i)
        modelIdx[i] = LoadAndRegisterModel(EMO_MODEL_PATHS[i]);

    std::vector<Cell> path;
    if (!BuildPathEntranceToExit(path) || path.size() < 10)
    {
        // 경로가 짧거나 실패 시 대충 분산 배치
        int fallbackPos[EMO_COUNT][2] = {
            {3,3}, {7,7}, {11,11}, {15,15}, {19,19}
        };

        for (int i = 0; i < EMO_COUNT; ++i)
        {
            if (modelIdx[i] < 0) continue;
            AddObjectGrid(modelIdx[i],
                fallbackPos[i][0], fallbackPos[i][1],
                0.6f, 0.8f,
                0.0f, 1.2f,
                0.04f, 0.08f,
                i);
        }
        return;
    }

    // 항상 같은 순서로 만나게 하기:
    // 기쁨 -> 슬픔 -> 버럭(분노) -> 까칠 -> 소심
    // (Config의 EmotionId 순서 기준)
    int order[EMO_COUNT] = {
        EMO_JOY, EMO_SADNESS, EMO_ANGER, EMO_DISGUST, EMO_FEAR
    };

    // 경로를 5등분 지점에 배치
    for (int k = 0; k < EMO_COUNT; ++k)
    {
        int emo = order[k];
        if (modelIdx[emo] < 0) continue;

        size_t idx = (size_t)((k + 1) * path.size() / (EMO_COUNT + 1));
        if (idx >= path.size()) idx = path.size() - 1;

        AddObjectGrid(modelIdx[emo],
            path[idx].x, path[idx].z,
            0.6f, 0.8f,
            0.0f, 1.2f,
            0.04f, 0.08f,
            emo);
    }
}

// ================== Collision ==================

bool IsWallCell(int gx, int gz)
{
    if (gx < 0 || gx >= MAZE_W || gz < 0 || gz >= MAZE_H)
        return true;
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

// ================== Rendering (Maze) ==================

void DrawWallBlock(float gx, float gz)
{
    float x0 = gx * cellSize;
    float x1 = (gx + 1) * cellSize;
    float z0 = gz * cellSize;
    float z1 = (gz + 1) * cellSize;

    float y0 = 0.0f;
    float y1 = wallHeight;

    float repeat = 1.0f;

    if (g_wallTex != 0)
        glBindTexture(GL_TEXTURE_2D, g_wallTex);
    else
        glBindTexture(GL_TEXTURE_2D, 0);

    glColor3f(1, 1, 1);

    glBegin(GL_QUADS);

    // 앞면
    glTexCoord2f(0, 0);           glVertex3f(x0, y0, z0);
    glTexCoord2f(repeat, 0);      glVertex3f(x1, y0, z0);
    glTexCoord2f(repeat, repeat);glVertex3f(x1, y1, z0);
    glTexCoord2f(0, repeat);      glVertex3f(x0, y1, z0);

    // 뒷면
    glTexCoord2f(0, 0);           glVertex3f(x1, y0, z1);
    glTexCoord2f(repeat, 0);      glVertex3f(x0, y0, z1);
    glTexCoord2f(repeat, repeat);glVertex3f(x0, y1, z1);
    glTexCoord2f(0, repeat);      glVertex3f(x1, y1, z1);

    // 왼쪽
    glTexCoord2f(0, 0);           glVertex3f(x0, y0, z1);
    glTexCoord2f(repeat, 0);      glVertex3f(x0, y0, z0);
    glTexCoord2f(repeat, repeat);glVertex3f(x0, y1, z0);
    glTexCoord2f(0, repeat);      glVertex3f(x0, y1, z1);

    // 오른쪽
    glTexCoord2f(0, 0);           glVertex3f(x1, y0, z0);
    glTexCoord2f(repeat, 0);      glVertex3f(x1, y0, z1);
    glTexCoord2f(repeat, repeat);glVertex3f(x1, y1, z1);
    glTexCoord2f(0, repeat);      glVertex3f(x1, y1, z0);

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
    glVertex3f(0, 0, 0);
    glVertex3f(w, 0, 0);
    glVertex3f(w, 0, h);
    glVertex3f(0, 0, h);
    glEnd();
}

void DrawMaze3D()
{
    DrawFloor();

    for (int z = 0; z < MAZE_H; ++z)
        for (int x = 0; x < MAZE_W; ++x)
            if (maze[z][x] == WALL)
                DrawWallBlock((float)x, (float)z);

    // 감정 구슬(SOR)
    DrawSORObjects(cellSize);
}

// ================== MiniMap ==================

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
    glColor4f(0, 0, 0, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(ox - 2, oy - 2);
    glVertex2f(ox + mapSize + 2, oy - 2);
    glVertex2f(ox + mapSize + 2, oy + mapSize + 2);
    glVertex2f(ox - 2, oy + mapSize + 2);
    glEnd();

    // 벽
    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    for (int z = 0; z < MAZE_H; ++z)
        for (int x = 0; x < MAZE_W; ++x)
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
    glEnd();

    // 감정 오브젝트 표시(색 점)
    for (const auto& obj : g_worldObjects)
    {
        if (obj.collected) continue;

        float gx = obj.mazeX + 0.5f;
        float gz = obj.mazeY + 0.5f;

        float px = ox + gx * cellW;
        float pz = oy + gz * cellH;

        // 감정별 색상
        switch (obj.emotionId)
        {
        case EMO_JOY:     glColor3f(1.0f, 0.9f, 0.1f); break;
        case EMO_SADNESS: glColor3f(0.2f, 0.5f, 1.0f); break;
        case EMO_ANGER:   glColor3f(1.0f, 0.1f, 0.1f); break;
        case EMO_DISGUST: glColor3f(0.2f, 1.0f, 0.3f); break;
        case EMO_FEAR:    glColor3f(0.7f, 0.3f, 1.0f); break;
        default:          glColor3f(1.0f, 1.0f, 0.0f); break;
        }

        float r = 3.5f;
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(px, pz);
        for (int i = 0; i <= 12; ++i)
        {
            float ang = (float)i / 12.0f * 2.0f * 3.1415926f;
            glVertex2f(px + std::cos(ang) * r, pz + std::sin(ang) * r);
        }
        glEnd();
    }

    // 플레이어(빨간 점)
    float pxN = camX / (MAZE_W * cellSize);
    float pzN = camZ / (MAZE_H * cellSize);

    float dotX = ox + pxN * mapSize;
    float dotY = oy + pzN * mapSize;

    glColor3f(1, 0, 0);
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

// ================== Interaction ==================

void TryCollectObject()
{
    float px = camX;
    float pz = camZ;

    float radius = 1.0f;
    float radius2 = radius * radius;

    int bestIndex = -1;

    for (int i = 0; i < (int)g_worldObjects.size(); ++i)
    {
        auto& obj = g_worldObjects[i];
        if (obj.collected) continue;

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

    if (bestIndex < 0)
    {
        std::cout << "주변에 습득할 감정 오브젝트가 없습니다.\n";
        return;
    }

    auto& obj = g_worldObjects[bestIndex];
    obj.collected = true;

    std::cout << "감정을 습득했습니다! emotionId=" << obj.emotionId << "\n";

    // 분노 감정이면 분노맵으로 전이
    if (obj.emotionId == EMO_ANGER && gGameState.stage == STAGE_MAZE)
    {
        gGameState.returnCamX = camX;
        gGameState.returnCamY = camY;
        gGameState.returnCamZ = camZ;

        gGameState.stage = STAGE_ANGER;
        gGameState.angerCleared = false;

        // 분노맵 입장 시 카메라를 중앙 근처로
        camX = 0.0f; camZ = -8.0f; camY = BASE_CAM_Y;
        camYaw = 0.0f; camPitch = 0.0f;

        AngerMap::Enter();
    }
}

// ================== Camera Update ==================

void UpdateCameraMaze(float dt)
{
    // 이동(WASD)
    float forwardX = std::cos(camYaw);
    float forwardZ = std::sin(camYaw);
    float rightX = -forwardZ;
    float rightZ = forwardX;

    float moveX = 0.0f, moveZ = 0.0f;

    if (keyDown['w'] || keyDown['W']) { moveX += forwardX; moveZ += forwardZ; }
    if (keyDown['s'] || keyDown['S']) { moveX -= forwardX; moveZ -= forwardZ; }
    if (keyDown['a'] || keyDown['A']) { moveX -= rightX;   moveZ -= rightZ; }
    if (keyDown['d'] || keyDown['D']) { moveX += rightX;   moveZ += rightZ; }

    // 방향키로도 이동 지원(원하면 WASD만 써도 됨)
    if (arrowDown[ARROW_UP]) { moveX += forwardX; moveZ += forwardZ; }
    if (arrowDown[ARROW_DOWN]) { moveX -= forwardX; moveZ -= forwardZ; }

    float len = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (len > 0.0001f)
    {
        moveX /= len; moveZ /= len;

        float dx = moveX * MOVE_SPEED * dt;
        float dz = moveZ * MOVE_SPEED * dt;

        float newX = camX + dx;
        float newZ = camZ + dz;

        if (!IsBlocked(newX, camZ)) camX = newX;
        if (!IsBlocked(camX, newZ)) camZ = newZ;
    }

    // 회전: 좌/우 방향키
    if (arrowDown[ARROW_LEFT])  camYaw -= TURN_SPEED * dt;
    if (arrowDown[ARROW_RIGHT]) camYaw += TURN_SPEED * dt;
}

void UpdateCameraAnger(float dt)
{
    // 분노맵에서는 충돌 최소화, 이동 단순화
    float forwardX = std::cos(camYaw);
    float forwardZ = std::sin(camYaw);

    float moveX = 0.0f, moveZ = 0.0f;
    if (arrowDown[ARROW_UP]) { moveX += forwardX; moveZ += forwardZ; }
    if (arrowDown[ARROW_DOWN]) { moveX -= forwardX; moveZ -= forwardZ; }

    float len = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (len > 0.0001f)
    {
        moveX /= len; moveZ /= len;
        camX += moveX * MOVE_SPEED * dt;
        camZ += moveZ * MOVE_SPEED * dt;
    }

    if (arrowDown[ARROW_LEFT])  camYaw -= TURN_SPEED * dt;
    if (arrowDown[ARROW_RIGHT]) camYaw += TURN_SPEED * dt;

    AngerMap::Update(dt);
}

// ================== Callbacks ==================

void DisplayCallback()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = (float)winWidth / (float)winHeight;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 0.1, 200.0);

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

    if (gGameState.stage == STAGE_MAZE)
    {
        DrawMaze3D();
        DrawMiniMap();
    }
    else if (gGameState.stage == STAGE_ANGER)
    {
        AngerMap::Render3D();
        AngerMap::Render2D(winWidth, winHeight);
    }

    glutSwapBuffers();
}

void IdleCallback()
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTime) / 1000.0f;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.1f) dt = 0.1f;
    lastTime = now;

    if (gGameState.stage == STAGE_MAZE)
        UpdateCameraMaze(dt);
    else
        UpdateCameraAnger(dt);

    glutPostRedisplay();
}

void ReshapeCallback(int w, int h)
{
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
    winWidth = w;
    winHeight = h;
    glViewport(0, 0, winWidth, winHeight);
}

void KeyboardDown(unsigned char key, int, int)
{
    if (key == 27) std::exit(0);

    // 상호작용 키: T
    if (key == 't' || key == 'T')
    {
        if (gGameState.stage == STAGE_MAZE)
        {
            TryCollectObject();
        }
        else if (gGameState.stage == STAGE_ANGER)
        {
            // 클리어 상태에서만 복귀
            if (AngerMap::IsCleared())
            {
                gGameState.stage = STAGE_MAZE;

                camX = gGameState.returnCamX;
                camY = gGameState.returnCamY;
                camZ = gGameState.returnCamZ;

                camYaw = 0.0f;
                camPitch = 0.0f;

                AngerMap::Exit();
            }
        }
        return;
    }

    keyDown[key] = true;
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

// ================== main ==================

int main(int argc, char** argv)
{
    ResetGameState();
    GenerateMaze();
    InitEmotionObjects();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("Emotion Maze");

    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    LoadWallTexture();

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
