#include "AngerMap.h"
#include <GL/freeglut.h>
#include <cmath>

namespace
{
    float tAccum = 0.0f;
    bool  cleared = false;
}

void AngerMap::Init()
{
    tAccum = 0.0f;
    cleared = false;
}

void AngerMap::Enter()
{
    tAccum = 0.0f;
    cleared = false;
}

void AngerMap::Exit()
{
    // 특별한 자원 해제 없으면 비워둬도 됨
}

void AngerMap::OnKeyDown(unsigned char k)
{
    // 분노맵에서 T를 누르면 "정화" 느낌으로 즉시 클리어
    if (k == 't' || k == 'T')
        cleared = true;
}

void AngerMap::Update(float dt)
{
    tAccum += dt;

    // 8초가 지나면 자동 클리어(시연용)
    if (tAccum >= 8.0f)
        cleared = true;
}

bool AngerMap::IsCleared()
{
    return cleared;
}

void AngerMap::Render3D()
{
    glPushMatrix();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    // 붉은 방 느낌의 바닥/벽(간단)
    glColor3f(0.15f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex3f(-20, 0, -20);
    glVertex3f(20, 0, -20);
    glVertex3f(20, 0, 20);
    glVertex3f(-20, 0, 20);
    glEnd();

    // 분노의 맥동 큐브들
    float pulse = 0.5f + 0.5f * sinf(tAccum * 6.0f);

    for (int i = 0; i < 8; ++i)
    {
        glPushMatrix();
        float x = -8.0f + i * 2.2f;
        float z = -4.0f + (i % 3) * 3.0f;
        glTranslatef(x, 1.0f + pulse * 0.8f, z);
        glColor3f(0.7f + pulse * 0.3f, 0.05f, 0.05f);
        glutSolidCube(1.2);
        glPopMatrix();
    }

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}

void AngerMap::Render2D(int w, int h)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glColor3f(1, 0.3f, 0.3f);
    const char* msg = "ANGER MAP - Press T to calm down";
    glRasterPos2i(20, h - 40);
    for (const char* p = msg; *p; ++p)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);

    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
