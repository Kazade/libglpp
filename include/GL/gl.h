#pragma once

#include <stdint.h>

extern "C" {

#define GL_MAX_STACK_DEPTH = 32
#define GL_MAX_MODELVIEW_STACK_DEPTH GL_MAX_STACK_DEPTH

/* GL types */
#define GLbyte      int8_t
#define GLshort     int16_t
#define GLint       int32_t
#define GLfloat     float
#define GLdouble    float
#define GLvoid      void
#define GLubyte     uint8_t
#define GLushort    uint16_t
#define GLuint      uint32_t
#define GLenum      uint32_t
#define GLsizei     int32_t
#define GLfixed     const uint32_t
#define GLclampf    float
#define GLbitfield  uint32_t
#define GLboolean   uint8_t
#define GL_FALSE    0
#define GL_TRUE     1

/* Data types */
#define GL_BYTE                 0x1400
#define GL_UNSIGNED_BYTE        0x1401
#define GL_SHORT                0x1402
#define GL_UNSIGNED_SHORT       0x1403
#define GL_INT                  0x1404
#define GL_UNSIGNED_INT         0x1405
#define GL_FLOAT                0x1406
#define GL_DOUBLE               0x140A
#define GL_2_BYTES              0x1407
#define GL_3_BYTES              0x1408
#define GL_4_BYTES              0x1409

/* Error constants */
#define GL_NO_ERROR             0
#define GL_INVALID_ENUM         0x0500
#define GL_INVALID_VALUE        0x0501
#define GL_INVALID_OPERATION    0x0502
#define GL_STACK_OVERFLOW       0x0503
#define GL_STACK_UNDERFLOW      0x0504
#define GL_OUT_OF_MEMORY        0x0505

/* Vertex attributes constants */
#define GL_VERTEX_ARRAY         0x8074
#define GL_NORMAL_ARRAY         0x8075
#define GL_COLOR_ARRAY          0x8076
#define GL_INDEX_ARRAY          0x8077
#define GL_TEXTURE_COORD_ARRAY  0x8078

/* Texture units */
#define GL_TEXTURE0             0x84C0
#define GL_TEXTURE1             0x84C1

/* Primitives */
#define GL_POINTS               0x01
#define GL_TRIANGLES            0x05
#define GL_TRIANGLE_STRIP       0x06
#define GL_TRIANGLE_FAN         0x07
#define GL_POINT_SPRITE_ARB     0x8861

/* Matrix constants */
#define GL_MODELVIEW            0x01
#define GL_PROJECTION           0x02
#define GL_TEXTURE              0x03

#define GLAPI extern
#define APIENTRY

/* Immediate mode */
GLAPI void APIENTRY glBegin(GLenum mode);
GLAPI void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glEnd();

/* Clear */
#define GL_COLOR_BUFFER_BIT     0x00004000
#define GL_DEPTH_BUFFER_BIT     0x00000100 /* Not implemented */
GLAPI void APIENTRY glClear(GLuint mode);

/* Matrix stacks */
GLAPI void APIENTRY glMatrixMode(GLenum mode);
GLAPI void APIENTRY glLoadIdentity();
GLAPI void APIENTRY glLoadMatrixf(const GLfloat *m);
GLAPI void APIENTRY glLoadTransposeMatrixf(const GLfloat *m);
GLAPI void APIENTRY glMultMatrixf(const GLfloat *m);
GLAPI void APIENTRY glMultTransposeMatrixf(const GLfloat *m);
GLAPI void APIENTRY glPushMatrix();
GLAPI void APIENTRY glPopMatrix();

GLAPI void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z);

/* State */
GLAPI void APIENTRY glEnable(GLenum cap);
GLAPI void APIENTRY glDisable(GLenum cap);

/* Texturing */
GLAPI void APIENTRY glActiveTexture(GLenum texture);
GLAPI void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param);
GLAPI void APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param);
GLAPI void APIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param);
GLAPI void APIENTRY glGenTextures(GLsizei n, GLuint *textures);
GLAPI void APIENTRY glDeleteTextures(GLsizei n, GLuint *textures);
GLAPI void APIENTRY glBindTexture(GLenum target, GLuint texture);

/* Client State */
GLAPI void APIENTRY glEnableClientState(GLenum cap);
GLAPI void APIENTRY glDisableClientState(GLenum cap);
GLAPI void APIENTRY glClientActiveTexture(GLenum texture);

GLAPI void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLAPI void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

GLAPI void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count);
GLAPI void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);

// Custom extensions
GLAPI void APIENTRY glInitContextDC();
GLAPI void APIENTRY glSwapBuffersDC();

}
