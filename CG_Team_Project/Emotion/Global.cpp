#include "Global.h"

int winWidth = 800;
int winHeight = 800;

GameState g_gameState = STATE_MODELER;

int maze[MAZE_H][MAZE_W];
bool visitedCell[CELL_H][CELL_W];


// ★ [통합] 함정 타이머 초기화
float g_slowTimer = 0.0f;
float g_confusionTimer = 0.0f;
float g_darknessTimer = 0.0f;

// ★ [통합] 텍스처 단계 초기화
int g_textureStage = 0;

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
GLuint g_oceanTex1 = 0;
GLuint g_oceanTex2 = 0;
GLuint g_lavaTex1 = 0;
GLuint g_lavaTex2 = 0;
GLuint g_lavaProgram = 0;

std::vector<GLuint> g_videoSad;
std::vector<GLuint> g_videoJoy;
const int VIDEO_FPS = 10;

GLfloat g_lightPos[] = { 10.0f, 20.0f, 10.0f, 0.0f };
GLfloat g_shadowMatrix[16];
std::vector<Flower> g_flowers;

