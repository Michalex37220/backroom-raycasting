#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define mapX 8
#define mapY 8
#define mapS 64

#define NUM_RAYS 120
#define FOV 60

#define PLAYER_RADIUS 10

int map[] =
{
    1,1,1,1,1,1,1,1,
    1,0,1,0,0,0,0,1,
    1,0,1,0,0,0,0,1,
    1,0,1,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,1,0,1,
    1,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1
};

float px, py, pdx, pdy, pa;

float degToRad(float a)
{
    return a * M_PI / 180.0;
}

float FixAng(float a)
{
    if (a > 359)
        a -= 360;

    if (a < 0)
        a += 360;

    return a;
}

void drawMap2D()
{
    int x, y, xo, yo;

    for (y = 0; y < mapY; y++)
    {
        for (x = 0; x < mapX; x++)
        {
            if (map[y * mapX + x] == 1)
                glColor3f(1,1,1);
            else
                glColor3f(0,0,0);

            xo = x * mapS;
            yo = y * mapS;

            glBegin(GL_QUADS);

            glVertex2i(xo + 1, yo + 1);
            glVertex2i(xo + 1, yo + mapS - 1);
            glVertex2i(xo + mapS - 1, yo + mapS - 1);
            glVertex2i(xo + mapS - 1, yo + 1);

            glEnd();
        }
    }
}

void drawPlayer2D()
{
    glColor3f(1,1,0);

    glPointSize(8);
    glLineWidth(4);

    glBegin(GL_POINTS);
    glVertex2i(px, py);
    glEnd();

    glBegin(GL_LINES);
    glVertex2i(px, py);
    glVertex2i(px + pdx * 20, py + pdy * 20);
    glEnd();
}

void Buttons(unsigned char key, int x, int y)
{
    float moveSpeed = 5;

    float nextX = px;
    float nextY = py;

    if (key == 'a')
    {
        pa += 5;
        pa = FixAng(pa);

        pdx = cos(degToRad(pa));
        pdy = -sin(degToRad(pa));
    }

    if (key == 'd')
    {
        pa -= 5;
        pa = FixAng(pa);

        pdx = cos(degToRad(pa));
        pdy = -sin(degToRad(pa));
    }

    if (key == 'w')
    {
        nextX = px + pdx * moveSpeed;
        nextY = py + pdy * moveSpeed;
    }

    if (key == 's')
    {
        nextX = px - pdx * moveSpeed;
        nextY = py - pdy * moveSpeed;
    }

    int mapPosX = (int)(nextX + pdx * PLAYER_RADIUS) / mapS;
    int mapPosY = (int)(nextY + pdy * PLAYER_RADIUS) / mapS;

    int mapIndex = mapPosY * mapX + mapPosX;

    if (mapIndex >= 0 && mapIndex < mapX * mapY)
    {
        if (map[mapIndex] == 0)
        {
            px = nextX;
            py = nextY;
        }
    }

    glutPostRedisplay();
}

void drawRays2D()
{
    glColor3f(0,1,1);

    glBegin(GL_QUADS);
    glVertex2i(526,0);
    glVertex2i(1006,0);
    glVertex2i(1006,160);
    glVertex2i(526,160);
    glEnd();

    glColor3f(0,0,1);

    glBegin(GL_QUADS);
    glVertex2i(526,160);
    glVertex2i(1006,160);
    glVertex2i(1006,320);
    glVertex2i(526,320);
    glEnd();

    int r, mx, my, mp, dof;
    float vx, vy, rx, ry, ra, xo, yo, disV, disH;

    ra = FixAng(pa + (FOV / 2));

    for (r = 0; r < NUM_RAYS; r++)
    {
        dof = 0;
        disV = 100000;

        float Tan = tan(degToRad(ra));

        if (cos(degToRad(ra)) > 0.001)
        {
            rx = (((int)px >> 6) << 6) + 64;
            ry = (px - rx) * Tan + py;

            xo = 64;
            yo = -xo * Tan;
        }
        else if (cos(degToRad(ra)) < -0.001)
        {
            rx = (((int)px >> 6) << 6) - 0.0001;
            ry = (px - rx) * Tan + py;

            xo = -64;
            yo = -xo * Tan;
        }
        else
        {
            rx = px;
            ry = py;
            dof = 8;
        }

        while (dof < 8)
        {
            mx = (int)(rx) >> 6;
            my = (int)(ry) >> 6;

            mp = my * mapX + mx;

            if (mp > 0 && mp < mapX * mapY && map[mp] == 1)
            {
                dof = 8;

                disV = cos(degToRad(ra)) * (rx - px)
                     - sin(degToRad(ra)) * (ry - py);
            }
            else
            {
                rx += xo;
                ry += yo;

                dof += 1;
            }
        }

        vx = rx;
        vy = ry;

        dof = 0;
        disH = 100000;

        if (fabs(Tan) > 0.0001)
            Tan = 1.0 / Tan;
        else
            Tan = 1000000;

        if (sin(degToRad(ra)) > 0.001)
        {
            ry = (((int)py >> 6) << 6) - 0.0001;
            rx = (py - ry) * Tan + px;

            yo = -64;
            xo = -yo * Tan;
        }
        else if (sin(degToRad(ra)) < -0.001)
        {
            ry = (((int)py >> 6) << 6) + 64;
            rx = (py - ry) * Tan + px;

            yo = 64;
            xo = -yo * Tan;
        }
        else
        {
            rx = px;
            ry = py;
            dof = 8;
        }

        while (dof < 8)
        {
            mx = (int)(rx) >> 6;
            my = (int)(ry) >> 6;

            mp = my * mapX + mx;

            if (mp > 0 && mp < mapX * mapY && map[mp] == 1)
            {
                dof = 8;

                disH = cos(degToRad(ra)) * (rx - px)
                     - sin(degToRad(ra)) * (ry - py);
            }
            else
            {
                rx += xo;
                ry += yo;

                dof += 1;
            }
        }

        glColor3f(0,0.8,0);

        if (disV < disH)
        {
            rx = vx;
            ry = vy;

            disH = disV;

            glColor3f(0,0.6,0);
        }

        glLineWidth(2);

        glBegin(GL_LINES);
        glVertex2i(px, py);
        glVertex2i(rx, ry);
        glEnd();

        int ca = FixAng(pa - ra);

        disH = disH * cos(degToRad(ca));

        if (disH < 0.0001)
            disH = 0.0001;

        int lineH = (mapS * 320) / disH;

        if (lineH > 320)
            lineH = 320;

        int lineOff = 160 - (lineH >> 1);

        glLineWidth(4);

        glBegin(GL_LINES);

        glVertex2i(r * 4 + 530, lineOff);
        glVertex2i(r * 4 + 530, lineOff + lineH);

        glEnd();

        ra = FixAng(ra - ((float)FOV / NUM_RAYS));
    }
}

void init()
{
    glClearColor(0.3,0.3,0.3,0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluOrtho2D(0,1024,510,0);

    px = 150;
    py = 400;

    pa = 90;

    pdx = cos(degToRad(pa));
    pdy = -sin(degToRad(pa));
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawMap2D();
    drawPlayer2D();
    drawRays2D();

    glutSwapBuffers();
}

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    glutInitWindowSize(1024,510);

    glutCreateWindow("Raycaster");

    init();

    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutKeyboardFunc(Buttons);

    glutMainLoop();

    return 0;
}
