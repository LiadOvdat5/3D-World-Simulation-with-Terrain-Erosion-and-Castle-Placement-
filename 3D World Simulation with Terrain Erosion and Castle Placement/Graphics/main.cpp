#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "glut.h"
#include <stdio.h>

const double PI = 3.14159;
const int GSZ = 100;
const int W = 600;
const int H = 600;

// ground
double ground[GSZ][GSZ] = { 0 };
double river[GSZ][GSZ] = { 0 };
double tmpg[GSZ][GSZ] = { 0 };
double originalGround[GSZ][GSZ] = { 0 };
double riverOriginal[GSZ][GSZ] = { 0 };


typedef struct
{
	double x, y, z;
} POINT3D;

// ego-motion
POINT3D eye = {0,13,20};
double dx = 0.1, dz = 0.1, dy =0.1;
double speed = 0;
double angular_speed = 0;
POINT3D dir;
double sight_angle = PI;

double angle = 0;


bool isCaptured = false;

unsigned char* bmp = NULL;
int gap = 15;

// Texture definitions
const int TW = 512;
const int TH = 512;

unsigned char tx0[TH][TW][3];
unsigned char tx1[1024][1024][3];


// Rain drop parameters
bool isRaining = false; // Rain status
const int numDrops = 300; // Adjust the number of raindrops
int dropHeight = 12;  // Height of the raindrop when it starts falling
double dropGroundErosion = 0.01;  // Amount of ground erosion when a raindrop hits the ground
const double riverSurfaceDepth = 0.3; // Depth of the river
const int erosionRadius = 1;  // Radius of the erosion area around the raindrop
const int checkForRiverRadius = 10;  // Radius to check for nearby river points

struct Raindrop {
	double x, y, z;  // Position of the raindrop
	double speed;    // Falling speed
};
Raindrop raindrops[numDrops];

// Castle placement parameters
bool isAreaFlattened = false;  // To track if we have already flattened an area
const int flattenSize = 7;  // Size of the flattened area
int searchRadius = 2;  // Surrounding radius to check for river and sea
int prevFlattenX, prevFlattenZ;  // Previous flattened area's center
double prevFlattenHeight;  // Previous flattened area's height






/*
* Reads picture in format 24-bit bmp
* in the end all the picture is stored in bmp as [bgrbgrbgrbgr.........]
*/
void ReadPicture(char* fname)
{
	int size;
	FILE* f = fopen(fname, "rb");
	BITMAPFILEHEADER bf;
	BITMAPINFOHEADER bi;

	fread(&bf, sizeof(bf), 1, f);
	fread(&bi, sizeof(bi), 1, f);
	size = bi.biWidth * bi.biHeight * 3;

	bmp = (unsigned char*) malloc(size);

	if(bmp!=NULL) fread(bmp, 1, size, f);

	fclose(f);
}

// Set Texture
void SetupTexture(int num_texture)
{
	int i, j, rnd = 0;
	int bigRadius = TH / 2 + 10, smallRadius = 40;
	int k;

	switch (num_texture)
	{
	case 0: // bricks
		for (i = 0; i < TH; i++)
			for (j = 0; j < TW; j++)
			{
				rnd = rand() % 30;

				// top half
				if (i > TH / 2)
				{
					if (i % (TH / 2) < 20 || j % (TW / 2) < 20) // cement
					{
						tx0[i][j][0] = 90 + rnd;
						tx0[i][j][1] = 90 + rnd;
						tx0[i][j][2] = 90 + rnd;
					}
					else // brick
					{
						tx0[i][j][0] = 210 + rnd;
						tx0[i][j][1] = 195 + rnd;
						tx0[i][j][2] = 170 + rnd;
					}
				}
				else // bottom half
				{
					if (i % (TH / 2) < 20 || (j % (TW / 4) < 20 && j > 30 && (j<TW / 2 || j>TW / 2 + 30)))
					{ // cement
						tx0[i][j][0] = 90 + rnd;
						tx0[i][j][1] = 90 + rnd;
						tx0[i][j][2] = 90 + rnd;
					}
					else // brick
					{
						tx0[i][j][0] = 210 + rnd;
						tx0[i][j][1] = 195 + rnd;
						tx0[i][j][2] = 170 + rnd;
					}

				}
			}
		break;
	case 1: // Road
		for (i = 0; i < TH; i++)
			for (j = 0; j < TW; j++)
			{
				rnd = rand() % 30;
				if (fabs(i - TW / 2) < 10 && j < TW / 2 || i<15 || i>TH - 15) // white
				{
					tx0[i][j][0] = 220 + rnd;
					tx0[i][j][1] = 220 + rnd;
					tx0[i][j][2] = 220 + rnd;
				}
				else // gray
				{
					tx0[i][j][0] = 120 + rnd;
					tx0[i][j][1] = 120 + rnd;
					tx0[i][j][2] = 120 + rnd;
				}
			}

		break;
	case 2: // Roundabout
		double distance;
		for (i = 0; i < TH; i++)
			for (j = 0; j < TW; j++)
			{
				distance = sqrt(pow(TH / 2 - i, 2) + pow(TW / 2 - j, 2));
				rnd = rand() % 30;
				if (distance > smallRadius && distance < bigRadius) // gray
				{
					tx0[i][j][0] = 120 + rnd;
					tx0[i][j][1] = 120 + rnd;
					tx0[i][j][2] = 120 + rnd;
				}
				else
				{
					tx0[i][j][0] = rnd;
					tx0[i][j][1] = rnd;
					tx0[i][j][2] = rnd;
				}

			}

		break;
	case 3: // crosswalk
		for (i = 0; i < TH; i++)
			for (j = 0; j < TW; j++)
			{
				rnd = rand() % 30;

				if (i > TH / 4 && i < 3 * TH / 4) //gray
				{
					tx0[i][j][0] = 120 + rnd;
					tx0[i][j][1] = 120 + rnd;
					tx0[i][j][2] = 120 + rnd;
				}
				else // white
				{
					tx0[i][j][0] = 220 + rnd;
					tx0[i][j][1] = 220 + rnd;
					tx0[i][j][2] = 220 + rnd;

				}
			}
		break;
	case 4: // wall with window (taken from file)
		k = 0; // index that runs in bmp
		for (i = 0; i < TH; i++)
			for (j = 0; j < TW; j++)
			{
				tx0[i][j][2] = bmp[k]; // blue
				tx0[i][j][1] = bmp[k + 1]; // green
				tx0[i][j][0] = bmp[k + 2];// red
				k += 3;
			}

		break;
	case 5: // wall with window (taken from file)
		k = 0; // index that runs in bmp
		for (i = 0; i < 1024; i++)
			for (j = 0; j < 1024; j++)
			{
				tx1[i][j][2] = bmp[k]; // blue
				tx1[i][j][1] = bmp[k + 1]; // green
				tx1[i][j][0] = bmp[k + 2];// red
				k += 3;
			}

		break;
	case 6: // fence
		for (i = 0; i < TH; i++)
			for (j = 0; j < TW; j++)
			{
				rnd = rand() % 30;
				if (i % 10 < 5) // white
				{
					tx0[i][j][0] = 220 + rnd;
					tx0[i][j][1] = 220 + rnd;
					tx0[i][j][2] = 220 + rnd;
				}
				else // gray
				{
					tx0[i][j][0] = 120 + rnd;
					tx0[i][j][1] = 120 + rnd;
					tx0[i][j][2] = 120 + rnd;
				}
			}
		break;


	}
}

// Initialize textures
void InitTextures() 
{
	char fname[40] = "wall_with_window.bmp";
	char fname1[40] = "wall_with_window1.bmp";
	char fname2[40] = "fence.bmp";
	char fname3[40] = "roof.bmp";
	char fname4[40] = "logo.bmp";
	char fname5[40] = "clouds.bmp";
	char fname6[40] = "castle_wall.bmp";

	SetupTexture(0); // bricks
	glBindTexture(GL_TEXTURE_2D, 0); // setting definitions of texture #0
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TW, TH, 0, GL_RGB, GL_UNSIGNED_BYTE, tx0);

	SetupTexture(1); // Road
	glBindTexture(GL_TEXTURE_2D, 1); // setting definitions of texture #1
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TW, TH, 0, GL_RGB, GL_UNSIGNED_BYTE, tx0);


	SetupTexture(2); // Roundabout
	glBindTexture(GL_TEXTURE_2D, 2); // setting definitions of texture #2
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TW, TH, 0, GL_RGB, GL_UNSIGNED_BYTE, tx0);


	SetupTexture(3); // Crosswalk
	glBindTexture(GL_TEXTURE_2D, 3); // setting definitions of texture #3
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TW, TH, 0, GL_RGB, GL_UNSIGNED_BYTE, tx0);

	ReadPicture(fname);
	SetupTexture(4); // Wall with  window
	glBindTexture(GL_TEXTURE_2D, 4); // setting definitions of texture #4
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TW, TH, 0, GL_RGB, GL_UNSIGNED_BYTE, tx0);

	ReadPicture(fname1);
	SetupTexture(4); // Wall with  window
	glBindTexture(GL_TEXTURE_2D, 5); // setting definitions of texture #5
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TW, TH, 0, GL_RGB, GL_UNSIGNED_BYTE, tx0);

	ReadPicture(fname2);
	SetupTexture(4); // fence
	glBindTexture(GL_TEXTURE_2D, 6); // setting definitions of texture #6
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TW, TH, 0, GL_RGB, GL_UNSIGNED_BYTE, tx0);

	ReadPicture(fname3);
	SetupTexture(4); // roof
	glBindTexture(GL_TEXTURE_2D, 7); // setting definitions of texture #7
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TW, TH, 0, GL_RGB, GL_UNSIGNED_BYTE, tx0);

	ReadPicture(fname4);
	SetupTexture(4); // logo
	glBindTexture(GL_TEXTURE_2D, 8); // setting definitions of texture #8
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TW, TH, 0, GL_RGB, GL_UNSIGNED_BYTE, tx0);

	ReadPicture(fname5);
	SetupTexture(5); // clouds
	glBindTexture(GL_TEXTURE_2D, 9); // setting definitions of texture #9
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, tx1);

	ReadPicture(fname6); // castle wall
	SetupTexture(4);
	glBindTexture(GL_TEXTURE_2D, 10); // setting definitions of texture #10
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TW, TH, 0, GL_RGB, GL_UNSIGNED_BYTE, tx0);

}


// Set surface of the ground
void SetSurface()
{
	int i, j;
	double max_height = 5;


	for (i = 0; i < GSZ;i++)
		for (j = 0; j < GSZ; j++)
		{
			ground[i][j] = max_height*(1+sin(j/3.0))+ max_height * (1 + sin(i / 3.0-angle));
		}

}

// Set random terrain
void SetupTerrain()
{
	int x1, y1, x2, y2;
	double delta=0.07;
	double a, b; // y = ax+b
	int i, j;

	if (rand() % 2 == 0) delta = -delta;

	x1 = rand() % GSZ;
	y1 = rand() % GSZ;
	x2 = rand() % GSZ;
	y2 = rand() % GSZ;
	// compute a and b
	if (x1 != x2)
	{
		a = (y2 - y1) / ((double)x2 - x1);
		b = y1 - a*x1;
		for (i = 0;i < GSZ;i++)
			for (j = 0; j < GSZ; j++)
			{
				if (i < a * j + b) ground[i][j] += delta;
				else ground[i][j] -= delta;

				// Set river surface
				river[i][j] = ground[i][j] - riverSurfaceDepth;
				riverOriginal[i][j] = river[i][j];

				originalGround[i][j] = ground[i][j];  // Store the original ground heights
				
            
			}
	}
}

// Smooth terrain
void Smooth()
{
	int i, j;

	for (i = 1;i < GSZ - 1;i++)
		for (j = 1;j < GSZ - 1;j++)
			tmpg[i][j] = (ground[i-1][j-1]+ 2*ground[i - 1][j]+ ground[i - 1][j + 1]+
				2*ground[i][j - 1] + 4*ground[i][j] + 2*ground[i][j + 1]+
				ground[i + 1][j - 1] + 2*ground[i+ 1][j] + ground[i + 1][j + 1]) / 16.0;
	// copy back to ground
	for (i = 1;i < GSZ - 1;i++)
		for (j = 1; j < GSZ - 1; j++)
		{
			ground[i][j] = tmpg[i][j];

			//double randomDepth = riverDepth + (rand() % 100 / 100.0) * riverDepth;
			river[i][j] = ground[i][j] - riverSurfaceDepth;
			riverOriginal[i][j] = river[i][j];

			originalGround[i][j] = ground[i][j];  // Store the original ground heights

		}

	
}

// Restore original ground
void RestoreOriginalArea()
{
	for (int i = 0; i < GSZ; i++)
		for (int j = 0; j < GSZ; j++)
		{
			ground[i][j] = originalGround[i][j];
			river[i][j] = riverOriginal[i][j];
		}
}

// Store original ground
void StoreOriginalGround()
{
	for (int i = 0; i < GSZ; i++)
		for (int j = 0; j < GSZ; j++)
			originalGround[i][j] = ground[i][j];
}

// Draw a cilynder
void DrawCilynder(int n)
{
	double alpha, teta = 2 * PI / n;

	for (alpha = 0; alpha <= 2 * PI; alpha += teta)
	{
		glBegin(GL_POLYGON);
		glColor3d((sin(alpha) + 1) / 3, (1 - fabs(sin(alpha))) * 0.8, alpha / (3 * PI));
		glVertex3d(sin(alpha), 1, cos(alpha)); // 1-st vertex
		glColor3d((sin(alpha + teta) + 1) / 3, (1 - fabs(sin(alpha + teta))) * 0.8, (alpha + teta) / (3 * PI));
		glVertex3d(sin(alpha + teta), 1, cos(alpha + teta)); // 2-nd vertex
		glColor3d((sin(alpha + teta) + 1) / 2, (1 - fabs(sin(alpha + teta))), (alpha + teta) / (2 * PI));
		glVertex3d(sin(alpha + teta), 0, cos(alpha + teta)); // 3-d vertex
		glColor3d((sin(alpha) + 1) / 2, 1 - fabs(sin(alpha)), alpha / (2 * PI));
		glVertex3d(sin(alpha), 0, cos(alpha)); // 4-th vertex
		glEnd();
	}
}

/*
* Cilynder with texture
* hrep is number of horizontal repeats of texture
* vrep is number of vertical repeats of texture
*/
void DrawTexturedCilynder(int n, int numTexture, double hrep, double vrep)
{
	double alpha, teta = 2 * PI / n;
	double hpart = hrep / n; // what part of a texture is printed on one face
	int counter;
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, numTexture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	for (alpha = 0, counter = 0; alpha <= 2 * PI; alpha += teta, counter++)
	{
		glColor3d(0.5 + 0.5 * fabs(sin(alpha)), 0.5 + 0.5 * fabs(sin(alpha)), 0.5 + 0.5 * fabs(sin(alpha)));
		glBegin(GL_POLYGON);
		glTexCoord2d(counter * hpart, vrep); glVertex3d(sin(alpha), 1, cos(alpha)); // 1-st vertex
		glTexCoord2d((counter + 1) * hpart, vrep); 		glVertex3d(sin(alpha + teta), 1, cos(alpha + teta)); // 2-nd vertex
		glTexCoord2d((counter + 1) * hpart, 0); 		glVertex3d(sin(alpha + teta), 0, cos(alpha + teta)); // 3-d vertex
		glTexCoord2d(counter * hpart, 0); 		glVertex3d(sin(alpha), 0, cos(alpha)); // 4-th vertex
		glEnd();
	}
	glDisable(GL_TEXTURE_2D);

}

/*
* Cilynder with texture and different radiuses on top and bottom
* hrep is number of horizontal repeats of texture
* vrep_top is number of vertical repeats of texture on top
* vrep_bottom is number of vertical repeats of texture on bottom
*/
void DrawTexturedCilynderWithDifferentRadiuses(int n, double topr, double bottomr, int numTexture, double hrep, double vrep_top, double vrep_bottom)
{
	double alpha, teta = 2 * PI / n;
	double hpart = hrep / n; // what part of a texture is printed on one face
	int counter;
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, numTexture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	for (alpha = 0, counter = 0; alpha <= 2 * PI; alpha += teta, counter++)
	{
		//		glColor3d(0.5 + 0.5 * fabs(sin(alpha)), 0.5 + 0.5 * fabs(sin(alpha)), 0.5 + 0.5 * fabs(sin(alpha)));
		glBegin(GL_POLYGON);
		glTexCoord2d(counter * hpart, vrep_top);	glVertex3d(topr * sin(alpha), 1, topr * cos(alpha)); // 1-st vertex
		glTexCoord2d((counter + 1) * hpart, vrep_top); 		glVertex3d(topr * sin(alpha + teta), 1, topr * cos(alpha + teta)); // 2-nd vertex
		glTexCoord2d((counter + 1) * hpart, vrep_bottom); 		glVertex3d(bottomr * sin(alpha + teta), 0, bottomr * cos(alpha + teta)); // 3-d vertex
		glTexCoord2d(counter * hpart, vrep_bottom); 		glVertex3d(bottomr * sin(alpha), 0, bottomr * cos(alpha)); // 4-th vertex
		glEnd();
	}
	glDisable(GL_TEXTURE_2D);

}

/*
* Cone with texture
* hrep is number of horizontal repeats of texture
* vrep is number of vertical repeats of texture
*/
void DrawTexturedCone(int n, int numTexture, double hrep, double vrep)
{
	double alpha, teta = 2 * PI / n;
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, numTexture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	for (alpha = 0; alpha <= 2 * PI; alpha += teta)
	{
		glColor3d(0.5 + 0.5 * fabs(sin(alpha)), 0.5 + 0.5 * fabs(sin(alpha)), 0.5 + 0.5 * fabs(sin(alpha)));
		glBegin(GL_POLYGON);
		glTexCoord2d(0, 0);	glVertex3d(sin(alpha), 0, cos(alpha)); // 1-st vertex
		glTexCoord2d(hrep / 2, vrep); 		glVertex3d(0, 1, 0); // 2-nd vertex
		glTexCoord2d(hrep, 0); 		glVertex3d(sin(alpha + teta), 0, cos(alpha + teta)); // 3-th vertex
		glEnd();
	}
	glDisable(GL_TEXTURE_2D);

}

// Draw a cilynder with different radiuses on top and bottom
void DrawCilynderWithDifferentRadiuses(int n, double topr, double bottomr)
{
	double alpha, teta = 2 * PI / n;

	for (alpha = 0; alpha <= 2 * PI; alpha += teta)
	{
		glBegin(GL_POLYGON);

		glVertex3d(topr * sin(alpha), 1, topr * cos(alpha)); // 1-st vertex
		glVertex3d(topr * sin(alpha + teta), 1, topr * cos(alpha + teta)); // 2-nd vertex
		glVertex3d(bottomr * sin(alpha + teta), 0, bottomr * cos(alpha + teta)); // 3-d vertex
		glVertex3d(bottomr * sin(alpha), 0, bottomr * cos(alpha)); // 4-th vertex

		glEnd();
	}
}

// Draw a sphere with n slices
void DrawSphere(int n, int slices)
{
	double beta, delta = PI / slices;
	int i;

	for (beta = -PI / 2, i = 0; beta <= PI / 2; beta += delta, i++)
	{
		glPushMatrix();
		glTranslated(0, sin(beta), 0);
		glScaled(1, sin(beta + delta) - sin(beta), 1);
		DrawCilynderWithDifferentRadiuses(n, cos(beta + delta), cos(beta));
		glPopMatrix();
	}
}

// Flatten area around a point
void FlattenArea(int row, int col, int half_size, double elevation)
{
	int i, j;

	for (i = row - half_size;i <= row + half_size;i++)
		for (j = col - half_size; j < col + half_size; j++)
		{
			ground[i][j] = elevation;
			river[i][j] = elevation - riverSurfaceDepth;

		}

}

// Set color of a polygon
void SetColor(double h)
{
	h = fabs(h);
	// rocks
	if(h>5) 	glColor3d(h / 10, h / 10, h/8);
	else 
		if (h > 0.2)  
		{
			glColor3d(0.2 + h / 20, 0.6 - h / 14, 0); //grass
		}
		else // sand
			glColor3d(0.9, 0.8, 0.7);
}

// Set normal to a polygon
void SetNormal(int row, int col)
{
	double nx, ny=1, nz;
	nx = ground[row][col - 1] - ground[row][col];
	nz = ground[row-1][col] - ground[row][col];
	glNormal3d(nx, ny, nz);
}

// Draw a ground surface
void DrawGroundSurdace()
{
	int i,j;

	glColor3d(0, 0, 0.3);

	for(i=1;i<GSZ;i++)
		for (j = 1;j < GSZ;j++)
		{
			// Draw the actual Ground
			glBegin(GL_POLYGON);
			SetColor(ground[i][j]);

			glVertex3d(j - GSZ / 2, ground[i][j], i - GSZ / 2);
			SetColor(ground[i][j-1]);

			glVertex3d(j - GSZ / 2-1, ground[i][j-1], i - GSZ / 2);
			SetColor(ground[i-1][j-1]);

			glVertex3d(j - GSZ / 2-1, ground[i-1][j-1], i - GSZ / 2-1);
			SetColor(ground[i-1][j]);

			glVertex3d(j - GSZ / 2, ground[i-1][j], i - GSZ / 2-1);
			glEnd();

			
			
		}

	// add water plane
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4d(0, 0.5, 0.8, 0.8); // the 4-th parameter is transparency
	glBegin(GL_POLYGON);
	glVertex3d(-GSZ/2, 0, -GSZ/2);
	glVertex3d(-GSZ / 2, 0, GSZ / 2);
	glVertex3d(GSZ / 2, 0, GSZ / 2);
	glVertex3d(GSZ / 2, 0, -GSZ / 2);
	glEnd();
	glDisable(GL_BLEND);


}

// Draw the river surface under ground surface
void DrawRiverSurface() {
	int i, j;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4d(0, 0, 0.8, 0.8);

	for (i = 3; i < GSZ; i++)
		for (j = 3; j < GSZ; j++)
		{
			
			glBegin(GL_POLYGON);

			glVertex3d(j - GSZ / 2, river[i][j], i - GSZ / 2);
			glVertex3d(j - GSZ / 2 - 1, river[i][j - 1], i - GSZ / 2);
			glVertex3d(j - GSZ / 2 - 1, river[i - 1][j - 1], i - GSZ / 2 - 1);
			glVertex3d(j - GSZ / 2, river[i - 1][j], i - GSZ / 2 - 1);
			
			glEnd();

			

		}
	
	glDisable(GL_BLEND);

}


// Eye motion
void ArrowsButtons(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_LEFT:
		angular_speed+=0.001;
		break;
	case GLUT_KEY_RIGHT:
		angular_speed -= 0.001;
		break;
	case GLUT_KEY_UP:
		speed += 0.01;
		break;

	case GLUT_KEY_DOWN:
		speed -= 0.01;
		break;
	case GLUT_KEY_PAGE_UP:
		eye.y += dy;
		break;
	case GLUT_KEY_PAGE_DOWN:
		eye.y -= dy;
		break;


	}
}

// Initialize raindrops
void InitializeRaindrops()
{
	for (int i = 0; i < numDrops; i++)
	{
		raindrops[i].x = rand() % GSZ - GSZ / 2;  // Random x position on terrain
		raindrops[i].z = rand() % GSZ - GSZ / 2;  // Random z position on terrain
		raindrops[i].y = dropHeight;                      // Fixed height for all raindrops
		raindrops[i].speed = 0.05 + (rand() % 10) / 100.0; // speed is random value between 0.05 and 0.15
	}
}

// Function to find the lowest neighboring point
void FlowDirection(int& dropX, int& dropZ) {
	// Step 1: Check for nearby river in a radius of checkForRiverRadius around the current point
	int riverX = -1, riverZ = -1;  // To store potential river coordinates

	for (int i = -checkForRiverRadius; i <= checkForRiverRadius; i++) {
		for (int j = -checkForRiverRadius; j <= checkForRiverRadius; j++) {
			int nx = dropX + i;
			int nz = dropZ + j;

			// Ensure we're within bounds
			if (nx >= 0 && nx < GSZ && nz >= 0 && nz < GSZ) {
				// Check if the point is part of the river
				if (river[nx][nz] < ground[nx][nz]) {
					riverX = nx;
					riverZ = nz;
					break;
				}
			}
		}
		if (riverX != -1) break;
	}

	// Step 2: If a river is found, search for the lowest ground point on its edge
	if (riverX != -1) {
		double lowestEdgeHeight = 100;
		int lowestEdgeX = -1, lowestEdgeZ = -1;

		// Search in a small radius around the river point
		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				int nx = riverX + i;
				int nz = riverZ + j;

				// Ensure we're within bounds and not the river point itself
				if (nx >= 0 && nx < GSZ && nz >= 0 && nz < GSZ && (nx != riverX || nz != riverZ)) {
					// Check if it's a ground point on the river edge
					if (ground[nx][nz] > river[nx][nz] && ground[nx][nz] < lowestEdgeHeight) {
						lowestEdgeHeight = ground[nx][nz];
						lowestEdgeX = nx;
						lowestEdgeZ = nz;
					}
				}
			}
		}

		// If a valid edge point is found, move the drop there
		if (lowestEdgeX != -1 && lowestEdgeZ != -1) {
			dropX = lowestEdgeX;
			dropZ = lowestEdgeZ;
		}
		return; // Exit since we found the edge
	}

	// Step 3: If no river is found, move the raindrop to the lowest neighboring point (same as original code)

	// Get the current height of the drop position
	double currentHeight = ground[dropX][dropZ];

	// Check the neighboring positions (north, south, east, west)
	int lowestX = dropX, lowestZ = dropZ;
	double lowestHeight = currentHeight;

	// Check north
	if (dropZ + 1 < GSZ && ground[dropX][dropZ + 1] < lowestHeight)
	{
		lowestX = dropX;
		lowestZ = dropZ + 1;
		lowestHeight = ground[lowestX][lowestZ];
	}

	// Check south
	if (dropZ - 1 >= 0 && ground[dropX][dropZ - 1] < lowestHeight)
	{
		lowestX = dropX;
		lowestZ = dropZ - 1;
		lowestHeight = ground[lowestX][lowestZ];
	}

	// Check east
	if (dropX + 1 < GSZ && ground[dropX + 1][dropZ] < lowestHeight)
	{
		lowestX = dropX + 1;
		lowestZ = dropZ;
		lowestHeight = ground[lowestX][lowestZ];
	}

	// Check west
	if (dropX - 1 >= 0 && ground[dropX - 1][dropZ] < lowestHeight)
	{
		lowestX = dropX - 1;
		lowestZ = dropZ;
		lowestHeight = ground[lowestX][lowestZ];
	}

	// Move the raindrop to the lowest neighboring point if no river is found
	dropX = lowestX;
	dropZ = lowestZ;
}


/*
void FlowDirection(int& dropX, int& dropZ)
{
	// Step 1: Check for nearby river in a radius of checkForRiverRadius around the current point
	int riverX = -1, riverZ = -1;  // To store potential river 

	for (int i = -checkForRiverRadius; i <= checkForRiverRadius; i++)
	{
		for (int j = -checkForRiverRadius; j <= checkForRiverRadius; j++)
		{
			int nx = dropX + i;
			int nz = dropZ + j;

			// Ensure we're within bounds
			if (nx >= 0 && nx < GSZ && nz >= 0 && nz < GSZ)
			{
				// Check if the point is a river surface (underground surface below the ground)
				if (river[nx][nz] < ground[nx][nz])
				{
					riverX = nx;
					riverZ = nz;
					break; // Break out once we find the river
				}
			}
		}
		if (riverX != -1) // Break the outer loop as well if river is found
		{
			break;
		}
	}

	// Step 2: If a river is found, move the raindrop to the river point
	if (riverX != -1)
	{
		dropX = riverX;
		dropZ = riverZ;
		return;  // Exit early since we've already found the river
	}

	// Step 3: If no river is found, move the raindrop to the lowest neighboring point (same as original code)

	// Get the current height of the drop position
	double currentHeight = ground[dropX][dropZ];

	// Check the neighboring positions (north, south, east, west)
	int lowestX = dropX, lowestZ = dropZ;
	double lowestHeight = currentHeight;

	// Check north
	if (dropZ + 1 < GSZ && ground[dropX][dropZ + 1] < lowestHeight)
	{
		lowestX = dropX;
		lowestZ = dropZ + 1;
		lowestHeight = ground[lowestX][lowestZ];
	}

	// Check south
	if (dropZ - 1 >= 0 && ground[dropX][dropZ - 1] < lowestHeight)
	{
		lowestX = dropX;
		lowestZ = dropZ - 1;
		lowestHeight = ground[lowestX][lowestZ];
	}

	// Check east
	if (dropX + 1 < GSZ && ground[dropX + 1][dropZ] < lowestHeight)
	{
		lowestX = dropX + 1;
		lowestZ = dropZ;
		lowestHeight = ground[lowestX][lowestZ];
	}

	// Check west
	if (dropX - 1 >= 0 && ground[dropX - 1][dropZ] < lowestHeight)
	{
		lowestX = dropX - 1;
		lowestZ = dropZ;
		lowestHeight = ground[lowestX][lowestZ];
	}

	// Move the raindrop to the lowest neighboring point if no river is found
	dropX = lowestX;
	dropZ = lowestZ;
}
*/

// Function to erode the terrain around a raindrop
void ErodeTerrain(int dropX, int dropZ)
{
	// check if the drop is on the edge of the map, if so - dont erode 
	if (dropX < 5 || dropX >= GSZ - 5 || dropZ < 5 || dropZ >= GSZ - 5)
	{
		return;
	}

	// Loop through the surrounding area in a square radius
	for (int i = -erosionRadius; i <= erosionRadius; i++)
	{
		double erosion = dropGroundErosion;


		// Check if it is a river point and not at the edges of the map
		if (ground[dropX][dropZ] < river[dropX][dropZ]) {
			erosion = dropGroundErosion * 3; // Erode more if it is a river point
		}

		// Loop through the surrounding area in a square radius
		int nx, nz;
		for (int j = -erosionRadius; j <= erosionRadius; j++)
		{
			nx = dropX + i;
			nz = dropZ + j;

			// Ensure that we're still within bounds of the terrain array
			if (nx >= 0 && nx < GSZ && nz >= 0 && nz < GSZ)
			{
				// Calculate distance from the drop center to create more realistic erosion (optional)
				double distance = sqrt(i * i + j * j);

				// Erode terrain more at the center and less toward the edges
				double erosionAmount = erosion * (1.0 - (distance / (erosionRadius + 1)));

				// Apply erosion to the terrain
				ground[nx][nz] -= erosionAmount;
			}
		}
	}
		/*
		*/
}


// Rain simulation with flow algorithm
void RainSimulation()
{
	for (int i = 0; i < numDrops; i++)
	{
		// Update the y position of each raindrop (falling down)
		raindrops[i].y -= raindrops[i].speed;

		// Get the x and z position of the raindrop
		int dropX = raindrops[i].x + GSZ / 2;
		int dropZ = raindrops[i].z + GSZ / 2;
		

		// if raindrop hits the ground level
		if (raindrops[i].y <= ground[dropX][dropZ]) 
		{
			// Flow the raindrop to the lowest neighboring point
			FlowDirection(dropX, dropZ);

			// Check if the new position is still land and not water
			if (ground[dropX][dropZ] > 0)
			{
				// Erode the terrain and surrounding area
				ErodeTerrain(dropX, dropZ);
			}

			// Reset raindrop to fall again from the top
			raindrops[i].y = 10;  // Fixed starting height
			raindrops[i].x = rand() % GSZ - GSZ / 2;  // Random x position on terrain
			raindrops[i].z = rand() % GSZ - GSZ / 2;  // Random z position on terrain
		}
		

		// Draw the raindrop
		glPushMatrix();
		glColor3d(0, 0, 0.8);  // Blue color for raindrop
		glTranslated(raindrops[i].x, raindrops[i].y, raindrops[i].z); // Position of the raindrop
		glScaled(0.05, 0.05, 0.05);  // Scale the raindrop to make it smaller
		DrawSphere(10, 10);  // Draw the raindrop as a small sphere
		glPopMatrix();
	}
}


// Draw Castle

void DrawCastle() {
	int numSteeples = 5;          // Number of steeples in the circle
	double castleRadius = flattenSize / 2 - 1.3;          // Radius of the circular layout
	double steepleHeight = 4.0;    // Height of each steeple
	double fenceHeight = 4.0;      // Height of the fence between steeples
	double steepleRadius = 0.7;    // Radius of the steeple base
	int cylinderSegments = 20;     // Number of segments for each cylindrical steeple

	
	

	// Draw steeples in a circle
	for (int i = 0; i < numSteeples; i++) {
		double angle = i * 2 * PI / numSteeples;
		double nextAngle = (i + 1) * 2 * PI / numSteeples;

		// Position of current and next steeple
		double x1 = castleRadius * cos(angle);
		double z1 = castleRadius * sin(angle);
		double x2 = castleRadius * cos(nextAngle);
		double z2 = castleRadius * sin(nextAngle);

		// Draw the steeple as a cylinder with a cone on top
		glPushMatrix();
		glTranslated(x1, 0, z1);           // Position steeple
		glRotated(-angle * 180 / PI, 0, 1, 0); // Rotate steeple towards center
		glScaled(steepleRadius, steepleHeight, steepleRadius); // Scale steeple to size

		DrawTexturedCilynder(cylinderSegments, 6, 1, 1); // Textured cylinder for steeple base

		glPopMatrix();

		glPushMatrix();
		glTranslated(x1, 0, z1);           // Position steeple
		glRotated(-angle * 180 / PI, 0, 1, 0); // Rotate steeple towards center
		glTranslated(0, steepleHeight, 0);  // Move up for cone
		glScaled(steepleRadius, 1, steepleRadius); // Scale cone to size
		DrawTexturedCone(cylinderSegments, 7, 1, 1); // Textured cone on top of steeple
		glPopMatrix();


		// Draw fence segment between this steeple and the next
		
		// Enable textures
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 10);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glPushMatrix();
		glBegin(GL_QUADS);
		glTexCoord2d(0, 0); glVertex3d(x1, 0, z1);
		glTexCoord2d(1, 0); glVertex3d(x2, 0, z2);
		glTexCoord2d(1, 1); glVertex3d(x2, fenceHeight, z2);
		glTexCoord2d(0, 1); glVertex3d(x1, fenceHeight, z1);
		glEnd();
		glPopMatrix();
		 
		glDisable(GL_TEXTURE_2D);


	}

}


// Flatten an area and check surroundings for sea and river
/*
void FindCastlePlace()
{
	// If there is a previously flattened area, restore its original heights
	if (isAreaFlattened)
	{
		for (int i = prevFlattenX - flattenSize / 2; i <= prevFlattenX + flattenSize / 2; i++)
		{
			for (int j = prevFlattenY - flattenSize / 2; j <= prevFlattenY + flattenSize / 2; j++)
			{
				ground[i][j] = originalGround[i][j];  // Restore the original ground heights
			}
		}
		printf("Restored previous area at (%d, %d)\n", prevFlattenX, prevFlattenY);
	}

	// Iterate through the terrain to find a suitable area
	for (int i = searchRadius; i < GSZ - flattenSize - searchRadius; i++)
	{
		for (int j = searchRadius; j < GSZ - flattenSize - searchRadius; j++)
		{
			bool nearRiver = false;
			bool nearSea = false;

			// Check the surroundings (not inside the area) for river and sea
			for (int x = i - searchRadius; x <= i + flattenSize + searchRadius; x++)
			{
				for (int y = j - searchRadius; y <= j + flattenSize + searchRadius; y++)
				{
					// Skip checking inside the 10x10 area itself
					if (x >= i && x < i + flattenSize && y >= j && y < j + flattenSize)
						continue;

					// Check if there's a river nearby
					if (river[x][y] > ground[x][y])
					{
						nearRiver = true;
					}

					// Check if there's a sea nearby
					if (ground[x][y] < 0)
					{
						nearSea = true;
					}

					// If both river and sea are found, break early
					if (nearRiver && nearSea)
					{
						break;
					}
				}
				if (nearRiver && nearSea)
				{
					break;
				}
			}

			// If the area is next to both river and sea
			if (nearRiver && nearSea)
			{
				// Store the original heights before flattening
				for (int x = i; x < i + flattenSize; x++)
				{
					for (int y = j; y < j + flattenSize; y++)
					{
						originalGround[x][y] = ground[x][y];
					}
				}

				// Flatten the area
				FlattenArea(i + flattenSize / 2, j + flattenSize / 2, flattenSize / 2);
				printf("Castle place found and flattened at (%d, %d)\n", i, j);

				// Track the newly flattened area
				isAreaFlattened = true;
				prevFlattenX = i + flattenSize / 2;
				prevFlattenY = j + flattenSize / 2;

				return;  // Exit once a place is found
			}
		}
	}

	// If no place is found
	printf("No place found\n");
}
*/
/*
void FindCastlePlace()
{
	// start from the previous flattened area
	int x = prevFlattenX, y = prevFlattenY + 1;

	// if there is a previously flattened area, restore its original heights
	if (y != 0)
	{
		for (int i = prevFlattenX - flattenSize / 2; i < prevFlattenX + flattenSize / 2; i++)
			for (int j = prevFlattenY - flattenSize / 2; j < prevFlattenY + flattenSize / 2; j++)
				ground[i][j] = originalGround[i][j];

		printf("Restored previous area at (%d, %d)\n", prevFlattenX, prevFlattenY);
	}

	// iterate through the terrain to find a suitable area
	for (x; x < GSZ - flattenSize - 2 * searchRadius; x++)
		for (y; y < GSZ - flattenSize - 2 * searchRadius; y++)
		{
			// check if the inside area is only ground
			bool isGround = true;
			for (int i = x + searchRadius; i < x + flattenSize + searchRadius; i++)
				for (int j = y + searchRadius; j < y + flattenSize + searchRadius; j++)
					if (ground[i][j] < 0 || ground[i][j] < river[i][j])
					{
						isGround = false;
						break;
					}
			
			if (!isGround) continue;

			// check the surroundings for river and sea
			bool nearRiver = false;
			bool nearSea = false;

			for (int i = x; i <= x + flattenSize + 2 * searchRadius; i++)
				for (int j = y; j <= y + flattenSize + 2 * searchRadius; j++)
				{
					// skip checking inside the 10x10 area itself
					if (i >= x + searchRadius && i < x + flattenSize && j >= y + searchRadius && j < y + flattenSize)
						continue;

					// check if there's a river nearby
					if (river[i][j] > ground[i][j])
						nearRiver = true;

					// check if there's a sea nearby
					if (ground[i][j] < 0)
						nearSea = true;

					// if both river and sea are found, break early
					if (nearRiver && nearSea)
						break;
				}

			// if the area is next to both river and sea
			if (nearRiver && nearSea)
			{
				// store the original heights before flattening for restoration
				for (int i = x + searchRadius; i < x + flattenSize + searchRadius; i++)
					for (int j = y + searchRadius; j < y + flattenSize + searchRadius; j++)
						originalGround[i][j] = ground[i][j];

				// flatten the area
				//FlattenArea(x + searchRadius + flattenSize/2, y + searchRadius + flattenSize / 2, flattenSize / 2);
				// Draw a sphere to visualize the flattened area
				glPushMatrix();
				glTranslated(x + searchRadius + flattenSize / 2 - GSZ / 2, ground[x + searchRadius + flattenSize / 2][y + searchRadius + flattenSize / 2], y + searchRadius + flattenSize / 2 - GSZ / 2);
				glScaled(flattenSize / 2, flattenSize / 2, flattenSize / 2);
				DrawSphere(10, 10);
				glPopMatrix();

				printf("Castle place found and flattened at (%d, %d)\n", x, y);

				// track the newly flattened area
				isAreaFlattened = true;
				prevFlattenX = x;
				prevFlattenY = y;

				return;  // exit once a place is found
			}


		}

	// if no place is found
	printf("No place found\n");
	prevFlattenX = 0;
	prevFlattenY = -1;	
	isAreaFlattened = false;


}
*/
void FindCastlePlace()
{
	int x = searchRadius, z = searchRadius;

	// If there is a previously flattened area, restore its original heights
	if (isAreaFlattened)
	{
		RestoreOriginalArea();

		printf("Restored previous area at (%d, %d)\n", prevFlattenX, prevFlattenZ);
		isAreaFlattened = false;  // Reset the flag

		// Start the search after the previous flattened area
		x = prevFlattenX + flattenSize;
		z = prevFlattenZ + flattenSize;
	}

	printf("Starting search from (%d, %d)\n", x, z);

	// Iterate through the terrain to find a suitable area
	bool areaFound = false;
	for (; x < GSZ - flattenSize - searchRadius; x++)
	{
		for (; z < GSZ - flattenSize - searchRadius; z++)
		{
			// Check if the inside area is only ground
			bool isGround = true;
			for (int i = x; i < x + flattenSize; i++)
			{
				for (int j = z; j < z + flattenSize; j++)
				{
					// Ensure that the area is not sea or river
					if (ground[i][j] < 0 || river[i][j] > ground[i][j])
					{
						isGround = false;
						break;
					}
				}
				if (!isGround) break;
			}

			if (!isGround) continue;  // If not ground, move to the next area

			// Check the surroundings for river and sea
			bool nearRiver = false, nearSea = false;
			for (int i = x - searchRadius; i <= x + flattenSize + searchRadius; i++)
			{
				for (int j = z - searchRadius; j <= z + flattenSize + searchRadius; j++)
				{
					// Skip checking inside the 10x10 area itself
					if (i >= x && i < x + flattenSize && j >= z && j < z + flattenSize)
						continue;

					// Check if there's a river nearby
					if (river[i][j] > ground[i][j])
						nearRiver = true;

					// Check if there's a sea nearby
					if (ground[i][j] < 0)
						nearSea = true;

					// If both river and sea are found, break early
					if (nearRiver && nearSea)
						break;
				}
				if (nearRiver && nearSea) break;
			}

			// If the area is next to both river and sea
			if (nearRiver && nearSea)
			{
				
				// find the lowest point around the area
				double lowestPoint = 1000;
				for (int i = x; i < x + flattenSize; i++)
					for (int j = z ; j < z + flattenSize ; j++)
						if (ground[i][j] < lowestPoint)
							lowestPoint = ground[i][j];


				/*
				// find the highest point around the area
				double highestPoint = -1;
				for (int i = x - searchRadius; i < x + flattenSize + searchRadius; i++)
					for (int j = z - searchRadius; j < z + flattenSize + searchRadius; j++)
						if (ground[i][j] > highestPoint)
							highestPoint = ground[i][j];
				*/
					

				// Flatten the area
				FlattenArea(x + flattenSize / 2, z + flattenSize / 2, flattenSize / 2, lowestPoint);

				printf("Castle place found and flattened at (%d, %d)\n", x, z);

				// Track the newly flattened area
				isAreaFlattened = true;
				prevFlattenX = x;
				prevFlattenZ = z;
				prevFlattenHeight = lowestPoint;

				areaFound = true;
				return;  // Exit once a place is found
			}
		}
		// Reset z for the next x iteration
		z = searchRadius;
	}

	if (!areaFound)
	{
		printf("No place found\n");
		isAreaFlattened = false;
	}
}





// Right click menu
void menu(int value)
{
	switch (value)
	{
	case 1: // start rain
		printf("Start rain\n");
		isRaining = true;
		break;
	case 2: // stop rain
		printf("Stop rain\n");
		isRaining = false;
		StoreOriginalGround(); // Store original ground before searching for castle place
		break;
	case 3: // smooth
		Smooth();
		break;
	case 4: // find fortress place
		printf("Find fortress place\n");
		FindCastlePlace();
		
		break;
	}
}


// OpenGL Functions 

void init()
{
	srand(time(0));

	glClearColor(0.7, 0.9, 1, 0);// color of window background
	glEnable(GL_DEPTH_TEST); // 3D
	glEnable(GL_NORMALIZE); // lighting normalization


	int i, j;
	for (i = 0; i < 1500; i++)
		SetupTerrain();
	for (i = 0; i < 10; i++)
		SetupTerrain();

	InitializeRaindrops();        // Initialize the raindrops

	// initialize the texture
	InitTextures();




}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clean frame and Z-buffer (fills memory with background color)
	glViewport(0, 0, W, H);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity(); // this starts all transformations to identity
	glFrustum(-1, 1, -1, 1, 0.7, 300); // camera definitions

	gluLookAt(eye.x, eye.y, eye.z, // point of projection
		eye.x + dir.x, eye.y - 0.6, eye.z + dir.z, // look to the sight angle direction
		0, 1, 0); // "Up - direction"

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // this starts all transformations to identity


	DrawGroundSurdace();
	DrawRiverSurface();

	if (isRaining)
	{
		// Simulate rain
		RainSimulation();
	}

	

	// Draw the castle
	if (isAreaFlattened)
	{
		glPushMatrix();
		glTranslated(prevFlattenZ - (GSZ / 2) + (flattenSize / 2),
			prevFlattenHeight, 
			prevFlattenX - (GSZ / 2) + (flattenSize / 2));
		DrawCastle();
		glPopMatrix();

	}


	glutSwapBuffers(); // show all
}

void idle()
{
	angle += 0.5;
	//	SetSurface();
		// ego-motion
	sight_angle += angular_speed;
	dir.x = sin(sight_angle);
	dir.z = cos(sight_angle);

	eye.x += speed * dir.x;
	eye.z += speed * dir.z;


	glutPostRedisplay(); //indirect call to display
}

void main(int argc, char* argv[]) 
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE|GLUT_DEPTH); // memory defs: color per pixel RGB, two buffers
	glutInitWindowSize(W, H);
	glutInitWindowPosition(400, 100);
	glutCreateWindow("3D Example");

	glutDisplayFunc(display); // refresh function is "display"
	glutIdleFunc(idle); // any changes put in idle


	// Right click menu
	glutCreateMenu(menu);
	glutAddMenuEntry("Start Rain", 1);
	glutAddMenuEntry("Stop Rain", 2);
	glutAddMenuEntry("Smooth", 3);
	glutAddMenuEntry("Find fortress place", 4);
	glutAttachMenu(GLUT_RIGHT_BUTTON);




	glutSpecialFunc(ArrowsButtons);
	init();

	glutMainLoop();
}