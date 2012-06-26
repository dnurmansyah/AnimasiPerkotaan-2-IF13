// terrain.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include "imageloader.h"
#include "vec3f.h"
#endif

float angle = 0;
using namespace std;

GLuint texture[2];
float lastx, lasty;
GLint stencilBits;

static int camera_z = 150;
static int camera_x = 0;
static int camera_y = 50;
static int posisi_z = 0;
static int posisi_x = 0;
static int posisi_y = 50;
float rot = 0;
int posMobil = 420;
int posBebek = 	160;


struct Gambar {
	unsigned long sizeX;
	unsigned long sizeY;
	char *data;
};
typedef struct Gambar Gambar; //struktur data untuk


//ukuran gambar #bisa di set sesuai kebutuhan


//mengambil gambar BMP
int GambarLoad(char *filename, Gambar *gambar) {
	FILE *file;
	unsigned long size; // ukuran gambar dalam bytes
	unsigned long i; // standard counter.
	unsigned short int plane; // number of planes in gambar

	unsigned short int bpp; // jumlah bits per pixel
	char temp; // temporary color storage for var warna sementara untuk memastikan filenya ada


	if ((file = fopen(filename, "rb")) == NULL) {
		printf("File Not Found : %s\n", filename);
		return 0;
	}
	// mencari file header bmp
	fseek(file, 18, SEEK_CUR);
	// read the width
	if ((i = fread(&gambar->sizeX, 4, 1, file)) != 1) {
		printf("Error reading width from %s.\n", filename);
		return 0;
	}
	//printf("Width of %s: %lu\n", filename, gambar->sizeX);
	// membaca nilai height
	if ((i = fread(&gambar->sizeY, 4, 1, file)) != 1) {
		printf("Error reading height from %s.\n", filename);
		return 0;
	}
	//printf("Height of %s: %lu\n", filename, gambar->sizeY);
	//menghitung ukuran gambar(asumsi 24 bits or 3 bytes per pixel).

	size = gambar->sizeX * gambar->sizeY * 3;
	// read the planes
	if ((fread(&plane, 2, 1, file)) != 1) {
		printf("Error reading planes from %s.\n", filename);
		return 0;
	}
	if (plane != 1) {
		printf("Planes from %s is not 1: %u\n", filename, plane);
		return 0;
	}
	// read the bitsperpixel
	if ((i = fread(&bpp, 2, 1, file)) != 1) {
		printf("Error reading bpp from %s.\n", filename);

		return 0;
	}
	if (bpp != 24) {
		printf("Bpp from %s is not 24: %u\n", filename, bpp);
		return 0;
	}
	// seek past the rest of the bitmap header.
	fseek(file, 24, SEEK_CUR);
	// read the data.
	gambar->data = (char *) malloc(size);
	if (gambar->data == NULL) {
		printf("Error allocating memory for color-corrected gambar data");
		return 0;
	}
	if ((i = fread(gambar->data, size, 1, file)) != 1) {
		printf("Error reading gambar data from %s.\n", filename);
		return 0;
	}
	for (i = 0; i < size; i += 3) { // membalikan semuan nilai warna (gbr - > rgb)
		temp = gambar->data[i];
		gambar->data[i] = gambar->data[i + 2];
		gambar->data[i + 2] = temp;
	}
	// we're done.
	return 1;
}

//mengambil tekstur
Gambar * loadTexture() {
	Gambar *gambar1;
	// alokasi memmory untuk tekstur
	gambar1 = (Gambar *) malloc(sizeof(Gambar));
	if (gambar1 == NULL) {
		printf("Error allocating space for gambar");
		exit(0);
	}
	//pic.bmp is a 64x64 picture
	if (!GambarLoad("beton.bmp", gambar1)) {
		exit(1);
	}
	return gambar1;
}
Gambar * loadTextureDua() {
	Gambar *gambar1;
	// alokasi memmory untuk tekstur
	gambar1 = (Gambar *) malloc(sizeof(Gambar));
	if (gambar1 == NULL) {
		printf("Error allocating space for gambar");
		exit(0);
	}
	//pic.bmp is a 64x64 picture
	if (!GambarLoad("water.bmp", gambar1)) {
		exit(1);
	}
	return gambar1;
}
//train 2D
//class untuk terain 2D
class Terrain {
private:
	int w; //Width
	int l; //Length
	float** hs; //Heights
	Vec3f** normals;
	bool computedNormals; //Whether normals is up-to-date
public:
	Terrain(int w2, int l2) {
		w = w2;
		l = l2;

		hs = new float*[l];
		for (int i = 0; i < l; i++) {
			hs[i] = new float[w];
		}

		normals = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals[i] = new Vec3f[w];
		}

		computedNormals = false;
	}

	~Terrain() {
		for (int i = 0; i < l; i++) {
			delete[] hs[i];
		}
		delete[] hs;

		for (int i = 0; i < l; i++) {
			delete[] normals[i];
		}
		delete[] normals;
	}

	int width() {
		return w;
	}

	int length() {
		return l;
	}

	//Sets the height at (x, z) to y
	void setHeight(int x, int z, float y) {
		hs[z][x] = y;
		computedNormals = false;
	}

	//Returns the height at (x, z)
	float getHeight(int x, int z) {
		return hs[z][x];
	}

	//Computes the normals, if they haven't been computed yet
	void computeNormals() {
		if (computedNormals) {
			return;
		}

		//Compute the rough version of the normals
		Vec3f** normals2 = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals2[i] = new Vec3f[w];
		}

		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum(0.0f, 0.0f, 0.0f);

				Vec3f out;
				if (z > 0) {
					out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
				}
				Vec3f in;
				if (z < l - 1) {
					in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
				}
				Vec3f left;
				if (x > 0) {
					left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
				}
				Vec3f right;
				if (x < w - 1) {
					right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
				}

				if (x > 0 && z > 0) {
					sum += out.cross(left).normalize();
				}
				if (x > 0 && z < l - 1) {
					sum += left.cross(in).normalize();
				}
				if (x < w - 1 && z < l - 1) {
					sum += in.cross(right).normalize();
				}
				if (x < w - 1 && z > 0) {
					sum += right.cross(out).normalize();
				}

				normals2[z][x] = sum;
			}
		}

		//Smooth out the normals
		const float FALLOUT_RATIO = 0.5f;
		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum = normals2[z][x];

				if (x > 0) {
					sum += normals2[z][x - 1] * FALLOUT_RATIO;
				}
				if (x < w - 1) {
					sum += normals2[z][x + 1] * FALLOUT_RATIO;
				}
				if (z > 0) {
					sum += normals2[z - 1][x] * FALLOUT_RATIO;
				}
				if (z < l - 1) {
					sum += normals2[z + 1][x] * FALLOUT_RATIO;
				}

				if (sum.magnitude() == 0) {
					sum = Vec3f(0.0f, 1.0f, 0.0f);
				}
				normals[z][x] = sum;
			}
		}

		for (int i = 0; i < l; i++) {
			delete[] normals2[i];
		}
		delete[] normals2;

		computedNormals = true;
	}

	//Returns the normal at (x, z)
	Vec3f getNormal(int x, int z) {
		if (!computedNormals) {
			computeNormals();
		}
		return normals[z][x];
	}
};
//end class


void initRendering() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);
}

//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
//load terain di procedure inisialisasi
Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for (int y = 0; y < image->height; y++) {
		for (int x = 0; x < image->width; x++) {
			unsigned char color = (unsigned char) image->pixels[3 * (y
					* image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}

	delete image;
	t->computeNormals();
	return t;
}

float _angle = 60.0f;
//buat tipe data terain
Terrain* _terrain;
Terrain* _terrainTanah;
Terrain* _terrainAir;

const GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
const GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };

const GLfloat light_ambient2[] = { 0.3f, 0.3f, 0.3f, 0.0f };
const GLfloat light_diffuse2[] = { 0.3f, 0.3f, 0.3f, 0.0f };

const GLfloat mat_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

void cleanup() {
	delete _terrain;
	delete _terrainTanah;
}

//untuk di display
void drawSceneTanah(Terrain *terrain, GLfloat r, GLfloat g, GLfloat b) {
	//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	/*
	 glMatrixMode(GL_MODELVIEW);
	 glLoadIdentity();
	 glTranslatef(0.0f, 0.0f, -10.0f);
	 glRotatef(30.0f, 1.0f, 0.0f, 0.0f);
	 glRotatef(-_angle, 0.0f, 1.0f, 0.0f);

	 GLfloat ambientColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
	 glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

	 GLfloat lightColor0[] = {0.6f, 0.6f, 0.6f, 1.0f};
	 GLfloat lightPos0[] = {-0.5f, 0.8f, 0.1f, 0.0f};
	 glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
	 glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);
	 */
	float scale = 300.0f / max(terrain->width() - 1, terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float) (terrain->width() - 1) / 2, 0.0f,
			-(float) (terrain->length() - 1) / 2);

	glColor3f(r, g, b);
	for (int z = 0; z < terrain->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < terrain->width(); x++) {
			Vec3f normal = terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z), z);
			normal = terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}

}

unsigned int LoadTextureFromBmpFile(char *filename);

void dasar()
{
glPushMatrix();
glBegin(GL_POLYGON);
glTexCoord2f(0.0, 0.0);
glVertex3f(1, 10, 0.0);
glTexCoord2f(1.0, 0.0);
glVertex3f(10, 10, 0.0);
glTexCoord2f(1.0, 1.0);
glVertex3f(10, 20, 0.0);
glTexCoord2f(0.0, 1.0);
glVertex3f(1, 20, 0.0);
glEnd();

glPopMatrix();
}
void drawBebek() {

	GLfloat xS = 20;
	GLfloat yS = 12;
	GLfloat zS = 50;
	//kepala
	glColor3f(0.88, 0.88, 0.1);
	//glPushMatrix();
	glPushMatrix();

	glTranslatef(xS + 1, yS + 3, zS);
	glScalef(3 * 4, 3 * 4, 3.5 * 4);

	glutSolidSphere(0.5, 20, 30);
	glPopMatrix();

	glColor3f(0.8, 0.3, 0.4);
	//glPushMatrix();

	glPushMatrix();
	glTranslatef(xS + 1, yS + 6, zS);
	glScalef(3 * 4, 3 * 4, 0.5 * 4);

	glutSolidSphere(0.5, 20, 30);
	glPopMatrix();

	GLfloat x = 0.6 - 50;
	GLfloat y = 2 + 12;
	GLfloat z = 1.4 + 24;

	glColor3f(0.7, 0.6, 0.4);
	glPushMatrix();
	glRotatef(95, 0, 1, 0);
	glTranslatef(x - 4.4, y + 2, z - 4.6);
	glScalef(1.5 * 4, 1.5 * 4, 1.5 * 4);

	glutSolidSphere(0.3, 20, 30);

	glPopMatrix();

	glColor3f(0.0, 0.0, 0.0);
	glPushMatrix();
	glRotatef(95, 0, 1, 0);
	glTranslatef(x - 4.7, y + 2.1, z - 3.4);
	glScalef(1.5 * 3, 1.5 * 3, 1.5 * 3);
	glutSolidSphere(0.18, 20, 30);

	glPopMatrix();

	glPushMatrix();
	glColor3f(0.7, 0.6, 0.4);
	glRotatef(83, 0, 1, 0);
	glTranslatef(x + 4.9, y + 2.1, z + 5.8);
	glScalef(1.5 * 4, 1.5 * 4, 1.5 * 4);
	glutSolidSphere(0.3, 20, 30);

	glPopMatrix();

	glPushMatrix();
	glColor3f(0.0, 0.0, 0.0);
	glRotatef(83, 0, 1, 0);
	glTranslatef(x + 4.9, y + 2.1, z + 6.65);
	glScalef(1.5 * 4, 1.5 * 4, 1.5 * 4);
	glutSolidSphere(0.18, 20, 30);

	glPopMatrix();

	glColor3f(0.88, 0.88, 0.3);

	//mulut


	glPushMatrix();

	glTranslatef(xS + 8, yS + 2, zS);
	glRotatef(180, 0, 0, 1);
	glScalef(4 * 3, 0.6 * 3, 2.5 * 3);

	glutSolidSphere(0.5, 20, 30);
	glPopMatrix();
	//

	glColor3f(0.88, 0.2, 0.5);

	glPushMatrix();

	glTranslatef(xS + 1, yS - 3, zS);
	glScalef(3 * 4, 3 * 4, 3.5 * 4);
	glRotatef(90, 1, 0, 0);

	glutSolidTorus(0.2, 0.3, 20, 20);

	//badan agak atas
	glPopMatrix();

	glColor3f(0.88, 0.88, 0.1);

	glPushMatrix();
	glTranslatef(xS + 1, yS - 6, zS - 0.4);
	glScalef(3 * 4, 3 * 4, 3.5 * 4);

	glutSolidSphere(0.5, 20, 30);
	glPopMatrix();

	//badan utama
	glPushMatrix();
	glColor3f(0.88, 0.88, 0.1);
	glTranslatef(xS - 7, yS - 10, zS - 0.3);
	glScalef(5 * 4, 4 * 4, 4 * 4);

	glutSolidSphere(0.5, 20, 30);
	glPopMatrix();

	//badan depan bawah
	glPushMatrix();
	glColor3f(0.88, 0.88, 0.1);
	glTranslatef(xS, yS - 8, zS - 0.2);
	glScalef(3 * 4, 3 * 4, 3.5 * 4);

	glutSolidSphere(0.5, 20, 30);
	glPopMatrix();
	//
	int sudutSayap = 80;
	//sayap
	for (int i = 0; i < 3; i++) {
		//kanan
		glPushMatrix();
		glTranslatef(xS - 6, yS - 5 - i, zS + 7);
		glRotatef(sudutSayap, 1, 0, 0);
		glRotatef(-15 + (i * 8), 0, 1, 0);
		glRotatef(-8.5, 0, 0, 1);
		glScalef(4 * 4.5 - i, 0.6 * 3, 2.5 * 3 - 1);

		glutSolidSphere(0.5, 20, 30);
		glPopMatrix();
		//kiri
		glPushMatrix();
		glTranslatef(xS - 6, yS - 5 - i, zS - 8.2);
		glRotatef(sudutSayap + 15, 1, 0, 0);
		glRotatef(-15 + (i * 8), 0, 1, 0);
		glRotatef(10.5, 0, 0, 1);
		glScalef(4 * 4.5 - i, 0.6 * 3, 2.5 * 3 - 1);

		glutSolidSphere(0.5, 20, 30);
		glPopMatrix();

	}

	//buntut
	int sudut = 145;
	for (int i = 0; i < 4; i++) {
		glPushMatrix();

		glTranslatef(xS - 12.5 + i, yS - 5 - i, zS);
		glRotatef(sudut, 0, 0, 1);
		glScalef(4 * 6.5, 0.6 * 6, (2.5 * 4) - i);

		glutSolidSphere(0.5, 20, 30);
		glPopMatrix();
		sudut += 10;
	}

	//glutSolidCone(2,20,30,40);
	/*
	 glScalef(0.5, 0.5, 0.5);
	 if (kualitasGambar > DRAFT) {
	 glutSolidCube(3);

	 } else {
	 glutWireCube(3);

	 }

	 glTranslatef(-4.8, -0.1, 0.0);
	 */
	/*
	 glTranslatef(4.6, 0.0, 0.0);
	 if (kualitasGambar > DRAFT)
	 glutSolidCube(1);
	 else
	 glutWireCube(1);
	 glPopMatrix();
	 */

	glPopMatrix();
}
void rumah(){

//tembok
 glPushMatrix();
 glColor3f(0,0,0.5);
 glRotatef(50,0,1,0);
 glutSolidCube(3);

//pintu
glPushMatrix();
glColor3f(0.5,0.8,0);
glTranslatef(-0.6,-1,1.46);
glScalef(7,10,1);
glutSolidCube(0.1);
glPopMatrix();
//jendela
glPushMatrix();
glColor3f(0.5,0.8,0);
glTranslatef(0.5,0.1,1.46);
glScalef(3,3,1);
glutSolidCube(0.1);
glPopMatrix();

glPushMatrix();
glColor3f(0.5,0.8,0);
glTranslatef(0.9,0.1,1.46);
glScalef(3,3,1);
glutSolidCube(0.1);
glPopMatrix();

glPushMatrix();
glColor3f(0.5,0.8,0);
glTranslatef(0.9,-0.3,1.46);
glScalef(3,3,1);
glutSolidCube(0.1);
glPopMatrix();

glPushMatrix();
glColor3f(0.5,0.8,0);
glTranslatef(0.5,-0.3,1.46);
glScalef(3,3,1);
glutSolidCube(0.1);
glPopMatrix();
glPopMatrix();

//atap
 glPushMatrix();
 glColor3f(0.8,0,0);
 glRotatef(5,0,1,0);
 glTranslatef(0,1.5,0);
 glScalef(3,1.3,3);
 glutSolidOctahedron();
 glPopMatrix();

}
void pohon(){
	glColor3f(0.8, 0.5, 0.2);
	//<<<<<<<<<<<<<<<<<<<< Batang >>>>>>>>>>>>>>>>>>>>>>>
	glPushMatrix();
	glScalef(0.2, 4, 0.2);
	glutSolidSphere(1.0, 20, 16);
	glPopMatrix();
	//<<<<<<<<<<<<<<<<<<<< end Batang >>>>>>>>>>>>>>>>>>>>>>>

	glColor3f(0.0, 1.0, 0.0);
	//<<<<<<<<<<<<<<<<<<<< Daun >>>>>>>>>>>>>>>>>>>>>>>
	glPushMatrix();
	glScalef(1, 1, 1.0);
	glTranslatef(0, 1, 0);
	glRotatef(270, 1, 0, 0);
	glutSolidCone(1,3,10,1);	
	glPopMatrix();

	glPushMatrix();
	glScalef(1, 1, 1.0);
	glTranslatef(0, 2, 0);
	glRotatef(270, 1, 0, 0);
	glutSolidCone(1,3,10,1);	
	glPopMatrix();

	glPushMatrix();
	glScalef(1, 1, 1.0);
	glTranslatef(0, 3, 0);
	glRotatef(270, 1, 0, 0);
	glutSolidCone(1,3,10,1);	
	glPopMatrix();
	//<<<<<<<<<<<<<<<<<<<< end Daun >>>>>>>>>>>>>>>>>>>>>>>


}
void mobilElips(){
    glPushMatrix();
	glTranslatef(-3,18,0);
	glScalef(12,8,12);
	glutSolidSphere(1,20,10);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0,15,0);
	glScalef(30,5,12);
	glutSolidSphere(1,20,10);
	glPopMatrix();

	glColor3f(0,0,0);
	glPushMatrix();
	glTranslatef(-15,12,9);
	glScalef(1,1,1);
	glutSolidTorus(2,3,40,20);
	glPopMatrix();
	
	glPushMatrix();
	glTranslatef(15,12,9);
	glScalef(1,1,1);
	glutSolidTorus(2,3,40,20);
	glPopMatrix();

	glColor3f(0,0,0);
	glPushMatrix();
	glTranslatef(-15,12,-9);
	glScalef(1,1,1);
	glutSolidTorus(2,3,40,20);
	glPopMatrix();
	
	glPushMatrix();
	glTranslatef(15,12,-9);
	glScalef(1,1,1);
	glutSolidTorus(2,3,40,20);
	glPopMatrix();
}
void mobilKotak(){
glPushMatrix();
    //bagian atas
    glColor3f(0.4, 0.2, 0.4);
    glTranslatef(0,20,0);
	glScalef(10,8,10);
	glutSolidCube(2);
	glPopMatrix();
	//bagian bawah
	glPushMatrix();
	glTranslatef(0,15,0);
	glScalef(25,5,10);
	glutSolidCube(2);
	glPopMatrix();
	//kaca
	glColor3f(1,0,1);
	glPushMatrix();
	glTranslatef(10,20,0);
	glScalef(0.1,6,7);
	glutSolidCube(2);
	glPopMatrix();

	glColor3f(1,0,1);
	glPushMatrix();
	glTranslatef(0,23,10);
	glScalef(8,3,0.1);
	glutSolidCube(2);
	glPopMatrix();

	glColor3f(1,0,1);
	glPushMatrix();
	glTranslatef(0,23,-10);
	glScalef(8,3,0.1);
	glutSolidCube(2);
	glPopMatrix();

	//lampu
	glColor3f(0.8, 1.0, 1.0);
	glPushMatrix();
	glTranslatef(25,16,-6);
	glScalef(0.1,6,7);
	glutSolidSphere(0.3,20,20);
	glPopMatrix();

	glColor3f(0.8, 1.0, 1.0);
	glPushMatrix();
	glTranslatef(25,16, 6);
	glScalef(0.1,6,7);
	glutSolidSphere(0.3,20,20);
	glPopMatrix();

	//bemper
	glColor3f(0,0,0);
	glPushMatrix();
	glTranslatef(25,11, 0);
	glScalef(1,1,10);
	glutSolidCube(2);
	glPopMatrix();

	//ban
	glColor3f(1,0,1);
	glPushMatrix();
	glTranslatef(10,20,0);
	glScalef(0.1,6,7);
	glutSolidCube(2);
	glPopMatrix();

	glColor3f(0,0,0);	
	glPushMatrix();
	glTranslatef(-15,10,11);
	glScalef(1.2,1.2,1.2);
	glutSolidTorus(2,3,40,20);
	glPopMatrix();
	
	glPushMatrix();
	glTranslatef(15,10,11);
	glScalef(1.2,1.2,1.2);
	glutSolidTorus(2,3,40,20);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-15,10,-11);
	glScalef(1.2,1.2,1.2);
	glutSolidTorus(2,3,40,20);
	glPopMatrix();
	
	glPushMatrix();
	glTranslatef(15,10,-11);
	glScalef(1.2,1.2,1.2);
	glutSolidTorus(2,3,40,20);
	glPopMatrix();
}






void display(void) {
	glClearStencil(0); //clear the stencil buffer
	glClearDepth(1.0f);
	glClearColor(0.0, 0.6, 0.8, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //clear the buffers
	glLoadIdentity();
	gluLookAt(camera_x, camera_y, camera_z, posisi_x, posisi_y, posisi_z, 0.0, 1.0, 0);
    
	//mobil
	glPushMatrix();
	glTranslatef(440,-13,280);
	glRotatef(90,0,1,0);
	mobilElips();
	glPopMatrix();
	
	glPushMatrix();
	glTranslatef(360,-13,280);
	glRotatef(270,0,1,0);
	mobilKotak();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(400,-13,280);
	glRotatef(270,0,1,0);
	mobilKotak();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(300,-13,330);	
	mobilKotak();
	glPopMatrix();
	
	//mobil jalan
	glPushMatrix();
	glTranslatef(posMobil,-13,180);	
	glRotatef(180,0,1,0);	
	mobilKotak();
	glPopMatrix();

	//pagar
	glColor3f(0.4, 0.0, 0.2);
	int k;
	for (k=0; k<17; k++){
	glPushMatrix();	
	glTranslatef(313+(10*k), 0, 230);
	glScalef(0.1, 1.2, 0.1);
	glutSolidSphere(10, 40, 20);	
	glPopMatrix();

	glPushMatrix();	
	glTranslatef(313+(10*k), -1, 230);
	glScalef(10, 17, 4);
	glutSolidCube(1);	
	glPopMatrix();	
	}

	int l;
	for (l=0; l<12; l++){
	glPushMatrix();	
	glTranslatef(260, 0, 235+(10*l));
	glScalef(0.1, 1.2, 0.1);
	glutSolidSphere(10, 40, 20);	
	glPopMatrix();

	glPushMatrix();	
	glTranslatef(260, -1, 235+(10*l));
	glScalef(4, 17, 10);
	glutSolidCube(1);	
	glPopMatrix();

	
	}
	//jalan
	glColor3f(0.0f, 0.0f, 0.0f);
	glPushMatrix();
	glTranslatef(150, -9, 800);	
	glRotatef(270, 1.0, 0.0, 0.0);
	glScalef(10, 45, 0);
	dasar();
	glPopMatrix();

	//jalan
	glColor3f(0.0f, 0.0f, 0.0f);
	glPushMatrix();
	glTranslatef(-220, -9, 330);	
	glRotatef(270, 1.0, 0.0, 0.0);
	glScalef(70, 10, 0);
	dasar();
	glPopMatrix();

	//dasar
	glColor3f(1.0f, 1.0f, 1.0f);
	glPushMatrix();
	glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,texture[0]);	
	glTranslatef(-220, -10, 800);	
	glRotatef(270, 1.0, 0.0, 0.0);
	glScalef(70, 45, 0);
	dasar();
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();

	//<<<<<<<<<<<<<<<<<<<< pohon >>>>>>>>>>>>>>>>>>>>>>>
	glPushMatrix();
	glColor3f(1, 0, 0);
	glTranslatef(-110, 0, 85);
	glScalef(10, 10, 10);	
	pohon();
	glPopMatrix();

	glPushMatrix();
	glColor3f(1, 0, 0);
	glTranslatef(-110, 0, -85);
	glScalef(10, 10, 10);	
	pohon();
	glPopMatrix();

	glPushMatrix();
	glColor3f(1, 0, 0);
	glTranslatef(-110, 0, 0);
	glScalef(10, 10, 10);	
	pohon();
	glPopMatrix();

	glPushMatrix();
	glColor3f(1, 0, 0);
	glTranslatef(0, 0, -85);
	glScalef(10, 10, 10);	
	pohon();
	glPopMatrix();

	glPushMatrix();
	glColor3f(1, 0, 0);
	glTranslatef(0, 0, 85);
	glScalef(10, 10, 10);	
	pohon();
	glPopMatrix();

	glPushMatrix();
	glColor3f(1, 0, 0);
	glTranslatef(125, 0, -85);
	glScalef(10, 10, 10);	
	pohon();
	glPopMatrix();

	glPushMatrix();
	glColor3f(1, 0, 0);
	glTranslatef(125, 0, 85);
	glScalef(10, 10, 10);	
	pohon();
	glPopMatrix();

	//<<<<<<<<<<<<<<<<<<<< end pohon >>>>>>>>>>>>>>>>>>>>>>>
	
	//hotel
	glPushMatrix();
	glTranslatef(400,200, 0);	
	glScalef(50, 150, 50);
	glRotatef(45, 0, 1, 0);
	rumah();
	glPopMatrix();

	//rumah warga
	int i;
	for ( i=0;i<5;i++){	
	glPushMatrix();
	glScalef(10, 10, 10);	
	glTranslatef(-10+(i*6), 1, 28);	
	glRotatef(40, 0, 1, 0);
	rumah();
	glPopMatrix();
	}

	int j;
	for ( j=0;j<5;j++){	
	glPushMatrix();
	glScalef(10, 10, 10);	
	glTranslatef(-10+(j*6), 1, 34);	
	glRotatef(40, 0, 1, 0);
	rumah();
	glPopMatrix();
	}

	


	//bebek
	glPushMatrix();
	glColor3f(1, 0, 0);
	glTranslatef(-40, 0, -20);
	glScalef(0.5, 0.5, 0.5);
	glRotatef(-30, 0, 1, 0);
	drawBebek();
	glPopMatrix();

	glPushMatrix();
	glColor3f(1, 0, 0);
	glTranslatef(0, 0, 0);
	glScalef(0.5, 0.5, 0.5);
	glRotatef(posBebek, 0, 1, 0);
	drawBebek();
	glPopMatrix();

	glPushMatrix();
	glColor3f(1, 0, 0);
	glTranslatef(30, 0, -20);
	glScalef(0.5, 0.5, 0.5);
	//glRotatef(, 0, 1, 0);
	drawBebek();
	glPopMatrix();


	
	glPushMatrix();	
	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrain, 0.3f, 0.9f, 0.0f);
	glPopMatrix();

	glPushMatrix();
	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrainTanah, 0.7f, 0.2f, 0.1f);
	glPopMatrix();

	glPushMatrix();
	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrainAir, 0.0f, 0.2f, 0.5f);
	glPopMatrix();
	glPushMatrix();
	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrainAir, 0.0f, 0.2f, 0.5f);
	glPopMatrix();

	glutSwapBuffers();
	glFlush();
	rot++;
	angle++;

}

void init(void) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glDepthFunc(GL_LESS);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_CULL_FACE);

	_terrain = loadTerrain("heightmap.bmp", 20);
	_terrainTanah = loadTerrain("heightmapTanah.bmp", 20);
	_terrainAir = loadTerrain("heightmapAir.bmp", 20);

	//binding texture
    //glClearColor(0.5, 0.5, 0.5, 0.0);
	Gambar *gambar1 = loadTexture();
	Gambar *gambar2 = loadTextureDua();

	if (gambar1 == NULL) {
		printf("Gambar was not returned from loadTexture\n");
		exit(0);
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Generate texture/ membuat texture
	glGenTextures(2, texture);

	//binding texture untuk membuat texture 2D
	glBindTexture(GL_TEXTURE_2D, texture[0]);

	//menyesuaikan ukuran textur ketika gambar lebih besar dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //
	//menyesuaikan ukuran textur ketika gambar lebih kecil dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //

	glTexImage2D(GL_TEXTURE_2D, 0, 3, gambar1->sizeX, gambar1->sizeY, 0, GL_RGB,
			GL_UNSIGNED_BYTE, gambar1->data);

	//tekstur air

	//binding texture untuk membuat texture 2D
	glBindTexture(GL_TEXTURE_2D, texture[2]);

	//menyesuaikan ukuran textur ketika gambar lebih besar dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //
	//menyesuaikan ukuran textur ketika gambar lebih kecil dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //

	glTexImage2D(GL_TEXTURE_2D, 0, 3, gambar2->sizeX, gambar2->sizeY, 0, GL_RGB,
			GL_UNSIGNED_BYTE, gambar2->data);	

}

static void kibor(int key, int x, int y) {
	switch (key) {

		case GLUT_KEY_HOME:	
		camera_y = camera_y+15;
		posisi_y = posisi_y + 15;	
		break;
	case GLUT_KEY_END:
		camera_y = camera_y-15;
		posisi_y = posisi_y - 15;	
		break;
	case GLUT_KEY_RIGHT:
		camera_x = camera_x+15;
		posisi_x = posisi_x + 15;		
		break;
	case GLUT_KEY_LEFT:
		camera_x = camera_x-15;
		posisi_x = posisi_x - 15;
		break;

	case GLUT_KEY_UP:
		camera_z = camera_z -15;	
		posisi_z = posisi_z - 15;
		break;
	case GLUT_KEY_DOWN:
		camera_z = camera_z+15;
		posisi_z = posisi_z + 15;
		break;	
	}
}

void keyboard(unsigned char key, int x, int y) {
	if (key == 'w') 
	{
		posisi_y = posisi_y - 15;
	}
	if (key == 's') 
	{
		posisi_y = posisi_y + 15;
	}
    if (key == 'm') 
	{
		posMobil = posMobil - 15;
		if (posMobil <= -120){
			posMobil = 420;
		}
	}

	if (key == 'b') 
	{
		posBebek = posBebek + 10;
		if (posBebek == 360){
			posBebek = 0;
		}
	}


}

void reshape(int w, int h) {
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (GLfloat) w / (GLfloat) h, 0.1, 1000.0);
	glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH); //add a stencil buffer to the window
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Sample Terain");
	init();

	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(kibor);

	glutKeyboardFunc(keyboard);

	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
	glColorMaterial(GL_FRONT, GL_DIFFUSE);

	glutMainLoop();
	return 0;
}






