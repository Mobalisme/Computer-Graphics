#include "Global.h"

int winWidth = 800;
int winHeight = 800;
GameState g_gameState = STATE_PLAYING;

int maze[MAZE_H][MAZE_W];
bool visitedCell[CELL_H][CELL_W];

// 변수 초기화 추가
float g_slowTimer = 0.0f;
float g_confusionTimer = 0.0f;
float g_darknessTimer = 0.0f;

float camX = 1.5f, camY = 0.8f, camZ = 1.5f;
float camYaw = 0.0f, camPitch = 0.0f;
float camVelY = 0.0f;
bool isOnGround = true;
bool keyDown[256] = { false };
bool arrowDown[4] = { false };

float g_cutsceneTime = 0.0f;
const float CUTSCENE_DURATION = 15.0f;
float savedCamX, savedCamY, savedCamZ;
float savedCamYaw, savedCamPitch;

std::string g_uiMessage = "";
float g_uiMessageTimer = 0.0f;

GLuint g_wallTex = 0;
GLuint g_floorTex = 0;
std::vector<GLuint> g_videoFrames;
const int VIDEO_FPS = 10;

GLfloat g_lightPos[] = { 10.0f, 20.0f, 10.0f, 0.0f };
GLfloat g_shadowMatrix[16];

// [NEW] 0: 기본, 1: 첫번째 획득 후, 2: 두번째...
int g_textureStage = 0;