#pragma once

namespace AngerMap
{
    void Init();
    void Enter();
    void Exit();
    void Update(float dt);

    void Render3D();
    void Render2D(int w, int h);

    void OnKeyDown(unsigned char k);
    bool IsCleared();
}
