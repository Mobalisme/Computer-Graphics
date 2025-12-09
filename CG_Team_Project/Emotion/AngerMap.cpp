#include "AngerMap.h"
#include "Global.h"

#include <GL/freeglut.h>
#include <cmath>

namespace
{
    float g_time = 0.0f;
    bool  g_inited = false;
    bool  g_cleared = false;

    // 분노맵 바닥 범위
    const float FLOOR_HALF = 22.0f;

    // ---- 분노맵 영상 패널(슬픔/기쁨과 동일한 방식) ----
    static void DrawVideoPanelIfAny()
    {
        if (g_videoFrames.empty()) return;

        int frameIdx = (int)(g_cutsceneTime * VIDEO_FPS) % (int)g_videoFrames.size();
        GLuint tex = g_videoFrames[frameIdx];
        if (!tex) return;

        glDisable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);

        // 화면 앞쪽에 떠있는 패널
        glPushMatrix();
        glTranslatef(0.0f, 2.2f, -6.5f);
        glRotatef(180.0f, 0, 1, 0);

        float w = 3.6f;
        float h = 2.0f;

        glColor3f(1, 1, 1);
        glBegin(GL_QUADS);

        // 뒤집힘 방지용 UV
        glTexCoord2f(0, 1); glVertex3f(-w, -h, 0);
        glTexCoord2f(1, 1); glVertex3f(w, -h, 0);
        glTexCoord2f(1, 0); glVertex3f(w, h, 0);
        glTexCoord2f(0, 0); glVertex3f(-w, h, 0);

        glEnd();

        glPopMatrix();

        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
    }

    static void DrawLavaFloorTextured()
    {
        if (!g_lavaTex) return;

        glDisable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_lavaTex);

        // 살짝 흐르는 UV 애니메이션
        float t = g_time;
        float uOff = t * 0.03f;
        float vOff = t * 0.02f;

        // 은은한 펄스 밝기
        float pulse = 0.85f + 0.15f * sinf(t * 1.4f);

        glColor3f(pulse, pulse, pulse);

        float tile = 6.0f; // 타일 반복 정도

        glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);

        glTexCoord2f(0 + uOff, 0 + vOff);         glVertex3f(-FLOOR_HALF, 0.0f, -FLOOR_HALF);
        glTexCoord2f(tile + uOff, 0 + vOff);      glVertex3f(FLOOR_HALF, 0.0f, -FLOOR_HALF);
        glTexCoord2f(tile + uOff, tile + vOff);   glVertex3f(FLOOR_HALF, 0.0f, FLOOR_HALF);
        glTexCoord2f(0 + uOff, tile + vOff);      glVertex3f(-FLOOR_HALF, 0.0f, FLOOR_HALF);

        glEnd();

        // 아주 약한 발광 느낌 추가
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 0.35f, 0.15f, 0.10f);

        glBegin(GL_QUADS);
        glVertex3f(-FLOOR_HALF, 0.01f, -FLOOR_HALF);
        glVertex3f(FLOOR_HALF, 0.01f, -FLOOR_HALF);
        glVertex3f(FLOOR_HALF, 0.01f, FLOOR_HALF);
        glVertex3f(-FLOOR_HALF, 0.01f, FLOOR_HALF);
        glEnd();

        glDisable(GL_BLEND);

        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
    }

    static void DrawAngerUI2D(int winW, int winH)
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, winW, 0, winH);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);

        glColor3f(1.0f, 0.3f, 0.2f);
        glRasterPos2f(20, winH - 30);
        const char* msg = "ANGER MAP  (R: Return)";
        for (const char* p = msg; *p; ++p)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);

        glEnable(GL_DEPTH_TEST);

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }
}

void AngerMap::Init()
{
    if (g_inited) return;
    g_inited = true;

    g_time = 0.0f;
    g_cleared = false;
}

void AngerMap::Enter()
{
    if (!g_inited) Init();

    g_time = 0.0f;
    g_cleared = false;

    // 분노씬 진입 시 영상도 0초부터
    g_cutsceneTime = 0.0f;
}

void AngerMap::Exit()
{
}

void AngerMap::Update(float dt)
{
    g_time += dt;

    // 별도 미션을 붙일거면 여기에 추가
}

void AngerMap::Render3D()
{
    // lava 텍스쳐 바닥
    DrawLavaFloorTextured();

    // 슬픔맵/기쁨맵과 동일하게 영상 패널 표시
    DrawVideoPanelIfAny();
}

void AngerMap::Render2D(int winW, int winH)
{
    DrawAngerUI2D(winW, winH);
}

void AngerMap::OnKeyDown(unsigned char key)
{
    // 화면 전환(복귀) 키가 분노씬에서도 확실히 동작하도록
    if (key == 'r' || key == 'R')
        g_cleared = true;
}

bool AngerMap::IsCleared()
{
    // 슬픔/기쁨처럼 시간 경과 자동 복귀도 허용
    if (g_cutsceneTime >= CUTSCENE_DURATION)
        return true;

    return g_cleared;
}
