#pragma once

typedef struct { 
    float x, y; 
} Proj;

typedef struct {
    float startX, startY, startZ;
    float endX, endY, endZ;
} LineCoordinate;

typedef struct {
    int  edgeCount;
    int  dimension;
    float *data;
} EdgeList;