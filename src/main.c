#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR

#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <glad/glad.h>
#include <glfw/glfw3.h>

#define NK_IMPLEMENTATION
#include <nuklear.h>

#define NK_GLFW_GL3_IMPLEMENTATION
#include "nuklear_glfw_gl3.h"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

// Structs
typedef struct {
    float startX, startY, startZ, endX, endY, endZ;
} lineCoordinate;

typedef struct { 
    float x, y; 
} Proj;

// Prototype definitions
Proj projectPerspective(float, float, float, float, float, float);
Proj projectNormal(float, float, float, float);
lineCoordinate *initCube();
int (*calculateEdges(void))[2][3];
void getXRotationMatrix(float dt, float *angle, float out[9]);
void getYRotationMatrix(float dt, float *angle, float out[9]);
void getZRotationMatrix(float dt, float *angle, float out[9]);
void matrixMultiply(float *, lineCoordinate *);

int main(void) {
    printf("Starting program...\n");
    if (!glfwInit()) {
        fprintf(stderr, "Failed to init GLFW\n");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    #endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *win = glfwCreateWindow(800, 800, "Demo", NULL, NULL);
    if (!win) {
        fprintf(stderr, "Failed to create GLFW window");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(win);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to init GLAD\n");
        return EXIT_FAILURE;
    }

    struct nk_glfw glfw = {0};
    //struct nk_context *ctx = nk_glfw3_init(&glfw, win, NK_GLFW3_INSTALL_CALLBACKS);
    struct nk_context *ctx = nk_glfw3_init(win, NK_GLFW3_INSTALL_CALLBACKS);
    lineCoordinate *cube = initCube();
    lineCoordinate *originalCube = initCube();
    if (!cube) {
        printf("Cube memory allocation failed for some reason");
    }

    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&atlas);
    struct nk_font_config cfg = nk_font_config(0);
    struct nk_font *font = nk_font_atlas_add_default(atlas, 14.0f, &cfg);
    nk_glfw3_font_stash_end();
    nk_style_set_font(ctx, &font->handle);

    float angle = 0.0f;
    double lastTime = glfwGetTime();
    float fpsAccumulator = 0.0f;
    int fpsFrameCount = 0;
    int rotateX = 0, rotateY = 0, rotateZ = 0;

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        nk_glfw3_new_frame();

        float rotationMatrix[9];
        int winWidth, winHeight;
        glfwGetFramebufferSize(win, &winWidth, &winHeight);
        float fw = (float)winWidth, fh = (float)winHeight;
        float cameraDistance = 10.0f;
        float now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;
        float fps = 1.0f/(dt>0.0001f?dt:0.0001f);

        fpsAccumulator += dt;
        fpsFrameCount++;

        if (fpsAccumulator >= 1.0f) {
            float fps = fpsFrameCount / fpsAccumulator;
            printf("FPS: %.2f\n", fps);
            printf("Last dt: %.8f\n", dt);
            char title[64];
            snprintf(title, sizeof(title), "FPS: %.1f", fps);
            glfwSetWindowTitle(win, title);
            fpsAccumulator -= 1.0f;
            fpsFrameCount = 0;
        }

        memcpy(cube, originalCube, 12 * (sizeof *cube)); // reset cube

        if (nk_begin(ctx, "Controls", nk_rect(10, 10, 150, 80), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, 10, 1);
            nk_checkbox_label(ctx, "Rotate X", &rotateX);
            nk_checkbox_label(ctx, "Rotate Y", &rotateY);
            nk_checkbox_label(ctx, "Rotate Z", &rotateZ);
        }
        nk_end(ctx);

        if (rotateX) {
            getXRotationMatrix(dt, &angle, rotationMatrix);
            matrixMultiply(rotationMatrix, cube);
        }

        if (rotateY) {
            getYRotationMatrix(dt, &angle, rotationMatrix);
            matrixMultiply(rotationMatrix, cube);
        }

        if (rotateZ) {
            getZRotationMatrix(dt, &angle, rotationMatrix);
            matrixMultiply(rotationMatrix, cube);
        }

        if (nk_begin(ctx, "Canvas", nk_rect(0, 0, fw, fh), NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND)) {
            struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);

            float scaleFactor = 0.25f;

            nk_end(ctx);

            for (int i = 0; i < 12; i++) {
                //Proj zero = projectPerspective(cube[i].startX, cube[i].startY, cube[i].startZ, cameraDistance, winWidth, winHeight);
                //Proj one = projectPerspective(cube[i].endX, cube[i].endY, cube[i].endZ, cameraDistance, winWidth, winHeight);
                Proj zero = projectNormal(cube[i].startX * scaleFactor, cube[i].startY * scaleFactor, winWidth, winHeight);
                Proj one = projectNormal(cube[i].endX * scaleFactor, cube[i].endY * scaleFactor, winWidth, winHeight);

                nk_stroke_line(canvas, zero.x, zero.y, one.x, one.y, 3.0f, nk_rgb(200, 200, 200));
            }
        }

        nk_glfw3_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
        glfwSwapBuffers(win);
    }

    nk_glfw3_shutdown();
    glfwDestroyWindow(win);
    glfwTerminate();
    free(cube);
    return EXIT_SUCCESS;
}

lineCoordinate *initCube() {
    int corners = 8, sides = 12, dimension = 3;
    int (*edgesList)[2][3] = calculateEdges(); // 6 * 2 * 3
    lineCoordinate *cube = malloc(sides * sizeof(lineCoordinate));
    if (!cube) return NULL;
    for (int i = 0; i < sides; i++) {
        for(int j = 0; j < 2; j++) {
            for (int k = 0; k < dimension; k++) {
                *((float *)((lineCoordinate *)((char *)cube + (i*sizeof(lineCoordinate) + (j * sizeof(float) * 3) + k * sizeof(float))))) = (float)edgesList[i][j][k];
            }
        }
    }
    free(edgesList);
    return cube;
}

int (*calculateEdges(void))[2][3] {
    const int dimention = 3;
    const int corners = 8; // 1 << dimention: 0001 -> 0010 -> 0100 -> 1000 = 8
    const int edges = 12;

    int (*verticesList)[3] = malloc(corners * sizeof(*verticesList));
    if (!verticesList) return NULL;
    for (int c = 0; c < corners; c++) {
        verticesList[c][0] = ((c >> 0)&1) ? +1.0f : -1.0f;
        verticesList[c][1] = ((c >> 1)&1) ? +1.0f : -1.0f;
        verticesList[c][2] = ((c >> 2)&1) ? +1.0f : -1.0f;
    }

    int (*edgeList)[2][3] = malloc(edges * sizeof *edgeList);
    if (!edgeList) {
        free(verticesList);
        return NULL;
    }
    int a = 0;
    for (int c = 0; c < corners; c++) {
        for (int bit = 0; bit < dimention; ++bit) {
            if (((c >> bit) & 1) == 0) {
                int u = c | (1 << bit);

                for (int k = 0; k < 3; k++) {
                    edgeList[a][0][k] = verticesList[c][k];
                    edgeList[a][1][k] = verticesList[u][k];
                }
                ++a;
            }
        }
    }
    free(verticesList);
    return edgeList;
}

Proj projectPerspective(float x, float y, float z, float camDist, float screenW, float screenH) {
    float z_cam = camDist - z;

    const float fovY = 45.0f * (M_PI/180.0f);
    const float f = 1.0f / tanf(fovY * 0.5f);
    float aspect = screenW / screenH;

    float Xc = (x * f / aspect) / z_cam;
    float Yc = (y * f) / z_cam;

    float sx = (Xc * 0.5f + 0.5f) * screenW;
    float sy = (Yc * 0.5f + 0.5f) * screenH;

    return (Proj){ sx, sy };
}

Proj projectNormal(float x, float y, float screenW, float screenH) {
    float sx = (x * 0.5f + 0.5f) * screenW;
    float sy = (y * 0.5f + 0.5f) * screenH;
    return (Proj){sx, sy};
}

void getXRotationMatrix(float dt, float *angle, float out[9]) {
    float radianPerSecond = 30.0f * (M_PI/180.0f);
    *angle += dt * radianPerSecond;
    float c = cosf(*angle);
    float s = sinf(*angle);

    out[0] =  c; out[1] = 0; out[2] =  s; // [cos, 0,  sin]
    out[3] =  0; out[4] = 1; out[5] =  0; // [0,   1,    0]
    out[6] = -s; out[7] = 0; out[8] =  c; // [-sin, 0, cos]
}

void getYRotationMatrix(float dt, float *angle, float out[9]) {
    float radianPerSecond = 30.0f * (M_PI/180.0f);
    *angle += dt * radianPerSecond;
    float c = cosf(*angle);
    float s = sinf(*angle);

    out[0] =  1; out[1] = 0; out[2] =  0;
    out[3] =  0; out[4] = c; out[5] =  -s;
    out[6] = 0; out[7] = s; out[8] =  c;
}

void getZRotationMatrix(float dt, float *angle, float out[9]) {
    float radianPerSecond = 30.0f * (M_PI/180.0f);
    *angle += dt * radianPerSecond;
    float c = cosf(*angle);
    float s = sinf(*angle);

    out[0] =  c; out[1] = -s; out[2] =  0;
    out[3] =  s; out[4] = c; out[5] =  0;
    out[6] = 0; out[7] = 0; out[8] =  1;
}

void matrixMultiply(float *m, lineCoordinate *cube) {
    for (size_t i = 0; i < 12; i++) {
        float sx = cube[i].startX, sy = cube[i].startY, sz = cube[i].startZ;
        float ex = cube[i].endX, ey = cube[i].endY, ez = cube[i].endZ;

        cube[i].startX = m[0]*sx + m[1]*sy + m[2]*sz;
        cube[i].startY = m[3]*sx + m[4]*sy + m[5]*sz;
        cube[i].startZ = m[6]*sx + m[7]*sy + m[8]*sz;

        cube[i].endX = m[0]*ex + m[1]*ey + m[2]*ez;
        cube[i].endY = m[3]*ex + m[4]*ey + m[5]*ez;
        cube[i].endZ = m[6]*ex + m[7]*ey + m[8]*ez;
    }
}

/* Bad
for (int i = 0; i < numOfSides; i++) {
    char *startAddress = (char *)cube;
    char *offset = startAddress + (i * sizeof(lineCoordinates)); 
    lineCoordinates *element = (lineCoordinates *)offset;
    *((float*)element) = startX;
    offset += sizeof(float);
    element = (lineCoordinates *)((char *)element + sizeof(float));
    *((float*)element) = startY;
    ... too tedious
    }
*/

/*

for (int i = 0; i < sides; i++) {
        for(int j = 0; j < 2; j++) {
            for (int k = 0; k < dimension; k++) {
                *((float *)((lineCoordinate *)((char *)cube + (i*sizeof(lineCoordinate) + j * sizeof(float))))) = paramArray[j];
            }
        }
    }

*/