#include <cmath>

#include "../include/GL/glu.h"
#include "private.h"

static float _cotan(float v) {
    return(1 / std::tan(v));
}

void APIENTRY gluPerspective(GLdouble fovy,  GLdouble aspect,  GLdouble zNear,  GLdouble zFar) {
    float m[16];

    auto f = _cotan(fovy / 2.0f);
    auto nmf = zNear - zFar;

    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zFar + zNear) / nmf;
    m[11] = -1.0f;
    m[14] = 2.0f * zFar * zNear / nmf;

    glMultMatrixf(m);
}
