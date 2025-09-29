#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Tempo global
float t = 0.0f;
bool paused = false; // controle de pausa
bool showOrbits = false; // controle das órbitas


// Configuração das órbitas e planetas
float orbitRadii[] = {16.0f, 20.0f, 24.0f, 28.0f, 35.0f, 48.0f, 60.0f, 72.0f};
float orbitSpeeds[] = {0.6f, 0.55f, 0.5f, 0.45f, 0.3f, 0.25f, 0.2f, 0.15f};

// Tamanhos dos planetas
float planetSizes[] = {0.75f, 1.35f, 1.5f, 0.9f, 4.5f, 5.25f, 3.2f, 3.0f};

// Rotação própria
float planetRotation[8] = {0,0,0,0,0,0,0,0};
float rotationSpeed[8]  = {2.0f,1.8f,1.6f,1.5f,1.2f,1.1f,1.0f,0.9f};

// IDs das texturas
GLuint planetTextures[8];

// Câmera
float camAngle = 0.0f;
float camDistance = 90.0f;
float camHeight = 5.0f;
float camAngleManual = 0.0f;

// Função de carregamento de textura com stb_image
GLuint loadTexture(const char* filename) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);

    if (!data) {
        printf("Erro ao carregar textura: %s\n", filename);
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    // Compatível com GLUT/GLU
    gluBuild2DMipmaps(GL_TEXTURE_2D, format, width, height, format, GL_UNSIGNED_BYTE, data);

    // Parâmetros
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return texID;
}

// Inicializar iluminação
void initLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightAmbient[]  = {0.05f,0.05f,0.05f,1.0f};
    GLfloat lightDiffuse[]  = {1.0f,1.0f,1.0f,1.0f};
    GLfloat lightSpecular[] = {1.0f,1.0f,1.0f,1.0f};

    glLightfv(GL_LIGHT0,GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0,GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0,GL_SPECULAR, lightSpecular);

    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.8f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.01f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.002f);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
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

    // Luz no centro
    GLfloat lightPos[] = {0.0f,0.0f,0.0f,1.0f};
    glLightfv(GL_LIGHT0,GL_POSITION,lightPos);

    // Sol
    glDisable(GL_LIGHTING);
    glColor3f(1.0f,1.0f,0.0f);
    glutSolidSphere(10.0,50,50);
    glEnable(GL_LIGHTING);

    // Desenhar órbitas
    if (showOrbits) {
        glDisable(GL_LIGHTING);
        glColor3f(0.5f, 0.5f, 0.5f);
        int segments = 200;
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
        float angle = t*orbitSpeeds[i];
        float x = orbitRadii[i]*cos(angle);
        float z = orbitRadii[i]*sin(angle);

        glPushMatrix();
            glTranslatef(x, 0, z);

            if(i==5){ // Saturno com anel
                glRotatef(30, 1, 0, 0);
                drawRing(planetSizes[i]*1.3f, planetSizes[i]*1.6f);
                glRotatef(-30, 1, 0, 0);
            }

            glRotatef(planetRotation[i],0,1,0);

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, planetTextures[i]);

            GLUquadric* quad = gluNewQuadric();
            gluQuadricTexture(quad, GL_TRUE);
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
    glClearColor(0,0,0,0);
    glEnable(GL_DEPTH_TEST);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);


    // Carregar texturas (adicione suas imagens aqui)
    planetTextures[0] = loadTexture("textures/mercury.png");
    planetTextures[1] = loadTexture("textures/venus.png");
    planetTextures[2] = loadTexture("textures/earth.png");
    planetTextures[3] = loadTexture("textures/mars.png");
    planetTextures[4] = loadTexture("textures/jupiter.png");
    planetTextures[5] = loadTexture("textures/saturn.png");
    planetTextures[6] = loadTexture("textures/uranus.png");
    planetTextures[7] = loadTexture("textures/neptune.png");
}

// Atualização
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

// Teclado
void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 'Z': camDistance -= 5; if(camDistance < 10) camDistance = 10; break; // Zoom in
        case 'z': camDistance += 5; if(camDistance > 200) camDistance = 200; break; // Zoom out
        case 'w': camHeight += 2; if(camHeight > 50) camHeight = 50; break; // Sobe a câmera
        case 's': camHeight -= 2; if(camHeight < 1) camHeight = 1; break; // Desce a câmera
        case 'a': camAngleManual -= 0.05f; break; // Rotaciona manualmente para esquerda
        case 'd': camAngleManual += 0.05f; break; // Rotaciona manualmente para direita
        case 'p': paused = !paused; break; // Pausar/despausar
        case 'o': showOrbits = !showOrbits; break; //Adicionar ou remover órbitas

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
