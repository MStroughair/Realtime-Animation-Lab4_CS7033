
#include <windows.h>
#include "Antons_maths_funcs.h"
#include "teapot.h" // teapot mesh
#include "Utilities.h"
#include "Particle.h"
#include "Collision.h"
#include "Skeleton.h"
#include "Bezier.h"
using namespace std;

const float width = 900, height = 900;
/*----------------------------------------------------------------------------
						MESH AND TEXTURE VARIABLES
----------------------------------------------------------------------------*/

Mesh cubeMapID, cubeID, palmID;
Mesh boneID, torsoID;

/*----------------------------------------------------------------------------
							CAMERA VARIABLES
----------------------------------------------------------------------------*/

vec3 startingPos = { 0.0f, 0.0f, 10.0f };
GLfloat pitCam = 0, yawCam = 0, rolCam = 0, frontCam = 0, sideCam = 0, speed = 1;
float rotateY = 50.0f, rotateLight = 0.0f;
EulerCamera cam(startingPos, 270.0f, 0.0f, 0.0f);

/*----------------------------------------------------------------------------
								SHADER VARIABLES
----------------------------------------------------------------------------*/
GLuint simpleShaderID, noTextureShaderID, cubeMapShaderID;
Shader shaderFactory;
/*----------------------------------------------------------------------------
							OTHER VARIABLES
----------------------------------------------------------------------------*/

const char* atlas_image = "../freemono.png";
const char* atlas_meta = "../freemono.meta";

float fontSize = 25.0f;
int textID = -1;
bool pause = false;
bool mode = false;
Torso skeleton;

vec3 point = vec3(6.0, 5.0, 0.0);
float xaxis = 0, yaxis = 0, zaxis = 0;
float a = 0, b = 0, t = 0;

Bezier curveArray[] = { 
	Bezier(vec3(-15.0, -10.0, 0.0), vec3(-10.0, -15.0, 1.0), vec3(-3.0, 1.8, 1.0), vec3(-3.0, 1.2, 1.0)), 
	Bezier(vec3(-10.0, -15.0, 0.0), vec3(-15.0, -10.0, 1.0), vec3(-3.0, 1.8, 1.0), vec3(-3.0, 1.2, 1.0)),
	Bezier(vec3(-15.0, -10.0, 0.0), vec3(-15.0, 10.0, 6.0), vec3(-6.0, 10.8, 1.0), vec3(-3.0, 1.2, 1.0))
};

bool backwards = false;
int currCurve = 0;
int NUM_CURVES = 3;
/*----------------------------------------------------------------------------
						FUNCTION DEFINITIONS
----------------------------------------------------------------------------*/

void drawloop(mat4 view, mat4 proj, GLuint framebuffer);

/*--------------------------------------------------------------------------*/

void init()
{
	if (!init_text_rendering(atlas_image, atlas_meta, width, height)) 
	{
		fprintf(stderr, "ERROR init text rendering\n");
		exit(1);
	}
	simpleShaderID = shaderFactory.CompileShader(SIMPLE_VERT, SIMPLE_FRAG);
	noTextureShaderID = shaderFactory.CompileShader(NOTEXTURE_VERT, NOTEXTURE_FRAG);
	cubeMapShaderID = shaderFactory.CompileShader(SKY_VERT, SKY_FRAG);

	cubeMapID.initCubeMap(vertices, 36, "desert");
	cubeID.init(CUBE_MESH);
	palmID.init(PALM_MESH);
	boneID.init(BONE_MESH);
	torsoID.init(TORSO_MESH);
	skeleton = Torso(torsoID, boneID, cubeID, palmID, cubeID);
	skeleton.updateJoints(point);
}

void display() 
{
	mat4 proj = perspective(87.0, (float)width / (float)(height), 1, 1000.0);
	mat4 view = look_at(cam.getPosition(), cam.getPosition() + cam.getFront(), cam.getUp());
	glViewport(0, 0, width, height);
	drawloop(view, proj, 0);
	draw_texts();
	glutSwapBuffers();
}

void updateScene() {
	static DWORD  last_frame;	//time when last frame was drawn
	static DWORD last_timer = 0;	//time when timer was updated
	DWORD  curr_time = timeGetTime();//for frame Rate Stuff.
	static bool first = true;
	if (first)
	{
		last_frame = curr_time;
		first = false;
	}
	float  delta = (curr_time - last_frame) * 0.001f;
	if (delta >= 0.03f) 
	{
		delta = 0.03f;
		last_frame = curr_time;
		glutPostRedisplay();

		cam.movForward(frontCam*speed);
		cam.movRight(sideCam*speed);
		cam.changeFront(pitCam, yawCam, rolCam);
		string output = "EULER_MODE: Front: [" + to_string(cam.getFront().v[0]) + ", " + to_string(cam.getFront().v[1]) + ", " + to_string(cam.getFront().v[2]) + "]\n";
		output += "Position: [" + to_string(cam.getPosition().v[0]) + ", " + to_string(cam.getPosition().v[1]) + ", " + to_string(cam.getPosition().v[2]) + "]\n";
		output += "Up: [" + to_string(cam.getUp().v[0]) + ", " + to_string(cam.getUp().v[1]) + ", " + to_string(cam.getUp().v[2]) + "]\n";
		output += "Pitch: " + to_string(cam.pitch) + "\n";
		output += "Yaw: " + to_string(cam.yaw) + "\n";
		output += "Roll: " + to_string(cam.roll) + "\n";

		update_text(textID, output.c_str());
		if (!pause)
		{
			t += 0.01f;
			a += 1.0f;
			b += 1.0f;
			if (t > 1)
			{
				t = 0;
				backwards = !backwards;
				currCurve++;
				if (currCurve >= NUM_CURVES)
					currCurve = 0;
			}
			if (a > 360)
				a = 0;
			if (b > 360)
				b = 0;
			point.v[0] = 5 * sin(a * ONE_DEG_IN_RAD);
			point.v[1] = 5 * cos(a * ONE_DEG_IN_RAD) + 10 * sin(b * ONE_DEG_IN_RAD);
			point.v[2] = 5 * cos(b * ONE_DEG_IN_RAD);

			if (mode)
			{
				if (skeleton.isUnstable(point))
				{
					//cout << "Reinitialising..." << endl;
					skeleton = Torso(torsoID, boneID, cubeID, palmID, cubeID);
				}

				point = curveArray[currCurve].calculatePosition(t);

				skeleton.updateJointsCCD(point);
			}
			else 
			{
				point.v[0] = 5 * sin(a * ONE_DEG_IN_RAD);
				point.v[1] = 10 * cos(a * ONE_DEG_IN_RAD);
				point.v[2] = 0;

				skeleton.updateJoints(point);
			}
		}
	}
	
}

#pragma region INPUT FUNCTIONS

void keypress(unsigned char key, int x, int y) {
	switch (key)
	{
	case ((char)27):
		exit(0);
		break;
	case('w'):
	case('W'):
		yaxis = 1;
		//printf("Moving Forward\n");
		break;
	case('s'):
	case('S'):
		yaxis = -1;
		//printf("Moving Backward\n");
		break;
	case('a'):
	case('A'):
		xaxis = -1;
		//printf("Moving Left\n");
		break;
	case('d'):
	case('D'):
		xaxis = 1;
		//printf("Moving Right\n");
		break;
	case('q'):
	case('Q'):
		zaxis = -1;
		//printf("Spinning Negative Roll\n");
		break;
	case('e'):
	case('E'):
		zaxis = 1;
		//printf("Spinning Positive Roll\n");
		break;
	}
}

void keypressUp(unsigned char key, int x, int y){
	switch (key)
	{
	case('w'):
	case('W'):
	case('s'):
	case('S'):
		yaxis = 0;
		break;
	case('a'):
	case('A'):
	case('d'):
	case('D'):
		xaxis = 0;
		break;
	case('q'):
	case('Q'):
	case('e'):
	case('E'):
		zaxis = 0;
		break;
	case(' '):
		pause = !pause;
		break;
	}
}

void specialKeypress(int key, int x, int y){
	switch (key)
	{
	case (GLUT_KEY_SHIFT_L):
	case (GLUT_KEY_SHIFT_R):
		speed = 4;
		break;
	case (GLUT_KEY_LEFT):
		//printf("Spinning Negative Yaw\n");
		yawCam = -1;
		break;
	case (GLUT_KEY_RIGHT):
		//printf("Spinning Positive Yaw\n");
		yawCam = 1;
		break;
	case (GLUT_KEY_UP):
		//printf("Spinning Positive Pit\n");
		pitCam = 1;
		break;
	case (GLUT_KEY_DOWN):
		//printf("Spinning Negative Pit\n");
		pitCam = -1;
		break;
	case(GLUT_KEY_F2):
		xaxis = 1;
		break;
	case(GLUT_KEY_F3):
		xaxis = -1;
		break;
	case(GLUT_KEY_F4):
		yaxis = 1;
		break;
	case(GLUT_KEY_F5):
		yaxis = -1;
		break;
	}
}

void specialKeypressUp(int key, int x, int y){
	switch (key)
	{
	case (GLUT_KEY_SHIFT_L):
	case (GLUT_KEY_SHIFT_R):
		speed = 1;
		break;
	case (GLUT_KEY_LEFT):
	case (GLUT_KEY_RIGHT):
		yawCam = 0;
		break;
	case (GLUT_KEY_UP):
	case (GLUT_KEY_DOWN):
		pitCam = 0;
		break;
	case(GLUT_KEY_F1):
		skeleton = Torso(torsoID, boneID, cubeID, palmID, cubeID);
		mode = !mode;
		break;
	case(GLUT_KEY_F2):
	case(GLUT_KEY_F3):
		xaxis = 0;
		break;
	case(GLUT_KEY_F4):
	case(GLUT_KEY_F5):
		yaxis = 0;
		break;
	}
}

void (mouse)(int x, int y){
}

#pragma endregion INPUT FUNCTIONS

int main(int argc, char** argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("GameApp");
	glutSetCursor(GLUT_CURSOR_CROSSHAIR);


	// Tell glut where the display function is
	glutWarpPointer(width / 2, height / 2);
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);

	// Input Function Initialisers //
	glutKeyboardFunc(keypress);
	glutPassiveMotionFunc(mouse);
	glutSpecialFunc(specialKeypress);
	glutSpecialUpFunc(specialKeypressUp);
	glutKeyboardUpFunc(keypressUp);

	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}

	init();
	textID = add_text("hi",
		-0.95, 0.9, fontSize, 1.0f, 1.0f, 1.0f, 1.0f);

	glutMainLoop();
	return 0;
}

void drawloop(mat4 view, mat4 proj, GLuint framebuffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glEnable(GL_DEPTH_TEST);								// enable depth-testing
	glDepthFunc(GL_LESS);									// depth-testing interprets a smaller value as "closer"
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clear the color and buffer bits to make a clean slate
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);					//Create a background	

	mat4 model = identity_mat4();

	vec3 Ls = vec3(0.6f, 0.3f, 0.6f);	//Specular Reflected Light
	vec3 Ld = vec3(0.8f, 0.8f, 0.8f);	//Diffuse Surface Reflectance
	vec3 La = vec3(0.8f, 0.8f, 0.8f);	//Ambient Reflected Light
	vec3 light = vec3(1.8*cos(0.0f), 1.8*sin(0.0f) + 1.0f, -5.0f);//light source location
	vec3 coneDirection = light + vec3(0.0f, -1.0f, 0.0f);
	float coneAngle = 40.0f;
	// object colour
	vec3 Ks = vec3(0.01f, 0.01f, 0.01f); // specular reflectance
	vec3 Kd = PURPLE;
	vec3 Ka = BLUE*0.2; // ambient reflectance
	float specular_exponent = 100.0f; //specular exponent - size of the specular elements

	drawCubeMap(cubeMapShaderID, cubeMapID.tex, view, proj, model, vec3(1.0, 1.0, 1.0), vec3(1.0, 1.0, 1.0), cam, cubeMapID, GL_TRIANGLES);
	skeleton.drawTorso(view, proj, identity_mat4(), noTextureShaderID, cam);
	model = translate(identity_mat4(), vec3(point.v[0], point.v[1], point.v[2]));
	drawObject(noTextureShaderID, view, proj, model, light, Ls, La, Ld, Ks, Kd, Ka, specular_exponent, cam, cubeID, coneAngle, coneDirection, GL_TRIANGLES);

	model = translate(identity_mat4(), skeleton.left.hand.palm->getPosition());
	drawObject(noTextureShaderID, view, proj, model, light, Ls, La, Ld, Ks, BLUE, Ka, specular_exponent, cam, cubeID, coneAngle, coneDirection, GL_TRIANGLES);
}