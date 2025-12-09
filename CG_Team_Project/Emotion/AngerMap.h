#pragma once

namespace AngerMap
{
    void Init();
    void Enter();
    void Exit();

    void Update(float dt);

    void Render3D();
    void Render2D(int winW, int winH);

    void OnKeyDown(unsigned char key);

    bool IsCleared();
}
