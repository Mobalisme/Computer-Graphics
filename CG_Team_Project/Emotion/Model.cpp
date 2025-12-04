// Model.cpp
#include "Model.h"

#include <fstream>
#include <sstream>
#include <iostream>

#include <GL/freeglut.h>

// 실제 전역 정의
std::vector<float> g_boatVertices;

// "1/2/3" 같은 문자열에서 맨 앞 정수(1)를 꺼내는 함수
static int ParseIndex(const std::string& s)
{
    std::stringstream ss(s);
    int idx = 0;
    ss >> idx;
    return idx;
}

bool LoadOBJ_Simple(const char* filename, std::vector<float>& outVertices)
{
    std::ifstream fin(filename);
    if (!fin.is_open())
    {
        std::cout << "[OBJ] 파일을 열 수 없습니다: " << filename << "\n";
        return false;
    }

    std::vector<float> tempV; // v에서 읽은 정점들

    std::string line;
    while (std::getline(fin, line))
    {
        if (line.size() < 2) continue;

        // v x y z
        if (line[0] == 'v' && line[1] == ' ')
        {
            std::istringstream iss(line.substr(2));
            float x, y, z;
            iss >> x >> y >> z;
            tempV.push_back(x);
            tempV.push_back(y);
            tempV.push_back(z);
        }
        // f a b c
        else if (line[0] == 'f' && line[1] == ' ')
        {
            std::istringstream iss(line.substr(2));
            std::string s1, s2, s3;
            iss >> s1 >> s2 >> s3;
            if (s1.empty() || s2.empty() || s3.empty())
                continue;

            int i1 = ParseIndex(s1);
            int i2 = ParseIndex(s2);
            int i3 = ParseIndex(s3);

            int vCount = (int)(tempV.size() / 3);
            if (i1 < 1 || i1 > vCount ||
                i2 < 1 || i2 > vCount ||
                i3 < 1 || i3 > vCount)
                continue;

            int idx[3];
            idx[0] = i1 - 1; // OBJ는 1부터 시작
            idx[1] = i2 - 1;
            idx[2] = i3 - 1;

            for (int k = 0; k < 3; ++k)
            {
                int base = idx[k] * 3;
                outVertices.push_back(tempV[base + 0]);
                outVertices.push_back(tempV[base + 1]);
                outVertices.push_back(tempV[base + 2]);
            }
        }
    }

    std::cout << "[OBJ] 로드 완료: " << filename
        << " (triangle vertices = " << outVertices.size() / 3 << ")\n";

    return !outVertices.empty();
}

void DrawOBJModel(const std::vector<float>& vertices,
    float x, float y, float z,
    float scale)
{
    if (vertices.empty())
        return;

    glPushMatrix();

    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.8f, 0.8f, 0.9f);

    glBegin(GL_TRIANGLES);
    int count = (int)vertices.size();
    for (int i = 0; i + 2 < count; i += 3)
    {
        float vx = vertices[i + 0];
        float vy = vertices[i + 1];
        float vz = vertices[i + 2];
        glVertex3f(vx, vy, vz);
    }
    glEnd();

    glEnable(GL_TEXTURE_2D);

    glPopMatrix();
}
