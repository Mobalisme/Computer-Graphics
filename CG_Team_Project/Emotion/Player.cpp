#include "Global.h"
#include <cmath>
#include <cstdlib>
#include <windows.h> // ★ Shift키 입력을 감지하기 위해 꼭 필요합니다!

void UpdateCamera(float dt) {
    float fx = cos(camYaw), fz = sin(camYaw);
    float rx = -fz, rz = fx;
    float mx = 0, mz = 0;

    // ============================================================
    // ★ [최종 속도 결정 로직] 
    // 우선순위: 좌절(1순위) > 달리기(2순위) > 걷기(3순위)
    // ============================================================

    // 1. 기본 걷기 속도 설정 (2.0f)
    float baseSpeed = 2.0f;

    // 2. Shift 키를 누르고 있으면 달리기 속도(5.0f)로 변경
    // (GetAsyncKeyState 함수를 쓰려면 맨 위에 #include <windows.h> 필수!)
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
        baseSpeed = 3.5f;
    }

    // 3. 하지만 '좌절' 함정에 걸려있다면? 무조건 0.1f로 고정 (최우선)
    // (함정에 걸리면 Shift를 눌러도 0.1f가 됩니다)
    float moveSpeed = (g_slowTimer > 0) ? 0.1f : baseSpeed;

    // ============================================================

    if (keyDown['w'] || keyDown['W']) { mx += fx; mz += fz; }
    if (keyDown['s'] || keyDown['S']) { mx -= fx; mz -= fz; }
    if (keyDown['a'] || keyDown['A']) { mx -= rx; mz -= rz; }
    if (keyDown['d'] || keyDown['D']) { mx += rx; mz += rz; }

    if (arrowDown[ARROW_LEFT]) camYaw -= TURN_SPEED_KEY * dt;
    if (arrowDown[ARROW_RIGHT]) camYaw += TURN_SPEED_KEY * dt;
    if (arrowDown[ARROW_UP]) camPitch += TURN_SPEED_KEY * dt;
    if (arrowDown[ARROW_DOWN]) camPitch -= TURN_SPEED_KEY * dt;

    if (camPitch > 1.2f) camPitch = 1.2f;
    if (camPitch < -1.2f) camPitch = -1.2f;

    float len = sqrt(mx * mx + mz * mz);
    if (len > 0.0001f) {
        // 위에서 결정한 moveSpeed를 적용하여 이동
        float nx = camX + (mx / len) * moveSpeed * dt;
        float nz = camZ + (mz / len) * moveSpeed * dt;

        if (g_gameState == STATE_CUTSCENE) {
            camX = nx; camZ = nz;
            if (keyDown[' ']) camY += moveSpeed * dt;
            if (keyDown['c'] || keyDown['C']) camY -= moveSpeed * dt;
        }
        else {
            if (!IsBlocked(nx, camZ)) camX = nx;
            if (!IsBlocked(camX, nz)) camZ = nz;
        }
    }
}

void UpdateJump(float dt) {
    camVelY += GRAVITY * dt;
    camY += camVelY * dt;
    if (camY <= BASE_CAM_Y) {
        camY = BASE_CAM_Y; camVelY = 0.0f; isOnGround = true;
    }
}

void KeyboardDown(unsigned char k, int, int) {
    if (k == 27) exit(0);
    if (k == ' ') {
        if (isOnGround && g_gameState == STATE_PLAYING) { camVelY = JUMP_SPEED; isOnGround = false; }
    }
    // TryCollectObject는 Main.cpp에서 처리
    keyDown[k] = true;
}
//void KeyboardUp(unsigned char k, int, int) { keyDown[k] = false; }

void SpecialDown(int k, int, int) {
    if (k == GLUT_KEY_LEFT) arrowDown[ARROW_LEFT] = true;
    if (k == GLUT_KEY_RIGHT) arrowDown[ARROW_RIGHT] = true;
    if (k == GLUT_KEY_UP) arrowDown[ARROW_UP] = true;
    if (k == GLUT_KEY_DOWN) arrowDown[ARROW_DOWN] = true;
}
void SpecialUp(int k, int, int) {
    if (k == GLUT_KEY_LEFT) arrowDown[ARROW_LEFT] = false;
    if (k == GLUT_KEY_RIGHT) arrowDown[ARROW_RIGHT] = false;
    if (k == GLUT_KEY_UP) arrowDown[ARROW_UP] = false;
    if (k == GLUT_KEY_DOWN) arrowDown[ARROW_DOWN] = false;
}