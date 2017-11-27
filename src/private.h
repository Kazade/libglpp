#pragma once

#include <cstdint>
#include <stack>
#include <dc/matrix3d.h>

/* Internal shared state and functions */
#include "../include/GL/gl.h"

static const uint32_t MAX_STACK_DEPTH = 32;

struct AttributePointer {
    GLint size = 2;
    GLenum type = GL_SHORT;
    GLsizei stride = 0;
    GLubyte* pointer = nullptr; // Byte (not void*) so we can perform arithmatic
};

enum AttributeArray {
    ATTRIBUTE_ARRAY_VERTEX = 0x1,
    ATTRIBUTE_ARRAY_NORMAL = 0x2,
    ATTRIBUTE_ARRAY_COLOR = 0x4,
    ATTRIBUTE_ARRAY_INDEX = 0x8,
    ATTRIBUTE_ARRAY_TEXCOORD0 = 0x16,
    ATTRIBUTE_ARRAY_TEXCOORD1 = 0x32,
};

/* Store queue address */
#define TA_SQ_ADDR (uint32_t *)(void *) \
    (0xe0000000 | (((uint32_t)0x10000000) & 0x03ffffe0))

struct __attribute__ ((aligned (32))) PVRCommand {
    uint32_t cmd[8];
};

struct __attribute__ ((aligned (32))) Mat4 {
    matrix_t __attribute__ ((aligned (32))) m;
    Mat4() {
        std::memset(&m, 0, sizeof(matrix_t));
        m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
    }

    Mat4(const matrix_t& array) {
        std::memcpy(&m, &array, sizeof(matrix_t));
    }
};

template<typename T, std::size_t N>
class __attribute__ ((aligned (32))) AlignedStack {
private:
    T items[N] __attribute__ ((aligned (32)));
    std::size_t count = 0;

public:
    AlignedStack():
        count(0) {}

    bool push(const T& val) {
        if(count == N - 1) {
            return false;
        }

        items[count++] = val;
        return true;
    }

    bool pop() {
        if(count == 0) return false;
        --count;
        return true;
    }

    T& top() {
        return items[count - 1];
    }
};


extern AlignedStack<Mat4, MAX_STACK_DEPTH>* CURRENT_STACK;
