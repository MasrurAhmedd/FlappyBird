#ifdef _WIN32
#include <windows.h>
#endif
#define BUILDING_COUNT 10

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <iostream>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <string>
using namespace std;

// ----------------------------- GAME STATE ENUMS --------------------------------
enum GameState {
    STATE_MENU = 0,
    STATE_RUNNING = 1,
    STATE_OVER = 2
};

GameState currentState = STATE_MENU;

// ----------------------------- GLOBAL VARIABLES -------------------------------
float buildingHeights[BUILDING_COUNT];
// Bird
float birdY = 0.0f;
float velocity = 0.0f;
float gravity = -0.008;
int flapState = 0;
int flapCounter = 0;

// Pipes
float pipeSpeed = 0.2f;
const int PIPE_COUNT = 4;
const float PIPE_WIDTH = 1.6f;
float pipeSpacing = 7.5f;
struct Pipe {
    float x;
    float gapY;
    float gapSize;
    float color[3];
};
Pipe pipeList[PIPE_COUNT];

// Screen bounds
const float SCREEN_LEFT = -10.0f;
const float SCREEN_RIGHT = 10.0f;
const float SCREEN_TOP = 10.0f;
const float SCREEN_BOTTOM = -10.0f;


// Score & Tracker
int score = 0;

// Background Time Cycle
float bgColor[3], dayColor[3] = {0.52f, 0.80f, 0.92f}, nightColor[3] = {0.08f, 0.09f, 0.32f};
float sunX = -8, sunY = 4;
float nightAngle = 0.0f;
bool isNight = false;

// Animation helpers
float cloudOffset = 0;

// ----------------------------- INITIALIZATION -------------------------------

void resetGame();

void randomizePipe(int i) {
    pipeList[i].x = SCREEN_RIGHT + i * pipeSpacing;
    pipeList[i].gapY = rand() % 8 - 4;


    pipeList[i].gapSize = fmax(2.0f, 3.0f - (score * 0.02f));


    int theme = rand() % 3;
    if (theme == 0) {
        pipeList[i].color[0] = 0.0f; pipeList[i].color[1] = 0.8f; pipeList[i].color[2] = 0.3f; // Bright Green
    } else if (theme == 1) {
        pipeList[i].color[0] = 0.25f; pipeList[i].color[1] = 0.6f; pipeList[i].color[2] = 1.0f; // Blue
    } else {
        pipeList[i].color[0] = 0.0f; pipeList[i].color[1] = 0.4f; pipeList[i].color[2] = 0.0f; // Dark green
    }
}

void initPipes() {
    for (int i = 0; i < PIPE_COUNT; ++i) {
        randomizePipe(i);
        pipeList[i].x += i * pipeSpacing;
    }
}
void initBuildings() {
    for (int i = 0; i < BUILDING_COUNT; ++i) {
        buildingHeights[i] = 2.5f + rand() % 3;
    }
}
void initColors() {
    for (int i = 0; i < 3; i++)
        bgColor[i] = dayColor[i];
}

void initGL() {
    glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0);
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(SCREEN_LEFT, SCREEN_RIGHT, SCREEN_BOTTOM, SCREEN_TOP);
    glMatrixMode(GL_MODELVIEW);
    srand(static_cast<unsigned int>(time(0)));

    resetGame();   // Set the game to initial state
}

void resetGame() {
    birdY = 0.0f;
    velocity = 0.0f;
    score = 0;

    initPipes();
    initColors();

    currentState = STATE_MENU;
}






//  ...................................................................................................................................................................................................



// ----------------------------- DRAWING UTILITIES -------------------------------

void drawCircle(float x, float y, float r, int sa = 0, int ea = 360) {
    glBegin(GL_POLYGON);
    for (int i = sa; i <= ea; i++) {
        float rad = i * 3.1416 / 180;
        glVertex2f(x + cos(rad) * r, y + sin(rad) * r);
    }
    glEnd();
}

// ----------------------------- DRAW RECTANGLE SHAPE -----------------------------
void drawRect(float x1, float y1, float x2, float y2, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_POLYGON);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

// ----------------------------- BACKGROUND MODULE -------------------------------

void updateBackgroundColor() {
    float target[3];
    if (!isNight)
        std::copy(std::begin(nightColor), std::end(nightColor), target);
    else
        std::copy(std::begin(dayColor), std::end(dayColor), target);

    for (int i = 0; i < 3; ++i) {
        if (abs(bgColor[i] - target[i]) > 0.001f) {
            bgColor[i] += (target[i] - bgColor[i]) * 0.002;
        }
    }

    if (fabs(bgColor[0] - nightColor[0]) < 0.002f)
        isNight = !isNight;

    glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0);
}

// ----------------------------- SUN / MOON -------------------------------
void drawSunMoon() {
    sunX = 8 * cos(nightAngle);
    sunY = 6 * sin(nightAngle);
    nightAngle += 0.0007;
    if (nightAngle > 6.283) nightAngle = 0;

    // Sun
    glColor3f(1.0, 1.0, 0.0);
    drawCircle(sunX, sunY, 0.6);

    // Moon (opposite end)
    glColor3f(1.0, 1.0, 1.0);
    drawCircle(-sunX, -sunY, 0.4);
}

void drawBuildings() {
    int index = 0;
    float startX = -10.0f;

    for (float x = startX; index < BUILDING_COUNT; x += 2.75f, index++) {
        float h = buildingHeights[index];

        // Main building body
        glColor3f(0.35f, 0.35f, 0.45f); // Grayish Blue tone
        drawRect(x, -8.5f, x + 2.4f, -8.5f + h, 0.35f, 0.35f, 0.45f);

        // Windows - loop through grid pattern
        float winStartY = -8.2f;
        float winTopY = -8.5f + h - 0.5f;

        for (float y = winStartY; y <= winTopY; y += 0.6f) {
            for (float wx = x + 0.3f; wx < x + 2.0f; wx += 0.5f) {

                if (isNight) {
                    glColor3f(1.0f, 1.0f, 0.2f); // Bright yellow at night
                }
                else {
                    glColor3f(0.7f, 0.9f, 1.0f); // Bluish windows in daytime
                }

                drawRect(wx, y, wx + 0.3f, y + 0.35f, 1, 1, 1);
            }
        }

        // Roof edge
        glColor3f(0.2f, 0.2f, 0.25f);
        drawRect(x, -8.5f + h, x + 2.4f, -8.5f + h + 0.2f, 0.2f, 0.2f, 0.25f);
    }
}

void drawMountains() {
    // Distant mountains (use grayish blue)
    glColor3f(0.4f, 0.45f, 0.5f);

    glBegin(GL_TRIANGLES);
    glVertex2f(-10, -6);
    glVertex2f(-6.5, 2);
    glVertex2f(-3, -6);
    glVertex2f(-5, -6);
    glVertex2f(-1.5, 1.5);
    glVertex2f(2, -6);
    glVertex2f(0, -6);
    glVertex2f(4, 2.5);
    glVertex2f(8, -6);
    glVertex2f(6, -6);
    glVertex2f(10, 1.8);
    glVertex2f(12, -6);
    glEnd();

    // Snow caps (bright white on higher peak tips)
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_TRIANGLES);
    glVertex2f(-6.5, 2); glVertex2f(-6.2, 1.5); glVertex2f(-6.8, 1.5);
    glVertex2f(-1.5, 1.5); glVertex2f(-1.2, 1.1); glVertex2f(-1.8, 1.1);
    glVertex2f(4, 2.5); glVertex2f(3.5, 1.8); glVertex2f(4.5, 1.8);
    glVertex2f(10, 1.8); glVertex2f(9.7, 1.3); glVertex2f(10.3, 1.3);
    glEnd();
}

void drawRiver() {
    // River base layer
    glColor3f(0.2f, 0.5f, 1.0f); // Water blue
    glBegin(GL_POLYGON);
    glVertex2f(SCREEN_LEFT, -9);
    glVertex2f(SCREEN_RIGHT, -9);
    glVertex2f(SCREEN_RIGHT, -10);
    glVertex2f(SCREEN_LEFT, -10);
    glEnd();

    // Light blue wave edges
    glColor3f(0.6f, 0.8f, 1.0f);
    for (float x = SCREEN_LEFT + 1; x < SCREEN_RIGHT; x += 1.5) {
        glBegin(GL_POLYGON);
        for (int i = 0; i < 360; i++) {
            float rad = i * 3.1416f / 180;
            float cx = x + cos(rad) * 0.15f;
            float cy = -9.2 + sin(rad) * 0.08f;
            glVertex2f(cx, cy);
        }
        glEnd();
    }
}

// ----------------------------- STARS -------------------------------
void drawStars() {
    if (!isNight) return;
    glColor3f(1.0, 1.0, 1.0);
    glPointSize(2);
    glBegin(GL_POINTS);
    for (int i = 0; i < 60; ++i) {
        float x = (rand() % 1800) / 100.0f - 9;
        float y = (rand() % 400) / 100.0f + 5;
        if (rand() % 3 == 0)
            glVertex2f(x, y);
    }
    glEnd();
}

// ----------------------------- CLOUDS -------------------------------
void drawOneCloud(float x, float y) {
    glColor3f(1.0, 1.0, 1.0);
    drawCircle(x, y, 0.6);
    drawCircle(x + 0.5, y + 0.2, 0.5);
    drawCircle(x + 1.0, y, 0.6);
    drawCircle(x + 0.5, y - 0.3, 0.4);
}

void drawClouds() {
    drawOneCloud(-8 + cloudOffset, 6.5);
    drawOneCloud(-1 + 1.2 * cloudOffset, 7.3);
    drawOneCloud(5 + 0.8 * cloudOffset, 5.9);
    cloudOffset += 0.002f;
    if (cloudOffset > 20)
        cloudOffset = -10;
}

// ----------------------------- BUILDINGS -------------------------------


// ----------------------------- GROUND -------------------------------
void drawGround() {
    glColor3f(0.2, 0.7, 0.3);
    glBegin(GL_POLYGON);
    glVertex2f(SCREEN_LEFT, SCREEN_BOTTOM);
    glVertex2f(SCREEN_RIGHT, SCREEN_BOTTOM);
    glVertex2f(SCREEN_RIGHT, -8.4);
    glVertex2f(SCREEN_LEFT, -8.4);
    glEnd();

    // Dashed road
    glColor3f(1.0, 1.0, 1.0);
    for (float x = SCREEN_LEFT; x < SCREEN_RIGHT; x += 1.5)
        drawRect(x, -8.7, x + 0.5, -8.6, 1, 1, 1);
}

// ----------------------------- TREES -------------------------------
void drawTree(float x, float y) {
    glColor3f(0.5f, 0.25f, 0.1f); // brown trunk
    drawRect(x - 0.1f, y, x + 0.1f, y + 0.5f, 0.5, 0.25, 0.1);
    glColor3f(0.0, 0.6, 0.0);
    drawCircle(x, y + 0.8f, 0.4);
    drawCircle(x + 0.2f, y + 0.6f, 0.3);
    drawCircle(x - 0.2f, y + 0.6f, 0.3);
}

void drawForest() {
    for (float x = -9.5; x < 9.5; x += 2.5)
        drawTree(x, -8.4);
}

// ----------------------------- BACKGROUND MASTER CALL -------------------------------
void drawBackground() {
    updateBackgroundColor();

    // Sky background
    drawRect(SCREEN_LEFT, SCREEN_BOTTOM, SCREEN_RIGHT, SCREEN_TOP, bgColor[0], bgColor[1], bgColor[2]);

    drawMountains();
    drawStars();
    drawSunMoon();
    drawClouds();
    drawBuildings();
    drawForest();
    drawGround();
    drawRiver();
}


//................................................................................................................................................................................................





// ----------------------------- Draw Pipe Pair -------------------------
void drawSinglePipe(Pipe& p) {
    float gapTop = p.gapY + p.gapSize;
    float gapBottom = p.gapY - p.gapSize;
    float x1 = p.x;
    float x2 = p.x + PIPE_WIDTH;

    // Pipe body color
    glColor3f(p.color[0], p.color[1], p.color[2]);

    //Bottom Pipe
    drawRect(x1, SCREEN_BOTTOM, x2, gapBottom, p.color[0], p.color[1], p.color[2]);

    //  Top Pipe
    drawRect(x1, gapTop, x2, SCREEN_TOP, p.color[0], p.color[1], p.color[2]);

    //Pipe caps
    glColor3f(0.2, 0.4, 0.2);  // Darker green caps
    drawCircle(x1 + PIPE_WIDTH / 2, gapTop, PIPE_WIDTH / 2.2, 0, 180);   // top
    drawCircle(x1 + PIPE_WIDTH / 2, gapBottom, PIPE_WIDTH / 2.2, 180, 360); // bottom

    //Side shading (3D shadow look)
    drawRect(x2, SCREEN_BOTTOM, x2 + 0.1, SCREEN_TOP, 0.0, 0.3, 0.0); // right side shadow
}

// ----------------------------- Pipe Manager -------------------------
void drawAllPipes() {
    for (int i = 0; i < PIPE_COUNT; ++i) {
        drawSinglePipe(pipeList[i]);

        // Move pipe to left
        pipeList[i].x -= pipeSpeed;

        // If pipe exits screen, recycle it to right
        if (pipeList[i].x + PIPE_WIDTH < SCREEN_LEFT) {
            randomizePipe(i);

            // REPOSITION to the furthest pipe
            float maxX = SCREEN_LEFT;
            for (int j = 0; j < PIPE_COUNT; ++j)
                if (pipeList[j].x > maxX)
                    maxX = pipeList[j].x;

            pipeList[i].x = maxX + pipeSpacing;
        }

        // SCORING logic
        float pipeMid = pipeList[i].x + PIPE_WIDTH / 2;
        if (abs(pipeMid + 5.0f) < 0.04f && currentState == STATE_RUNNING) {
            score++;

            //  Difficulty increase as score grows
            if (score % 5 == 0 && pipeSpeed < 0.09f)
                pipeSpeed += 0.005f;

            if (score % 10 == 0) {
                for (int j = 0; j < PIPE_COUNT; ++j)
                    pipeList[j].gapSize -= 0.1f;

                if (pipeSpacing > 5.0f)
                    pipeSpacing -= 0.3f;
            }
        }

        //  COLLISION detection
        float birdLeft = -5.6f;
        float birdRight = -4.4f;
        float birdTop = birdY + 0.6f;
        float birdBottom = birdY - 0.6f;

        bool xOverlap = birdRight > pipeList[i].x && birdLeft < pipeList[i].x + PIPE_WIDTH;
        bool yOutOfGap = birdTop > pipeList[i].gapY + pipeList[i].gapSize ||
                         birdBottom < pipeList[i].gapY - pipeList[i].gapSize;

        if (xOverlap && yOutOfGap && currentState == STATE_RUNNING) {
            currentState = STATE_OVER;
        }
    }
}



//.................................................................................................................................................................................................


// --------------------------- Bird Physics Per Frame ------------------------

void updateBirdPhysics() {
    velocity += gravity;
    birdY += velocity;

    // Limit to screen bounds (game over if exceeded)
    if (birdY < SCREEN_BOTTOM + 1.0f || birdY > SCREEN_TOP - 1.0f) {
        currentState = STATE_OVER;
    }
}

// --------------------------- Mouse Click: Flap -----------------------------

void handleMouse(int btn, int state, int x, int y) {
    if (btn == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (currentState == STATE_MENU) {
            currentState = STATE_RUNNING;
        }
        else if (currentState == STATE_RUNNING) {
            velocity = 0.20f; // Upward boost
        }
        else if (currentState == STATE_OVER) {
            resetGame(); // Restart on click at game over
        }
    }
}

//...................................................................................................................................................................................................


// --------------------------- TEXT DRAWERS ------------------------

void drawText(float x, float y, void* font, const string& str) {
    glRasterPos2f(x, y);
    for (char ch : str) {
        glutBitmapCharacter(font, ch);
    }
}


void drawScore() {
    glColor3f(0, 0, 0);
    string scoreStr = "Score: " + to_string(score);
    drawText(-9.5, 9, GLUT_BITMAP_HELVETICA_18, scoreStr);
}


void drawMenuScreen() {
    glColor3f(0.0f, 0.3f, 0.6f);
    drawText(-2.8, 3.5, GLUT_BITMAP_TIMES_ROMAN_24, "FLAPPY BIRD ");

    drawText(-2.5, 2.5, GLUT_BITMAP_HELVETICA_18, "Group: 9");





    drawText(-2.0, 1.5, GLUT_BITMAP_HELVETICA_18, "COMPUTER GRAPHICS PROJECT ");

    glColor3f(0.1f, 0.1f, 0.1f);
    drawText(-1.8, -2.5, GLUT_BITMAP_TIMES_ROMAN_24, "Click Anywhere to Start");
}


void drawGameOverScreen() {
    glColor3f(0.8, 0.0, 0.0);
    drawText(-1.5, 2, GLUT_BITMAP_TIMES_ROMAN_24, "Game Over");

    glColor3f(0.1f, 0.1f, 0.1f);
    string scoreStr = "Final Score: " + to_string(score);
    drawText(-2.0, 1.0, GLUT_BITMAP_HELVETICA_18, scoreStr);

    drawText(-2.3, -1.0, GLUT_BITMAP_HELVETICA_18, "Click Anywhere to Restart");
}


void drawWingsAnimation(float y) {
    float offsetY;

    switch (flapState) {
        case 0: offsetY = 0.25f; break;
        case 1: offsetY = 0.0f; break;
        case 2: offsetY = -0.25f; break;
    }

    glColor3f(0.95, 0.95, 0.95); // Light grey wings
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 360; i++) {
        float rad = i * 3.1416f / 180;
        glVertex2f(-5.4 + cos(rad) * 0.3f, y + offsetY + sin(rad) * 0.3f);
    }
    glEnd();

    // Animate wing position every few frames
    flapCounter++;
    if (flapCounter > 6) {
        flapCounter = 0;
        flapState = (flapState + 1) % 3;
    }
}

void drawBird(float y) {
    // 🟡 Body
    glColor3f(0.0f, 0.0f, 0.0f); // Outline
    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i++) {
        float rad = i * 3.1416f / 180;
        glVertex2f(-5.0 + cos(rad) * 0.63f, y + sin(rad) * 0.63f);
    }
    glEnd();

    glColor3f(1.0, 1.0, 0.0); // Yellow body
    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i++) {
        float rad = i * 3.1416f / 180;
        glVertex2f(-5.0 + cos(rad) * 0.6f, y + sin(rad) * 0.6f);
    }
    glEnd();

    // 👀 Eye
    glColor3f(0.0, 0.0, 0.0); // Black outline
    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i++) {
        float rad = i * 3.1416f / 180;
        glVertex2f(-4.7 + cos(rad) * 0.13f, y + 0.2 + sin(rad) * 0.13f);
    }
    glEnd();

    glColor3f(1.0, 1.0, 1.0); // White eyeball
    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i++) {
        float rad = i * 3.1416f / 180;
        glVertex2f(-4.7 + cos(rad) * 0.10f, y + 0.2 + sin(rad) * 0.10f);
    }
    glEnd();

    glColor3f(0.0, 0.0, 0.0); // Pupil
    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i++) {
        float rad = i * 3.1416f / 180;
        glVertex2f(-4.7 + cos(rad) * 0.03f, y + 0.26 + sin(rad) * 0.03f);
    }
    glEnd();

    // 👄 Beak (Two Half Circles)
    glColor3f(1.0, 0.3, 0.0); // Orange beak

    // Top beak
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 180; i++) {
        float rad = i * 3.1416f / 180;
        glVertex2f(-4.4 + cos(rad) * 0.18f, y - 0.1 + sin(rad) * 0.18f);
    }
    glEnd();

    // Bottom beak
    glBegin(GL_POLYGON);
    for (int i = 180; i <= 360; i++) {
        float rad = i * 3.1416f / 180;
        glVertex2f(-4.4 + cos(rad) * 0.18f, y - 0.15 + sin(rad) * 0.18f);
    }
    glEnd();

    // 🪽 Wings
    drawWingsAnimation(y);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Always run background
    drawBackground();

    if (currentState == STATE_MENU) {
        drawMenuScreen();
    }
    else if (currentState == STATE_RUNNING) {
        updateBirdPhysics();
        drawBird(birdY);
        drawAllPipes();
        drawScore();
    }
    else if (currentState == STATE_OVER) {
        drawBird(birdY);
        drawAllPipes();
        drawGameOverScreen();
    }

    glFlush();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    glutPostRedisplay();
}



void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0); // ESC = exit
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(1000, 600);
    glutInitWindowPosition(150, 100);
    glutCreateWindow("Flappy Bird MASRUR");
     initBuildings();
    initGL();
    glutDisplayFunc(display);
    glutMouseFunc(handleMouse);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
