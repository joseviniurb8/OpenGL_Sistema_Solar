#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// -------------------- Estado geral --------------------
float t = 0.0f;
bool paused = false;       // controle de pausa
bool showOrbits = false;   // controle das órbitas

// Configuração das órbitas e planetas
float orbitRadii[]  = {16.0f, 20.0f, 24.0f, 28.0f, 35.0f, 48.0f, 60.0f, 72.0f};
float orbitSpeeds[] = {0.6f, 0.55f, 0.5f, 0.45f, 0.3f, 0.25f, 0.2f, 0.15f};

// Tamanhos dos planetas
float planetSizes[] = {0.75f, 1.35f, 1.5f, 0.9f, 4.5f, 5.25f, 3.2f, 3.0f};

// Rotação própria
float planetRotation[8] = {0,0,0,0,0,0,0,0};
float rotationSpeed[8]  = {2.0f,1.8f,1.6f,1.5f,1.2f,1.1f,1.0f,0.9f};

// IDs das texturas
GLuint planetTextures[8];
GLuint sunTexture;

// Câmera
float camAngle = 0.0f;
float camDistance = 90.0f;
float camHeight = 5.0f;
float camAngleManual = 0.0f;

// -------------------- Estrelas (céu) --------------------
struct Star {
    float x, y, z;   // posição
    float base;      // brilho base [0..1]
    float phase;     // fase para cintilação
};
const int NUM_STARS = 2000;
Star stars[NUM_STARS];
const float STAR_RADIUS = 220.0f; // maior que as órbitas (far <= 300)

// ======================================================
// Carregar textura (seguro): força RGBA, corrige alinhamento e
// permite optar por CLAMP_TO_EDGE (para o Sol).
// ======================================================
GLuint loadTexture(const char* filename, bool clampToEdge = false) {
    int w, h, n;
    stbi_set_flip_vertically_on_load(1);                // evita textura invertida
    unsigned char* data = stbi_load(filename, &w, &h, &n, 4); // força 4 canais (RGBA)

    if (!data) {
        printf("Erro ao carregar textura: %s\n", filename);
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // *** evita listras verticais (alinhamento de linhas) ***
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // filtros
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // wrap
    GLint wrap = clampToEdge ? GL_CLAMP_TO_EDGE : GL_REPEAT;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

    // upload + mipmaps
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    return texID;
}

// -------------------- Iluminação --------------------
void initLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Ambiente global quase zero (lado noturno bem escuro)
    GLfloat globalAmbient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    // Espectro do Sol (branco)
    GLfloat lightDiffuse[]  = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat lightSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    // Atenuação ~ 1/(kc + kq*d^2) (ajuste fino conforme desejar)
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.70f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION,   0.00f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION,0.001f);
}

// Gerar estrelas numa esfera
void initStars() {
    srand((unsigned)time(NULL));
    for (int i = 0; i < NUM_STARS; ++i) {
        // amostragem uniforme na esfera
        float u = (float)rand() / RAND_MAX;  // 0..1
        float v = (float)rand() / RAND_MAX;  // 0..1
        float theta = 2.0f * M_PI * u;
        float z = 2.0f * v - 1.0f;           // -1..1
        float r = sqrtf(1.0f - z*z);
        float x = r * cosf(theta);
        float y = r * sinf(theta);
        stars[i].x = STAR_RADIUS * x;
        stars[i].y = STAR_RADIUS * y;
        stars[i].z = STAR_RADIUS * z;

        stars[i].base  = 0.6f + 0.4f * ((float)rand() / RAND_MAX);  // 0.6..1.0
        stars[i].phase = 20.0f * ((float)rand() / RAND_MAX);        // fase aleatória
    }
}

// Desenhar o céu estrelado
void drawStars() {
    glDisable(GL_LIGHTING);
    // não escrever no depth para não interferir nos planetas
    glDepthMask(GL_FALSE);
    glPointSize(1.5f);

    glBegin(GL_POINTS);
    for (int i = 0; i < NUM_STARS; ++i) {
        // cintilação leve
        float tw = 0.85f + 0.15f * (0.5f + 0.5f * sinf(0.6f * t + stars[i].phase));
        float b = stars[i].base * tw;
        glColor3f(b, b, b);
        glVertex3f(stars[i].x, stars[i].y, stars[i].z);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
}

// Desenhar anel de Saturno
void drawRing(float innerRadius, float outerRadius) {
    int numSegments = 100;
    glDisable(GL_LIGHTING);
    glColor3f(0.8f, 0.8f, 0.6f);
    glBegin(GL_QUAD_STRIP);
    for (int i=0; i<=numSegments; i++) {
        float angle = 2.0f * M_PI * i / numSegments;
        float xInner = innerRadius * cos(angle);
        float zInner = innerRadius * sin(angle);
        float xOuter = outerRadius * cos(angle);
        float zOuter = outerRadius * sin(angle);
        glVertex3f(xInner, 0, zInner);
        glVertex3f(xOuter, 0, zOuter);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Câmera
    float camX = camDistance * cos(camAngle + camAngleManual);
    float camZ = camDistance * sin(camAngle + camAngleManual);
    float camY = camHeight;
    gluLookAt(camX, camY, camZ, 0, 0, 0, 0, 1, 0);

    // Céu estrelado primeiro
    drawStars();

    // Luz no centro (vinda do Sol) — define SEMPRE após gluLookAt
    GLfloat lightPos[] = {0.0f,0.0f,0.0f,1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // ----------- Sol (texturizado, “auto-iluminado”) -----------
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, sunTexture);

    GLUquadric* sun = gluNewQuadric();
    gluQuadricTexture(sun, GL_TRUE);
    gluQuadricNormals(sun, GLU_SMOOTH);
    gluSphere(sun, 10.0, 50, 50);
    gluDeleteQuadric(sun);

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    // -----------------------------------------------------------

    // Desenhar órbitas (opcional)
    if (showOrbits) {
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 1.0f);
        int segments = 800;
        for(int i=0; i<8; i++) {
            glBegin(GL_LINE_LOOP);
            for(int j=0; j<segments; j++) {
                float theta = 2.0f * M_PI * (float)j / (float)segments;
                float x = orbitRadii[i] * cos(theta);
                float z = orbitRadii[i] * sin(theta);
                glVertex3f(x, 0, z);
            }
            glEnd();
        }
        glEnable(GL_LIGHTING);
    }

    // Planetas
    for(int i=0; i<8; i++){
        float angle = t * orbitSpeeds[i];
        float x = orbitRadii[i] * cos(angle);
        float z = orbitRadii[i] * sin(angle);

        glPushMatrix();
            glTranslatef(x, 0, z);

            if(i==5){ // Saturno: anel
                glRotatef(30, 1, 0, 0);
                drawRing(planetSizes[i]*1.3f, planetSizes[i]*1.6f);
                glRotatef(-30, 1, 0, 0);
            }

            glRotatef(planetRotation[i], 0, 1, 0);

            // --- materiais + textura (modulados pela luz) ---
            glColor3f(1.0f, 1.0f, 1.0f); // não tingir a textura

            GLfloat matDiffuse[]  = {1.0f, 1.0f, 1.0f, 1.0f};
            GLfloat matAmbient[]  = {1.0f, 1.0f, 1.0f, 1.0f}; // leve fill light
            GLfloat matSpecular[] = {0.8f, 0.8, 0.8, 1.0f}; // brilho sutil
            glMaterialfv(GL_FRONT, GL_AMBIENT,  matAmbient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE,  matDiffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
            glMaterialf (GL_FRONT, GL_SHININESS, 30.0f);

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, planetTextures[i]);

            GLUquadric* quad = gluNewQuadric();
            gluQuadricTexture(quad, GL_TRUE);
            gluQuadricNormals(quad, GLU_SMOOTH);
            gluSphere(quad, planetSizes[i], 30, 30);
            gluDeleteQuadric(quad);

            glDisable(GL_TEXTURE_2D);
        glPopMatrix();
    }

    glutSwapBuffers();
}

void reshape(int w,int h){
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)w/h, 0.5, 300.0);
    glMatrixMode(GL_MODELVIEW);
}

void init(){
    glClearColor(0,0,0,1);          // alpha = 1
    glEnable(GL_DEPTH_TEST);

    // Combine textura com iluminação; normaliza normais após escalas/rotes
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_NORMALIZE);

    // Céu
    initStars();

    // Texturas
    sunTexture       = loadTexture("textures/sun.jpg", true); // clamp evita halo na borda
    planetTextures[0]= loadTexture("textures/mercury.jpg");
    planetTextures[1]= loadTexture("textures/venus.jpg");
    planetTextures[2]= loadTexture("textures/earth.jpg");
    planetTextures[3]= loadTexture("textures/mars.jpg");
    planetTextures[4]= loadTexture("textures/jupiter.jpg");
    planetTextures[5]= loadTexture("textures/saturn.jpg");
    planetTextures[6]= loadTexture("textures/uranus.jpg");
    planetTextures[7]= loadTexture("textures/neptune.jpg");
}

void update(int value) {
    if (!paused) {
        t += 0.05f;
        camAngle += 0.002f;
        if(camAngle > 2*M_PI) camAngle -= 2*M_PI;

        for(int i=0; i<8; i++){
            planetRotation[i] += rotationSpeed[i];
            if(planetRotation[i] > 360) planetRotation[i] -= 360;
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 'Z': camDistance -= 5; if(camDistance < 10) camDistance = 10; break;
        case 'z': camDistance += 5; if(camDistance > 200) camDistance = 200; break;
        case 'w': camHeight += 2;   if(camHeight > 50) camHeight = 50;     break;
        case 's': camHeight -= 2;   if(camHeight < 1)  camHeight = 1;      break;
        case 'a': camAngleManual -= 0.05f; break;
        case 'd': camAngleManual += 0.05f; break;
        case 'p': paused = !paused; break;
        case 'o': showOrbits = !showOrbits; break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1000,800);
    glutCreateWindow("Sistema Solar");

    init();
    initLighting();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}
