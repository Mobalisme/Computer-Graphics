#pragma once
#include <GL/freeglut.h>
#include <vector>
#include <random>
#include <string>

// ================== 상수 설정 ==================
const int MAZE_W = 25;
const int MAZE_H = 25;
const int CELL_W = (MAZE_W - 1) / 2;
const int CELL_H = (MAZE_H - 1) / 2;
const int WALL = 1;
const int PATH = 0;

const float CELL_SIZE = 1.0f;
const float WALL_HEIGHT = 3.0f;

const float MOVE_SPEED = 2.0f;
const float TURN_SPEED_KEY = 1.5f;
const float GRAVITY = -9.8f;
const float JUMP_SPEED = 4.5f;
const float BASE_CAM_Y = 0.8f;

enum { ARROW_LEFT = 0, ARROW_RIGHT, ARROW_UP, ARROW_DOWN };
enum GameState { STATE_MODELER, STATE_PLAYING, STATE_CUTSCENE, STATE_JOY_SCENE, STATE_ENDING };




struct Flower {
    float x, y, z;
    float r, g, b;
    float scale;
};

// ================== 전역 변수 선언 ==================
extern int winWidth, winHeight;
extern GameState g_gameState;

extern int maze[MAZE_H][MAZE_W];
extern bool visitedCell[CELL_H][CELL_W];

extern float camX, camY, camZ;
extern float camYaw, camPitch;
extern float camVelY;
extern bool isOnGround;
extern bool keyDown[256];
extern bool arrowDown[4];

extern float g_cutsceneTime;
extern const float CUTSCENE_DURATION;
extern float savedCamX, savedCamY, savedCamZ;
extern float savedCamYaw, savedCamPitch;

extern GLuint g_wallTex;
extern GLuint g_floorTex;
extern GLuint g_oceanTex1;
extern GLuint g_oceanTex2;
extern std::vector<GLuint> g_videoFrames;
extern const int VIDEO_FPS;

extern GLfloat g_lightPos[4];
extern GLfloat g_shadowMatrix[16];

extern std::string g_uiMessage; // 화면 메시지
extern float g_uiMessageTimer;  // 메시지 타이머

// ★ [통합] 텍스처 단계 및 함정 변수
extern int g_textureStage;
extern float g_slowTimer;      // 좌절 (느려짐)
extern float g_confusionTimer; // 혼란 (벽 꿀렁거림)
extern float g_darknessTimer;  // 외로움 (어두워짐)

extern std::vector<Flower> g_flowers;

// [NEW] 텍스처 단계 변수
extern int g_textureStage;
extern std::vector<GLuint> g_videoSad; // 슬픔 영상
extern std::vector<GLuint> g_videoJoy; // 기쁨 영상

// ================== 함수 선언 ==================
void GenerateMaze();
bool IsBlocked(float x, float z);
bool IsWall(int gx, int gz);

void LoadTextures();
void ChangeWallTexture(int stage); // [NEW] 텍스처 변경 함수
void InitStars();
void DrawMaze3D();
void DrawMiniMap();
void DrawNightScene();
void DrawJoyScene();

void UpdateCamera(float dt);
void UpdateJump(float dt);
void KeyboardDown(unsigned char k, int x, int y);
void KeyboardUp(unsigned char k, int x, int y);
void SpecialDown(int k, int x, int y);
void SpecialUp(int k, int x, int y);

void ReturnToMaze();
void TryCollectObject();
void InitEmotionObjects();
void InitFlowers();
void DrawEndingCredits();

void InitModeler();
void DrawModeler();
void HandleModelerMouse(int button, int state, int x, int y);
void HandleModelerMotion(int x, int y);
void HandleModelerKeyboard(unsigned char key, int x, int y);