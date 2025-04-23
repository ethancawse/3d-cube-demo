#pragma once

#include <types.h>

void getXRotationMatrix(float angle, float out[9]);
void getYRotationMatrix(float angle, float out[9]);
void getZRotationMatrix(float angle, float out[9]);
void matrixMultiply3x3(float *, lineCoordinate *);