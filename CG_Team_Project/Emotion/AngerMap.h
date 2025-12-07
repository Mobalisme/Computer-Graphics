#pragma once

namespace AngerMap
{
    void Enter();
    void Exit();

    void Update(float dt);

    void Render3D();
    void Render2D(int winW, int winH);

    bool IsCleared();
}
