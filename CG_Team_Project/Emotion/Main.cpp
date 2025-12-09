#include "Global.h"
#include "SOR_Objects.h"
#include <iostream>
#include <ctime>
#include <queue>
#include <algorithm>
#include <string> 
#include <cmath>


struct EmotionData {
    // 작은 감정
    int frustrationCount = 0;
    int confusionCount = 0;
    int lonelinessCount = 0;

    // 큰 감정
    bool hasSadness = false;
    bool hasAnger = false;
    bool hasHappiness = false;
};

EmotionData g_emotionData; // 변수 이름도 겹치지 않게 변경!

// 외부 함수 선언
void GenerateMaze();
// InitEmotionObjects는 이 파일에 구현함
void DrawMaze3D();
void DrawMiniMap();
void DrawNightScene();
void LoadTextures();
void InitStars();
void UpdateCamera(float dt);
void UpdateJump(float dt);
void KeyboardDown(unsigned char k, int, int);
void KeyboardUp(unsigned char k, int, int);
void SpecialDown(int k, int, int);
void SpecialUp(int k, int, int);

// 그림자 행렬 생성 함수
void BuildShadowMatrix(GLfloat shadowMat[4][4], GLfloat light[4], GLfloat plane[4]) {
    GLfloat dot = plane[0] * light[0] + plane[1] * light[1] + plane[2] * light[2] + plane[3] * light[3];
    shadowMat[0][0] = dot - light[0] * plane[0]; shadowMat[1][0] = 0.f - light[0] * plane[1]; shadowMat[2][0] = 0.f - light[0] * plane[2]; shadowMat[3][0] = 0.f - light[0] * plane[3];
    shadowMat[0][1] = 0.f - light[1] * plane[0]; shadowMat[1][1] = dot - light[1] * plane[1]; shadowMat[2][1] = 0.f - light[1] * plane[2]; shadowMat[3][1] = 0.f - light[1] * plane[3];
    shadowMat[0][2] = 0.f - light[2] * plane[0]; shadowMat[1][2] = 0.f - light[2] * plane[1]; shadowMat[2][2] = dot - light[2] * plane[2]; shadowMat[3][2] = 0.f - light[2] * plane[3];
    shadowMat[0][3] = 0.f - light[3] * plane[0]; shadowMat[1][3] = 0.f - light[3] * plane[1]; shadowMat[2][3] = 0.f - light[3] * plane[2]; shadowMat[3][3] = dot - light[3] * plane[3];
}

void SetupLighting() {
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_NORMALIZE);

    // ★ [추가된 부분] 외로움(Darkness) 상태면 빛을 아주 약하게(0.2), 아니면 밝게(0.8)
    float bright = (g_darknessTimer > 0) ? 0.05f : 0.8f;

    // 조명 색상에 bright 변수 적용
    GLfloat diff[] = { bright, bright, bright, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, g_lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);

    // ★ [새로 추가된 부분] 검은 안개(Fog) 효과
    if (g_darknessTimer > 0) {
        glEnable(GL_FOG); // 안개 켜기

        // 안개 색상을 검은색(0,0,0)으로 설정
        GLfloat fogColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glFogfv(GL_FOG_COLOR, fogColor);

        // 안개 농도 설정
        glFogi(GL_FOG_MODE, GL_EXP2); // 자연스럽게 진해지는 모드
        glFogf(GL_FOG_DENSITY, 0.3f); // ★ 이 숫자가 클수록 앞이 안 보입니다 (0.3 ~ 0.5 추천)

        glHint(GL_FOG_HINT, GL_NICEST); // 가장 좋은 품질로 렌더링
    }
    else {
        glDisable(GL_FOG); // 평소에는 안개 끄기
    }

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}

void ReturnToMaze() {
    std::cout << "\n>>> 컷신 종료. 복귀합니다. <<<\n";
    g_gameState = STATE_PLAYING;
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    // 위치 복구
    camX = savedCamX; camY = savedCamY; camZ = savedCamZ;
    camYaw = savedCamYaw; camPitch = savedCamPitch;
    camVelY = 0.0f; isOnGround = true;

    // 복귀 위치 벽 뚫기 (안전장치)
    int gx = (int)(camX / CELL_SIZE), gz = (int)(camZ / CELL_SIZE);
    if (gx >= 0 && gx < MAZE_W && gz >= 0 && gz < MAZE_H) maze[gz][gx] = PATH;

    glutPostRedisplay();
}

// [수정된 함수 1] 자동으로 부딪혔을 때 처리 (함정, 결계)
void CheckTrapCollision() {
    float px = camX, pz = camZ;

    // 1. 전체 감지 범위 (넉넉하게 잡습니다)
    // 이 범위 안에 들어와야 정밀 검사를 시작합니다.
    float detectionRange = 1.5f;

    for (int i = 0; i < (int)g_worldObjects.size(); ++i) {
        if (g_worldObjects[i].collected) continue;
        if (g_worldObjects[i].type == 0) continue;

        float ox = (g_worldObjects[i].mazeX + 0.5f) * CELL_SIZE;
        float oz = (g_worldObjects[i].mazeY + 0.5f) * CELL_SIZE;

        float dx = px - ox;
        float dz = pz - oz;
        float dist = sqrt(dx * dx + dz * dz); // 원형 거리

        // 일단 근처에 있는지 확인
        if (dist <= detectionRange) {
            int t = g_worldObjects[i].type;

            // ====================================================
            // [골렘(Type 99) : 끈적임 없는 "상자형" 충돌]
            // ====================================================
            if (t == 99) {
                // (1) 증명 완료 시 통과
                if (g_emotionData.hasSadness && g_emotionData.hasAnger && g_emotionData.hasHappiness) {
                    std::cout << ">> 증명 완료! 골렘이 사라집니다.\n";
                    g_worldObjects[i].collected = true;
                    g_uiMessage = "GATE OPENED!";
                    g_uiMessageTimer = 3.0f;
                }
                else {
                    // (2) 충돌 물리 처리 (핵심!)

                    // 골렘의 '몸집' 크기 (반지름) : 0.6 칸
                    float bodySize = 0.6f * CELL_SIZE;

                    // 플레이어가 골렘의 몸집 '안'으로 들어왔는지 네모나게 검사 (AABB)
                    // 절대값(abs)을 써서 X축, Z축 각각 검사합니다.
                    if (abs(dx) < bodySize && abs(dz) < bodySize) {

                        // ★ 들어왔다면 "몸집보다 아주 조금 더 밖"으로 밀어냅니다.
                        // 0.61로 밀어내면, 0.6보다 크기 때문에 다음 검사에서 걸리지 않습니다!
                        float pushOut = bodySize + 0.05f;

                        // 더 깊이 들어온 축(Axis)을 기준으로 밀어냄
                        if (abs(dx) > abs(dz)) {
                            // X축 방향으로 밖으로 밀기
                            if (dx > 0) camX = ox + pushOut; // 오른쪽 밖으로
                            else        camX = ox - pushOut; // 왼쪽 밖으로
                        }
                        else {
                            // Z축 방향으로 밖으로 밀기
                            if (dz > 0) camZ = oz + pushOut; // 아래쪽 밖으로
                            else        camZ = oz - pushOut; // 위쪽 밖으로
                        }
                    }

                    // 메시지는 근처에만 있어도 띄움
                    g_uiMessage = "PROVE YOUR EMOTIONS";
                    g_uiMessageTimer = 0.1f;
                }
                continue;
            }

            // ====================================================
            // [함정 로직] (변경 없음)
            // ====================================================
            g_worldObjects[i].collected = true;

            if (t == 1) { // 좌절
                g_emotionData.frustrationCount++;
                g_slowTimer = 5.0f;
                g_uiMessage = "Frustration!";
                g_uiMessageTimer = 2.0f;
            }
            else if (t == 2) { // 혼란
                g_emotionData.confusionCount++;
                g_confusionTimer = 5.0f;
                g_uiMessage = "Confusion!";
                g_uiMessageTimer = 2.0f;
            }
            else if (t == 3) { // 외로움
                g_emotionData.lonelinessCount++;
                g_darknessTimer = 5.0f;
                g_uiMessage = "Loneliness!";
                g_uiMessageTimer = 2.0f;
            }
        }
    }
}

// [수정된 함수 2] 'R' 키를 눌렀을 때 처리 (큰 감정 획득)
// [수정된 함수] 'R' 키를 눌렀을 때 처리 (큰 감정 획득 + 벽지 변경)
void TryCollectObject() {
    float px = camX, pz = camZ;
    float r2 = 2.0f; // 상호작용 범위
    int best = -1;

    // 가장 가까운 오브젝트 찾기
    for (int i = 0; i < (int)g_worldObjects.size(); ++i) {
        if (g_worldObjects[i].collected) continue;

        // Type 0인 것(큰 감정)만 찾습니다.
        if (g_worldObjects[i].type != 0) continue;

        float ox = (g_worldObjects[i].mazeX + 0.5f) * CELL_SIZE;
        float oz = (g_worldObjects[i].mazeY + 0.5f) * CELL_SIZE;
        float d2 = (ox - px) * (ox - px) + (oz - pz) * (oz - pz);
        if (d2 <= r2) { r2 = d2; best = i; }
    }

    if (best != -1) {
        // 큰 감정 획득!
        g_worldObjects[best].collected = true;
        std::cout << "감정 습득! 컷신 시작.\n";

        // 감정 상태 업데이트
        if (!g_emotionData.hasSadness) g_emotionData.hasSadness = true;
        else if (!g_emotionData.hasAnger) g_emotionData.hasAnger = true;
        else g_emotionData.hasHappiness = true;

        // =========================================================
        // ★ [복구된 부분] 벽 텍스처(색깔) 변경 코드
        // =========================================================
        g_textureStage++;              // 단계 증가
        ChangeWallTexture(g_textureStage); // ★ 주석 해제! 이제 벽 색이 바뀝니다.

        // 컷신 시작 로직
        savedCamX = camX; savedCamY = camY; savedCamZ = camZ;
        savedCamYaw = camYaw; savedCamPitch = camPitch;

        g_gameState = STATE_CUTSCENE;
        g_cutsceneTime = 0.0f;

        // 카메라 컷신 위치로 이동
        camX = 0.0f; camY = 5.0f; camZ = -5.0f;
        camYaw = 3.14159f; camPitch = -0.2f;
        InitStars();
    }
}

void KeyDownHelper(unsigned char k, int x, int y) {
    if (k == 'r' || k == 'R') TryCollectObject();
    KeyboardDown(k, x, y);
}

// [새로 추가/교체] 키를 뗄 때 호출되는 함수
void KeyboardUp(unsigned char k, int x, int y) {
    // 1. 해당 키 끄기
    keyDown[k] = false;

    // 2. ★ [핵심] Shift 키 때문에 꼬이지 않도록 대소문자 둘 다 강제로 끄기
    if (k >= 'a' && k <= 'z') {
        // 소문자를 뗐으면, 대문자도 같이 끔 (예: 'w' 떼면 'W'도 끔)
        keyDown[k - 32] = false; 
    }
    else if (k >= 'A' && k <= 'Z') {
        // 대문자를 뗐으면, 소문자도 같이 끔 (예: 'W' 떼면 'w'도 끔)
        keyDown[k + 32] = false; 
    }
}

// [새로 추가] 화면에 메시지를 띄우는 함수
void DrawUIMessage() {
    // 시간이 다 됐거나 메시지가 없으면 그리지 않음
    if (g_uiMessageTimer <= 0) return;

    // 2D 화면 모드로 전환
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, winWidth, 0, winHeight);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);

    // 글자 색상 (깜빡이는 효과)
    float blink = sinf(glutGet(GLUT_ELAPSED_TIME) * 0.01f);
    if (blink > 0) glColor3f(1.0f, 0.0f, 0.0f); // 빨강
    else glColor3f(1.0f, 1.0f, 0.0f);           // 노랑

    // 글자 위치 설정 (우측 상단, 미니맵 아래쪽)
    float textX = winWidth - 280;
    float textY = winHeight - 230;

    glRasterPos2f(textX, textY);

    // 글자 한 자 한 자 출력
    for (char c : g_uiMessage) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // 원래 설정 복구
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING); glEnable(GL_TEXTURE_2D);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

// 투명한 하늘색 결계를 그리는 함수
void DrawBarrier() {
    // 미로 도착점 좌표 (InitEmotionObjects에서 넣은 위치와 같아야 함)
    float bx = (MAZE_W - 2 + 0.5f) * CELL_SIZE;
    float bz = (MAZE_H - 2 + 0.5f) * CELL_SIZE;

    // 만약 이미 결계가 해제(collected)되었다면 그리지 않음
    // (결계 오브젝트를 찾아서 상태 확인)
    for (auto& obj : g_worldObjects) {
        if (obj.type == 99 && obj.collected) return;
    }

    glPushMatrix();
    glTranslatef(bx, 0.0f, bz); // 위치 이동

    // ★ 투명 처리를 위한 블렌딩 활성화
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 하늘색 (R:0.5, G:0.8, B:1.0), 투명도(Alpha): 0.5
    glColor4f(0.5f, 0.8f, 1.0f, 0.5f);

    // 결계 모양 (큰 큐브)
    glTranslatef(0.0f, 1.5f, 0.0f); // 바닥에서 약간 띄움
    glScalef(1.8f, 3.0f, 1.8f);     // 크기 키우기 (길막음 용)
    glutSolidCube(1.0f);

    // 블렌딩 끄기 (다른 물체에 영향 안 주게)
    glDisable(GL_BLEND);
    glPopMatrix();
}

// [새로 추가] 출구 막는 벽(Gate)과 글씨 그리기
// [새로 추가] 마인크래프트 철 골렘 그리기
// 마인크래프트 철 골렘 그리기 (수정됨: 크기 축소 및 위치 조정)
// 마인크래프트 철 골렘 그리기 (수정됨: 높이 재조정으로 다리 노출)
// 마인크래프트 철 골렘 그리기 (수정됨: 플레이어 시선 추적)
void DrawGolem() {
    int endX = MAZE_W - 2;
    int endY = MAZE_H - 2;

    for (const auto& obj : g_worldObjects) {
        if (obj.type == 99) {
            if (obj.collected) return;

            float gx = (endX + 0.5f) * CELL_SIZE;
            float gz = (endY + 0.5f) * CELL_SIZE;

            glPushMatrix();
            glTranslatef(gx, 0.0f, gz); // 골렘 위치로 이동

            // 1. 크기 및 높이 설정 (이전과 동일)
            glScalef(0.4f, 0.4f, 0.4f);
            glTranslatef(0.0f, -0.6f, 0.0f);

            // =========================================================
            // ★ [핵심 수정] 플레이어를 바라보도록 회전 각도 계산
            // =========================================================
            // 플레이어와 골렘 사이의 거리 차이 계산
            float dx = camX - gx;
            float dz = camZ - gz;

            // atan2 함수로 각도(Radian)를 구하고, Degree(도)로 변환
            // 180.0f / 3.141592f 는 라디안을 도로 바꾸는 공식입니다.
            float angle = atan2(dx, dz) * 180.0f / 3.141592f;

            // 계산된 각도만큼 Y축 회전 (항상 플레이어를 봄)
            glRotatef(angle, 0.0f, 1.0f, 0.0f);
            // =========================================================

            // --- [이하 골렘 그리기 코드는 100% 동일합니다] ---

            // [1] 다리
            glColor3f(0.85f, 0.85f, 0.85f);
            glPushMatrix(); glTranslatef(-0.4f, 0.75f, 0.0f); glScalef(0.6f, 1.5f, 0.6f); glutSolidCube(1.0f); glPopMatrix();
            glPushMatrix(); glTranslatef(0.4f, 0.75f, 0.0f); glScalef(0.6f, 1.5f, 0.6f); glutSolidCube(1.0f); glPopMatrix();

            // [2] 몸통
            glPushMatrix(); glTranslatef(0.0f, 2.0f, 0.0f); glScalef(1.0f, 1.2f, 0.7f); glutSolidCube(1.0f); glPopMatrix();
            glPushMatrix(); glTranslatef(0.0f, 2.8f, 0.0f); glScalef(1.8f, 0.8f, 0.9f); glutSolidCube(1.0f); glPopMatrix();

            // [3] 팔
            glPushMatrix(); glTranslatef(-1.1f, 2.0f, 0.0f); glScalef(0.5f, 2.2f, 0.6f); glutSolidCube(1.0f); glPopMatrix();
            glPushMatrix(); glTranslatef(1.1f, 2.0f, 0.0f); glScalef(0.5f, 2.2f, 0.6f); glutSolidCube(1.0f); glPopMatrix();

            // [4] 머리
            glPushMatrix(); glTranslatef(0.0f, 3.5f, -0.1f); glScalef(0.7f, 0.8f, 0.7f); glutSolidCube(1.0f); glPopMatrix();

            // [5] 코
            glPushMatrix(); glTranslatef(0.0f, 3.4f, 0.3f); glScalef(0.2f, 0.4f, 0.2f); glutSolidCube(1.0f); glPopMatrix();

            // [6] 눈
            glColor3f(0.8f, 0.0f, 0.0f);
            glPushMatrix(); glTranslatef(-0.2f, 3.6f, 0.26f); glScalef(0.15f, 0.1f, 0.1f); glutSolidCube(1.0f); glPopMatrix();
            glPushMatrix(); glTranslatef(0.2f, 3.6f, 0.26f); glScalef(0.15f, 0.1f, 0.1f); glutSolidCube(1.0f); glPopMatrix();

            glPopMatrix(); // 전체 복구
            break;
        }
    }
}

void Display() {
    if (g_gameState == STATE_CUTSCENE) {
        DrawNightScene(); glutSwapBuffers(); return;
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
    DrawSORObjects(CELL_SIZE); // 그림자 포함 그리기
    DrawGolem();
    DrawMiniMap();
    DrawUIMessage();
    glutSwapBuffers();
}

void Idle() {
    static int lastTime = 0;
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTime) / 1000.0f;
    if (dt > 0.1f) dt = 0.1f;
    lastTime = now;

    if (g_slowTimer > 0) g_slowTimer -= dt;
    if (g_confusionTimer > 0) g_confusionTimer -= dt;
    if (g_darknessTimer > 0) g_darknessTimer -= dt;

    CheckTrapCollision();

    // ★ [추가] 메시지 타이머 감소
    if (g_uiMessageTimer > 0) {
        g_uiMessageTimer -= dt;
    }

    if (g_gameState == STATE_CUTSCENE) {
        UpdateCamera(dt);
        g_cutsceneTime += dt;
        if (g_cutsceneTime >= CUTSCENE_DURATION) ReturnToMaze();
    }
    else {
        UpdateCamera(dt);
        UpdateJump(dt);
    }
    glutPostRedisplay();
}

void Reshape(int w, int h) { winWidth = w; winHeight = h; glViewport(0, 0, w, h); }

// 오브젝트 배치 (InitEmotionObjects 구현)
// Main.cpp의 InitEmotionObjects 함수를 이걸로 교체하세요

void InitEmotionObjects() {
    // ==========================================
    // 1. 모델 로드 및 안전장치
    // ==========================================
    int idxSad = LoadAndRegisterModel("sad.txt");
    int idxAnger = LoadAndRegisterModel("anger.txt");
    int idxHappy = LoadAndRegisterModel("happy.txt");
    int idxTrap = LoadAndRegisterModel("trap.txt");

    if (idxSad < 0) idxSad = 0;
    if (idxAnger < 0) idxAnger = idxSad;
    if (idxHappy < 0) idxHappy = idxSad;

    // ==========================================
    // 2. 함정(작은 감정) 랜덤 배치
    // ==========================================
    for (int i = 0; i < 15; ++i) {
        int tx, ty;
        do {
            tx = rand() % (MAZE_W - 2) + 1;
            ty = rand() % (MAZE_H - 2) + 1;
        } while (maze[ty][tx] != PATH);

        int trapType = (rand() % 3) + 1;
        AddObjectGrid(idxTrap, tx, ty, 1.0f, 0.5f, 0, 5.0f, 0.02f, 0.2f, trapType);
    }

    // ==========================================
    // 3. 경로 찾기 (BFS) 및 큰 감정 배치
    // ==========================================
    struct Node { int x, z; };
    std::vector<Node> path;
    int sx = 1, sz = 1, ex = MAZE_W - 2, ez = MAZE_H - 2;
    static bool v[MAZE_H][MAZE_W];
    static Node p[MAZE_H][MAZE_W];
    for (int i = 0; i < MAZE_H; i++)for (int j = 0; j < MAZE_W; j++) v[i][j] = false;

    std::queue<Node> q; q.push({ sx,sz }); v[sz][sx] = true;
    bool found = false;
    int dx[] = { 0,0,-1,1 }, dy[] = { -1,1,0,0 };

    while (!q.empty()) {
        Node c = q.front(); q.pop();
        if (c.x == ex && c.z == ez) { found = true; break; }
        for (int i = 0; i < 4; i++) {
            int nx = c.x + dx[i], nz = c.z + dy[i];
            if (nx >= 0 && nx < MAZE_W && nz >= 0 && nz < MAZE_H && !v[nz][nx] && maze[nz][nx] == PATH) {
                v[nz][nx] = true; p[nz][nx] = c; q.push({ nx,nz });
            }
        }
    }

    if (found) {
        Node cur = { ex,ez };
        while (!(cur.x == sx && cur.z == sz)) {
            path.push_back(cur); cur = p[cur.z][cur.x];
        }
        path.push_back({ sx, sz });
        std::reverse(path.begin(), path.end());

        // 3개의 큰 감정 배치
        for (int i = 0; i < 3; ++i) {
            int k = (int)((i + 1) / 4.0f * path.size());
            if (k < path.size()) {
                int currentModelIdx = 0;
                if (i == 0) currentModelIdx = idxSad;    // 슬픔
                else if (i == 1) currentModelIdx = idxAnger;  // 분노
                else currentModelIdx = idxHappy;  // 행복

                AddObjectGrid(currentModelIdx, path[k].x, path[k].z, 2.0f, 0.6f, 0, 1.5f, 0.01f, 0.3f);
            }
        }
    }

    // ==========================================
    // 4. [여기에 추가됨!] 결계 오브젝트 배치 (Type: 99)
    // ==========================================
    int endX = MAZE_W - 2;
    int endY = MAZE_H - 2;
    // 모델 ID는 0(투명), Type은 99(결계)
    AddObjectGrid(0, endX, endY, 0, 0, 0, 0, 0, 0, 99);
}

int main(int argc, char** argv) {
    GenerateMaze();
    std::srand((unsigned int)std::time(0));
    InitEmotionObjects(); // 여기서 호출

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("Emotion Maze: Final Split");

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