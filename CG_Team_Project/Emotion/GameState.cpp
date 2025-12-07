#include "GameState.h"

GameState gGameState;

void ResetGameState()
{
    gGameState = GameState{};
    gGameState.stage = STAGE_MAZE;
    gGameState.mainEmotionStep = 0;
    gGameState.returnCamX = 1.5f;
    gGameState.returnCamY = BASE_CAM_Y;
    gGameState.returnCamZ = 1.5f;
    gGameState.angerCleared = false;
}
