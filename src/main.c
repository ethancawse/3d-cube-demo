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
#include <GLFW/glfw3.h>

#define NK_IMPLEMENTATION
#include <nuklear.h>

#define NK_GLFW_GL3_IMPLEMENTATION
#include "nuklear_glfw_gl3.h"

#include <types.h>

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

// Prototypes
Proj projectPerspective3D(float, float, float, float, float, float, float);
Proj projectNormal3D(float, float, float, float, float);
EdgeList createCube(int);
void getXRotationMatrix3D(float, float out[9]);
void getYRotationMatrix3D(float, float out[9]);
void getZRotationMatrix3D(float, float out[9]);
void matrixMultiply3D(float *, LineCoordinate *);

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
    struct nk_context *ctx = nk_glfw3_init(win, NK_GLFW3_INSTALL_CALLBACKS);
    LineCoordinate *cube = initCube3D();
    LineCoordinate *originalCube = initCube3D();
    if (!cube) {
        printf("Cube memory allocation failed for some reason");
    }

    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&atlas);
    struct nk_font_config cfg = nk_font_config(0);
    struct nk_font *font = nk_font_atlas_add_default(atlas, 14.0f, &cfg);
    nk_glfw3_font_stash_end();
    nk_style_set_font(ctx, &font->handle);

    float angleX = 0.0f;
    float angleY = 0.0f;
    float angleZ = 0.0f;
    double lastTime = glfwGetTime();
    float fpsAccumulator = 0.0f;
    int fpsFrameCount = 0;
    int rotateX = 0, rotateY = 0, rotateZ = 0;
    int projectionType = 0;
    float scaleFactor = 0.5f;
    float cameraDistance = 1.5f;
    float fovY = 90.0f;
    float radianPerSecond = 50.0f * (M_PI/180.0f);
    int dimension = 3;

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        nk_glfw3_new_frame();

        float rotationMatrix[9];
        int frameBufferWidth, frameBufferHeight, winWidth, winHeight;
        glfwGetFramebufferSize(win, &frameBufferWidth, &frameBufferHeight);
        glViewport(0, 0, frameBufferWidth, frameBufferHeight);
        glfwGetWindowSize(win, &winWidth, &winHeight);

        float now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;
        float fps = 1.0f/(dt>0.0001f?dt:0.0001f);

        fpsAccumulator += dt;
        fpsFrameCount++;

        if (fpsAccumulator >= 1.0f) {
            float fps = fpsFrameCount / fpsAccumulator;
            printf("FPS: %.2f\n", fps);
            char title[64];
            snprintf(title, sizeof(title), "FPS: %.1f", fps);
            glfwSetWindowTitle(win, title);
            fpsAccumulator -= 1.0f;
            fpsFrameCount = 0;
        }
        memcpy(cube, originalCube, 12 * sizeof(*cube)); // reset cube

        if (nk_begin(ctx, "Rotation Controls", nk_rect(10, 10, 150, 90), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, 10, 1);
            nk_checkbox_label(ctx, "Rotate X", &rotateX);
            nk_checkbox_label(ctx, "Rotate Y", &rotateY);
            nk_checkbox_label(ctx, "Rotate Z", &rotateZ);
        }
        nk_end(ctx);

        if (nk_begin(ctx, "Projection Controls", nk_rect(170, 10, 170, 80), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, 10, 1);
            nk_checkbox_label(ctx, "Perspective", &projectionType);
            nk_property_float(ctx, "Distance", 0.1f, &cameraDistance, 10, 0.1f, 0.1f);
            nk_property_float(ctx, "FOV", 20, &fovY, 180, 5.0f, 5.0f);
        }
        nk_end(ctx);

        getXRotationMatrix3D(angleX, rotationMatrix);
        matrixMultiply3D(rotationMatrix, cube);
        getYRotationMatrix3D(angleY, rotationMatrix);
        matrixMultiply3D(rotationMatrix, cube);
        getZRotationMatrix3D(angleZ, rotationMatrix);
        matrixMultiply3D(rotationMatrix, cube);

        if (rotateX) angleX += dt * radianPerSecond;
        if (rotateY) angleY += dt * radianPerSecond;
        if (rotateZ) angleZ += dt * radianPerSecond;

        if (nk_begin(ctx, "Canvas", nk_rect(0, 0, (float)winWidth, (float)winHeight), NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND)) {
            struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
            Proj zero, one;
            for (int i = 0; i < 12; i++) {
                if (projectionType) {
                    zero = projectPerspective3D(cube[i].startX, cube[i].startY, cube[i].startZ, (float)winWidth, (float)winHeight, cameraDistance, fovY);
                    one = projectPerspective3D(cube[i].endX, cube[i].endY, cube[i].endZ, (float)winWidth, (float)winHeight, cameraDistance, fovY);
                } else {
                    zero = projectNormal3D(cube[i].startX, cube[i].startY, (float)winWidth, (float)winHeight, scaleFactor);
                    one = projectNormal3D(cube[i].endX, cube[i].endY, (float)winWidth, (float)winHeight, scaleFactor);
                }
                nk_stroke_line(canvas, zero.x, zero.y, one.x, one.y, 5.0f, nk_rgb(200, 200, 200));
            }
        }
        nk_end(ctx);

        nk_glfw3_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
        glfwSwapBuffers(win);
    }
    nk_glfw3_shutdown();
    glfwDestroyWindow(win);
    glfwTerminate();
    free(cube);
    return EXIT_SUCCESS;
}

EdgeList createCube(int dimension) {
    EdgeList list = {0};
    int verts = 1 << dimension;
    int edges = dimension * (1 << (dimension - 1));

    float *coords = malloc(verts * dimension * sizeof(float));
    float *edgeData = malloc(edges * 2 * dimension * sizeof(float));
    if (!coords || !edgeData) {
        free(coords);
        free(edgeData);
        return list;
    }

    for (int v = 0; v < verts; ++v)
      for (int d = 0; d < dimension; ++d)
        coords[v*dimension + d] = (v & (1<<d)) ? +0.5f : -0.5f;

    int idx = 0;
    for (int v = 0; v < verts; ++v) {
      for (int d = 0; d < dimension; ++d) {
        int nb = v ^ (1 << d);
        if (v < nb) {
          memcpy(&edgeData[idx * 2 * dimension],
                 &coords[v * dimension],
                 dimension * sizeof(float));
          memcpy(&edgeData[idx * 2 * dimension + dimension],
                 &coords[nb * dimension],
                 dimension * sizeof(float));
          ++idx;
        }
      }
    }

    free(coords);
    list.edgeCount = edges;
    list.dimension = dimension;
    list.data = edgeData;
    return list;
}

Proj projectNormal3D(float x, float y, float screenW, float screenH, float scaleFactor) {
    return (Proj){x * screenW * scaleFactor + (screenW * 0.5f), y * screenH * scaleFactor + (screenH * 0.5f)};
}

Proj projectPerspective3D(float x, float y, float z, float screenW, float screenH, float camDistance, float fovY) {
    float aspectRatio = screenW / screenH;
    float fovY_radius = fovY * M_PI / 180.0f;
    float f = 1.0f / tan(fovY_radius / 2.0f);
    float zCameraOffset = z + camDistance;
    if (zCameraOffset < 0.01f) zCameraOffset = 0.01f;

    float xNormalizedDeviceCoords = (x * f / aspectRatio) / zCameraOffset;
    float yNormalizedDeviceCoords = (y * f) / zCameraOffset;

    float screenX = (xNormalizedDeviceCoords * 0.5f + 0.5f) * screenW;
    float screenY = (1.0f - (yNormalizedDeviceCoords * 0.5f + 0.5f)) * screenH; 

    return (Proj){screenX, screenY};
}

void getXRotationMatrix3D(float angle, float out[9]) {
    float c = cosf(angle);
    float s = sinf(angle);

    out[0] =  c; out[1] = 0; out[2] =  s; // [cos, 0,  sin]
    out[3] =  0; out[4] = 1; out[5] =  0; // [0,   1,    0]
    out[6] = -s; out[7] = 0; out[8] =  c; // [-sin, 0, cos]
}

void getYRotationMatrix3D(float angle, float out[9]) {
    float c = cosf(angle);
    float s = sinf(angle);

    out[0] = 1; out[1] = 0; out[2] = 0;
    out[3] = 0; out[4] = c; out[5] = -s;
    out[6] = 0; out[7] = s; out[8] = c;
}

void getZRotationMatrix3D(float angle, float out[9]) {
    float c = cosf(angle);
    float s = sinf(angle);

    out[0] =  c; out[1] = -s; out[2] =  0;
    out[3] =  s; out[4] = c; out[5] =  0;
    out[6] = 0; out[7] = 0; out[8] =  1;
}

void matrixMultiply3D(float *m, LineCoordinate *cube) {
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

/*
Old manual implementation

LineCoordinate *initCube3D() {
    int corners = 8, sides = 12, dimension = 3;
    float (*edgesList)[2][3] = calculateEdges3D();
    if (!edgesList) return NULL;
    LineCoordinate *cube = malloc(sides * sizeof(LineCoordinate));
    if (!cube) return NULL;
    for (int i = 0; i < sides; i++) {
        for(int j = 0; j < 2; j++) {
            for (int k = 0; k < dimension; k++) {
                // God awful way of filling the structs with manual pointer offsets, only did this to prove that I know how pointers work
                *((float *)((LineCoordinate *)((char *)cube + (i*sizeof(LineCoordinate) + (j * sizeof(float) * 3) + k * sizeof(float))))) = edgesList[i][j][k];
            }
        }
    }
    free(edgesList);
    return cube;
}

float (*calculateEdges3D(void))[2][3] {
    float (*edgeList)[2][3] = malloc(12 * sizeof(*edgeList));
    if (!edgeList) return NULL;
    
    float temp[12][2][3] = {
        {{-0.5, -0.5, -0.5}, {0.5, -0.5, -0.5}},
        {{-0.5, -0.5, -0.5}, {-0.5, 0.5, -0.5}},
        {{-0.5, -0.5, -0.5}, {-0.5, -0.5, 0.5}}, 
        {{0.5, 0.5, 0.5}, {-0.5, 0.5, 0.5}},
        {{0.5, 0.5, 0.5}, {0.5, -0.5, 0.5}},
        {{0.5, 0.5, 0.5}, {0.5, 0.5, -0.5}},
        {{-0.5, 0.5, -0.5}, {-0.5, 0.5, 0.5}},
        {{-0.5, 0.5, -0.5}, {0.5, 0.5, -0.5}},
        {{-0.5, -0.5, 0.5}, {-0.5, 0.5, 0.5}},
        {{-0.5, -0.5, 0.5}, {0.5, -0.5, 0.5}},
        {{0.5, -0.5, -0.5}, {0.5, -0.5, 0.5}},
        {{0.5, -0.5, -0.5}, {0.5, 0.5, -0.5}},
    };

    memcpy(edgeList, temp, sizeof(temp));
    return edgeList;
}
*/