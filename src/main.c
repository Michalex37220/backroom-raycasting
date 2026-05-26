#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>
#define PI 3.14

float px, py, pdx, pdy, pa;

void drawPlayer()
{
	glColor3f(1,1,0);
	glPointSize(8);
	glBegin(GL_POINTS);
	glVertex2i(px, py);
	glEnd();

	glLineWidth(3);
	glBegin(GL_LINES);
	glVertex2i(px, py);
	glVertex2i(px + pdx * 5, py + pdy *5);
	glEnd();
}

int mapX = 8;
int mapY = 8;
int mapS = 64;
int map[]=
{
	1,1,1,1,1,1,1,1,
	1,0,1,0,0,0,0,1,
	1,0,1,0,0,0,0,1,
	1,0,1,0,0,0,0,1,
	1,0,0,0,0,0,0,1,
	1,0,0,0,0,1,0,1,
	1,0,0,0,0,0,0,1,
	1,1,1,1,1,1,1,1,
};

void drawMap2D()
{
	int x, y, x0, y0;
	for (y = 0; y < mapY;y++) {
		for (x = 0; x < mapX;x++) {
			if (map[y*mapX+x] == 1) {
				glColor3f(1,1,1);
			}
			else {
				glColor3f(0,0,0);
			}
			x0 = x*mapS;
			y0 = y*mapS;
			glBegin(GL_QUADS);
			glVertex2i(x0+1, y0+1);
			glVertex2i(x0+1, y0 + mapS-1);
			glVertex2i(x0 + mapS-1, y0+mapS-1);
			glVertex2i(x0 + mapS-1, y0+1);
			glEnd();
		}
	}
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawMap2D();
	drawPlayer();
	glutSwapBuffers();
}

void buttons(unsigned char key, int x, int y)
{
	if (key == 'a') {
		pa -= 0.1;
		if (pa < 0) {
			pa += 2*PI; 
		}
		pdx = cos(pa) * 5;
		pdy = sin(pa) * 5;
	}
	if (key == 'd') {
			pa += 0.1;
			if (pa > 2*PI) {
				pa -= 2 * PI;
			}
			pdx = cos(pa)*5;
			pdy = sin(pa) * 5;
		}
	if (key == 'w') {
			px += pdx;
			py += pdy;
		}
	if (key == 's') {
			px -= pdx;
			py -= pdy;
		}
	glutPostRedisplay();
}
void init()
{
	glClearColor(0.3,0.3,0.3,0);
	gluOrtho2D(0,1024,512,0);
	px = 300;
	py = 300;
	pdx = cos(pa) * 5;
	pdy = sin(pa) * 5;
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(1024, 512);
	glutCreateWindow("Backroom Raycasting");
	init();
	glutDisplayFunc(display);
	glutKeyboardFunc(buttons);
	glutMainLoop();
}
