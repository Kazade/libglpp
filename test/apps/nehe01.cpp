#include "../../include/GL/gl.h"
#include "../../include/GL/glu.h"


int main(int argc, char* argv[]) {
    glInitContextDC();

    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_PROJECTION);

    gluPerspective(45.0f, GLfloat(640) / GLfloat(480), 0.1, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for(;;) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        glTranslatef(-1.5f,0.0f,-6.0f);

        glBegin(GL_TRIANGLES);
            glVertex3f( 0.0f, 1.0f, 0.0f);
            glVertex3f(-1.0f,-1.0f, 0.0f);
            glVertex3f( 1.0f,-1.0f, 0.0f);
        glEnd();

        glTranslatef(3.0f,0.0f,0.0f);
/*
        glBegin(GL_QUADS);
            glVertex3f(-1.0f, 1.0f, 0.0f);
            glVertex3f( 1.0f, 1.0f, 0.0f);
            glVertex3f( 1.0f,-1.0f, 0.0f);
            glVertex3f(-1.0f,-1.0f, 0.0f);
        glEnd(); */

        glSwapBuffersDC();
    }

    return 0;
}
