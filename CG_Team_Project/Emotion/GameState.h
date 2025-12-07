#pragma once

#include "Config.h"

// 어떤 스테이지(맵)인지
enum StageId {
    STAGE_MAZE = 0,
    STAGE_ANGER
};

// 전역 게임 상태
struct GameState
{
    StageId stage = STAGE_MAZE;

    // 메인 감정 진행 단계(원하면 확장)
    int mainEmotionStep = 0;

    // 미로에서 분노 구슬을 먹고 분노맵으로 갈 때,
    // 복귀 위치 저장
    float returnCamX = 1.5f;
    float returnCamY = BASE_CAM_Y;
    float returnCamZ = 1.5f;

    // 분노맵 클리어 여부(간단 플래그)
    bool angerCleared = false;
};

extern GameState gGameState;

void ResetGameState();
