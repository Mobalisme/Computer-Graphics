#include "Global.h"
#include "SOR_Objects.h"
#include "AngerMap.h"

#include <iostream>
#include <ctime>
#include <queue>
#include <algorithm>
#include <cmath>

// -------------------- 모델 설정 --------------------
// 네 요구 반영: 당분간 model_data.txt 하나로 통일
static const bool  USE_SINGLE_MODEL = true;
static const char* MODEL_SINGLE = "model_data.txt";
static const char* MODEL_SAD = "sad.txt";
static const char* MODEL_ANGER = "anger.txt";
static const char* MODEL_JOY = "happy.txt";

// -------------------- 그림자 행렬 --------------------
static void BuildShadowMatrix(GLfloat shadowMat[4][4], const GLfloat light[4], const GLfloat plane[4])
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

// -------------------- 미로 복귀 --------------------
void ReturnToMaze()
{
    g_gameState = STATE_PLAYING;
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    camX = savedCamX; camY = savedCamY; camZ = savedCamZ;
    camYaw = savedCamYaw; camPitch = savedCamPitch;

    camVelY = 0.0f;
    isOnGround = true;

    glutPostRedisplay();
}

// -------------------- 디버그 텔레포트 --------------------
static void TeleportToEmotion(int emotionId)
{
    for (auto& o : g_worldObjects)
    {
        if (o.collected) continue;
        if (o.emotionId != emotionId) continue;

        camX = (o.mazeX + 0.5f) * CELL_SIZE;
        camZ = (o.mazeY + 0.5f) * CELL_SIZE;
        camY = BASE_CAM_Y;

        camVelY = 0.0f;
        isOnGround = true;

        camYaw = 0.0f;
        camPitch = 0.0f;
        return;
    }
}

// -------------------- 감정 오브젝트 배치 --------------------
void InitEmotionObjects()
{
    ClearSORObjects();

    const char* sadPath = USE_SINGLE_MODEL ? MODEL_SINGLE : MODEL_SAD;
    const char* angerPath = USE_SINGLE_MODEL ? MODEL_SINGLE : MODEL_ANGER;
    const char* joyPath = USE_SINGLE_MODEL ? MODEL_SINGLE : MODEL_JOY;

    int idxSad = LoadAndRegisterModel(sadPath);
    int idxAnger = LoadAndRegisterModel(angerPath);
    int idxJoy = LoadAndRegisterModel(joyPath);

    if (idxSad < 0 && idxAnger < 0 && idxJoy < 0)
    {
        std::cout << "[SOR] No model files found. Check working directory.\n";
        return;
    }

    if (idxSad < 0)   idxSad = (idxAnger >= 0 ? idxAnger : idxJoy);
    if (idxAnger < 0) idxAnger = idxSad;
    if (idxJoy < 0)   idxJoy = idxSad;

    // 시작~도착 최단 경로(BFS) 기반 배치
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
        AddObjectGrid(idxSad, 3, 3, 1.2f, 0.6f, 0.0f, 1.5f, 0.01f, 0.2f, EMO_SADNESS);
        AddObjectGrid(idxAnger, 5, 3, 1.2f, 0.6f, 0.0f, 1.5f, 0.01f, 0.2f, EMO_ANGER);
        AddObjectGrid(idxJoy, 7, 3, 1.2f, 0.6f, 0.0f, 1.5f, 0.01f, 0.2f, EMO_JOY);
        return;
    }

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

    AddObjectGrid(idxSad, path[k1].x, path[k1].z, 1.2f, 0.6f, 0.0f, 1.5f, 0.01f, 0.2f, EMO_SADNESS);
    AddObjectGrid(idxAnger, path[k2].x, path[k2].z, 1.2f, 0.6f, 0.0f, 1.5f, 0.01f, 0.2f, EMO_ANGER);
    AddObjectGrid(idxJoy, path[k3].x, path[k3].z, 1.2f, 0.6f, 0.0f, 1.5f, 0.01f, 0.2f, EMO_JOY);
}

// -------------------- 감정 오브젝트 습득/진입 --------------------
void TryCollectObject()
{
    if (g_gameState != STATE_PLAYING) return;

    float px = camX, pz = camZ;
    float bestD2 = 1.0f;
    int best = -1;

    for (int i = 0; i < (int)g_worldObjects.size(); ++i)
    {
        auto& o = g_worldObjects[i];
        if (o.collected) continue;

        float ox = (o.mazeX + 0.5f) * CELL_SIZE;
        float oz = (o.mazeY + 0.5f) * CELL_SIZE;
        float d2 = (ox - px) * (ox - px) + (oz - pz) * (oz - pz);

        if (d2 <= bestD2)
        {
            bestD2 = d2;
            best = i;
        }
    }

    if (best < 0) return;

    auto& obj = g_worldObjects[best];
    obj.collected = true;

    if (obj.emotionId >= 0 && obj.emotionId < EMO_MAIN_COUNT)
        g_mainCollected[obj.emotionId] = true;

    if (g_textureStage < 3)
    {
        g_textureStage++;
        ChangeWallTexture(g_textureStage);
    }

    savedCamX = camX; savedCamY = camY; savedCamZ = camZ;
    savedCamYaw = camYaw; savedCamPitch = camPitch;

    g_cutsceneTime = 0.0f;

    if (obj.emotionId == EMO_SADNESS)
    {
        g_gameState = STATE_SAD_SCENE;

        // 저시점(발목 느낌) + 영상 거리 확보
        camX = 0.0f;
        camY = 0.28f;
        camZ = -2.2f;

        camYaw = 3.14159f;
        camPitch = -0.03f;

        InitStars();
    }
    else if (obj.emotionId == EMO_ANGER)
    {
        g_gameState = STATE_ANGER_SCENE;

        camX = 0.0f; camY = 2.0f; camZ = -8.0f;
        camYaw = 0.0f; camPitch = 0.0f;

        AngerMap::Enter();
    }
    else if (obj.emotionId == EMO_JOY)
    {
        g_gameState = STATE_JOY_SCENE;

        camX = 0.0f; camY = 5.0f; camZ = 15.0f;
        camYaw = -1.5708f; camPitch = -0.2f;

        InitFlowers();
    }
}

// -------------------- 키 입력 라우팅 --------------------
static void KeyDownHelper(unsigned char k, int x, int y)
{
    if (k == 't' || k == 'T')
    {
        TryCollectObject();
        if (g_gameState == STATE_ANGER_SCENE)
            AngerMap::OnKeyDown(k);
        return;
    }

    if (g_gameState == STATE_PLAYING)
    {
        if (k == '1') { TeleportToEmotion(EMO_SADNESS); return; }
        if (k == '2') { TeleportToEmotion(EMO_ANGER);   return; }
        if (k == '3') { TeleportToEmotion(EMO_JOY);     return; }
    }

    if (g_gameState == STATE_ANGER_SCENE)
        AngerMap::OnKeyDown(k);

    KeyboardDown(k, x, y);
}

// -------------------- 디스플레이 --------------------
static void Display()
{
    if (g_gameState == STATE_SAD_SCENE)
    {
        DrawNightScene();
        glutSwapBuffers();
        return;
    }
    if (g_gameState == STATE_ANGER_SCENE)
    {
        glClearColor(0.02f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60.0, (float)winWidth / winHeight, 0.1, 200.0);

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

        AngerMap::Render3D();
        AngerMap::Render2D(winWidth, winHeight);

        glutSwapBuffers();
        return;
    }
    if (g_gameState == STATE_JOY_SCENE)
    {
        DrawJoyScene();
        glutSwapBuffers();
        return;
    }
    if (g_gameState == STATE_ENDING)
    {
        DrawEndingCredits();
        glutSwapBuffers();
        return;
    }

    // -------- 미로 플레이 --------
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)winWidth / winHeight, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float dx = cosf(camYaw) * cosf(camPitch);
    float dy = sinf(camPitch);
    float dz = sinf(camYaw) * cosf(camPitch);

    gluLookAt(camX, camY, camZ,
        camX + dx, camY + dy, camZ + dz,
        0, 1, 0);

    glEnable(GL_TEXTURE_2D);

    DrawMaze3D();
    DrawSORObjects(CELL_SIZE);
    DrawMiniMap();

    glutSwapBuffers();
}

// -------------------- 업데이트 --------------------
static void Idle()
{
    static int lastTime = 0;
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTime) / 1000.0f;
    if (dt > 0.1f) dt = 0.1f;
    lastTime = now;

    if (g_gameState == STATE_SAD_SCENE)
    {
        UpdateCamera(dt);
        g_cutsceneTime += dt;

        if (g_cutsceneTime >= CUTSCENE_DURATION)
            ReturnToMaze();
    }
    else if (g_gameState == STATE_ANGER_SCENE)
    {
        UpdateCamera(dt);
        AngerMap::Update(dt);
        g_cutsceneTime += dt;

        if (AngerMap::IsCleared())
        {
            AngerMap::Exit();
            ReturnToMaze();
        }
    }
    else if (g_gameState == STATE_JOY_SCENE)
    {
        UpdateCamera(dt);
        g_cutsceneTime += dt;

        if (g_cutsceneTime >= CUTSCENE_DURATION)
            ReturnToMaze();
    }
    else if (g_gameState == STATE_ENDING)
    {
        g_cutsceneTime += dt;
    }
    else
    {
        UpdateCamera(dt);
        UpdateJump(dt);

        bool allMain = g_mainCollected[EMO_SADNESS] &&
            g_mainCollected[EMO_ANGER] &&
            g_mainCollected[EMO_JOY];

        float exitX = (MAZE_W - 2.0f) * CELL_SIZE;
        float exitZ = (MAZE_H - 2.0f) * CELL_SIZE;

        float dx = camX - exitX;
        float dz = camZ - exitZ;

        if (allMain && (dx * dx + dz * dz) < 1.0f)
        {
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

// -------------------- main --------------------
int main(int argc, char** argv)
{
    std::srand((unsigned int)std::time(nullptr));

    GenerateMaze();
    InitEmotionObjects();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("Emotion.exe : AI Robot Emotion Maze");

    SetupLighting();

    GLfloat plane[] = { 0, 1, 0, 0 };
    BuildShadowMatrix((GLfloat(*)[4])g_shadowMatrix, g_lightPos, plane);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    LoadTextures();
    AngerMap::Init();

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
