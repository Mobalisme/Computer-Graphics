#include "Global.h"

int winWidth = 800;
int winHeight = 800;
GameState g_gameState = STATE_PLAYING;

int maze[MAZE_H][MAZE_W];
bool visitedCell[CELL_H][CELL_W];

float camX = 1.5f, camY = BASE_CAM_Y, camZ = 1.5f;
float camYaw = 0.0f, camPitch = 0.0f;
float camVelY = 0.0f;
bool isOnGround = true;
bool keyDown[256] = { false };
bool arrowDown[4] = { false };

float g_cutsceneTime = 0.0f;
const float CUTSCENE_DURATION = 15.0f;
float savedCamX = 0.0f, savedCamY = 0.0f, savedCamZ = 0.0f;
float savedCamYaw = 0.0f, savedCamPitch = 0.0f;

// 주된 감정 수집 여부
bool g_mainCollected[EMO_MAIN_COUNT] = { false, false, false };

GLuint g_wallTex = 0;
GLuint g_floorTex = 0;
std::vector<GLuint> g_videoFrames;
const int VIDEO_FPS = 10;

GLfloat g_lightPos[] = { 10.0f, 20.0f, 10.0f, 0.0f };
GLfloat g_shadowMatrix[16];
std::vector<Flower> g_flowers;

// 0: 기본, 1~3: 주된 감정 수집 단계
int g_textureStage = 0;
