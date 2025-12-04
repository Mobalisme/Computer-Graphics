// Model.h
#pragma once

#include <vector>

// 마야에서 Export한 OBJ(배) 모델의 정점 데이터
// (x, y, z 순서로 차례대로 저장)
extern std::vector<float> g_boatVertices;

// 매우 단순한 OBJ 로더
//  - v x y z
//  - f a b c   (항상 삼각형이라고 가정)
bool LoadOBJ_Simple(const char* filename, std::vector<float>& outVertices);

// OBJ 정점 데이터를 월드 좌표 (x,y,z)에 scale 배 크기로 렌더링
void DrawOBJModel(const std::vector<float>& vertices,
    float x, float y, float z,
    float scale);
