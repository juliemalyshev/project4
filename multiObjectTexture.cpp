//Multi-Object, Multi-Texture Example
//Stephen J. Guy, 2021

//This example demonstrates:
// Loading multiple models (a cube and a knot)
// Using multiple textures (wood and brick)
// Instancing (the teapot is drawn in two locations)
// Continuous keyboard input - arrows (moves knot up/down/left/right continuous when being held)
// Keyboard modifiers - shift (up/down arrows move knot in/out of screen when shift is pressed)
// Single key events - pressing 'c' changes color of a random teapot
// Mixing textures and colors for models
// Phong lighting
// Binding multiple textures to one shader

const char* INSTRUCTIONS =
"***************\n"
"This demo shows multiple objects being draw at once along with user interaction.\n"
"\n"
"Up/down/left/right - Moves the knot.\n"
"c - Changes to teapot to a random color.\n"
"***************\n"
;

//Mac OS build: g++ multiObjectTest.cpp -x c glad/glad.c -g -F/Library/Frameworks -framework SDL2 -framework OpenGL -o MultiObjTest
//Linux build:  g++ multiObjectTest.cpp -x c glad/glad.c -g -lSDL2 -lSDL2main -lGL -ldl -I/usr/include/SDL2/ -o MultiObjTest

#include "glad/glad.h"  //Include order can matter here
#if defined(__APPLE__) || defined(__linux__)
 #include <SDL2/SDL.h>
 #include <SDL2/SDL_opengl.h>
#else
 #include <SDL.h>
 #include <SDL_opengl.h>
#endif
#include <cstdio>
#include <GL/glut.h>
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
using namespace std;

// Shader sources
const GLchar* vertexSource =
    "#version 150 core\n"
    "in vec3 position;"
    "in vec3 inColor;"
    "in vec2 inTexcoord;"
    "out vec3 Color;"
    "out vec2 texcoord;"
    "uniform mat4 model;"
    "uniform mat4 view;"
    "uniform mat4 proj;"
    "void main() {"
    "   Color = inColor;"
    "   texcoord = inTexcoord;"
    "   gl_Position = proj * view * model * vec4(position,1.0);"
    "}";

const GLchar* fragmentSource =
    "#version 150 core\n"
    "in vec3 texcoord;"

    "uniform sampler2D tex;" //defaults to texture 0

    "in vec3 Color;"
    "out vec4 outColor;"
    "void main() {"
    "   vec3 Color = texture(tex, texcoord).rgb;"
    "   outColor = vec4(Color, 1.0);"
    "}";

int screenWidth = 1000;
int screenHeight = 800;
float timePast = 0;
float time_per_frame = 0.5;
int whichKey = 0;
struct key{
  glm::vec3 position;
} ;

//SJG: Store the object coordinates
//You should have a representation for the state of each object
float objx=0, objy=0, objz=0;
float colR=1, colG=1, colB=1;
float velocity = 2.0f;
//You should have a representation for the state of each object
float objWx=0, objWy=0, objWz=0;
float doory,doorz = 0;
glm::vec3 wallPositions[9];
bool collideKey = false;
bool collideWall = false;
bool collideDoor = false;


bool DEBUG_ON = true;
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
bool fullscreen = false;
void Win2PPM(int width, int height);

//srand(time(NULL));
float rand01(){
	return rand()/(float)RAND_MAX;
}
float CameraPosX = -0.4;
float CameraPosY = 2.0;
float CameraPosZ = 4.5;
//camPosX = -0.4;
//  camPosY = -1+j;
//  camPosZ = -2.5+i;
float CameraDirX = 0.0;
float CameraDirY = 1.0;
float CameraDirZ = 0;

float CameraUpX = 1.0;
float CameraUpY = 0.0;
float CameraUpZ = 0.0;
float CameraAngle = atan2(CameraDirY,CameraDirX);

float keyx,keyy,keyz;

int map[5][5];
void drawGeometry(int shaderProgram, int model1_start, int model1_numVerts, int model2_start, int model2_numVerts,int square_start, int square_numVerts,int sphere_start, int sphere_numVerts);
void drawSquare();
bool isWalkable(float x, float y);
void setCamDirFromAngle(float camAngle);
void setCamDirFromAngle(float camAngle){
  CameraDirY = sin(camAngle);
  CameraDirX = cos(camAngle);
}
int main(int argc, char *argv[]){
	SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)

	//Ask SDL to get a recent version of OpenGL (3.2 or greater)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	//Create a window (offsetx, offsety, width, height, flags)
	SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 100, screenWidth, screenHeight, SDL_WINDOW_OPENGL);

	//Create a context to draw in
	SDL_GLContext context = SDL_GL_CreateContext(window);

	//Load OpenGL extentions with GLAD
	if (gladLoadGLLoader(SDL_GL_GetProcAddress)){
		printf("\nOpenGL loaded\n");
		printf("Vendor:   %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));
		printf("Version:  %s\n\n", glGetString(GL_VERSION));
	}
	else {
		printf("ERROR: Failed to initialize OpenGL context.\n");
		return -1;
	}

	//Here we will load two different model files



	//Load Model 1
	ifstream modelFile;
	modelFile.open("models/teapot.txt");
	int numLines = 0;
	modelFile >> numLines;
	float* model1 = new float[numLines];
	for (int i = 0; i < numLines; i++){
		modelFile >> model1[i];
	}
	printf("%d\n",numLines);
	int numVertsTeapot = numLines/8;
	modelFile.close();

	//Load Model 2
	modelFile.open("models/knot.txt");
	numLines = 0;
	modelFile >> numLines;
	float* model2 = new float[numLines];
	for (int i = 0; i < numLines; i++){
		modelFile >> model2[i];
	}
	printf("%d\n",numLines);
	int numVertsKnot = numLines/8;
	modelFile.close();

  //Load Model 3
	modelFile.open("models/cube.txt");
	numLines = 0;
	modelFile >> numLines;
	float* model3 = new float[numLines];
	for (int i = 0; i < numLines; i++){
		modelFile >> model3[i];
	}
	printf("%d\n",numLines);
	int numVertsCube = numLines/8;
	modelFile.close();

//Load Model 4
modelFile.open("models/sphere.txt");
numLines = 0;
modelFile >> numLines;
string s;
float* model4 = new float[numLines];
for (int i = 0; i < numLines; i++){
  modelFile >> model4[i];
}
printf("%d\n",numLines);
int numVertsSphere = numLines/8;
modelFile.close();

  //Load the vertex Shader
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexSource, NULL);
  glCompileShader(vertexShader);
  // Load fragment shader
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
  glCompileShader(fragmentShader);

  //Join the vertex and fragment shaders together into one program
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glBindFragDataLocation(shaderProgram, 0, "outColor"); // set output
  glLinkProgram(shaderProgram); //run the linker

	//SJG: I load each model in a different array, then concatenate everything in one big array
	// This structure works, but there is room for improvement here. Eg., you should store the start
	// and end of each model a data structure or array somewhere.
	//Concatenate model arrays
	float* modelData = new float[(numVertsTeapot+numVertsKnot+numVertsCube+numVertsSphere)*8];
	copy(model1, model1+numVertsTeapot*8, modelData);
	copy(model2, model2+numVertsKnot*8, modelData+numVertsTeapot*8);
  copy(model3, model3+numVertsCube*8, (modelData+numVertsTeapot*8+numVertsKnot*8));
  copy(model4, model4+numVertsSphere*8, (modelData+numVertsTeapot*8+numVertsKnot*8+numVertsCube*8));

	int totalNumVerts = numVertsTeapot+numVertsKnot+numVertsCube+numVertsSphere;
	int startVertTeapot = 0;  //The teapot is the first model in the VBO
	int startVertKnot = numVertsTeapot; //The knot starts right after the taepot
  int startVertCube = numVertsTeapot + numVertsKnot; //The squarestarts right after the knot
  int startVertSphere = numVertsTeapot + numVertsKnot + numVertsCube; //The sphere starts right after the square





	//// Allocate Texture 0 (Wood) ///////
	SDL_Surface* surface = SDL_LoadBMP("wood.bmp");
	if (surface==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex0;
    glGenTextures(1, &tex0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex0);

    //What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //Load the texture into memory
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w,surface->h, 0, GL_BGR,GL_UNSIGNED_BYTE,surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

    SDL_FreeSurface(surface);
    //// End Allocate Texture ///////


	//// Allocate Texture 1 (Brick) ///////
	SDL_Surface* surface1 = SDL_LoadBMP("brick.bmp");
	if (surface==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex1;
    glGenTextures(1, &tex1);

    //Load the texture into memory
    glActiveTexture(GL_TEXTURE1);

    glBindTexture(GL_TEXTURE_2D, tex1);
    //What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //How to filter
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface1->w,surface1->h, 0, GL_BGR,GL_UNSIGNED_BYTE,surface1->pixels);
    glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

    SDL_FreeSurface(surface1);
	//// End Allocate Texture ///////

  //// Allocate Texture 3 (Plate) ///////
  SDL_Surface* surface2 = SDL_LoadBMP("plate.bmp");
  if (surface2==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex2;
    glGenTextures(1, &tex2);

    //Load the texture into memory
    glActiveTexture(GL_TEXTURE2);

    glBindTexture(GL_TEXTURE_2D, tex2);
    //What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //How to filter
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface2->w,surface2->h, 0, GL_BGR,GL_UNSIGNED_BYTE,surface2->pixels);
    glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

    SDL_FreeSurface(surface2);
  //// End Allocate Texture ///////

  //// Allocate Texture 4 (Plate) ///////
  SDL_Surface* surface3 = SDL_LoadBMP("PoolWater.bmp");
  if (surface3==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex3;
    glGenTextures(1, &tex3);

    //Load the texture into memory
    glActiveTexture(GL_TEXTURE3);

    glBindTexture(GL_TEXTURE_2D, tex3);
    //What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //How to filter
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface3->w,surface3->h, 0, GL_BGR,GL_UNSIGNED_BYTE,surface3->pixels);
    glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture

    SDL_FreeSurface(surface3);
  //// End Allocate Texture ///////

	//Build a Vertex Array Object (VAO) to store mapping of shader attributse to VBO
	GLuint vao;
	glGenVertexArrays(1, &vao); //Create a VAO
	glBindVertexArray(vao); //Bind the above created VAO to the current context

	//Allocate memory on the graphics card to store geometry (vertex buffer object)
	GLuint vbo[1];
	glGenBuffers(1, vbo);  //Create 1 buffer called vbo
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
	glBufferData(GL_ARRAY_BUFFER, totalNumVerts*8*sizeof(float), modelData, GL_STATIC_DRAW); //upload vertices to vbo
	//GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW = geometry changes infrequently
	//GL_STREAM_DRAW = geom. changes frequently.  This effects which types of GPU memory is used

	int texturedShader = InitShader("textured-Vertex.glsl", "textured-Fragment.glsl");

	//Tell OpenGL how to set fragment shader input
	GLint posAttrib = glGetAttribLocation(texturedShader, "position");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), 0);
	  //Attribute, vals/attrib., type, isNormalized, stride, offset
	glEnableVertexAttribArray(posAttrib);

	//GLint colAttrib = glGetAttribLocation(phongShader, "inColor");
	//glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
	//glEnableVertexAttribArray(colAttrib);

	GLint normAttrib = glGetAttribLocation(texturedShader, "inNormal");
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));
	glEnableVertexAttribArray(normAttrib);

	GLint texAttrib = glGetAttribLocation(texturedShader, "inTexcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));

	GLint uniView = glGetUniformLocation(texturedShader, "view");
	GLint uniProj = glGetUniformLocation(texturedShader, "proj");

	glBindVertexArray(0); //Unbind the VAO in case we want to create a new one


	glEnable(GL_DEPTH_TEST);

	printf("%s\n",INSTRUCTIONS);

	//Event Loop (Loop forever processing each event as fast as possible)
	SDL_Event windowEvent;
	bool quit = false;

	while (!quit){
    float t_start = SDL_GetTicks()/1000.f;
		while (SDL_PollEvent(&windowEvent)){  //inspect all events in the queue

			if (windowEvent.type == SDL_QUIT) quit = true;
			//List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
			//Scancode referes to a keyboard position, keycode referes to the letter (e.g., EU keyboards)
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE)
				quit = true; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f){ //If "f" is pressed
				fullscreen = !fullscreen;
				SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Toggle fullscreen
			}

			//SJG: Use key input to change the state of the object
			//     We can use the ".mod" flag to see if modifiers such as shift are pressed
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_UP){ //If "up key" is pressed
				if (windowEvent.key.keysym.mod & KMOD_SHIFT) objx -= .1; //Is shift pressed?
				else   //CameraPosZ -= 0.1;//(velocity*CameraDirZ* time_per_frame * 0.2 );
              //CameraPosY -= 0.1*CameraDirY;//(velocity*CameraDirY* time_per_frame * 0.2 );
              //CameraPosZ-= CameraDirZ*0.1;
              //CameraPosY-= CameraDirY*0.1;
              if(isWalkable(objy,objz+=(velocity * time_per_frame * 0.03 ))){
              objz+=(velocity * time_per_frame * 0.03);
            }else{
              objz-=0.09;
            }


			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_DOWN){ //If "down key" is pressed
				if (windowEvent.key.keysym.mod & KMOD_SHIFT) objx += .1; //Is shift pressed?
				else CameraPosZ+= CameraDirZ*0.1;
        //CameraPosY+= CameraDirY*0.1;
        if(isWalkable(objy,objz-=(velocity * time_per_frame * 0.03 ))){
        objz-=(velocity * time_per_frame * 0.03);
      }else{
        objz+=0.09;
      }

			}
				if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_LEFT){ //If "up key" is pressed
				//CameraDirY += CameraAngle*0.3;//(velocity * time_per_frame * 0.2);
        //CameraAngle += 0.1f;
  			//CameraDirY = sin(CameraAngle);
  			//CameraDirZ = -cos(CameraAngle);
        if(isWalkable(objy-=(velocity*time_per_frame * 0.03 ),objz)){
        objy-=(velocity* time_per_frame * 0.03);
        }else{
        objy+=0.09;
        }

			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_RIGHT){ //If "down key" is pressed
				//CameraDirY -=CameraAngle*0.3;
        //CameraAngle += 0.1f;
        //CameraDirY = -sin(CameraAngle);
        //CameraDirZ = cos(CameraAngle);

        if(isWalkable(objy+=(velocity*time_per_frame * 0.03),objz)){
        objy+=(velocity*time_per_frame * 0.03 );
      }else{
        objy-=0.09;
      }
			}
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_c){ //If "c" is pressed
				colR = rand01();
				colG = rand01();
				colB = rand01();
			}
      if (windowEvent.type == SDL_MOUSEMOTION && windowEvent.button.button == SDL_BUTTON(SDL_BUTTON_LEFT)){ //If "c" is pressed
        glm::mat4 view = glm::lookAt(
        glm::vec3(3.f, 0.f, 0.f),  //Cam Position
        glm::vec3(0.0f, 0.0f+sin(20), 0.0f),  //Look at point
        glm::vec3( CameraUpX,  CameraUpY, CameraUpZ)); //Up
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
			}
      /*glm::mat4 view = glm::lookAt(
      glm::vec3(3.f, objy, objz),  //Cam Position
      glm::vec3(0.0f, 1.0f, 0.0f),  //Look at point
      glm::vec3( CameraUpX,  CameraUpY, CameraUpZ)); //Up
      glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));*/
		}
    //cout<<velocity<<endl;
		// Clear the screen to default color
		glClearColor(.2f, 0.4f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(texturedShader);


		timePast = SDL_GetTicks()/1000.f;

		glm::mat4 view = glm::lookAt(
		glm::vec3(7.f, 2.f, 0.f),  //Cam Position
		glm::vec3(1.0f, 2.0f, 2.0f),  //Look at point
		glm::vec3(0.0f, 0.0f, 1.0f)); //Up
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
    /*glm::mat4 view = glm::lookAt(
    glm::vec3( CameraPosX,  CameraPosY, CameraPosZ),  //Cam Position
    glm::vec3( CameraDirX,  CameraDirY, CameraDirZ), //Up,  //Look at point
    glm::vec3( CameraUpX,  CameraUpY, CameraUpZ)); //Up
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));*/

		glm::mat4 proj = glm::perspective(3.14f/4, screenWidth / (float) screenHeight, 1.0f, 10.0f); //FOV, aspect, near, far
		glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex0);
		glUniform1i(glGetUniformLocation(texturedShader, "tex0"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex1);
		glUniform1i(glGetUniformLocation(texturedShader, "tex1"), 1);

    glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, tex2);
		glUniform1i(glGetUniformLocation(texturedShader, "tex2"), 2);

    glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, tex3);
		glUniform1i(glGetUniformLocation(texturedShader, "tex3"), 3);

		glBindVertexArray(vao);

		drawGeometry(texturedShader, startVertTeapot, numVertsTeapot, startVertKnot, numVertsKnot, startVertCube, numVertsCube, startVertSphere, numVertsSphere);


		SDL_GL_SwapWindow(window); //Double buffering
    float t_end = SDL_GetTicks();
		float time_per_frame = t_end-t_start;
	}

	//Clean Up
	glDeleteProgram(texturedShader);
    glDeleteBuffers(1, vbo);
    glDeleteVertexArrays(1, &vao);

	SDL_GL_DeleteContext(context);
	SDL_Quit();
	return 0;
}
float distanceTest(int x1, int y1, int x2, int y2)
{
  //double calcdistance = sqrt(distancex - distancey);
    // Calculating distance
    return sqrt(pow(x2 - x1, 2) +
                pow(y2 - y1, 2));
}

void drawGeometry(int shaderProgram, int model1_start, int model1_numVerts, int model2_start, int model2_numVerts, int square_start, int square_numVerts,int sphere_start, int sphere_numVerts){
  //Load Map
  std::ifstream infile("map2.txt");
  int numLines = 0;
  int mapArray[26];
  std::string line;
  int r = 0;
while (std::getline(infile, line))
{
  std::istringstream iss(line);
  int idx;
  while (iss>>idx ) {

	  mapArray[r] = idx;
    r++;
	}
  numLines++;
}
//int map[mapArray[0]][mapArray[1]];
int nextLine=2;
for(int i = 0; i<numLines;i++){
  for(int j = 0; j< 5; j++){
    map[i][j] = mapArray[nextLine];
    nextLine++;
  }
  //nextLine+=5;
}

  //cout<<floor(objy)<<" "<<floor(objz+4)<<endl;

	GLint uniColor = glGetUniformLocation(shaderProgram, "inColor");
	glm::vec3 colVec(colR,colG,colB);
	glUniform3fv(uniColor, 1, glm::value_ptr(colVec));

  GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");

	//************
	//Draw model #1 the first time
	//This model is stored in the VBO starting a offest model1_start and with model1_numVerts num of verticies
	//*************

	//Rotate model (matrix) based on how much time has past
	glm::mat4 model = glm::mat4(1);
	model = glm::rotate(model,timePast * 3.14f/2,glm::vec3(0.0f, 1.0f, 1.0f));
	model = glm::rotate(model,timePast * 3.14f/4,glm::vec3(1.0f, 0.0f, 0.0f));
	//model = glm::scale(model,glm::vec3(.2f,.2f,.2f)); //An example of scale
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader

	//Set which texture to use (-1 = no texture)
	glUniform1i(uniTexID, 1);

	//Draw an instance of the model (at the position & orientation specified by the model matrix above)
	//glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)


	//************
	//Draw model #1 the second time
	//This model is stored in the VBO starting a offest model1_start and with model1_numVerts num. of verticies
	//*************

	//Translate the model (matrix) left and back
	model = glm::mat4(1); //Load intentity
	model = glm::translate(model,glm::vec3(-2,-1,-.4));
	//model = glm::scale(model,2.f*glm::vec3(1.f,1.f,0.5f)); //scale example
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	//Set which texture to use (0 = wood texture ... bound to GL_TEXTURE0)
	glUniform1i(uniTexID, 0);

  //Draw an instance of the model (at the position & orientation specified by the model matrix above)
	//glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)


  //Draw an instance of the model (at the position & orientation specified by the model matrix above)
//  glDrawArrays(GL_TRIANGLES, square_start,square_numVerts); //(Primitive Type, Start Vertex, Num Verticies)

  //************
	//Draw model #4 once
	//This model is stored in the VBO starting a offest model2_start and with model2_numVerts num of verticies
	//*************

	//Translate the model (matrix) based on where objx/y/z is
	// ... these variables are set when the user presses the arrow keys
	/*model = glm::mat4(1);
	model = glm::scale(model,glm::vec3(.2f,.2f,.2f)); //scale this model
	model = glm::translate(model,glm::vec3(-1,-0.5,-1.4));

	//Set which texture to use (1 = brick texture ... bound to GL_TEXTURE1)

	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
  glUniform1i(uniTexID, 2);
	//Draw an instance of the model (at the position & orientation specified by the model matrix above)
	glDrawArrays(GL_TRIANGLES, sphere_start, sphere_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
*/
  model = glm::mat4(1); //Load intentity
  model = glm::rotate(model,6.3f,glm::vec3(0.0f, 1.0f, 1.0f));
  model = glm::scale(model,glm::vec3(.5f,.4f,.2f)); //scale this model
  model = glm::translate(model,glm::vec3(-.5,-1.5,-1));
  glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
  glUniform1i(uniTexID, 2);
  //Draw an instance of the model (at the position & orientation specified by the model matrix above)
//  glDrawArrays(GL_TRIANGLES, square_start,square_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
    //************
  	//Draw sqauare
  	//This model is stored in the VBO starting a offest square_start and with square_numVerts num of verticies
  	//*************

    int t = 0;

    for(int i = 0; i<mapArray[0];i++){
      for(int j = 0; j<mapArray[0]; j++){

    	//Translate the model (matrix) based on where objx/y/z is
    	// ... these variables are set when the user presses the arrow keys
      model = glm::mat4(1); //Load intentity
      model = glm::translate(model,glm::vec3(-2,j,i));
      //model = glm::scale(model,glm::vec3(.4f,.4f,.4f)); //scale this model


    	//Set which texture to use (1 = brick texture ... bound to GL_TEXTURE1)
    	//glUniform1i(uniTexID, 1);

      glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(uniTexID, 0);
    	//Draw an instance of the model (at the position & orientation specified by the model matrix above)
    	glDrawArrays(GL_TRIANGLES, square_start,square_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
      //DRAW WALLS
      //cout<<map[i][j]<<endl;


      //DRAW WALL
      if(map[i][j]==2 ){


        model = glm::mat4(1); //Load intentity
        model = glm::translate(model,glm::vec3(-1,j,i));
        //model = glm::scale(model,glm::vec3(.4f,.4f,.4f)); //scale this model

        objWx = -1;
        objWy = j;
        objWz = i;
        //cout<<t<<endl;

        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(uniTexID, 1);
        //cout<<abs(distanceTest(objWy,objWz,objy,objz))<<endl;

        wallPositions[t] = glm::vec3(objWx,objWy,objWz);


      	//Set which texture to use (1 = brick texture ... bound to GL_TEXTURE1)
      	//glUniform1i(uniTexID, 1);
      	//glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

      	//Draw an instance of the model (at the position & orientation specified by the model matrix above)
      	glDrawArrays(GL_TRIANGLES, square_start,square_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
        //cout<<abs(distanceTest(wallPositions[t].y,wallPositions[t].z,objy,objz))<<endl;

        t++;
      }
      //DRAW DOOR
      if(map[i][j]==3 ){

        model = glm::mat4(1); //Load intentity
        //model = glm::scale(model,glm::vec3(.4f,.4f,.4f)); //scale this model
      	model = glm::translate(model,glm::vec3(-1,j,i));
        doory = j;
        doorz = i;
        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(uniTexID, whichKey);
        if(collideDoor == true && collideKey  == true){

        }else{
        //Draw an instance of the model (at the position & orientation specified by the model matrix above)
        glDrawArrays(GL_TRIANGLES, square_start,square_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
      }
      }
      //DRAW US
     if(map[i][j] == 4){
        //************
        //Draw model #2 once
        //This model is stored in the VBO starting a offest model2_start and with model2_numVerts num of verticies
        //*************

        //Translate the model (matrix) based on where objx/y/z is
        // ... these variables are set when the user presses the arrow keys
        model = glm::mat4(1);

        model = glm::translate(model,glm::vec3(-1,j+objy,i+objz));
        model = glm::scale(model,glm::vec3(.3f,.3f,.3f)); //scale this model

        //Set which texture to use (1 = brick texture ... bound to GL_TEXTURE1)

        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(uniTexID, 1);

        //Draw an instance of the model (at the position & orientation specified by the model matrix above)
        glDrawArrays(GL_TRIANGLES, model2_start, model2_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
        if(distanceTest(j+objy,i+objz,keyy,keyz)<=0.1){
         collideKey = true;

        }
        if(distanceTest(j+objy,i+objz,doory,doorz)<=0.1){
         collideDoor = true;

        }


      }
      //  camPosX = -0.4;





      //DRAW KEY
      if(map[i][j]==5 || map[i][j]==6 ){

        model = glm::mat4(1); //Load intentity
        model = glm::translate(model,glm::vec3(-1,j,i));
        model = glm::scale(model,glm::vec3(.4f,.4f,.4f)); //scale this model*/
      //  glm::mat4 model = glm::mat4(1);
      	model = glm::rotate(model,timePast * 3.14f/2,glm::vec3(0.0f, 1.0f, 1.0f));
      	model = glm::rotate(model,timePast * 3.14f/4,glm::vec3(1.0f, 0.0f, 0.0f));
      	//model = glm::scale(model,glm::vec3(.2f,.2f,.2f)); //An example of scale
      	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
      	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader

        if(map[i][j]==5){
          whichKey = 2;
        }
        if(map[i][j]==6){
          whichKey = 3;
        }
        //Set which texture to use (1 = brick texture ... bound to GL_TEXTURE1)
        keyx = -1;
        keyy = j;
        keyz = i;
        if(collideKey == false){
        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(uniTexID, whichKey);
        //Draw an instance of the model (at the position & orientation specified by the model matrix above)
        //glDrawArrays(GL_TRIANGLES, sphere_start, sphere_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
        glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
        }

        velocity = 2.0;
      }

  }

}

}
bool isWalkable(float x, float y){

      //cout<<" "<<map[(int)ceil(y+4)][(int)ceil(x)]<<endl;
      cout<<" "<<(int)ceil(y+4)<<(int)ceil(x)<<endl;
      //cout<<map[3][1]<<endl;
      if(collideKey == true){
        return true;
      }
      if(x<-0.2 || y< -4.5 || x>4.3 || y>0.2){
        return false;
      }

      if(map[(int)ceil(y+4)][(int)ceil(x)]==3 || map[(int)ceil(y+4)][(int)ceil(x)]==2){
        return false;

      }
      if(map[(int)floor(y+4)][(int)floor(x)]==3 || map[(int)floor(y+4)][(int)floor(x)]==2){
        return false;

      }

  return true;
}
// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile){
	FILE *fp;
	long length;
	char *buffer;

	// open the file containing the text of the shader code
	fp = fopen(shaderFile, "r");

	// check for errors in opening the file
	if (fp == NULL) {
		printf("can't open shader source file %s\n", shaderFile);
		return NULL;
	}

	// determine the file size
	fseek(fp, 0, SEEK_END); // move position indicator to the end of the file;
	length = ftell(fp);  // return the value of the current position

	// allocate a buffer with the indicated number of bytes, plus one
	buffer = new char[length + 1];

	// read the appropriate number of bytes from the file
	fseek(fp, 0, SEEK_SET);  // move position indicator to the start of the file
	fread(buffer, 1, length, fp); // read all of the bytes

	// append a NULL character to indicate the end of the string
	buffer[length] = '\0';

	// close the file
	fclose(fp);

	// return the string
	return buffer;
}

// Create a GLSL program object from vertex and fragment shader files
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName){
	GLuint vertex_shader, fragment_shader;
	GLchar *vs_text, *fs_text;
	GLuint program;

	// check GLSL version
	printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Create shader handlers
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Read source code from shader files
	vs_text = readShaderSource(vShaderFileName);
	fs_text = readShaderSource(fShaderFileName);

	// error check
	if (vs_text == NULL) {
		printf("Failed to read from vertex shader file %s\n", vShaderFileName);
		exit(1);
	} else if (DEBUG_ON) {
		printf("Vertex Shader:\n=====================\n");
		printf("%s\n", vs_text);
		printf("=====================\n\n");
	}
	if (fs_text == NULL) {
		printf("Failed to read from fragent shader file %s\n", fShaderFileName);
		exit(1);
	} else if (DEBUG_ON) {
		printf("\nFragment Shader:\n=====================\n");
		printf("%s\n", fs_text);
		printf("=====================\n\n");
	}

	// Load Vertex Shader
	const char *vv = vs_text;
	glShaderSource(vertex_shader, 1, &vv, NULL);  //Read source
	glCompileShader(vertex_shader); // Compile shaders

	// Check for errors
	GLint  compiled;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		printf("Vertex shader failed to compile:\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(vertex_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Load Fragment Shader
	const char *ff = fs_text;
	glShaderSource(fragment_shader, 1, &ff, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);

	//Check for Errors
	if (!compiled) {
		printf("Fragment shader failed to compile\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(fragment_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Create the program
	program = glCreateProgram();

	// Attach shaders to program
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	// Link and set program to use
	glLinkProgram(program);

	return program;
}
