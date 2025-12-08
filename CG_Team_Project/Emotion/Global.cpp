#include "Global.h"

int winWidth = 800;
int winHeight = 800;

GameState g_gameState = STATE_PLAYING;

int  maze[MAZE_H][MAZE_W];
bool visitedCell[CELL_H][CELL_W];

float camX = 1.5f, camY = BASE_CAM_Y, camZ = 1.5f;
float camYaw = 0.0f, camPitch = 0.0f;
float camVelY = 0.0f;
bool  isOnGround = true;

bool keyDown[256] = { false };
bool arrowDown[4] = { false };

float g_cutsceneTime = 0.0f;
const float CUTSCENE_DURATION = 12.0f;

float savedCamX = 0.0f, savedCamY = 0.0f, savedCamZ = 0.0f;
float savedCamYaw = 0.0f, savedCamPitch = 0.0f;

// 주된 감정 수집 여부
bool g_mainCollected[EMO_MAIN_COUNT] = { false, false, false };

// 텍스처 단계(0~3)
int g_textureStage = 0;

GLuint g_wallTex = 0;
GLuint g_floorTex = 0;

// ★ 슬픔 맵 수면(Ocean) 텍스처
GLuint g_sadWaterTex = 0;

std::vector<Flower> g_flowers;

// 영상 프레임(선택)
std::vector<GLuint> g_videoFrames;
const int VIDEO_FPS = 10;

// 조명/그림자
GLfloat g_lightPos[4] = { 10.0f, 20.0f, 10.0f, 0.0f };
GLfloat g_shadowMatrix[16];
