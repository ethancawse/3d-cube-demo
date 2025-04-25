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
Proj projectOrthoN(const float *, int, float, float, float);
EdgeList createCubeN(int);
void projectHyper4Dto3D(const float *pt4, int dim, float screenW, float screenH, float cam4DDistance, float fov4D, Proj *out, float cameraDistance, float fovY);
void flatten4Dto3D(const float *pt4, float out3[3]);
void getRotationMatrix3D(AngleList, float out[9], int);
void getRotationMatrix4D(AngleList, float out[16], int);
void matrixMultiplyN(const float *, EdgeList *);

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

    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&atlas);
    struct nk_font_config cfg = nk_font_config(0);
    struct nk_font *font = nk_font_atlas_add_default(atlas, 14.0f, &cfg);
    nk_glfw3_font_stash_end();
    nk_style_set_font(ctx, &font->handle);

    AngleList angles = {0};
    double lastTime = glfwGetTime();
    float fpsAccumulator = 0.0f;
    int fpsFrameCount = 0;
    int rotateX = 0, rotateY = 0, rotateZ = 0;
    int rotateXY = 0, rotateXZ = 0, rotateXW = 0, rotateYZ = 0, rotateYW = 0, rotateZW = 0;
    int projectionType = 0;
    float scaleFactor = 0.5f;
    float cameraDistance = 1.5f;
    float fovY = 90.0f;
    float hyperCamDistance = 1.5f;
    float hyperFov = 90.0f;
    float radianPerSecond = 50.0f * (M_PI/180.0f);
    int dimension = 3, oldDimension = 3;

    EdgeList cube = createCubeN(dimension);
    EdgeList originalCube = createCubeN(dimension);
    if (!cube.data || !originalCube.data) printf("Cube memory allocation failed for some reason.");

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        nk_glfw3_new_frame();

        float rotationMatrix[16];
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
            printf("FPS: %.1f\n", fps);
            char title[64];
            snprintf(title, sizeof(title), "FPS: %.1f", fps);
            glfwSetWindowTitle(win, title);
            fpsAccumulator -= 1.0f;
            fpsFrameCount = 0;
        }
        memcpy(cube.data, originalCube.data, cube.edgeCount * 2 * cube.dimension * sizeof(float)); // reset cube

        if (nk_begin(ctx, "Dimension Controls", nk_rect(10, 10, 150, 70), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, 25, 1);
            nk_property_int(ctx, "Dimension:", 3, &dimension, 5, 1, 10.0f);
        }
        nk_end(ctx);

        if (dimension == 3) {
            if (nk_begin(ctx, "3D Rotation Controls", nk_rect(170, 10, 170, 90), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR)) {
                nk_layout_row_dynamic(ctx, 10, 1);
                nk_checkbox_label(ctx, "Rotate X", &rotateX);
                nk_checkbox_label(ctx, "Rotate Y", &rotateY);
                nk_checkbox_label(ctx, "Rotate Z", &rotateZ);
            }
            nk_end(ctx);

            if (nk_begin(ctx, "3D Projection Controls", nk_rect(350, 10, 190, !projectionType ? 60 : 100), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR)) {
                nk_layout_row_dynamic(ctx, 15, 1);
                nk_checkbox_label(ctx, "Perspective", &projectionType);
                if (projectionType) {
                    nk_property_float(ctx, "Distance", 0.1f, &cameraDistance, 10, 0.1f, 0.1f);
                    nk_property_float(ctx, "FOV", 20, &fovY, 180, 5.0f, 5.0f);
                }
            }
            nk_end(ctx);
        } else if (dimension == 4) {
            if (nk_begin(ctx, "4D Rotation Controls", nk_rect(170, 10, 170, 90), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR)) {
                nk_layout_row_dynamic(ctx, 10, 2);
                nk_checkbox_label(ctx, "XY", &rotateXY);
                nk_checkbox_label(ctx, "XZ", &rotateXZ);
                nk_checkbox_label(ctx, "XW", &rotateXW);
                nk_checkbox_label(ctx, "YZ", &rotateYZ);
                nk_checkbox_label(ctx, "YW", &rotateYW);
                nk_checkbox_label(ctx, "ZW", &rotateZW);
            }
            nk_end(ctx);

            if (nk_begin(ctx, "4D Projection Controls", nk_rect(350, 10, 200, !projectionType ? 60 : 135), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR)) {
                nk_layout_row_dynamic(ctx, 15, 1);
                nk_checkbox_label(ctx, "Perspective", &projectionType);
                if (projectionType) {
                    nk_property_float(ctx, "Distance", 0.1f, &cameraDistance, 10, 0.1f, 0.1f);
                    nk_property_float(ctx, "Hyper Distance", 0.1f, &hyperCamDistance, 10, 0.1f, 0.1f);
                    nk_property_float(ctx, "FOV", 20, &fovY, 180, 5.0f, 5.0f);
                    nk_property_float(ctx, "Hyper FOV", 20, &hyperFov, 180, 5.0f, 5.0f);
                }
            }
            nk_end(ctx);
        }

        if (dimension != oldDimension) {
            free(cube.data);
            free(originalCube.data);
            cube = createCubeN(dimension);
            originalCube = createCubeN(dimension);
            if (oldDimension == 3) {
                rotateX = 0; rotateY = 0; rotateZ = 0;
                angles.x = 0.0f; angles.y = 0.0f; angles.z = 0.0f;
            } else if (oldDimension == 4) {
                rotateXW = 0; rotateXY = 0; rotateXZ = 0; rotateYW = 0; rotateYZ = 0; rotateZW = 0;
                angles.u = 0.0f; angles.v = 0.0f; angles.w = 0.0f; angles.x = 0.0f; angles.y = 0.0f; angles.z = 0.0f;
            }
            projectionType = 0;
            oldDimension = dimension;
        }

        if (dimension == 3) {
            for (int i = 0; i < dimension; i++) {
                getRotationMatrix3D(angles, rotationMatrix, i);
                matrixMultiplyN(rotationMatrix, &cube);
            }
            if (rotateX) angles.x += dt * radianPerSecond;
            if (rotateY) angles.y += dt * radianPerSecond;
            if (rotateZ) angles.z += dt * radianPerSecond;
        } else if (dimension == 4) {
            float mat4[16];
            getRotationMatrix4D(angles, mat4, 0); matrixMultiplyN(mat4, &cube);
            getRotationMatrix4D(angles, mat4, 1); matrixMultiplyN(mat4, &cube);
            getRotationMatrix4D(angles, mat4, 2); matrixMultiplyN(mat4, &cube);
            getRotationMatrix4D(angles, mat4, 3); matrixMultiplyN(mat4, &cube);
            getRotationMatrix4D(angles, mat4, 4); matrixMultiplyN(mat4, &cube);
            getRotationMatrix4D(angles, mat4, 5); matrixMultiplyN(mat4, &cube);

            if (rotateXY) angles.u += dt * radianPerSecond;
            if (rotateXZ) angles.v += dt * radianPerSecond;
            if (rotateXW) angles.w += dt * radianPerSecond;
            if (rotateYZ) angles.x += dt * radianPerSecond;
            if (rotateYW) angles.y += dt * radianPerSecond;
            if (rotateZW) angles.z += dt * radianPerSecond;
        }

        if (nk_begin(ctx, "Canvas", nk_rect(0, 0, (float)winWidth, (float)winHeight), NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND)) {
            struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
            Proj zero, one;
            float start3D[3], end3D[3];
            for (int i = 0; i < cube.edgeCount; i++) {
                if (projectionType && dimension == 3) {
                    float *start = cube.data + ((i * 2 + 0) * 3);
                    float *end = cube.data + ((i * 2 + 1) * 3);
                    zero = projectPerspective3D(start[0], start[1], start[2], winWidth, winHeight, cameraDistance, fovY);
                    one  = projectPerspective3D(end[0], end[1], end[2], winWidth, winHeight, cameraDistance, fovY);

                } else if (projectionType && dimension == 4) {
                    float *start4D = cube.data + ((i * 2 + 0 ) * 4);
                    float *end4D = cube.data + ((i * 2 + 1) * 4);
                    projectHyper4Dto3D(start4D, 4, winWidth, winHeight, hyperCamDistance, hyperFov, &zero, cameraDistance, fovY);
                    projectHyper4Dto3D(end4D, 4, winWidth, winHeight, hyperCamDistance, hyperFov, &one, cameraDistance, fovY);
                
                } else {
                    float *startN = cube.data + ((i * 2 + 0) * cube.dimension);
                    float *endN = cube.data + ((i * 2 + 1) * cube.dimension);
                    if (dimension == 4) {
                        flatten4Dto3D(startN, start3D);
                        flatten4Dto3D(endN, end3D);
                    }
                    zero = projectOrthoN(dimension == 3 ? startN : start3D, 3, winWidth, winHeight, scaleFactor);
                    one  = projectOrthoN(dimension == 3 ? endN : end3D, 3, winWidth, winHeight, scaleFactor);
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
    free(cube.data);
    free(originalCube.data);
    return EXIT_SUCCESS;
}

EdgeList createCubeN(int dimension) {
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
          memcpy(&edgeData[idx * 2 * dimension], &coords[v * dimension], dimension * sizeof(float));
          memcpy(&edgeData[idx * 2 * dimension + dimension], &coords[nb * dimension], dimension * sizeof(float));
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

Proj projectOrthoN(const float *pt, int dimension, float screenW, float screenH, float scaleFactor) {
    float x = (dimension > 0 ? pt[0] : 0.0f);
    float y = (dimension > 1 ? pt[1] : 0.0f);

    return (Proj){ x * screenW  * scaleFactor + (screenW  * 0.5f), y * screenH  * scaleFactor + (screenH  * 0.5f) };
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

void projectHyper4Dto3D(const float *pt4, int dim, float screenW, float screenH, float cam4DDistance, float fov4D, Proj *out, float cameraDistance, float fovY) {
    float f4 = 1.0f / tanf(fov4D * (M_PI/180.0f) * 0.5f);
    float wOffset = cam4DDistance + pt4[3];
    if (wOffset < 0.01f) wOffset = 0.01f;
    float x3 = (pt4[0]*f4/ ((float)screenW / screenH)) / wOffset;
    float y3 = (pt4[1]*f4) / wOffset;
    float z3 = (pt4[2]*f4) / wOffset;
    *out = projectPerspective3D(x3, y3, z3, screenW, screenH, cameraDistance, fovY);
}

void flatten4Dto3D(const float *pt4, float out3[3]) {
    out3[0] = pt4[0];
    out3[1] = pt4[1];
    out3[2] = pt4[2] + pt4[3];
}

void getRotationMatrix3D(AngleList angle, float out[9], int axisType) {
    float c;
    float s;
    if (axisType == 0) {
        // Visual X rotation
        c = cosf(angle.x);
        s = sinf(angle.x);
        out[0] = c; out[1] = 0; out[2] = s;
        out[3] = 0; out[4] = 1; out[5] = 0; 
        out[6] = -s; out[7] = 0; out[8] = c; 
    } else if (axisType == 1) {
        // Visual Y rotation
        c = cosf(angle.y);
        s = sinf(angle.y);
        out[0] = 1; out[1] = 0; out[2] = 0;
        out[3] = 0; out[4] = c; out[5] = -s;
        out[6] = 0; out[7] = s; out[8] = c;
    } else {
        // Visual Z rotation
        c = cosf(angle.z);
        s = sinf(angle.z);
        out[0] =  c; out[1] = -s; out[2] =  0;
        out[3] =  s; out[4] = c; out[5] =  0;
        out[6] = 0; out[7] = 0; out[8] =  1;
    }
}

void getRotationMatrix4D(AngleList angle, float out[16], int planeType) {
    memset(out, 0, 16 * sizeof(float));
    out[0] = 1.0f; out[5] = 1.0f; out[10] = 1.0f; out[15] = 1.0f;
    float c, s;
    switch (planeType) {
        case 0: // XY‐plane
            c = cosf(angle.u);
            s = sinf(angle.u);
            out[0] = c; out[1] = -s;
            out[4] = s; out[5] = c;
            break;
        case 1: // XZ‐plane
            c = cosf(angle.v);
            s = sinf(angle.v);
            out[0] = c; out[2] = -s;
            out[8] = s; out[10] = c;
            break;
        case 2: // XW‐plane
            c = cosf(angle.w);
            s = sinf(angle.w);
            out[0] = c; out[3] = -s;
            out[12] = s; out[15] = c;
            break;
        case 3: // YZ‐plane
            c = cosf(angle.x);
            s = sinf(angle.x);
            out[5] = c; out[6] = -s;
            out[9] = s; out[10] = c;
            break;
        case 4: // YW‐plane
            c = cosf(angle.y);
            s = sinf(angle.y);
            out[5] = c; out[7] = -s;
            out[13] = s; out[15] = c;
            break;
        case 5: // ZW‐plane
            c = cosf(angle.z);
            s = sinf(angle.z);
            out[10] = c; out[11] = -s;
            out[14] = s; out[15] = c;
            break;
        default:
        // leave as identity
        break;
    }
}

void matrixMultiplyN(const float *m, EdgeList *list) {
    int edge = list->edgeCount;
    int dim = list->dimension;

    float temp[dim]; // malloc dim sized temp

    for (int i = 0; i < edge; ++i) {
        for (int j = 0; j < 2; ++j) {
            float *points = list->data + ((i * 2 + j) * dim);

            for (int k = 0; k < dim; ++k) {
                float sum = 0.0f;
                for (int l = 0; l < dim; ++l) {
                    sum += m[k * dim + l] * points[l];
                }
                temp[k] = sum;
            }
            memcpy(points, temp, dim * sizeof(float));
        }
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

Proj projectNormal3D(float x, float y, float screenW, float screenH, float scaleFactor) {
    return (Proj){x * screenW * scaleFactor + (screenW * 0.5f), y * screenH * scaleFactor + (screenH * 0.5f)};
}
*/