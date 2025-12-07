#include "Global.h"
#include "SOR_Objects.h"
#include "AngerMap.h"

#include <iostream>
#include <ctime>
#include <cmath>
#include <queue>
#include <algorithm>
#include <string>

// 그림자 행렬 생성
static void BuildShadowMatrix(GLfloat shadowMat[4][4], GLfloat light[4], GLfloat plane[4])
{
    GLfloat dot = plane[0] * light[0] + plane[1] * light[1] + plane[2] * light[2] + plane[3] * light[3];

    shadowMat[0][0] = dot - light[0] * plane[0]; shadowMat[1][0] = 0.f - light[0] * plane[1]; shadowMat[2][0] = 0.f - light[0] * plane[2]; shadowMat[3][0] = 0.f - light[0] * plane[3];
    shadowMat[0][1] = 0.f - light[1] * plane[0]; shadowMat[1][1] = dot - light[1] * plane[1]; shadowMat[2][1] = 0.f - light[1] * plane[2]; shadowMat[3][1] = 0.f - light[1] * plane[3];
    shadowMat[0][2] = 0.f - light[2] * plane[0]; shadowMat[1][2] = 0.f - light[2] * plane[1]; shadowMat[2][2] = dot - light[2] * plane[2]; shadowMat[3][2] = 0.f - light[2] * plane[3];
    shadowMat[0][3] = 0.f - light[3] * plane[0]; shadowMat[1][3] = 0.f - light[3] * plane[1]; shadowMat[2][3] = 0.f - light[3] * plane[2]; shadowMat[3][3] = dot - light[3] * plane[3];
}

static void SetupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);

    GLfloat diff[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, g_lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}

// ------------------------------------------------------------
// 공통 복귀
void ReturnToMaze()
{
    std::cout << "\n>>> 감정 경험 종료. 미로로 복귀합니다. <<<\n";

    g_gameState = STATE_PLAYING;
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    camX = savedCamX; camY = savedCamY; camZ = savedCamZ;
    camYaw = savedCamYaw; camPitch = savedCamPitch;
    camVelY = 0.0f; isOnGround = true;

    // 복귀 지점이 벽으로 막혔을 경우를 대비한 안전 처리
    int gx = (int)(camX / CELL_SIZE);
    int gz = (int)(camZ / CELL_SIZE);
    if (gx >= 0 && gx < MAZE_W && gz >= 0 && gz < MAZE_H)
        maze[gz][gx] = PATH;

    glutPostRedisplay();
}

// ------------------------------------------------------------
// 주된 감정 HUD (간단 텍스트)
static void DrawMainEmotionHUD()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winWidth, 0, winHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    auto drawStr = [](float x, float y, const std::string& s)
    {
        glRasterPos2f(x, y);
        for (char c : s) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, c);
    };

    float x = 15.0f;
    float y = winHeight - 20.0f;

    glColor3f(0.9f, 0.9f, 0.9f);
    drawStr(x, y, "Main Emotions");

    y -= 18.0f;

    glColor3f(0.6f, 0.8f, 1.0f);
    drawStr(x, y, std::string("1) Sadness  : ") + (g_mainCollected[EMO_SADNESS] ? "COLLECTED" : "REMAIN"));

    y -= 16.0f;

    glColor3f(1.0f, 0.5f, 0.3f);
    drawStr(x, y, std::string("2) Anger    : ") + (g_mainCollected[EMO_ANGER] ? "COLLECTED" : "REMAIN"));

    y -= 16.0f;

    glColor3f(1.0f, 0.9f, 0.3f);
    drawStr(x, y, std::string("3) Joy      : ") + (g_mainCollected[EMO_JOY] ? "COLLECTED" : "REMAIN"));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ------------------------------------------------------------
// 감정 오브젝트 상호작용
void TryCollectObject()
{
    if (g_gameState != STATE_PLAYING)
        return;

    float px = camX, pz = camZ;
    float r2 = 1.0f;
    int best = -1;

    for (int i = 0; i < (int)g_worldObjects.size(); ++i)
    {
        auto& o = g_worldObjects[i];
        if (o.collected) continue;

        float ox = (o.mazeX + 0.5f) * CELL_SIZE;
        float oz = (o.mazeY + 0.5f) * CELL_SIZE;
        float d2 = (ox - px) * (ox - px) + (oz - pz) * (oz - pz);

        if (d2 <= r2)
        {
            r2 = d2;
            best = i;
        }
    }

    if (best < 0)
        return;

    auto& obj = g_worldObjects[best];

    // 주된 감정을 순서대로만 진행하고 싶다면 아래 조건을 활성화
    // (현재는 배치 자체가 순서형이므로 기본 비활성)
    // if (obj.emotionId != g_textureStage) return;

    obj.collected = true;

    // 수집 상태 기록
    if (obj.emotionId >= 0 && obj.emotionId < EMO_MAIN_COUNT)
        g_mainCollected[obj.emotionId] = true;

    // 벽 텍스처 단계 진행(주된 감정 3개 기준)
    g_textureStage++;
    ChangeWallTexture(g_textureStage);

    // 복귀용 카메라 저장
    savedCamX = camX; savedCamY = camY; savedCamZ = camZ;
    savedCamYaw = camYaw; savedCamPitch = camPitch;

    // 감정별 전용 경험 맵 진입
    if (obj.emotionId == EMO_SADNESS)
    {
        std::cout << "[SADNESS] Enter scene\n";
        g_gameState = STATE_SAD_SCENE;
        g_cutsceneTime = 0.0f;

        camX = 0.0f; camY = 5.0f; camZ = -5.0f;
        camYaw = 3.14159f; camPitch = -0.2f;
        InitStars();
    }
    else if (obj.emotionId == EMO_ANGER)
    {
        std::cout << "[ANGER] Enter scene\n";
        g_gameState = STATE_ANGER_SCENE;
        g_cutsceneTime = 0.0f;

        camX = 0.0f; camY = 2.0f; camZ = -8.0f;
        camYaw = 0.0f; camPitch = 0.0f;

        AngerMap::Enter();
    }
    else if (obj.emotionId == EMO_JOY)
    {
        std::cout << "[JOY] Enter scene\n";
        g_gameState = STATE_JOY_SCENE;
        g_cutsceneTime = 0.0f;

        camX = 0.0f; camY = 5.0f; camZ = 15.0f; 
        camYaw = -1.5708f; camPitch = -0.2f;

        InitFlowers();
    }
    else
    {
        // 파생 감정 등은 추후 확장
        std::cout << "[EMOTION] Collected object with emotionId=" << obj.emotionId << "\n";
    }
}

// ------------------------------------------------------------
// 숫자키 텔레포트(디버그/발표용)
static void TeleportToEmotion(int emotionId)
{
    for (auto& o : g_worldObjects)
    {
        if (o.collected) continue;
        if (o.emotionId != emotionId) continue;

        camX = (o.mazeX + 0.5f) * CELL_SIZE;
        camZ = (o.mazeY + 0.5f) * CELL_SIZE;
        camY = BASE_CAM_Y;
        camYaw = 0.0f; camPitch = 0.0f;
        return;
    }
}

// ------------------------------------------------------------
// 키 입력 라우팅
static void KeyDownHelper(unsigned char k, int x, int y)
{
    // 상호작용 T
    if (k == 't' || k == 'T')
    {
        if (g_gameState == STATE_PLAYING)
        {
            TryCollectObject();
            return;
        }
        // 분노/슬픔/기쁨 경험 중이라도 강제 복귀 허용(발표/디버그)
        if (g_gameState == STATE_ANGER_SCENE && AngerMap::IsCleared())
        {
            AngerMap::Exit();
            ReturnToMaze();
            return;
        }
    }

    // 숫자키 텔포 (미로에서만)
    if (g_gameState == STATE_PLAYING)
    {
        if (k == '1') { TeleportToEmotion(EMO_SADNESS); return; }
        if (k == '2') { TeleportToEmotion(EMO_ANGER);   return; }
        if (k == '3') { TeleportToEmotion(EMO_JOY);     return; }
    }

    KeyboardDown(k, x, y);
}

// ------------------------------------------------------------
// 디스플레이
static void Display()
{
    if (g_gameState == STATE_SAD_SCENE)
    {
        DrawNightScene();
        glutSwapBuffers();
        return;
    }
    else if (g_gameState == STATE_ANGER_SCENE)
    {
        glClearColor(0.02f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60.0, (float)winWidth / winHeight, 0.1, 200.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        float dx = cos(camYaw) * cos(camPitch);
        float dy = sin(camPitch);
        float dz = sin(camYaw) * cos(camPitch);

        gluLookAt(camX, camY, camZ,
                  camX + dx, camY + dy, camZ + dz,
                  0, 1, 0);

        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);

        AngerMap::Render3D();
        AngerMap::Render2D(winWidth, winHeight);

        glutSwapBuffers();
        return;
    }
    else if (g_gameState == STATE_JOY_SCENE)
    {
        DrawJoyScene();
        glutSwapBuffers();
        return;
    }
    else if (g_gameState == STATE_ENDING)
    {
        DrawEndingCredits();
        glutSwapBuffers();
        return;
    }

    // -------------------- 미로 플레이 렌더 --------------------
    glClearColor(0.05f, 0.05f, 0.05f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)winWidth / winHeight, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float dx = cos(camYaw) * cos(camPitch);
    float dy = sin(camPitch);
    float dz = sin(camYaw) * cos(camPitch);

    gluLookAt(camX, camY, camZ,
              camX + dx, camY + dy, camZ + dz,
              0, 1, 0);

    glEnable(GL_TEXTURE_2D);
    DrawMaze3D();
    DrawSORObjects(CELL_SIZE);

    DrawMiniMap();
    DrawMainEmotionHUD();

    glutSwapBuffers();
}

// ------------------------------------------------------------
// 업데이트
static void Idle()
{
    static int lastTime = 0;
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTime) / 1000.0f;
    if (dt > 0.1f) dt = 0.1f;
    lastTime = now;

    // 1) 슬픔 경험
    if (g_gameState == STATE_SAD_SCENE)
    {
        UpdateCamera(dt);
        g_cutsceneTime += dt;
        if (g_cutsceneTime >= CUTSCENE_DURATION)
            ReturnToMaze();
    }
    // 2) 분노 경험
    else if (g_gameState == STATE_ANGER_SCENE)
    {
        UpdateCamera(dt);
        AngerMap::Update(dt);
        g_cutsceneTime += dt;

        // 클리어 즉시 자동 복귀
        if (AngerMap::IsCleared())
        {
            AngerMap::Exit();
            ReturnToMaze();
        }
    }
    // 3) 기쁨 경험
    else if (g_gameState == STATE_JOY_SCENE)
    {
        UpdateCamera(dt);
        g_cutsceneTime += dt;

        if (g_cutsceneTime >= 15.0f)
            ReturnToMaze();
    }
    // 4) 엔딩
    else if (g_gameState == STATE_ENDING)
    {
        g_cutsceneTime += dt;
    }
    // 5) 일반 미로 탐험
    else
    {
        UpdateCamera(dt);
        UpdateJump(dt);

        float exitX_Limit = (MAZE_W - 2.0f) * CELL_SIZE;
        float exitZ_Min = (MAZE_H - 3.0f) * CELL_SIZE;
        float exitZ_Max = (MAZE_H - 1.0f) * CELL_SIZE;

        // 주된 감정 3개 수집 후 탈출 지점 도달 시 엔딩
        if (camX > exitX_Limit &&
            camZ > exitZ_Min && camZ < exitZ_Max &&
            isOnGround && g_textureStage >= 3)
        {
            std::cout << ">>> 미로 탈출 성공! 엔딩 시작 <<<\n";
            g_gameState = STATE_ENDING;
            g_cutsceneTime = 0.0f;
        }
    }

    glutPostRedisplay();
}

static void Reshape(int w, int h)
{
    winWidth = w;
    winHeight = h;
    glViewport(0, 0, w, h);
}

// ------------------------------------------------------------
// 주된 감정 오브젝트 배치
void InitEmotionObjects()
{
    ClearSORObjects();

    int idxSad   = LoadAndRegisterModel("sad.txt");
    int idxAnger = LoadAndRegisterModel("anger.txt");
    int idxJoy   = LoadAndRegisterModel("happy.txt");

    if (idxSad < 0)
    {
        std::cout << "[WARNING] sad.txt not found. Check working directory.\n";
    }
    if (idxAnger < 0) idxAnger = idxSad;
    if (idxJoy < 0)   idxJoy = idxSad;

    // 시작 -> 도착 경로를 찾아 그 위에 순서대로 배치
    struct Node { int x, z; };
    std::vector<Node> path;

    int sx = 1, sz = 1;
    int ex = MAZE_W - 2, ez = MAZE_H - 2;

    static bool visited[MAZE_H][MAZE_W];
    static Node parent[MAZE_H][MAZE_W];

    for (int z = 0; z < MAZE_H; ++z)
        for (int x = 0; x < MAZE_W; ++x)
            visited[z][x] = false;

    std::queue<Node> q;
    q.push({ sx, sz });
    visited[sz][sx] = true;

    int dx4[] = { 0, 0, -1, 1 };
    int dz4[] = { -1, 1, 0, 0 };

    bool found = false;

    while (!q.empty())
    {
        Node c = q.front(); q.pop();
        if (c.x == ex && c.z == ez) { found = true; break; }

        for (int i = 0; i < 4; ++i)
        {
            int nx = c.x + dx4[i];
            int nz = c.z + dz4[i];
            if (nx < 0 || nx >= MAZE_W || nz < 0 || nz >= MAZE_H) continue;
            if (visited[nz][nx]) continue;
            if (maze[nz][nx] != PATH) continue;

            visited[nz][nx] = true;
            parent[nz][nx] = c;
            q.push({ nx, nz });
        }
    }

    if (found)
    {
        Node cur = { ex, ez };
        while (!(cur.x == sx && cur.z == sz))
        {
            path.push_back(cur);
            cur = parent[cur.z][cur.x];
        }
        path.push_back({ sx, sz });
        std::reverse(path.begin(), path.end());
    }

    if (path.size() < 8)
    {
        // 경로가 너무 짧으면 임의 위치 배치
        std::cout << "[WARNING] Path too short. Placing emotions near start.\n";
        AddObjectGrid(idxSad,   3, 3, 2.0f, 0.6f, 0, 1.5f, 0.01f, 0.3f, EMO_SADNESS);
        AddObjectGrid(idxAnger, 5, 3, 2.0f, 0.6f, 0, 1.5f, 0.01f, 0.3f, EMO_ANGER);
        AddObjectGrid(idxJoy,   7, 3, 2.0f, 0.6f, 0, 1.5f, 0.01f, 0.3f, EMO_JOY);
        return;
    }

    // 경로 비율 지점에 순서 배치
    auto pickIndex = [&](float ratio)
    {
        int k = (int)(ratio * (float)path.size());
        if (k < 1) k = 1;
        if (k >= (int)path.size()) k = (int)path.size() - 1;
        return k;
    };

    int k1 = pickIndex(0.25f);
    int k2 = pickIndex(0.50f);
    int k3 = pickIndex(0.75f);

    AddObjectGrid(idxSad,   path[k1].x, path[k1].z, 2.0f, 0.6f, 0, 1.5f, 0.01f, 0.3f, EMO_SADNESS);
    AddObjectGrid(idxAnger, path[k2].x, path[k2].z, 2.0f, 0.6f, 0, 1.5f, 0.01f, 0.3f, EMO_ANGER);
    AddObjectGrid(idxJoy,   path[k3].x, path[k3].z, 2.0f, 0.6f, 0, 1.5f, 0.01f, 0.3f, EMO_JOY);
}

// ------------------------------------------------------------
// 메인
int main(int argc, char** argv)
{
    std::srand((unsigned int)std::time(0));

    GenerateMaze();
    InitEmotionObjects();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("Emotion.exe : AI Robot Emotion Maze");

    SetupLighting();

    GLfloat plane[] = { 0, 1, 0, 0 };
    BuildShadowMatrix((GLfloat(*)[4])g_shadowMatrix, g_lightPos, plane);

    glClearColor(0.05f, 0.05f, 0.05f, 1);
    glEnable(GL_DEPTH_TEST);

    LoadTextures();

    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(KeyDownHelper);
    glutKeyboardUpFunc(KeyboardUp);
    glutSpecialFunc(SpecialDown);
    glutSpecialUpFunc(SpecialUp);
    glutIdleFunc(Idle);

    glutMainLoop();
    return 0;
}