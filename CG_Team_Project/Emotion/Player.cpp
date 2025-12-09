#include "Global.h"
#include <cmath>
#include <cstdlib>

void UpdateCamera(float dt) {
    float fx = cos(camYaw), fz = sin(camYaw);
    float rx = -fz, rz = fx;
    float mx = 0, mz = 0;

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

    float currentSpeed = MOVE_SPEED;
    if (g_slowTimer > 0.0f) {
        currentSpeed = MOVE_SPEED * 0.25f;
    }

    if (arrowDown[ARROW_LEFT]) camYaw -= TURN_SPEED_KEY * dt;

    float len = sqrt(mx * mx + mz * mz);
    if (len > 0.0001f) {
        float nx = camX + (mx / len) * currentSpeed * dt;
        float nz = camZ + (mz / len) * currentSpeed * dt;

        if (g_gameState == STATE_CUTSCENE || g_gameState == STATE_JOY_SCENE) {
            camX = nx; camZ = nz;
            if (keyDown[' ']) camY += currentSpeed * dt;
            if (keyDown['c'] || keyDown['C']) camY -= currentSpeed * dt;
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
void KeyboardUp(unsigned char k, int, int) { keyDown[k] = false; }

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