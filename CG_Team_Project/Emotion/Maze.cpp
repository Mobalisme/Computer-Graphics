#include "Global.h"
#include <algorithm>
#include <random>

static std::mt19937 rng;

struct Dir { int dx, dy; };
static const Dir dirs4[4] = { {0,-1}, {0,1}, {-1,0}, {1,0} };

static void InitRandom()
{
    std::random_device rd;
    rng.seed(rd());
}

static void InitMazeData()
{
    for (int y = 0; y < MAZE_H; ++y)
        for (int x = 0; x < MAZE_W; ++x)
            maze[y][x] = WALL;

    for (int cy = 0; cy < CELL_H; ++cy)
        for (int cx = 0; cx < CELL_W; ++cx)
            visitedCell[cy][cx] = false;
}

static void OpenCell(int cx, int cy)
{
    maze[2 * cy + 1][2 * cx + 1] = PATH;
}

static void GenerateMazeDFS(int cx, int cy)
{
    visitedCell[cy][cx] = true;
    OpenCell(cx, cy);

    std::vector<int> order = { 0,1,2,3 };
    std::shuffle(order.begin(), order.end(), rng);

    for (int i : order)
    {
        int ncx = cx + dirs4[i].dx;
        int ncy = cy + dirs4[i].dy;

        if (ncx < 0 || ncx >= CELL_W || ncy < 0 || ncy >= CELL_H) continue;
        if (visitedCell[ncy][ncx]) continue;

        int wx = (2 * cx + 1 + 2 * ncx + 1) / 2;
        int wy = (2 * cy + 1 + 2 * ncy + 1) / 2;
        maze[wy][wx] = PATH;

        GenerateMazeDFS(ncx, ncy);
    }
}

void GenerateMaze()
{
    InitRandom();
    InitMazeData();
    GenerateMazeDFS(0, 0);

    // 입구/출구 열기
    maze[1][0] = PATH;
    maze[MAZE_H - 2][MAZE_W - 1] = PATH;
}

bool IsWall(int gx, int gz)
{
    if (gx < 0 || gx >= MAZE_W || gz < 0 || gz >= MAZE_H) return true;
    return (maze[gz][gx] == WALL);
}

bool IsBlocked(float x, float z)
{
    float r = 0.2f;

    int l = (int)((x - r) / CELL_SIZE);
    int ri = (int)((x + r) / CELL_SIZE);
    int t = (int)((z - r) / CELL_SIZE);
    int b = (int)((z + r) / CELL_SIZE);

    return (IsWall(l, t) || IsWall(ri, t) || IsWall(l, b) || IsWall(ri, b));
}
