#pragma once
#include <GL/freeglut.h>
#include <vector>
#include <random>

// ===== 미로 상수 =====
const int MAZE_W = 25;
const int MAZE_H = 25;
const int CELL_W = (MAZE_W - 1) / 2;
const int CELL_H = (MAZE_H - 1) / 2;

const int WALL = 1;
const int PATH = 0;

const float CELL_SIZE = 1.0f;
const float WALL_HEIGHT = 3.0f;

// ===== 이동/점프 =====
const float MOVE_SPEED = 2.0f;
const float GRAVITY = -9.8f;
const float JUMP_SPEED = 4.5f;
const float BASE_CAM_Y = 0.8f;
const float TURN_SPEED_KEY = 1.8f;

enum { ARROW_LEFT = 0, ARROW_RIGHT, ARROW_UP, ARROW_DOWN };

// ===== 주된 감정 3종 =====
enum EmotionId
{
    EMO_SADNESS = 0,
    EMO_ANGER = 1,
    EMO_JOY = 2,
    EMO_MAIN_COUNT = 3
};

// ===== 게임 상태 =====
enum GameState
{
    STATE_PLAYING = 0,
    STATE_SAD_SCENE,
    STATE_ANGER_SCENE,
    STATE_JOY_SCENE,
    STATE_ENDING
};

// ===== 기쁨 씬 소품 =====
struct Flower
{
    float x, y, z;
    float r, g, b;
    float scale;
};

// ===== 전역 =====
extern int winWidth, winHeight;
extern GameState g_gameState;

extern int  maze[MAZE_H][MAZE_W];
extern bool visitedCell[CELL_H][CELL_W];

extern float camX, camY, camZ;
extern float camYaw, camPitch;
extern float camVelY;
extern bool  isOnGround;

extern bool keyDown[256];
extern bool arrowDown[4];

extern float g_cutsceneTime;
extern const float CUTSCENE_DURATION;

extern float savedCamX, savedCamY, savedCamZ;
extern float savedCamYaw, savedCamPitch;

extern bool  g_mainCollected[EMO_MAIN_COUNT];
extern int   g_textureStage;

extern GLuint g_wallTex;
extern GLuint g_floorTex;

extern std::vector<Flower> g_flowers;

// (선택) 영상 프레임(이미지 시퀀스)
extern std::vector<GLuint> g_videoFrames;
extern const int VIDEO_FPS;

extern GLfloat g_lightPos[4];
extern GLfloat g_shadowMatrix[16];

// ===== 함수 =====
void GenerateMaze();
bool IsBlocked(float x, float z);
bool IsWall(int gx, int gz);

void LoadTextures();
void ChangeWallTexture(int stage);

void DrawMaze3D();
void DrawMiniMap();

void InitStars();
void DrawNightScene();

void InitFlowers();
void DrawJoyScene();

void DrawEndingCredits();

void UpdateCamera(float dt);
void UpdateJump(float dt);

void KeyboardDown(unsigned char k, int x, int y);
void KeyboardUp(unsigned char k, int x, int y);
void SpecialDown(int k, int x, int y);
void SpecialUp(int k, int x, int y);

void TryCollectObject();
void InitEmotionObjects();
void ReturnToMaze();
