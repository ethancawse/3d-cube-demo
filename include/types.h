#pragma once

typedef struct { 
    float x, y; 
} Proj;

typedef struct {
    int  edgeCount;
    int  dimension;
    float *data;
} EdgeList;

typedef struct {
    float u;
    float v;
    float w;
    float x;
    float y;
    float z;
} AngleList;