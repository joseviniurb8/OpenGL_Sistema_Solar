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

// Estado geral 
float t = 0.0f;               // tempo "global" (animações e cintilação das estrelas)
bool paused = false;          // controle de pausa
bool showOrbits = false;      // liga/desliga desenho das órbitas

// Configuração das órbitas e planetas
float orbitRadii[]  = {16.0f, 20.0f, 24.0f, 28.0f, 35.0f, 48.0f, 60.0f, 72.0f}; // raios das órbitas
float orbitSpeeds[] = {0.6f, 0.55f, 0.5f, 0.45f, 0.3f, 0.25f, 0.2f, 0.15f};     // velocidade angular

// Tamanhos dos planetas
float planetSizes[] = {0.75f, 1.35f, 1.5f, 0.9f, 4.5f, 5.25f, 3.2f, 3.0f};     // raio de cada planeta

// Rotação própria
float planetRotation[8] = {0,0,0,0,0,0,0,0};  // ângulo de rotação local (dia/noite)
float rotationSpeed[8]  = {2.0f,1.8f,1.6f,1.5f,1.2f,1.1f,1.0f,0.9f}; // vel. rotação

// IDs das texturas
GLuint planetTextures[8];      // texturas dos planetas (uma para cada)
GLuint sunTexture;             // textura do Sol

// Câmera
float camAngle = 0.0f;         // rotação automática da câmera em torno do centro
float camDistance = 90.0f;     // distância da câmera ao centro (zoom)
float camHeight = 5.0f;
float camAngleManual = 0.0f;   // offset manual via teclado (a/d)

// Estrelas (céu)
struct Star {
    float x, y, z;   // posição 3D fixa na esfera de céu
    float base;      // brilho base [0..1]
    float phase;     // fase para cintilação (desloca o seno)
};

const int NUM_STARS = 2000;                 // quantidade de estrelas
Star stars[NUM_STARS];
const float STAR_RADIUS = 220.0f;           // raio da esfera de estrelas (atrás de tudo)

// Carregar textura (seguro): força RGBA, corrige alinhamento e
// permite optar por CLAMP_TO_EDGE (para o Sol).
GLuint loadTexture(const char* filename, bool clampToEdge = false) {
    int w, h, n;
    stbi_set_flip_vertically_on_load(1);                // evita textura invertida verticalmente
    unsigned char* data = stbi_load(filename, &w, &h, &n, 4); // força 4 canais (RGBA)

    if (!data) {
        printf("Erro ao carregar textura: %s\n", filename);
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);              // gera ID de textura
    glBindTexture(GL_TEXTURE_2D, texID);   // seleciona a textura atual

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // evita listras (alinhamento por 1 byte)

    // filtros de min/mag (mipmap para longe, linear para perto)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // tipo de repetição nas bordas (clamp evita halos em PNG com transparência)
    GLint wrap = clampToEdge ? GL_CLAMP_TO_EDGE : GL_REPEAT;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

    // envia pixels para a GPU + gera mipmaps (via GLU)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);                  // libera RAM
    return texID;                           // retorna handle da textura
}

void initLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Ambiente global aumentado um pouco
    GLfloat globalAmbient[] = {0.05f, 0.05f, 0.05f, 1.0f}; // Aumentado de 0.02 para 0.05
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    // Luz ambiente da fonte de luz também aumentada
    GLfloat lightAmbient[]  = {0.2f, 0.2f, 0.2f, 1.0f};  // Aumentado de 0.1 para 0.2
    GLfloat lightDiffuse[]  = {2.5f, 2.5f, 2.0f, 1.0f};
    GLfloat lightSpecular[] = {1.5f, 1.5f, 1.5f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    // Atenuação mantida
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.001f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0001f);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}

// Gerar estrelas numa esfera
void initStars() {
    srand((unsigned)time(NULL));            // seed do RNG (varia por execução)
    for (int i = 0; i < NUM_STARS; ++i) {
        // amostragem uniforme na esfera (transforma de coordenadas esféricas para cartesianas)
        float u = (float)rand() / RAND_MAX;
        float v = (float)rand() / RAND_MAX;
        float theta = 2.0f * M_PI * u;
        float z = 2.0f * v - 1.0f;
        float r = sqrtf(1.0f - z*z);
        float x = r * cosf(theta);
        float y = r * sinf(theta);
        stars[i].x = STAR_RADIUS * x;       // empurra para o raio do céu
        stars[i].y = STAR_RADIUS * y;
        stars[i].z = STAR_RADIUS * z;

        stars[i].base  = 0.6f + 0.4f * ((float)rand() / RAND_MAX); // brilho base aleatório
        stars[i].phase = 20.0f * ((float)rand() / RAND_MAX);       // fase da cintilação
    }
}

// Desenhar o céu estrelado
void drawStars() {
    glDisable(GL_LIGHTING);     // estrelas não recebem iluminação (pontos simples)
    glDepthMask(GL_FALSE);      // não escreve no depth (ficam "atrás" sem bloquear)
    glPointSize(1.5f);          // tamanho de cada estrela

    glBegin(GL_POINTS);
    for (int i = 0; i < NUM_STARS; ++i) {
        // cintilação leve (varia brilho com seno no tempo)
        float tw = 0.85f + 0.15f * (0.5f + 0.5f * sinf(0.6f * t + stars[i].phase));
        float b = stars[i].base * tw;      // brilho final
        glColor3f(b, b, b);                // cor cinza clara proporcional
        glVertex3f(stars[i].x, stars[i].y, stars[i].z); // plot do ponto
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
}

// Desenhar anel de Saturno
void drawRing(float innerRadius, float outerRadius) {
    int numSegments = 100;
    glDisable(GL_LIGHTING);
    glColor3f(0.8f, 0.8f, 0.6f);// tom amarelado
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // limpa buffers de cor e profundidade
    glLoadIdentity();                                   // reseta matriz modelview

    // Câmera (orbita o centro 0,0,0 e olha para o Sol)
    float camX = camDistance * cos(camAngle + camAngleManual);
    float camZ = camDistance * sin(camAngle + camAngleManual);
    float camY = camHeight;
    gluLookAt(camX, camY, camZ, 0, 0, 0, 0, 1, 0);     // (eye) -> (center) com (up)

    // Céu estrelado primeiro (fundo da cena)
    drawStars();

    // Luz no centro (vinda do Sol) — precisa ser setada após a câmera
    GLfloat lightPos[] = {0.0f,0.0f,0.0f,1.0f};        // luz pontual no (0,0,0)
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // Sol - com material emissivo para brilhar intensamente
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, sunTexture);
    
    // Material emissivo - faz o Sol brilhar com sua própria luz
    GLfloat sunEmission[] = {1.5f, 1.5f, 1.0f, 1.0f};  // Amarelo brilhante
    GLfloat sunDiffuse[]  = {1.0f, 1.0f, 0.8f, 1.0f};  // Cor difusa
    glMaterialfv(GL_FRONT, GL_EMISSION, sunEmission);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, sunDiffuse);
    
    // Desenha o Sol COM iluminação, mas com emissão própria
    GLUquadric* sun = gluNewQuadric();
    gluQuadricTexture(sun, GL_TRUE);
    gluQuadricNormals(sun, GLU_SMOOTH);
    gluSphere(sun, 10.0, 50, 50);
    gluDeleteQuadric(sun);
    
    // Reseta a emissão para os planetas não brilharem
    GLfloat noEmission[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);
    
    glDisable(GL_TEXTURE_2D);

    // Desenhar órbitas (opcional)
    if (showOrbits) {
        glDisable(GL_LIGHTING);                         // linhas simples, sem iluminação
        glColor3f(1.0f, 1.0f, 1.0f);                    // cor das órbitas
        int segments = 800;                             // resolução de cada órbita
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
        glEnable(GL_LIGHTING);                          // volta iluminação
    }

    // Planetas (translada para a órbita, gira eixo, aplica material e desenha esfera texturizada)
    for(int i=0; i<8; i++){
        float angle = t * orbitSpeeds[i];               // ângulo orbital atual
        float x = orbitRadii[i] * cos(angle);
        float z = orbitRadii[i] * sin(angle);

        glPushMatrix();
            glTranslatef(x, 0, z);

            if(i==5){                                    // Saturno anel
                glRotatef(30, 1, 0, 0);                  // inclinação do anel
                drawRing(planetSizes[i]*1.3f, planetSizes[i]*1.6f);
                glRotatef(-30, 1, 0, 0);
            }

            glRotatef(planetRotation[i], 0, 1, 0);       // rotação própria (dia/noite)

            // --- materiais + textura (modulados pela luz) ---
            glColor3f(1.0f, 1.0f, 1.0f);

            GLfloat matDiffuse[]  = {1.0f, 1.0f, 1.0f, 1.0f};
            GLfloat matAmbient[]  = {0.6f, 0.6f, 0.6f, 1.0f};  // Aumentado para refletir mais luz ambiente
            GLfloat matSpecular[] = {0.8f, 0.8f, 0.8f, 1.0f};  // Especular mantido
            glMaterialfv(GL_FRONT, GL_AMBIENT,  matAmbient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE,  matDiffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
            glMaterialf (GL_FRONT, GL_SHININESS, 50.0f);       // Brilho moderado

            glEnable(GL_TEXTURE_2D);                            // ativa textura do planeta
            glBindTexture(GL_TEXTURE_2D, planetTextures[i]);    // seleciona textura i

            GLUquadric* quad = gluNewQuadric();                 // esfera do planeta
            gluQuadricTexture(quad, GL_TRUE);
            gluQuadricNormals(quad, GLU_SMOOTH);
            gluSphere(quad, planetSizes[i], 30, 30);            // desenha esfera
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
    glClearColor(0,0,0,0);        // cor do fundo (preto)
    glEnable(GL_DEPTH_TEST);      // habilita teste de profundidade (oclusão correta)

    // Combine textura com iluminação (textura * luz) e normaliza normais ao escalar
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_NORMALIZE);

    // Céu (gera estrelas)
    initStars();

    // Texturas (carrega arquivos para GPU; clamp no Sol evita halo da borda)
    sunTexture       = loadTexture("textures/sun.jpg", true);
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
        t += 0.05f;                         // avança tempo (animações)
        camAngle += 0.002f;                 // gira câmera lentamente
        if(camAngle > 2*M_PI) camAngle -= 2*M_PI;

        for(int i=0; i<8; i++){
            planetRotation[i] += rotationSpeed[i];     // gira planetas
            if(planetRotation[i] > 360) planetRotation[i] -= 360;
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 'Z': camDistance -= 5; if(camDistance < 10)  camDistance = 10;  break; // aproxima
        case 'z': camDistance += 5; if(camDistance > 200) camDistance = 200; break; // afasta
        case 'w': camHeight += 2;   if(camHeight > 50)    camHeight = 50;    break; // sobe
        case 's': camHeight -= 2;   if(camHeight < 1)     camHeight = 1;     break; // desce
        case 'a': camAngleManual -= 0.05f; break;  // gira manualmente para a esquerda
        case 'd': camAngleManual += 0.05f; break;  // gira manualmente para a direita
        case 'p': paused = !paused;        break;  // pausa/continua animação
        case 'o': showOrbits = !showOrbits;break;  // mostra/esconde órbitas
    }
    glutPostRedisplay();
}

int main(int argc, char** argv){
    glutInit(&argc, argv);                                   
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);// double buffer + cor + depth
    glutInitWindowSize(1000,800);                            // tamanho da janela
    glutCreateWindow("Sistema Solar");                       // cria janela

    init();                                                  // estados iniciais (texturas/estrelas)
    initLighting();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}