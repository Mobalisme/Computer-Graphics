#include "Global.h"
#include "SOR_Objects.h"
#include <iostream>
#include <ctime>
#include <queue>
#include <algorithm>
#include <string>
#include <cmath>
#include <set>
#include <utility>
#include <ctime>

struct EmotionData {
    int frustrationCount = 0;
    int confusionCount = 0;
    int lonelinessCount = 0;

    bool hasSadness = false;
    bool hasAnger = false;
    bool hasHappiness = false;
};

EmotionData g_emotionData;

// 외부 함수들
void GenerateMaze();
void DrawMaze3D();
void DrawMiniMap();
void DrawNightScene();
void DrawJoyScene();
void DrawAngerScene();          // ★ [NEW] 분노 컷신 씬
void DrawEndingCredits();       // ★ 엔딩 크레딧
void LoadTextures();
void InitStars();
void UpdateCamera(float dt);
void UpdateJump(float dt);

// 모델러 관련(이미 쓰고 있으니 선언 유지)
void DrawModeler();
void InitModeler();
void HandleModelerKeyboard(unsigned char k, int x, int y);
void HandleModelerMouse(int button, int state, int x, int y);
void HandleModelerMotion(int x, int y);

// 감정 오브젝트 초기화 (SOR_Modeler.cpp에서 Start Game 시 호출)
void InitEmotionObjects();

// --------------------------------------------

void BuildShadowMatrix(GLfloat shadowMat[4][4], GLfloat light[4], GLfloat plane[4]) {
    GLfloat dot = plane[0] * light[0] + plane[1] * light[1] + plane[2] * light[2] + plane[3] * light[3];
    shadowMat[0][0] = dot - light[0] * plane[0]; shadowMat[1][0] = 0.f - light[0] * plane[1]; shadowMat[2][0] = 0.f - light[0] * plane[2]; shadowMat[3][0] = 0.f - light[0] * plane[3];
    shadowMat[0][1] = 0.f - light[1] * plane[0]; shadowMat[1][1] = dot - light[1] * plane[1]; shadowMat[2][1] = 0.f - light[1] * plane[2]; shadowMat[3][1] = 0.f - light[1] * plane[3];
    shadowMat[0][2] = 0.f - light[2] * plane[0]; shadowMat[1][2] = 0.f - light[2] * plane[1]; shadowMat[2][2] = dot - light[2] * plane[2]; shadowMat[3][2] = 0.f - light[2] * plane[3];
    shadowMat[0][3] = 0.f - light[3] * plane[0]; shadowMat[1][3] = 0.f - light[3] * plane[1]; shadowMat[2][3] = 0.f - light[3] * plane[2]; shadowMat[3][3] = dot - light[3] * plane[3];
}

void SetupLighting() {
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_NORMALIZE);

    // ★ 외로움 상태면 빛 약하게
    float bright = (g_darknessTimer > 0) ? 0.1f : 0.8f;
    GLfloat diff[] = { bright, bright, bright, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, g_lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);

    // ★ 외로움 상태면 검은 안개
    if (g_darknessTimer > 0) {
        glEnable(GL_FOG);
        GLfloat fogColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogi(GL_FOG_MODE, GL_EXP2);
        glFogf(GL_FOG_DENSITY, 0.3f);
        glHint(GL_FOG_HINT, GL_NICEST);
    }
    else {
        glDisable(GL_FOG);
    }

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}

// 함정 및 골렘 충돌 처리
void CheckTrapCollision() {
    float px = camX, pz = camZ;
    float detectionRange = 1.5f;

    for (int i = 0; i < (int)g_worldObjects.size(); ++i) {
        if (g_worldObjects[i].collected) continue;
        if (g_worldObjects[i].type == 0) continue; // 메인 감정은 'R'키로 획득

        float ox = (g_worldObjects[i].mazeX + 0.5f) * CELL_SIZE;
        float oz = (g_worldObjects[i].mazeY + 0.5f) * CELL_SIZE;

        float dx = px - ox;
        float dz = pz - oz;
        float dist = sqrt(dx * dx + dz * dz);

        if (dist <= detectionRange) {
            int t = g_worldObjects[i].type;

            // [골렘 Type 99] : 길막
            if (t == 99) {
                // 3개 감정 모두 모았으면 통과
                if (g_emotionData.hasSadness && g_emotionData.hasAnger && g_emotionData.hasHappiness) {
                    g_worldObjects[i].collected = true;
                    g_uiMessage = "GATE OPENED!";
                    g_uiMessageTimer = 3.0f;
                }
                else {
                    // 감정 부족하면 밀어내기
                    float bodySize = 0.6f * CELL_SIZE;
                    if (abs(dx) < bodySize && abs(dz) < bodySize) {
                        float pushOut = bodySize + 0.05f;
                        if (abs(dx) > abs(dz)) {
                            camX = (dx > 0) ? ox + pushOut : ox - pushOut;
                        }
                        else {
                            camZ = (dz > 0) ? oz + pushOut : oz - pushOut;
                        }
                    }
                    g_uiMessage = "COLLECT ALL 3 EMOTIONS!";
                    g_uiMessageTimer = 0.1f;
                }
                continue;
            }

            // [함정 Type 1~3]
            g_worldObjects[i].collected = true;
            if (t == 1) { // 좌절
                g_slowTimer = 5.0f;
                g_uiMessage = "Frustration (Slow)!";
            }
            else if (t == 2) { // 혼란
                g_confusionTimer = 5.0f;
                g_uiMessage = "Confusion (Dizzy)!";
            }
            else if (t == 3) { // 외로움
                g_darknessTimer = 5.0f;
                g_uiMessage = "Loneliness (Darkness)!";
            }
            g_uiMessageTimer = 2.0f;
        }
    }
}

void ReturnToMaze() {
    std::cout << "\n>>> 컷신 종료. 복귀합니다. <<<\n";
    g_gameState = STATE_PLAYING;
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    camX = savedCamX; camY = savedCamY; camZ = savedCamZ;
    camYaw = savedCamYaw; camPitch = savedCamPitch;
    camVelY = 0.0f; isOnGround = true;

    int gx = (int)(camX / CELL_SIZE), gz = (int)(camZ / CELL_SIZE);
    if (gx >= 0 && gx < MAZE_W && gz >= 0 && gz < MAZE_H) maze[gz][gx] = PATH;
    glutPostRedisplay();
}

void TryCollectObject() {
    float px = camX, pz = camZ;
    float r2 = 2.0f;
    int best = -1;

    for (int i = 0; i < (int)g_worldObjects.size(); ++i) {
        if (g_worldObjects[i].collected) continue;
        if (g_worldObjects[i].type != 0) continue; // 메인 감정만

        float ox = (g_worldObjects[i].mazeX + 0.5f) * CELL_SIZE;
        float oz = (g_worldObjects[i].mazeY + 0.5f) * CELL_SIZE;
        float d2 = (ox - px) * (ox - px) + (oz - pz) * (oz - pz);
        if (d2 <= r2) { r2 = d2; best = i; }
    }

    if (best != -1) {
        g_worldObjects[best].collected = true;

        // 감정 획득 체크 (순서: Sad -> Anger -> Happiness)
        if (!g_emotionData.hasSadness) g_emotionData.hasSadness = true;
        else if (!g_emotionData.hasAnger) g_emotionData.hasAnger = true;
        else g_emotionData.hasHappiness = true;

        // 벽 색상 변경
        g_textureStage++;
        ChangeWallTexture(g_textureStage);

        // 컷신 진입 전 카메라 저장
        savedCamX = camX; savedCamY = camY; savedCamZ = camZ;
        savedCamYaw = camYaw; savedCamPitch = camPitch;

        // ----------------------------
        // ★ 1) 슬픔 컷신
        // ----------------------------
        if (g_textureStage == 1) {
            std::cout << ">> 슬픔 컷신\n";
            g_gameState = STATE_CUTSCENE;
            g_cutsceneTime = 0.0f;

            camX = 0.0f;
            camY = 5.0f;
            camZ = -5.0f;
            camYaw = 1.5708f;   // +Z 방향
            camPitch = 0.25f;

            InitStars();
        }
        // ----------------------------
        // ★ 2) 분노 컷신 (용암 셰이더)
        // ----------------------------
        else if (g_textureStage == 2) {
            std::cout << ">> 분노 컷신\n";
            g_gameState = STATE_ANGER_SCENE;   // ★ [NEW]
            g_cutsceneTime = 0.0f;

            // 카메라를 동일한 '영상/연출 방향' 기준으로 유지
            camX = 0.0f;
            camY = 5.0f;
            camZ = -5.0f;
            camYaw = 1.5708f;
            camPitch = 0.20f;
        }
        // ----------------------------
        // ★ 3) 기쁨 컷신
        // ----------------------------
        else if (g_textureStage == 3) {
            std::cout << ">> 기쁨 컷신\n";
            g_gameState = STATE_JOY_SCENE;
            g_cutsceneTime = 0.0f;

            camX = 0.0f; camY = 5.0f; camZ = 15.0f;
            camYaw = -1.57f; camPitch = -0.2f;
        }
        // ----------------------------
        // ★ 예외 안전장치
        // ----------------------------
        else {
            g_gameState = STATE_CUTSCENE;
            g_cutsceneTime = 0.0f;

            camX = 0.0f;
            camY = 5.0f;
            camZ = -5.0f;
            camYaw = 1.5708f;
            camPitch = 0.25f;

            InitStars();
        }
    }
}

void DrawGolem() {
    int endX = MAZE_W - 2;
    int endY = MAZE_H - 2;

    for (const auto& obj : g_worldObjects) {
        if (obj.type == 99 && !obj.collected) {
            float gx = (endX + 0.5f) * CELL_SIZE;
            float gz = (endY + 0.5f) * CELL_SIZE;

            glPushMatrix();
            glTranslatef(gx, 0.0f, gz);

            float dx = camX - gx, dz = camZ - gz;
            float angle = atan2(dx, dz) * 180.0f / 3.141592f;
            glRotatef(angle, 0, 1, 0);

            glScalef(0.4f, 0.4f, 0.4f);
            glTranslatef(0.0f, -0.6f, 0.0f);

            glColor3f(0.85f, 0.85f, 0.85f);
            glPushMatrix(); glTranslatef(-0.4f, 0.75f, 0.0f); glScalef(0.6f, 1.5f, 0.6f); glutSolidCube(1.0f); glPopMatrix();
            glPushMatrix(); glTranslatef(0.4f, 0.75f, 0.0f); glScalef(0.6f, 1.5f, 0.6f); glutSolidCube(1.0f); glPopMatrix();
            glPushMatrix(); glTranslatef(0.0f, 2.0f, 0.0f); glScalef(1.0f, 1.2f, 0.7f); glutSolidCube(1.0f); glPopMatrix();
            glPushMatrix(); glTranslatef(0.0f, 2.8f, 0.0f); glScalef(1.8f, 0.8f, 0.9f); glutSolidCube(1.0f); glPopMatrix();
            glPushMatrix(); glTranslatef(-1.1f, 2.0f, 0.0f); glScalef(0.5f, 2.2f, 0.6f); glutSolidCube(1.0f); glPopMatrix();
            glPushMatrix(); glTranslatef(1.1f, 2.0f, 0.0f); glScalef(0.5f, 2.2f, 0.6f); glutSolidCube(1.0f); glPopMatrix();
            glPushMatrix(); glTranslatef(0.0f, 3.5f, -0.1f); glScalef(0.7f, 0.8f, 0.7f); glutSolidCube(1.0f); glPopMatrix();

            glColor3f(0.8f, 0.0f, 0.0f);
            glPushMatrix(); glTranslatef(-0.2f, 3.6f, 0.26f); glScalef(0.15f, 0.1f, 0.1f); glutSolidCube(1.0f); glPopMatrix();
            glPushMatrix(); glTranslatef(0.2f, 3.6f, 0.26f); glScalef(0.15f, 0.1f, 0.1f); glutSolidCube(1.0f); glPopMatrix();

            glPopMatrix();
            break;
        }
    }
}

// 화면 UI 메시지
void DrawUIMessage() {
    if (g_uiMessageTimer <= 0) return;
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, winWidth, 0, winHeight);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);

    float blink = sinf(glutGet(GLUT_ELAPSED_TIME) * 0.01f);
    if (blink > 0) glColor3f(1.0f, 0.0f, 0.0f); else glColor3f(1.0f, 1.0f, 0.0f);

    glRasterPos2f(winWidth / 2 - 50, winHeight - 100);
    for (char c : g_uiMessage) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);

    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING); glEnable(GL_TEXTURE_2D);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

// -----------------------------------------------------
// 탈출 경로 기반 오브젝트 배치 로직 (네 코드 그대로)
// -----------------------------------------------------
static std::vector<std::pair<int, int>> ComputeEscapePath()
{
    const int sx = 1, sy = 1;
    const int ex = MAZE_W - 2, ey = MAZE_H - 2;

    std::queue<std::pair<int, int>> q;
    bool visited[MAZE_H][MAZE_W] = {};
    std::pair<int, int> parent[MAZE_H][MAZE_W];

    for (int y = 0; y < MAZE_H; ++y)
        for (int x = 0; x < MAZE_W; ++x)
            parent[y][x] = { -1, -1 };

    if (maze[sy][sx] != PATH || maze[ey][ex] != PATH)
        return {};

    visited[sy][sx] = true;
    q.push({ sx, sy });

    const int dx[4] = { 1, -1, 0, 0 };
    const int dy[4] = { 0, 0, 1, -1 };

    while (!q.empty()) {
        auto p = q.front(); q.pop();
        int x = p.first;
        int y = p.second;

        if (x == ex && y == ey) break;

        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i], ny = y + dy[i];
            if (nx < 0 || nx >= MAZE_W || ny < 0 || ny >= MAZE_H) continue;
            if (visited[ny][nx]) continue;
            if (maze[ny][nx] != PATH) continue;

            visited[ny][nx] = true;
            parent[ny][nx] = { x, y };
            q.push({ nx, ny });
        }
    }

    if (!visited[ey][ex])
        return {};

    std::vector<std::pair<int, int>> path;
    int cx = ex, cy = ey;
    while (!(cx == sx && cy == sy)) {
        path.push_back({ cx, cy });
        auto p = parent[cy][cx];
        if (p.first == -1) break;
        cx = p.first; cy = p.second;
    }
    path.push_back({ sx, sy });

    std::reverse(path.begin(), path.end());
    return path;
}

void InitEmotionObjects() {
    g_worldObjects.clear();

    int idxUser = LoadAndRegisterModel("user_model.txt");
    int idxSad = LoadAndRegisterModel("sad.txt");
    int idxAnger = LoadAndRegisterModel("anger.txt");
    int idxHappy = LoadAndRegisterModel("happy.txt");
    int idxTrap = LoadAndRegisterModel("trap.txt");

    if (idxSad < 0) idxSad = 0;
    if (idxAnger < 0) idxAnger = 0;
    if (idxHappy < 0) idxHappy = 0;
    if (idxTrap < 0) idxTrap = 0;

    if (idxUser < 0) {
        std::cout << "[SYSTEM] User model not found. Using default.\n";
        idxUser = (idxSad >= 0) ? idxSad : 0;
    }

    std::vector<std::pair<int, int>> escapePath = ComputeEscapePath();

    if (escapePath.size() < 5) {
        for (int i = 0; i < 3; ++i) {
            int tx, ty;
            do {
                tx = rand() % (MAZE_W - 2) + 1;
                ty = rand() % (MAZE_H - 2) + 1;
            } while (maze[ty][tx] != PATH);
            int emotions[3] = { idxUser, idxAnger, idxHappy };
            AddObjectGrid(emotions[i], tx, ty, 0.2f, 0.2f, 0);
        }

        int trapTotal = (rand() % 6) + 1;
        for (int i = 0; i < trapTotal; ++i) {
            int tx, ty;
            do {
                tx = rand() % (MAZE_W - 2) + 1;
                ty = rand() % (MAZE_H - 2) + 1;
            } while (maze[ty][tx] != PATH);
            int trapType = (rand() % 3) + 1;
            AddObjectGrid(idxTrap, tx, ty, 1.0f, 0.2f, trapType);
        }

        AddObjectGrid(0, MAZE_W - 2, MAZE_H - 2, 0.0f, 0.0f, 99);
        return;
    }

    std::vector<std::pair<int, int>> candidates;
    for (size_t i = 1; i + 1 < escapePath.size(); ++i) {
        candidates.push_back(escapePath[i]);
    }

    std::shuffle(candidates.begin(), candidates.end(),
        std::mt19937{ (unsigned)std::time(nullptr) });

    int emotions[3] = { idxUser, idxAnger, idxHappy };

    std::set<long long> used;
    auto key = [](int x, int y) -> long long {
        return (long long)y * 1000LL + x;
        };

    for (int i = 0; i < 3; ++i) {
        int tx = -1, ty = -1;

        if (i < (int)candidates.size()) {
            tx = candidates[i].first;
            ty = candidates[i].second;
        }
        else {
            do {
                tx = rand() % (MAZE_W - 2) + 1;
                ty = rand() % (MAZE_H - 2) + 1;
            } while (maze[ty][tx] != PATH);
        }

        used.insert(key(tx, ty));
        AddObjectGrid(emotions[i], tx, ty, 0.2f, 0.2f, 0);
    }

    int trapTotal = (rand() % 6) + 1;

    for (int i = 0; i < trapTotal; ++i) {
        int tx, ty;

        int guard = 0;
        do {
            tx = rand() % (MAZE_W - 2) + 1;
            ty = rand() % (MAZE_H - 2) + 1;
            if (++guard > 5000) break;
        } while (maze[ty][tx] != PATH || used.count(key(tx, ty)) > 0);

        int trapType = (rand() % 3) + 1;
        AddObjectGrid(idxTrap, tx, ty, 1.0f, 0.2f, trapType);

        used.insert(key(tx, ty));
    }

    AddObjectGrid(0, MAZE_W - 2, MAZE_H - 2, 0.0f, 0.0f, 99);
}

// -----------------------------------------------------

void Display() {

    if (g_gameState == STATE_MODELER) {
        DrawModeler();
        glutSwapBuffers();
        return;
    }

    if (g_gameState == STATE_CUTSCENE) {
        DrawNightScene();
        glutSwapBuffers();
        return;
    }

    if (g_gameState == STATE_ANGER_SCENE) {   // ★ [NEW]
        DrawAngerScene();
        glutSwapBuffers();
        return;
    }

    if (g_gameState == STATE_JOY_SCENE) {
        DrawJoyScene();
        glutSwapBuffers();
        return;
    }

    if (g_gameState == STATE_ENDING) {
        DrawEndingCredits();
        glutSwapBuffers();
        return;
    }

    SetupLighting();
    glClearColor(0.05f, 0.05f, 0.05f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0, (float)winWidth / winHeight, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    float dx = cos(camYaw) * cos(camPitch), dy = sin(camPitch), dz = sin(camYaw) * cos(camPitch);
    gluLookAt(camX, camY, camZ, camX + dx, camY + dy, camZ + dz, 0, 1, 0);

    glEnable(GL_TEXTURE_2D);
    DrawMaze3D();
    DrawSORObjects(CELL_SIZE);
    DrawGolem();
    DrawMiniMap();
    DrawUIMessage();
    glutSwapBuffers();
}

void KeyDownHelper(unsigned char k, int x, int y) {
    if (g_gameState == STATE_MODELER) {
        HandleModelerKeyboard(k, x, y);
    }
    else {
        if (k == 'r' || k == 'R') TryCollectObject();
        KeyboardDown(k, x, y);
    }
    glutPostRedisplay();
}

// 마우스 콜백
void MouseHelper(int button, int state, int x, int y) {
    if (g_gameState == STATE_MODELER) {
        HandleModelerMouse(button, state, x, y);
    }
}
void MotionHelper(int x, int y) {
    if (g_gameState == STATE_MODELER) {
        HandleModelerMotion(x, y);
    }
}

void Idle() {
    if (g_gameState == STATE_MODELER) return;

    static int lastTime = 0;
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTime) / 1000.0f;
    if (dt > 0.1f) dt = 0.1f;
    lastTime = now;

    if (g_slowTimer > 0) g_slowTimer -= dt;
    if (g_confusionTimer > 0) g_confusionTimer -= dt;
    if (g_darknessTimer > 0) g_darknessTimer -= dt;
    if (g_uiMessageTimer > 0) g_uiMessageTimer -= dt;

    CheckTrapCollision();

    if (g_gameState == STATE_CUTSCENE) {
        UpdateCamera(dt);
        g_cutsceneTime += dt;
        if (g_cutsceneTime >= CUTSCENE_DURATION) ReturnToMaze();
    }
    else if (g_gameState == STATE_ANGER_SCENE) {   // ★ [NEW]
        UpdateCamera(dt);
        g_cutsceneTime += dt;
        if (g_cutsceneTime >= CUTSCENE_DURATION) ReturnToMaze();
    }
    else if (g_gameState == STATE_JOY_SCENE) {
        UpdateCamera(dt);
        g_cutsceneTime += dt;
        if (g_cutsceneTime >= 15.0f) ReturnToMaze();
    }
    else if (g_gameState == STATE_ENDING) {
        g_cutsceneTime += dt;
    }
    else {
        UpdateCamera(dt);
        UpdateJump(dt);

        float exitX_Limit = (MAZE_W - 2.0f) * CELL_SIZE;
        float exitZ_Min = (MAZE_H - 3.0f) * CELL_SIZE;
        float exitZ_Max = (MAZE_H - 1.0f) * CELL_SIZE;

        if (camX > exitX_Limit && camZ > exitZ_Min && camZ < exitZ_Max && isOnGround && g_textureStage >= 3) {
            g_gameState = STATE_ENDING;
            g_cutsceneTime = 0.0f;
        }
    }

    glutPostRedisplay();
}

void Reshape(int w, int h) {
    winWidth = w; winHeight = h;
    glViewport(0, 0, w, h);
}

int main(int argc, char** argv) {
    GenerateMaze();
    std::srand((unsigned int)std::time(0));

    // 시작 상태를 모델러로
    g_gameState = STATE_MODELER;
    InitModeler();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("Emotion Maze: Create Your Emotion");

    SetupLighting();
    GLfloat plane[] = { 0, 1, 0, 0 };
    BuildShadowMatrix((GLfloat(*)[4])g_shadowMatrix, g_lightPos, plane);

    glClearColor(0.05f, 0.05f, 0.05f, 1);
    glEnable(GL_DEPTH_TEST);

    LoadTextures();  // ★ Lava 텍스처 + Ocean + Video 포함

    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(KeyDownHelper);
    glutKeyboardUpFunc(KeyboardUp);
    glutSpecialFunc(SpecialDown);
    glutSpecialUpFunc(SpecialUp);

    glutMouseFunc(MouseHelper);
    glutMotionFunc(MotionHelper);

    glutIdleFunc(Idle);

    glutMainLoop();
    return 0;
}
