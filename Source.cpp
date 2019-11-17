#ifdef __APPLE__
// DEVELOPED ON WINDOWS -- THESE INCLUDES ARENT TESTED
#include <GL/glut.h>
#include <OpenGL/gl.h>
#include <stdlib.h>
#include <math.h> 
#else
//#include <GL/gl.h>
#include <stdlib.h>
#include <GL/glut.h>
#define _USE_MATH_DEFINES
#include <math.h>
#endif
#include <stdio.h>
#include <iostream>


static GLenum spinMode = GL_TRUE;
static GLenum singleStep = GL_FALSE;

//TIMEFRAMES IN ORDER OF PLANETS
static double ElapsedHours = 0.0;
static double ElapsedDays = 0.0;
static double mercuryHourOfDay = 0.0; static double mercuryDayOfYear = 0.0; //Mercury
static double venusHourOfDay = 0.0; static double venusDayOfYear = 0.0; //Venus
static double earthHourOfDay = 0.0; static double earthDayOfYear = 0.0; //Earth
static double marsHourOfDay = 0.0; static double marsDayOfYear = 0.0; //Mars
static double jupiterHourOfDay = 0.0; static double jupiterDayOfYear = 0.0; //Jupiter
static double saturnHourOfDay = 0.0; static double saturnDayOfYear = 0.0; //Saturn
static double uranusHourOfDay = 0.0; static double uranusDayOfYear = 0.0; //Uranus
static double neptuneHourOfDay = 0.0; static double neptuneDayOfYear = 0.0; //Neptune
static double AnimateIncrement = 2.0;

//VARIABLES FOR ROCKET PHYSICS
double posX = 10.0f;
double velX = 0.0f;
double posY = 10.0f;
double velY = 0.0f;
double posZ = 50.0f;
double velZ = -0.08f;
double maxAcceleration = 3.0f;

//VARIABLE FOR SUN TO FLUX COLOR
GLdouble sunG = 0.9f;

//VARIABLES FOR TEXTURE
unsigned int bmpWidth, bmpHeight, bmpImageSize;
unsigned char* bmpData;
unsigned char* visibleBmpData;

GLuint loadBitmapData(const char * imagepath) {
	// DATA READ FROM BMP FILE HEADER
	unsigned char header[54];
	unsigned int dataPos; 

	// OPEN FILE
	FILE * file = fopen(imagepath, "rb");
	if (!file) { printf("Image could not be opened\n"); return 0; }

	if (fread(header, 1, 54, file) != 54) {
		printf("Not a correct BMP file\n");
		return false;
	}
	if (header[0] != 'B' || header[1] != 'M') {
		printf("Not a correct BMP file\n");
		return 0;
	}

	// READ INTS FROM BYTE ARRAY
	dataPos = *(int*)&(header[0x0A]);
	bmpImageSize = *(int*)&(header[0x22]);
	bmpWidth = *(int*)&(header[0x12]);
	bmpHeight = *(int*)&(header[0x16]);

	// GUESS MISSING INFORMATION ON MISFORMATTED BMPS
	if (bmpImageSize == 0)    bmpImageSize = bmpWidth*bmpHeight * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54;

	// CREATE BUFFER
	bmpData = new unsigned char[bmpImageSize];
	visibleBmpData = new unsigned char[bmpImageSize];

	// READ DATA FROM FILE INTO BUFFER
	fread(bmpData, 1, bmpImageSize, file);
	fclose(file);

	// un-BGR
	for (int i = 0; i < bmpWidth*bmpHeight; i++) {
		unsigned char blue = bmpData[3 * i + 0];
		unsigned char green = bmpData[3 * i + 1];   // not really needed, green is in correct position
		unsigned char red = bmpData[3 * i + 2];
		bmpData[3 * i + 0] = red;
		bmpData[3 * i + 1] = green;
		bmpData[3 * i + 2] = blue;
	}
}

void Set2DTextureFromOffsetBitmap(double offset) {// expects image to be a multiple of 4 in width so no "bmp" format end-of-row padding occurs and width/2 works nicely, other bitmap loading functions probably won't work either if not true   !!
	int offsetCols = int(offset * double (bmpWidth));
	if (offsetCols >= bmpWidth)  // need to wrap to zero if offset = 1.0, protect against rounding errors causing > 1 as well
		offsetCols = 0;
	unsigned int width = bmpWidth/2;  // only half the bitmap can be seen on the visible (textured) side of a sphere
	unsigned int height = bmpHeight;

	unsigned char* data = visibleBmpData;
	unsigned int colOffset = offsetCols * 3;
	for (int row = 0; row < height; row++) {
		unsigned int rowOffset = row * bmpWidth * 3;
		for (int colByte = 0; colByte < 3*width; colByte++) {
			*data = bmpData[rowOffset + colOffset + colByte];
			data++;
		}
	}
	// GIVE IMAGE TO OPENGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, visibleBmpData);
}

static void Animate(void)
{
	//ADD AMBIENT LIGHT
	GLfloat ambientColor[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

	// CLEAR RENDERING WINDOW
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (spinMode) {
		// UPDATE ANIMATION STATE IN ORDER OF PLANETS
		// LOGIC DERIVED FROM ACTUAL DAY AND YEAR LENGTH PER PLANET
		mercuryHourOfDay = fmod(ElapsedHours,58); mercuryDayOfYear = fmod(ElapsedDays,88); //Mercury
		venusHourOfDay = fmod(ElapsedHours, 243); venusDayOfYear = fmod(ElapsedDays, 225); //Venus
		earthHourOfDay = fmod(ElapsedHours, 24); earthDayOfYear = fmod(ElapsedDays, 365); //Earth
		marsHourOfDay = fmod(ElapsedHours, 25); marsDayOfYear = fmod(ElapsedDays, 687); //Mrs
		jupiterHourOfDay = fmod(ElapsedHours, 10); jupiterDayOfYear = fmod(ElapsedDays, 4307); //Jupiter
		saturnHourOfDay = fmod(ElapsedHours, 11); saturnDayOfYear = fmod(ElapsedDays, 10759); //Saturn
		uranusHourOfDay = fmod(ElapsedHours, 17); uranusDayOfYear = fmod(ElapsedDays, 30660); //Uranus
		neptuneHourOfDay = fmod(ElapsedHours, 18); neptuneDayOfYear = fmod(ElapsedDays, 59860); //Neptune
	}

	// CLEAR CURRENT MODELVIEW
	glLoadIdentity();
	glTranslated(0.0, -10.0, -90.0); 	// Back off to be able to view from the origin
	glRotated(15.0, 1.0, 0.0, 0.0); // Rotate the plane of the elliptic (rotate the model's plane about the x axis by fifteen degrees)
	glPushMatrix();

	//CREATE SUN WHICH IS ALSO A POSITIONED LIGHT
	GLfloat lightColor0[] = { 1.5f, 1.5f, 1.0f, 1.0f };
	GLfloat lightPos0[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	
	glPushMatrix();
	glRotated(360.0*ElapsedDays / 1000.0, 0.0, 1.0, 0.0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);;
	glTranslated(0.0, 0.0, 0.0);
	glScaled(3.5, 3.5, 3.5);
	glDisable(GL_LIGHTING);
	glColor3d(1.0f, sunG, 0.0f);
	glutWireSphere(1, 15, 15);
	
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
	glEnable(GL_LIGHTING);
	
	glPopMatrix(); glPushMatrix();

	// DRAW MERCURY
	// FIRST POSITION AROUND SUN,   USE ElapsedDays TO DETERMINE POSITION
	glRotated(360.0*mercuryDayOfYear / 88.0, 0.0, 1.0, 0.0);
	glTranslated(6.0, -0.3, 0.0);
	glScaled(1.2, 1.2, 1.2);
	// SECOND, ROTATE ON ITS AXIS,  USE ElapsedHours TO DETERMINE ROATATION
	glRotated(360.0*mercuryHourOfDay / 58.0, 0.0, 1.0, 0.0);
	// THIRD, COLOR AND DRAW PLANET AS WIREFRAME
	glColor3d(0.7, 0.6, 0.5); glutSolidSphere(1, 15, 15);

	glPopMatrix(); glPushMatrix();

	//DRAW VENUS
	glRotated(360.0*venusDayOfYear / 225.0, 0.0, 1.0, 0.0);
	glTranslated(10.0, 0.01, 0.0);
	glScaled(1.4, 1.4, 1.4);
	glRotated(360.0*venusHourOfDay / 243.0, 0.0, 1.0, 0.0);
	glColor3d(0.8, 0.5, 0.4); glutSolidSphere(1, 15, 15);

	glPopMatrix(); glPushMatrix();

	// DRAW AND TEXTURE EARTH WITH BMAP TO LOOK LIKE EARTH
	Set2DTextureFromOffsetBitmap(earthHourOfDay / 24.0);
	glEnable(GL_TEXTURE_2D);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glRotated(360.0*earthDayOfYear / 365.0, 0.0, 1.0, 0.0);
	glTranslated(14.0, -0.05, 0.0);
	glScaled(1.5, 1.5, 1.5);
	glRotated(360.0*earthHourOfDay / 24.0, 0.0, 1.0, 0.0);
	glColor3d(1, 1, 1);
	glutSolidSphere(1, 15, 15);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	// DRAW MOON,  USE DayOfYear TO CONTROL ORBIT
	glRotated(360.0*12.0*earthDayOfYear / 365.0, 0.0, 1.0, 0.0);
	glTranslated(1.9, 0.04, 0.0);
	glColor3d(0.7, 0.7, 0.7); glutWireSphere(0.3, 5, 5);

	glPopMatrix(); glPushMatrix();

	//DRAW MARS
	glRotated(360.0*marsDayOfYear / 687.0, 0.0, 1.0, 0.0);
	glTranslated(18.0, -0.2, 0.0);
	glScaled(1.3, 1.3, 1.3);
	glRotated(360.*marsHourOfDay / 24.4, 0.0, 1.0, 0.0);
	glColor3d(1.0, 0.2, 0.2); glutWireSphere(1, 15, 15);

	glPopMatrix(); glPushMatrix();

	//DRAW JUPITER
	glRotated(360.0*jupiterDayOfYear / 4307.0, 0.0, 1.0, 0.0);
	glTranslated(26.0, -0.03, 0.0);
	glScaled(2.2, 2.2, 2.2);
	glRotated(360.*jupiterHourOfDay / 10.0, 0.0, 1.0, 0.0);
	glColor3d(0.8, 0.6, 0.4); glutWireSphere(1, 15, 15);

	glPopMatrix(); glPushMatrix();

	//DRAW SATURN WITH NO RING
	glRotated(360.0*saturnDayOfYear / 10759.0, 0.0, 1.0, 0.0);
	glTranslated(34.0, 0.1, 0.0);
	glScaled(1.7, 1.7, 1.7);
	glRotated(360.*saturnHourOfDay / 10.45, 0.0, 1.0, 0.0);
	glColor3d(0.8, 0.6, 0.8); glutWireSphere(1, 15, 15);
	
	glPopMatrix(); glPushMatrix();

	//DRAW URANUS
	glRotated(360.0*uranusDayOfYear / 30660.0, 0.0, 1.0, 0.0);
	glTranslated(42.0, 0.02, 0.0);
	glScaled(1.6, 1.6, 1.6);
	glRotated(360.*uranusHourOfDay / 17.0, 0.0, 1.0, 0.0);
	glColor3d(0.8, 0.8, 1.0); glutWireSphere(1, 15, 15);

	glPopMatrix(); glPushMatrix();

	//DRAW NEPTUNE
	glRotated(360.0*neptuneDayOfYear / 59860.0, 0.0, 1.0, 0.0);
	glTranslated(50.0, -0.1, 0.0);
	glScaled(1.5, 1.5, 1.5);
	glRotated(360.*neptuneHourOfDay / 18.0, 0.0, 1.0, 0.0);
	glColor3d(0.7, 0.7, 1.0); glutWireSphere(1, 15, 15);

	glPopMatrix(); glPushMatrix();

	//DRAW BAD ROCKETY THING
	double distanceFromSun = sqrt((posX*posX) + (posY*posY) + (posZ*posZ));
	double acceleration = 0;
	if (distanceFromSun != 0) 
		acceleration = 1 / (distanceFromSun*distanceFromSun);
	if (acceleration > maxAcceleration)
		acceleration = maxAcceleration;
	
	double sunStrength = -0.2;
	double accelZ = 0;
	if (distanceFromSun != 0) {
		accelZ = (sunStrength)*(posZ/distanceFromSun)*acceleration;
	}
	velZ += accelZ;
	posZ += velZ;
	double accelX = 0;
	if (distanceFromSun != 0) {
		accelX = (sunStrength)*(posX / distanceFromSun)*acceleration;
	}
	velX += accelX;
	posX += velX;
	double accelY = 0;
	if (distanceFromSun != 0) {
		accelY = (sunStrength)*(posY / distanceFromSun)*acceleration;
	}
	velY += accelY;
	posY += velY;


	glTranslated(posX, posY, posZ);  // y = -10 to match sun
	glColor3d(0.9, 0.9, 0.9);
	glutWireCone(1.0, -10, 10, 10);

	glPopMatrix(); glPushMatrix();

	//TEXT INSTRUCTIONS
	glDisable(GL_LIGHTING);
	glColor3d(1.0, 1.0, 1.0);
	glRasterPos2i(-43, 54);
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'T');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'o');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ' ');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 's');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 't');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'o');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'p');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ' ');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'p');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'l');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'a');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'n');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'e');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 't');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 's');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ' ');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'a');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'n');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'i');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'm');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'a');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 't');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'i');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'o');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'n');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ' ');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'r');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'i');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'g');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'h');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 't');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ' ');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'c');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'l');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'i');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'c');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'k');
	glColor3d(1.0, 1.0, 1.0);
	glRasterPos2i(-43, 52);
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'T');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'o');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ' ');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 's');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 't');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'a');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'r');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 't');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ' ');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'p');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'l');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'a');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'n');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'e');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 't');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 's');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ' ');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'a');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'n');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'i');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'm');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'a');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 't');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'i');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'o');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'n');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ' ');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'l');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'e');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'f');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 't');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ' ');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'c');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'l');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'i');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'c');
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'k');
	glEnable(GL_LIGHTING);

	//STARS IN BG
	glDisable(GL_LIGHTING);
	glEnable(GL_LIGHTING);
	
	// FLUSH PIPELINE AND SWAP BUFFERS
	glFlush();
	glutSwapBuffers();
	if (singleStep) {
		spinMode = GL_FALSE;
	}
	// REQUEST REDRAW FOR ANIMATION PURPOSES
	glutPostRedisplay();
}

// INITIALIZE OPENGL RENDERING MODES
void OpenGLInit(void)
{
	//LIGHTING
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_NORMALIZE);

	// SET REPEAT WRAP FLAGS AND FILTER TO ADJUST IMAGES 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//                                            LOAD BITMAP IMAGE HERE
	loadBitmapData("C:\\Users\\Pootrick\\Documents\\Visual Studio 2015\\Projects\\pRaleyFinal\\earth-24.bmp");

	glShadeModel(GL_SMOOTH);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);
}

// TRANSFORM AND MOUSE FUNCTIONS
void transform() {
	ElapsedHours += AnimateIncrement;
	ElapsedDays += AnimateIncrement / 24.0;

	// COLOR CONTROL FOR SUN, FLUCTUATES BETWEEN YELLOW AND ORANGE
	sunG -= 0.002;
	if (sunG <= 0.7f)
		sunG = 0.9f;

}
void mouseFcn(GLint button, GLint action, GLint x, GLint y) {
	switch (button) {
	case GLUT_LEFT_BUTTON://  Start the animation.
		if (action == GLUT_DOWN)
			glutIdleFunc(transform);
		break;
	case GLUT_RIGHT_BUTTON://  Stop the animation.
		if (action == GLUT_DOWN)
			glutIdleFunc(NULL);
		break;
	default:
		break;
	}
}

// MAIN ROUTINE
// SETUP OPENGL, HOOK UP CALLBACKS, START MAIN LOOP
int main(int argc, char** argv)
{
	// NEED TO DOUBLE BUFFER FOR ANIMATION
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	// CREATE AND POSITION GRAPHICS WINDOW
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(900, 700);
	glutCreateWindow("solarSystem_C++_OpenGL");

	// INITIALIZE OPENGL
	OpenGLInit();

	// TRANSFORM AND MOUSE FUNCTIONS
	glutIdleFunc(transform);
	glutMouseFunc(mouseFcn);

	// SETUP PROJECTION VIEW MATRIX (not very well! haha)
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, 1.0, 1, 1000);

	// SELECT MODELVIEW MATRIX
	glMatrixMode(GL_MODELVIEW);

	// CALLBACK FOR GRAPHICS IMAGE REDRAWING
	glutDisplayFunc(Animate);

	// START THE MAIN LOOP,  glutMainLoop NEVER RETURNS
	glutMainLoop();
	return(0);
}