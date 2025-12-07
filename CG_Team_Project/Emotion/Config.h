#pragma once

// ================== Window ==================
constexpr int   WIN_W = 800;
constexpr int   WIN_H = 800;

// ================== Maze ==================
constexpr int   MAZE_W = 25;  // 홀수
constexpr int   MAZE_H = 25;  // 홀수

constexpr int   WALL = 1;
constexpr int   PATH = 0;

constexpr float CELL_SIZE = 1.0f;
constexpr float WALL_HEIGHT = 1.6f;

// ================== Camera ==================
constexpr float BASE_CAM_Y = 0.8f;
constexpr float MOVE_SPEED = 2.0f;   // m/s
constexpr float TURN_SPEED = 1.5f;   // rad/s (방향키 회전)

// ================== Texture ==================
constexpr const char* WALL_TEX_CANDIDATES[] = {
    "Textures/wall.png",
    "../Textures/wall.png",
    "../../Textures/wall.png"
};

// ================== Emotion SOR model paths ==================
// 필요하면 파일명만 교체
constexpr const char* EMO_MODEL_PATHS[] = {
    "Models/joy.txt",
    "Models/sadness.txt",
    "Models/anger.txt",
    "Models/disgust.txt",
    "Models/fear.txt"
};

enum EmotionId {
    EMO_JOY = 0,
    EMO_SADNESS,
    EMO_ANGER,
    EMO_DISGUST,
    EMO_FEAR,
    EMO_COUNT
};
