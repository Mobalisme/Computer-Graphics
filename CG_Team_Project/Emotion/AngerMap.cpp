#include "AngerMap.h"

#include <GL/freeglut.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>

namespace
{
    float g_time = 0.0f;
    bool  g_inited = false;
    bool  g_cleared = false;

    struct LavaSpot
    {
        float x, z;
        float radius;
        float phase;
    };

    std::vector<LavaSpot> g_spots;

    // 분노맵 바닥 범위(카메라/씬 스케일에 맞춰 필요시 조절)
    const float FLOOR_HALF = 20.0f;

    static float Rand01()
    {
        return (float)(std::rand() % 10000) / 10000.0f;
    }

    static void EnsureSpots()
    {
        if (!g_spots.empty()) return;

        // 고정성 있는 랜덤
        // (매 실행마다 약간 달라도 괜찮으면 이대로,
        //  완전 고정 원하면 seed를 상수로 바꿔도 됨)
        std::srand((unsigned int)std::time(nullptr));

        const int COUNT = 12;
        g_spots.reserve(COUNT);

        for (int i = 0; i < COUNT; ++i)
        {
            LavaSpot s;
            s.x = (Rand01() * 2.0f - 1.0f) * (FLOOR_HALF * 0.75f);
            s.z = (Rand01() * 2.0f - 1.0f) * (FLOOR_HALF * 0.75f);
            s.radius = 1.2f + Rand01() * 1.8f;   // 1.2 ~ 3.0
            s.phase = Rand01() * 10.0f;

            g_spots.push_back(s);
        }
    }

    static float HeatFromSpots(float x, float z)
    {
        float heat = 0.0f;

        for (auto& s : g_spots)
        {
            float dx = x - s.x;
            float dz = z - s.z;
            float d2 = dx * dx + dz * dz;

            float r = s.radius;
            float r2 = r * r;

            if (d2 < r2)
            {
                float d = std::sqrt(d2);
                float t = 1.0f - (d / r); // 중심 1 -> 가장자리 0

                // 은은한 펄스 (너무 과하면 0.15f -> 0.08f로)
                float pulse = 0.85f + 0.15f * std::sinf(g_time * 1.2f + s.phase);

                heat += t * pulse;
            }
        }

        // 과도한 과열 방지
        if (heat > 1.0f) heat = 1.0f;
        return heat;
    }

    static float CrackMask(float x, float z)
    {
        // 균열 느낌의 얇은 패턴 생성
        // sin들을 합쳐서 "임계값 근처"만 강조
        float v =
            std::fabs(std::sinf(x * 1.15f) +
                std::sinf(z * 1.25f) +
                std::sinf((x + z) * 0.75f));

        // v가 특정 구간에서만 1에 가까워지게
        float mask = 0.0f;

        // 임계 범위를 좁게 잡아 "얇은 균열"처럼
        if (v > 2.55f)
            mask = (v - 2.55f) * 3.0f; // 0~대략 1

        if (mask > 1.0f) mask = 1.0f;
        return mask;
    }

    static void DrawLavaFloor()
    {
        EnsureSpots();

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);

        // 바닥을 작은 그리드로 그려 색을 섬세하게 변화
        const int N = 90;
        const float step = (FLOOR_HALF * 2.0f) / (float)N;

        for (int iz = 0; iz < N; ++iz)
        {
            float z0 = -FLOOR_HALF + iz * step;
            float z1 = z0 + step;

            glBegin(GL_TRIANGLE_STRIP);

            for (int ix = 0; ix <= N; ++ix)
            {
                float x = -FLOOR_HALF + ix * step;

                auto emitVertex = [&](float xx, float zz)
                    {
                        float heat = HeatFromSpots(xx, zz);
                        float crack = CrackMask(xx, zz);

                        // ---- 기본 바닥색: 짙은 빨간-갈색(암석) ----
                        float baseR = 0.18f;
                        float baseG = 0.05f;
                        float baseB = 0.03f;

                        // ---- 균열부 어두운 톤(갈색/검붉은 라인) ----
                        // crack이 있을수록 바닥이 더 "거칠고 어두운 선"처럼 보이게
                        float darken = crack * 0.08f;

                        // ---- 용암 발광 톤(핫스팟) ----
                        // heat이 있을수록 내부 발광이 은은하게 올라오게
                        float lavaR = 0.75f;
                        float lavaG = 0.18f;
                        float lavaB = 0.05f;

                        float r = baseR * (1.0f - heat) + lavaR * heat - darken;
                        float g = baseG * (1.0f - heat) + lavaG * heat - darken * 0.6f;
                        float b = baseB * (1.0f - heat) + lavaB * heat - darken * 0.3f;

                        if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;

                        glColor3f(r, g, b);
                        glVertex3f(xx, 0.0f, zz);
                    };

                emitVertex(x, z0);
                emitVertex(x, z1);
            }

            glEnd();
        }

        // ---- 균열 "빛"을 아주 약하게 덧칠(과하면 촌스러워짐) ----
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glBegin(GL_LINES);
        for (int i = 0; i < 1500; ++i)
        {
            float x = (Rand01() * 2.0f - 1.0f) * FLOOR_HALF;
            float z = (Rand01() * 2.0f - 1.0f) * FLOOR_HALF;

            float c = CrackMask(x, z);
            if (c <= 0.0f) continue;

            // 아주 미세한 붉은 발광
            float a = 0.08f * c;
            glColor4f(0.9f, 0.2f, 0.05f, a);

            float len = 0.25f + Rand01() * 0.35f;
            float ang = Rand01() * 6.28318f;

            float x2 = x + std::cos(ang) * len;
            float z2 = z + std::sin(ang) * len;

            glVertex3f(x, 0.01f, z);
            glVertex3f(x2, 0.01f, z2);
        }
        glEnd();

        glDisable(GL_BLEND);

        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
    }

    static void DrawAngerSkyDome()
    {
        // 아주 단순한 붉은 대기
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);

        glColor3f(0.08f, 0.01f, 0.01f);
        glutSolidSphere(60.0, 16, 16);

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
        const char* msg = "ANGER MAP";
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
    g_spots.clear();
}

void AngerMap::Enter()
{
    // 필요 시 Enter 시점에 초기화
    if (!g_inited) Init();
    g_time = 0.0f;
    g_cleared = false;
}

void AngerMap::Exit()
{
    // 특별 정리 필요 없으면 비워도 됨
}

void AngerMap::Update(float dt)
{
    g_time += dt;

    // 분노맵 클리어 조건은 프로젝트에 맞게 교체 가능
    // 지금은 "시간 경과형 더미" 대신
    // 사용자가 실제 미션을 붙일 때 여기를 바꾸면 됨.
}

void AngerMap::Render3D()
{
    // 카메라/프로젝션은 Main에서 이미 세팅한다고 가정

    // 바닥 용암/균열
    DrawLavaFloor();

    // 주변 대기(선택)
    // DrawAngerSkyDome();
}

void AngerMap::Render2D(int winW, int winH)
{
    DrawAngerUI2D(winW, winH);
}

void AngerMap::OnKeyDown(unsigned char key)
{
    // 임시: 테스트용으로 'r' 누르면 클리어 처리
    if (key == 'r' || key == 'R')
        g_cleared = true;
}

bool AngerMap::IsCleared()
{
    return g_cleared;
}
