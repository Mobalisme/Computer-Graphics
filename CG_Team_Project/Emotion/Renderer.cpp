#include "Global.h"
#include "SOR_Objects.h" // Ʈ ׸
#include <iostream>
#include <string>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Star { float x, y, z; float brightness; };
std::vector<Star> g_stars;

GLuint LoadTextureFromFile(const char* filename) {
    int w, h, c;
    unsigned char* d = stbi_load(filename, &w, &h, &c, 0);
    if (!d) return 0;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    GLenum f = (c == 4) ? GL_RGBA : (c == 3 ? GL_RGB : GL_RED);
    glTexImage2D(GL_TEXTURE_2D, 0, f, w, h, 0, f, GL_UNSIGNED_BYTE, d);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(d);
    return tex;
}

void ChangeWallTexture(int stage) {
    // 1.  wall0.png / wall0.jpg  ܰ躰  ãϴ.
    std::string path = "Textures/wall" + std::to_string(stage) + ".png";
    GLuint newTex = LoadTextureFromFile(path.c_str());

    if (newTex == 0) {
        // png  jpg õ
        path = "Textures/wall" + std::to_string(stage) + ".jpg";
        newTex = LoadTextureFromFile(path.c_str());
    }

    // 2. [ٽ]    ãҴµ, װ 'ó (0ܰ)'̶?
    // -> ⺻ Wall.png  εմϴ. (ȸ  )
    if (newTex == 0 && stage == 0) {
        std::cout << "[SYSTEM] 'wall0' not found. Falling back to default 'Wall.png'\n";

        newTex = LoadTextureFromFile("Textures/Wall.png");
        if (newTex == 0) newTex = LoadTextureFromFile("Wall.png");
        if (newTex == 0) newTex = LoadTextureFromFile("wall.png");
    }

    // 3. ؽó 
    if (newTex != 0) {
        if (g_wallTex != 0) glDeleteTextures(1, &g_wallTex); //  ޸ 
        g_wallTex = newTex;
        std::cout << "[SYSTEM] Wall texture set to Stage " << stage << "\n";
    }
    else {
        std::cout << "[WARNING] Failed to load wall texture for Stage " << stage << "\n";
    }
}

void LoadTextures() {

    ChangeWallTexture(0);

    // ٴ ε
    g_floorTex = LoadTextureFromFile("Textures/floor.jpeg");
    if (g_floorTex == 0) g_floorTex = LoadTextureFromFile("floor.jpeg");
    if (g_floorTex == 0) g_floorTex = LoadTextureFromFile("Textures/floor.jpg");
    if (g_floorTex == 0) g_floorTex = LoadTextureFromFile("floor.jpg");

    if (g_floorTex) std::cout << "[SYSTEM] Floor texture loaded.\n";
    else std::cout << "[WARNING] Floor texture NOT found.\n";

    //   ε
    g_videoFrames.clear();
    std::cout << "[SYSTEM] Loading video frames...\n";
    for (int i = 1; i <= 120; ++i) {
        std::string path = "Video/" + std::to_string(i) + ".jpg";
        GLuint t = LoadTextureFromFile(path.c_str());

        //  ݺ(REPEAT)  ̱(CLAMP) 
        if (t != 0) {
            glBindTexture(GL_TEXTURE_2D, t);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            g_videoFrames.push_back(t);
        }
    }
}

void DrawWallBlock(float gx, float gz) {
    float x0 = gx * CELL_SIZE, x1 = (gx + 1) * CELL_SIZE;
    float z0 = gz * CELL_SIZE, z1 = (gz + 1) * CELL_SIZE;
    glBindTexture(GL_TEXTURE_2D, g_wallTex); glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);  glTexCoord2f(0, 0); glVertex3f(x0, 0, z0); glTexCoord2f(1, 0); glVertex3f(x1, 0, z0); glTexCoord2f(1, 1); glVertex3f(x1, WALL_HEIGHT, z0); glTexCoord2f(0, 1); glVertex3f(x0, WALL_HEIGHT, z0);
    glNormal3f(0, 0, -1); glTexCoord2f(0, 0); glVertex3f(x1, 0, z1); glTexCoord2f(1, 0); glVertex3f(x0, 0, z1); glTexCoord2f(1, 1); glVertex3f(x0, WALL_HEIGHT, z1); glTexCoord2f(0, 1); glVertex3f(x1, WALL_HEIGHT, z1);
    glNormal3f(-1, 0, 0); glTexCoord2f(0, 0); glVertex3f(x0, 0, z1); glTexCoord2f(1, 0); glVertex3f(x0, 0, z0); glTexCoord2f(1, 1); glVertex3f(x0, WALL_HEIGHT, z0); glTexCoord2f(0, 1); glVertex3f(x0, WALL_HEIGHT, z1);
    glNormal3f(1, 0, 0);  glTexCoord2f(0, 0); glVertex3f(x1, 0, z0); glTexCoord2f(1, 0); glVertex3f(x1, 0, z1); glTexCoord2f(1, 1); glVertex3f(x1, WALL_HEIGHT, z1); glTexCoord2f(0, 1); glVertex3f(x1, WALL_HEIGHT, z0);
    glEnd(); glBindTexture(GL_TEXTURE_2D, 0);
}

void DrawFloor() {
    float w = MAZE_W * CELL_SIZE, h = MAZE_H * CELL_SIZE;
    glBindTexture(GL_TEXTURE_2D, g_floorTex); glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0);       glVertex3f(0, 0, 0);
    glTexCoord2f(MAZE_W, 0);  glVertex3f(w, 0, 0);
    glTexCoord2f(MAZE_W, MAZE_H); glVertex3f(w, 0, h);
    glTexCoord2f(0, MAZE_H);  glVertex3f(0, 0, h);
    glEnd(); glBindTexture(GL_TEXTURE_2D, 0);
}

void DrawMaze3D() {
    DrawFloor();
    for (int z = 0; z < MAZE_H; ++z) for (int x = 0; x < MAZE_W; ++x)
        if (maze[z][x] == WALL) DrawWallBlock((float)x, (float)z);
}

void DrawMiniMap() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, winWidth, 0, winHeight);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_TEXTURE_2D); glDisable(GL_LIGHTING);

    float ms = 200.0f, ox = winWidth - ms - 10, oy = winHeight - ms - 10;
    float cw = ms / MAZE_W, ch = ms / MAZE_H;

    glColor4f(0, 0, 0, 0.6f); glRectf(ox - 2, oy - 2, ox + ms + 2, oy + ms + 2);
    glColor3f(1, 1, 1);
    for (int z = 0; z < MAZE_H; ++z) for (int x = 0; x < MAZE_W; ++x)
        if (maze[z][x] == WALL) glRectf(ox + x * cw, oy + z * ch, ox + (x + 1) * cw, oy + (z + 1) * ch);

    glColor3f(1, 1, 0);
    for (const auto& obj : g_worldObjects) {
        if (!obj.collected) {
            float cx = ox + (obj.mazeX + 0.5f) * cw, cy = oy + (obj.mazeY + 0.5f) * ch;
            glRectf(cx - 2, cy - 2, cx + 2, cy + 2);
        }
    }

    glColor3f(1, 0, 0);
    float px = ox + (camX / (MAZE_W * CELL_SIZE)) * ms, py = oy + (camZ / (MAZE_H * CELL_SIZE)) * ms;
    glBegin(GL_TRIANGLE_FAN); glVertex2f(px, py);
    for (int i = 0; i <= 16; ++i) glVertex2f(px + cos(i / 16.0f * 6.28f) * 4, py + sin(i / 16.0f * 6.28f) * 4);
    glEnd();

    glEnable(GL_DEPTH_TEST); glEnable(GL_TEXTURE_2D);
    if (g_gameState == STATE_PLAYING) glEnable(GL_LIGHTING);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

// ================== [NEW]   ׷̵ (Ƽ ) ==================

void DrawSkyCylinder() {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST); // ̹Ƿ  ڿ ׸

    glPushMatrix();
    // ϴ ī޶ ġ  (ٴϰ  ->   ȿ)
    glTranslatef(camX, camY, camZ);

    float radius = 200.0f; // ſ ū 
    float topY = 150.0f;
    float botY = -100.0f;
    int slices = 32;

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; ++i) {
        float theta = (float)i / slices * 2.0f * 3.141592f;
        float x = cosf(theta) * radius;
        float z = sinf(theta) * radius;

        // : £  (ϴ)
        glColor3f(0.1f, 0.05f, 0.2f);
        glVertex3f(x, topY, z);

        // Ʒ: Ÿ  ( )
        glColor3f(0.9f, 0.4f, 0.2f);
        glVertex3f(x, botY, z);
    }
    glEnd();

    // ¾ ׸ (ϴð Բ ȸϵ ⿡ )
    glPushMatrix();
    // Z (-180) ¾ ġ
    glTranslatef(0.0f, 10.0f, -180.0f);
    glColor3f(1.0f, 0.6f, 0.1f);
    glutSolidSphere(15.0, 30, 30); // ¾ ũ Ȯ
    glPopMatrix();

    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}


// 2.    (ȫ/)
void DrawSunsetClouds() {
    glDisable(GL_LIGHTING);
    glColor3f(0.8f, 0.5f, 0.5f); // ȫ 

    float move = g_cutsceneTime * 1.5f;

    for (int i = 0; i < 10; ++i) {
        glPushMatrix();
        float cx = -80.0f + (float)((i * 25 + (int)move) % 160);
        float cy = 20.0f + (i % 3) * 4.0f;
        float cz = -50.0f + (i * 15) % 80;

        glTranslatef(cx, cy, cz);
        glScalef(2.5f, 0.8f, 1.2f);

        glutSolidSphere(4.0, 12, 12);
        glTranslatef(2.5f, 0.2f, 0.5f); glutSolidSphere(3.0, 12, 12);
        glTranslatef(-4.0f, -0.1f, -0.5f); glutSolidSphere(2.8, 12, 12);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

// 3.   ( ῡ  Ӱ ϰ )
void DrawHillSunset() {
    glDisable(GL_TEXTURE_2D);

    int steps = 60;
    float range = 60.0f; //   а
    float stepSize = range * 2.0f / steps;

    for (int z = 0; z < steps; ++z) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int x = 0; x <= steps; ++x) {
            float px = -range + x * stepSize;
            float pz = -range + z * stepSize;
            float pzNext = -range + (z + 1) * stepSize;

            float h1 = 8.0f * exp(-(px * px + pz * pz) / 400.0f); //   ϸϰ
            float h2 = 8.0f * exp(-(px * px + pzNext * pzNext) / 400.0f);

            float c1 = 0.2f + h1 * 0.03f;
            float c2 = 0.2f + h2 * 0.03f;

            glNormal3f(px, h1, pz);
            glColor3f(0.3f, 0.25f + c1, 0.1f);
            glVertex3f(px, h1, pz);

            glNormal3f(px, h2, pzNext);
            glColor3f(0.3f, 0.25f + c2, 0.1f);
            glVertex3f(px, h2, pzNext);
        }
        glEnd();
    }
}

void DrawJoyScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ī޶ 
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0, (float)winWidth / winHeight, 0.1, 500.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    float dx = cos(camYaw) * cos(camPitch);
    float dy = sin(camPitch);
    float dz = sin(camYaw) * cos(camPitch);
    gluLookAt(camX, camY, camZ,
        camX + dx, camY + dy, camZ + dz,
        0.0f, 1.0f, 0.0f);

    // 1. ϴ
    DrawSkyCylinder();

    // 2.  & Ȱ
    glEnable(GL_FOG);
    GLfloat fogColor[] = { 0.9f, 0.5f, 0.3f, 1.0f };
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.01f);

    GLfloat sunPos[] = { 0.0f, 20.0f, -100.0f, 1.0f };
    GLfloat sunColor[] = { 1.0f, 0.8f, 0.6f, 1.0f };
    glEnable(GL_LIGHTING);
    glLightfv(GL_LIGHT0, GL_POSITION, sunPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunColor);

    // 3. [ ũ] ī޶  ߿ ġ
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glEnable(GL_TEXTURE_2D);

    int frameIdx = (int)(g_cutsceneTime * 10);
    if (frameIdx >= (int)g_videoFrames.size()) frameIdx = (int)g_videoFrames.size() - 1;
    if (frameIdx < 0) frameIdx = 0;

    if (!g_videoFrames.empty()) glBindTexture(GL_TEXTURE_2D, g_videoFrames[frameIdx]);
    else glBindTexture(GL_TEXTURE_2D, g_wallTex);

    glPushMatrix();
    // [ġ ] 
    // Z = -5.0f : ī޶(Z=15)  
    // Y = 10.0f : (Y=5)  (ϴÿ  )
    glTranslatef(0.0f, 10.0f, -5.0f);
    glColor3f(1.0f, 1.0f, 1.0f);

    // [ũ]  ũ  (16:9 )
    float hw = 8.0f;   // 
    float hh = 4.5f;   // 

    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);

    // [Ϲ ]
    glTexCoord2f(0, 1); glVertex3f(-hw, -hh, 0); // ϴ
    glTexCoord2f(1, 1); glVertex3f(hw, -hh, 0); // ϴ
    glTexCoord2f(1, 0); glVertex3f(hw, hh, 0); // 
    glTexCoord2f(0, 0); glVertex3f(-hw, hh, 0); // »
    glEnd();
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);

    // 4. []   ->  ٴ 
    // DrawHillSunset(); //   

    //  ο (ٴ) ׷  
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.15f, 0.1f, 0.05f); // ο  
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    float groundSize = 200.0f;
    glVertex3f(-groundSize, 0.0f, -groundSize);
    glVertex3f(groundSize, 0.0f, -groundSize);
    glVertex3f(groundSize, 0.0f, groundSize);
    glVertex3f(-groundSize, 0.0f, groundSize);
    glEnd();

    // 5. 
    DrawSunsetClouds();

    glDisable(GL_FOG);
}
void InitStars() {
    g_stars.clear();
    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_real_distribution<float> pos(-80.0f, 80.0f), h(10.0f, 60.0f), b(0.5f, 1.0f);
    for (int i = 0; i < 400; ++i) g_stars.push_back({ pos(gen), h(gen), pos(gen), b(gen) });
}

float GetWaveHeight(float x, float z, float t) {
    return sinf(x * 0.5f + t) * 0.5f + sinf(z * 0.3f + t * 0.8f) * 0.5f + sinf((x + z) * 0.2f + t * 1.5f) * 0.2f;
}

void DrawNightScene() {
    glClearColor(0.01f, 0.01f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0, (float)winWidth / winHeight, 0.1, 300.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    float dx = cos(camYaw) * cos(camPitch), dy = sin(camPitch), dz = sin(camYaw) * cos(camPitch);
    float waveY = sinf(g_cutsceneTime * 1.2f) * 0.15f;
    gluLookAt(camX, camY + waveY, camZ, camX + dx, camY + dy + waveY, camZ + dz, 0, 1, 0);

    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    glPointSize(2.0f); glBegin(GL_POINTS);
    for (const auto& s : g_stars) {
        float tw = 0.8f + 0.2f * sinf(g_cutsceneTime * 5.0f + s.x);
        glColor3f(s.brightness * tw, s.brightness * tw, s.brightness * tw);
        glVertex3f(s.x, s.y, s.z);
    }
    glEnd();

    glEnable(GL_TEXTURE_2D);
    int frameIdx = (int)(g_cutsceneTime * 10);
    if (frameIdx >= (int)g_videoFrames.size()) frameIdx = (int)g_videoFrames.size() - 1;
    if (frameIdx < 0) frameIdx = 0;
    if (!g_videoFrames.empty()) glBindTexture(GL_TEXTURE_2D, g_videoFrames[frameIdx]);
    else glBindTexture(GL_TEXTURE_2D, g_wallTex);

    glPushMatrix(); glTranslatef(0.0f, 25.0f, 45.0f);
    float flk = 0.95f + (rand() % 5) * 0.01f; glColor3f(flk, flk, flk);
    glBegin(GL_QUADS); glTexCoord2f(0, 1); glVertex3f(-24, -13.5, 0); glTexCoord2f(1, 1); glVertex3f(24, -13.5, 0); glTexCoord2f(1, 0); glVertex3f(24, 13.5, 0); glTexCoord2f(0, 0); glVertex3f(-24, 13.5, 0); glEnd(); glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    float seaY = -1.0f;
    if (g_cutsceneTime > 12.5f) seaY += (g_cutsceneTime - 12.5f) * (g_cutsceneTime - 12.5f) * 6.0f;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.15f, 0.4f, 0.85f);
    for (float z = -100; z < 100; z += 2.5f) {
        glBegin(GL_QUAD_STRIP);
        for (float x = -100; x <= 100; x += 2.5f) {
            glVertex3f(x, seaY + GetWaveHeight(x, z, g_cutsceneTime), z);
            glVertex3f(x, seaY + GetWaveHeight(x, z + 2.5f, g_cutsceneTime), z + 2.5f);
        }
        glEnd();
    }
    glDisable(GL_BLEND);

    if (seaY < 5.0f) {
        glPushMatrix();
        float bY = seaY + GetWaveHeight(0, 5.0f, g_cutsceneTime);
        glTranslatef(0, bY + 1.0f, 5.0f);
        glRotatef(cosf(g_cutsceneTime * 0.8f) * 4.0f, 1, 0, 0); glRotatef(sinf(g_cutsceneTime * 0.6f) * 2.0f, 0, 0, 1); glRotatef(180, 0, 1, 0);
        glScalef(2.5f, 2.5f, 2.5f);
        glColor3f(0.45f, 0.25f, 0.1f); glBegin(GL_TRIANGLES);
        glVertex3f(-1, 0.5, -1.5); glVertex3f(1, 0.5, -1.5); glVertex3f(0, 0, 1.5);
        glVertex3f(-1, 0.5, -1.5); glVertex3f(0, 0, 1.5); glVertex3f(0, 0, -1);
        glVertex3f(1, 0.5, -1.5); glVertex3f(0, 0, -1); glVertex3f(0, 0, 1.5);
        glColor3f(0.6f, 0.45f, 0.25f); glVertex3f(-1, 0.5, -1.5); glVertex3f(0, 0.5, 1.8); glVertex3f(1, 0.5, -1.5); glEnd();
        glColor3f(0.3f, 0.2f, 0.1f); glLineWidth(6.0f); glBegin(GL_LINES); glVertex3f(0, 0.5, 0); glVertex3f(0, 3.5, 0); glEnd();
        glColor4f(0.95f, 0.95f, 0.95f, 0.9f); glEnable(GL_BLEND); glBegin(GL_TRIANGLES); glVertex3f(0, 3.5, 0); glVertex3f(0, 1.0, 0); glVertex3f(1.5, 1.2, 0.3); glEnd(); glDisable(GL_BLEND);
        glPopMatrix();
    }
}

void InitFlowers() {
}
void DrawFlowers() {}
void DrawClouds() {}

// ================== [NEW]  ũ  ==================

struct FireParticle {
    float x, y;      // ġ
    float vx, vy;    // ӵ
    float r, g, b;   // 
    float life;      //  (1.0 -> 0.0)
    float size;      // ũ
};

std::vector<FireParticle> g_fireworks; //  ƼŬ 

// ؽƮ   Լ
void DrawString(float x, float y, void* font, const char* string) {
    const char* c;
    glRasterPos2f(x, y);
    for (c = string; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}

// ؽƮ  
int GetTextWidth(void* font, const char* string) {
    int width = 0;
    for (const char* c = string; *c != '\0'; c++) {
        width += glutBitmapWidth(font, *c);
    }
    return width;
}

// [ٽ]  Ʈ  ׸ Լ
void UpdateAndDrawFireworks() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); //  ġ 

    // 1.   ( Ȯ)
    // Ӹ  Ȯ ο  ߻
    if (rand() % 20 == 0) {
        float cx = (float)(rand() % winWidth);     //  X ġ
        float cy = (float)(rand() % winHeight);    //  Y ġ

        //   (, ʷ, Ķ, ,  )
        float r = (rand() % 10) * 0.1f;
        float g = (rand() % 10) * 0.1f;
        float b = (rand() % 10) * 0.1f;

        // ϳ   ƼŬ 50 
        for (int i = 0; i < 50; ++i) {
            FireParticle p;
            p.x = cx;
            p.y = cy;

            //   ӵ 
            float angle = (rand() % 360) * 3.141592f / 180.0f;
            float speed = (rand() % 50 + 50) * 0.1f; // ӵ 

            p.vx = cos(angle) * speed;
            p.vy = sin(angle) * speed;

            p.r = r; p.g = g; p.b = b;
            p.life = 1.0f;
            p.size = (rand() % 3 + 2.0f); // ũ 2~4

            g_fireworks.push_back(p);
        }
    }

    // 2. ƼŬ ̵  ׸
    glPointSize(3.0f);
    glBegin(GL_POINTS);

    // iterator Ͽ ȸϸ  ϰ 
    for (auto it = g_fireworks.begin(); it != g_fireworks.end(); ) {
        //  
        it->x += it->vx;
        it->y += it->vy;
        it->vy -= 0.15f; // ߷  (Ʒ )
        it->life -= 0.015f; //   ( )

        // ׸
        if (it->life > 0.0f) {
            glColor4f(it->r, it->g, it->b, it->life); //  
            glVertex2f(it->x, it->y);
            ++it;
        }
        else {
            //   ƼŬ 
            it = g_fireworks.erase(it);
        }
    }
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //   
}

void DrawEndingCredits() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // : 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2D  
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, winWidth, 0, winHeight);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_TEXTURE_2D);

    // [߰]  Ʈ ( ڿ ̵  ׸)
    UpdateAndDrawFireworks();

    // ----------------------------------------------------
    // ũ ؽƮ (  )
    struct CreditLine {
        std::string text;
        void* font;
        float space;
        float r, g, b;
    };

    std::vector<CreditLine> lines = {
        { "Emotion.exe", GLUT_BITMAP_TIMES_ROMAN_24, 60, 1.0f, 1.0f, 0.0f },
        { "A 3D Maze Project", GLUT_BITMAP_HELVETICA_18, 100, 0.8f, 0.8f, 0.8f },

        { "DEVELOPED BY", GLUT_BITMAP_HELVETICA_18, 40, 0.5f, 0.5f, 1.0f },
        { "Team_REDBULL", GLUT_BITMAP_TIMES_ROMAN_24, 80, 1.0f, 1.0f, 1.0f },

        { "PROGRAMMING", GLUT_BITMAP_HELVETICA_18, 40, 0.5f, 0.5f, 1.0f },
        { "Maze Algorithm & OpenGL Engine", GLUT_BITMAP_HELVETICA_18, 30, 1.0f, 1.0f, 1.0f },
        { "Physics & Camera System", GLUT_BITMAP_HELVETICA_18, 80, 1.0f, 1.0f, 1.0f },

        { "ART & DESIGN", GLUT_BITMAP_HELVETICA_18, 40, 0.5f, 0.5f, 1.0f },
        { "3D Modeling (SOR)", GLUT_BITMAP_HELVETICA_18, 30, 1.0f, 1.0f, 1.0f },
        { "Texture & Environment", GLUT_BITMAP_HELVETICA_18, 80, 1.0f, 1.0f, 1.0f },

        { "Producer", GLUT_BITMAP_HELVETICA_18, 40, 0.5f, 0.5f, 1.0f },
        { "20225155_moon", GLUT_BITMAP_HELVETICA_18, 30, 1.0f, 1.0f, 1.0f },

        { "20224284_park", GLUT_BITMAP_HELVETICA_18, 30, 1.0f, 1.0f, 1.0f },

        { "20224195_sung", GLUT_BITMAP_HELVETICA_18, 30, 1.0f, 1.0f, 1.0f },

        { "Thank You For Playing", GLUT_BITMAP_TIMES_ROMAN_24, 0, 1.0f, 0.5f, 0.0f }
    };

    float scrollSpeed = 60.0f;
    float startY = -50.0f + (g_cutsceneTime * scrollSpeed);
    float currentY = startY;

    for (const auto& line : lines) {
        if (currentY > -50 && currentY < winHeight + 50) {
            int w = GetTextWidth(line.font, line.text.c_str());
            float x = (winWidth - w) / 2.0f;
            glColor3f(line.r, line.g, line.b);
            DrawString(x, currentY, line.font, line.text.c_str());
        }
        currentY -= line.space;
    }

    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}