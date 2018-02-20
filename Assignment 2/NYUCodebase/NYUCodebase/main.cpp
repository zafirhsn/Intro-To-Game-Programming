#define STB_IMAGE_IMPLEMENTATION
#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#include "stb_image.h"


#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif


SDL_Window* displayWindow;

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	displayWindow = SDL_CreateWindow("Assignment 2: Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	//Set the size and offset of rendering area (in pixels)	
	glViewport(0, 0, 640, 360);

	//Load the shader program
	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");

	//Load the matrices
	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;
	Matrix leftPadMatrix;
	Matrix rightPadMatrix;
	Matrix ballMatrix;

	//Sets an orthographic projection in a matrix
	projectionMatrix.SetOrthoProjection(-5.33f, 5.33f, -3.0f, 3.0f, -1.0f, 1.0f);

	//Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//This is how we will keep track of time
	float lastFrameTicks = 0.0;
	float dist = 0.0;

	//Set the color for untextured polygons
	program.SetColor(1.0f, 1.0f, 1.0f, 1.0f);

	//Set clear color of screen
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	//Position and sides of left paddle
	float leftPadPos[] = { -4.875, -0.35 };
	float leftPadRight = -4.75;
	float leftPadTop = 0.3;
	float leftPadBottom = -1.0;

	//Position and sides of right paddle
	float rightPadPos[] = { 4.875, -0.35 };
	float rightPadLeft = 4.75;
	float rightPadTop = 0.3;
	float rightPadBottom = -1.0; 

	//Position and sides of ball
	float ballPos[] = { -0.125, 0.375 };
	float ballTop = 0.5;
	float ballBottom = 0.25;
	float ballRight = 0.0;
	float ballLeft = -0.25;
	int xdirection = 1;
	int ydirection = 1;
	float angle = 0.0;

	//Top and bottom of bars
	float topBarBottom = 2.75;
	float bottomBarTop = -2.75;

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);

		//Use the specified program ID
		glUseProgram(program.programID);

		//Keeping time
		float ticks = (float)SDL_GetTicks() / 1000.0;
		float elasped = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		dist += elasped;

		//Pass the matrices to our program
		program.SetModelMatrix(modelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);

		float topBarVertices[] = {
			-4.75, 2.75, //bottom left
			4.75, 3.0, //top right
			-4.75, 3.0, //top left
			4.75, 3.0, //top right
			4.75, 2.75, //bottom right
			-4.75, 2.75 //bottom keft
		};
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, topBarVertices);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);

		float bottomBarVertices[] = {
			-4.75, -2.75, //bottom left
			4.75, -3.0, //top right
			-4.75, -3.0, //top left
			4.75, -3.0, //top right
			4.75, -2.75, //bottom right
			-4.75, -2.75 //bottom keft
		};

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, bottomBarVertices);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);


		//Define an array of vertex data
		float leftPadVertices[] = {
			-5.00, -1.0, //bottom left
			-4.75, -1.0, //bottom right
			-4.75, 0.3, //top right
			-4.75, 0.3, //top right
			-5.00, 0.3, //top left
			-5.00, -1.0  //bottom left
		};

		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		//Player 1 moves the left paddle
		if (keys[SDL_SCANCODE_UP]) {
			leftPadMatrix.Translate(0.0, 0.001, 0.0);
			leftPadTop += 0.001;
			leftPadBottom += 0.001;
		}
		if (keys[SDL_SCANCODE_DOWN]) {
			leftPadMatrix.Translate(0.0, -0.001, 0.0);
			leftPadTop -= 0.001;
			leftPadBottom -= 0.001;
		}
		
		program.SetModelMatrix(leftPadMatrix);

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, leftPadVertices);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);

		//Move the CPU paddle on the right
		rightPadMatrix.Translate(0.0, (sin(angle) * 0.0009) * ydirection, 0.0);
		rightPadTop += (sin(angle) * 0.0009) * ydirection;
		rightPadBottom += (sin(angle) * 0.0009) * ydirection;

		program.SetModelMatrix(rightPadMatrix);

		float rightPadVertices[] = {
			 5.00, -1.0, //bottom left
			 4.75, -1.0, //bottom right
			 4.75, 0.3, //top right
			 4.75, 0.3, //top right
			 5.00, 0.3, //top left
			 5.00, -1.0  //bottom left
		};

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, rightPadVertices);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);

		//Move the ball based on angle
		ballMatrix.Translate((cos(angle) * 0.001) * xdirection, (sin(angle) * 0.001) * ydirection, 0.0);
		ballRight += (cos(angle) * 0.001) * xdirection;
		ballLeft += (cos(angle) * 0.001) * xdirection;
		ballTop += (sin(angle) * 0.001) * ydirection;
		ballBottom += (sin(angle) * 0.001) * ydirection;
		
		//See if ball and right paddle collided
		if (!(ballBottom > rightPadTop) && !(ballTop < rightPadBottom) && !(ballRight < rightPadLeft)) {
			//If the ball hit the top part of the paddle, make it bounce back at an angle between 35 - 54
			if (ballBottom < rightPadTop && ballBottom >= (rightPadTop - 0.3)) {
				angle = rand() % 20 + 35.0;
			}
			//If the ball hit the middle part of the paddle, make it bounce back at an angle between 0 - 10
			else if (ballBottom < rightPadTop && ballBottom >= (rightPadTop - 1.0)) {
				angle = (rand() % 10) * 1.0;
			}
			//If the ball hit the bottom part of the paddle, make it bounce back at an angle between 35 - 54
			else if (ballBottom < rightPadTop && ballBottom >= (rightPadTop - 1.3)) {
				angle = rand() % 20 + 35.0;
			}
			xdirection *= -1;
		};

		//See if ball and left paddle collided
		if (!(ballBottom > leftPadTop) && !(ballTop < leftPadBottom) && !(ballLeft > leftPadRight)) {
			//If the ball hit the top part of the paddle, make it bounce back at an angle between 35 - 54
			if (ballBottom < leftPadTop && ballBottom >= (leftPadTop - 0.3)) {
				angle = rand() % 20 + 35.0;
			}
			//If the ball hit the middle part of the paddle, make it bounce back at an angle between 0 - 10
			else if (ballBottom < leftPadTop && ballBottom >= (leftPadTop - 1.0)) {
				angle = (rand() % 10) * 1.0;
			}
			//If the ball hit the bottom part of the paddle, make it bounce back at an angle between 35 - 54
			else if (ballBottom < leftPadTop && ballBottom >= (leftPadTop - 1.3)) {
				angle = rand() % 20 + 35.0;
			}
			xdirection *= -1;
		}

		//See if the ball hit the top bar
		if (!(ballTop < topBarBottom)) {
			ydirection *= -1;
		}

		//See if the ball hit the bottom bar
		if (!(ballBottom > bottomBarTop)) {
			ydirection *= -1;
		}

		if (ballLeft <= -5.0) {
			std::cout << "Right player has won!";
			ballMatrix.Identity();
			rightPadMatrix.Identity();
			angle = 0.0;
			rightPadLeft = 4.75;
			rightPadTop = 0.3;
			rightPadBottom = -1.0;

			ballTop = 0.5;
			ballBottom = 0.25;
			ballRight = 0.0;
			ballLeft = -0.25;
		}
		
		if (ballRight >= 5.0) {
			std::cout << "Left player has won!";
			ballMatrix.Identity();
			rightPadMatrix.Identity();
			angle = 0.0;
			rightPadLeft = 4.75;
			rightPadTop = 0.3;
			rightPadBottom = -1.0;

			ballTop = 0.5;
			ballBottom = 0.25;
			ballRight = 0.0;
			ballLeft = -0.25;
		}

		program.SetModelMatrix(ballMatrix);

		float ballVertices[] = { 
			0.0, 0.25, 
			0.0, 0.50, 
			-0.25, 0.25,
			0.0, 0.50, 
			-0.25, 0.50, 
			-0.25, 0.25
		};

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ballVertices);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
