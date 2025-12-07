#include "AngerMap.h"
#include <GL/freeglut.h>
#include <cmath>

namespace
{
    float g_time = 0.0f;
    bool  g_cleared = false;
}

void AngerMap::Enter()
{
    g_time = 0.0f;
    g_cleared = false;
}

void AngerMap::Exit()
{
}

void AngerMap::Update(float dt)
{
    if (g_cleared) return;

    g_time += dt;

    // 데모형 분노 경험: 5초 버티면 클리어
    if (g_time >= 5.0f)
        g_cleared = true;
}

static void DrawAngerRoom(float t)
{
    const float size = 20.0f;
    const float h = 4.0f;

    glDisable(GL_TEXTURE_2D);

    // 바닥
    glColor3f(0.6f, 0.05f, 0.02f);
    glBegin(GL_QUADS);
    glVertex3f(-size, 0, -size);
    glVertex3f( size, 0, -size);
    glVertex3f( size, 0,  size);
    glVertex3f(-size, 0,  size);
    glEnd();

    // 벽
    glColor3f(0.08f, 0.02f, 0.02f);
    glBegin(GL_QUADS);
    // +Z
    glVertex3f(-size, 0, size); glVertex3f(size, 0, size);
    glVertex3f(size, h, size);  glVertex3f(-size, h, size);
    // -Z
    glVertex3f(size, 0, -size);  glVertex3f(-size, 0, -size);
    glVertex3f(-size, h, -size); glVertex3f(size, h, -size);
    // +X
    glVertex3f(size, 0, size); glVertex3f(size, 0, -size);
    glVertex3f(size, h, -size); glVertex3f(size, h, size);
    // -X
    glVertex3f(-size, 0, -size); glVertex3f(-size, 0, size);
    glVertex3f(-size, h, size);  glVertex3f(-size, h, -size);
    glEnd();

    // 분노 코어
    float pulse = 1.0f + 0.15f * std::sin(t * 6.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glPushMatrix();
    glTranslatef(0.0f, 1.2f, 0.0f);
    glScalef(pulse, pulse, pulse);
    glColor4f(1.0f, 0.2f, 0.05f, 0.6f);
    glutSolidSphere(0.6, 24, 24);
    glPopMatrix();

    glDisable(GL_BLEND);
}

void AngerMap::Render3D()
{
    DrawAngerRoom(g_time);
}

void AngerMap::Render2D(int winW, int winH)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    glColor3f(1.0f, 0.3f, 0.2f);
    glRasterPos2f(20, winH - 30);

    const char* msg = g_cleared
        ? "[ANGER] Cleared! Returning to maze..."
        : "[ANGER] Endure 5 seconds...";

    for (const char* p = msg; *p; ++p)
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

bool AngerMap::IsCleared()
{
    return g_cleared;
}
