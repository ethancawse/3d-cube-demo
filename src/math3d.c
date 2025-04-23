#include <math3d.h>
#include <math.h>

void matrixMultiply3D(float *m, lineCoordinate *cube) {
    for (int i = 0; i < 12; i++) {
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

void getXRotationMatrix(float angle, float out[9]) {
    float c = cosf(angle);
    float s = sinf(angle);

    out[0] =  c; out[1] = 0; out[2] =  s; // [cos, 0,  sin]
    out[3] =  0; out[4] = 1; out[5] =  0; // [0,   1,    0]
    out[6] = -s; out[7] = 0; out[8] =  c; // [-sin, 0, cos]
}

void getYRotationMatrix(float angle, float out[9]) {
    float c = cosf(angle);
    float s = sinf(angle);

    out[0] = 1; out[1] = 0; out[2] = 0;
    out[3] = 0; out[4] = c; out[5] = -s;
    out[6] = 0; out[7] = s; out[8] = c;
}

void getZRotationMatrix(float angle, float out[9]) {
    float c = cosf(angle);
    float s = sinf(angle);

    out[0] =  c; out[1] = -s; out[2] =  0;
    out[3] =  s; out[4] = c; out[5] =  0;
    out[6] = 0; out[7] = 0; out[8] =  1;
}