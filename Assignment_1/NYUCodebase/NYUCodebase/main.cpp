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

	displayWindow = SDL_CreateWindow("Assignment 1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

	//Set the size and offset of rendering area (in pixels)	
	//glViewport(0, 0, 800, 400);

#ifdef _WINDOWS
	glewInit();
#endif

	//Load the shader program
	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	GLuint grass = LoadTexture(RESOURCE_FOLDER"grass.png");
	GLuint sun = LoadTexture(RESOURCE_FOLDER"star.png");
	GLuint bush = LoadTexture(RESOURCE_FOLDER"rpgTile155.png");
	GLuint cactus = LoadTexture(RESOURCE_FOLDER"cactus.png");

	//Load the matrices
	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;
	Matrix sunModelMatrix;

	//Sets an orthographic projection in a matrix
	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	//Use the specified program ID
	glUseProgram(program.programID);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float lastFrameTicks = 0.0;
	float dist = 0.0;
	program.SetColor(0.8f, 0.8f, 0.6f, 1.0f);
	glClearColor(0.2f, 0.5f, 0.90f, 0.9f);

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);


		float ticks = (float)SDL_GetTicks() / 1000.0;
		float elasped = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		dist += elasped;

		//Pass the matrices to our program
		program.SetModelMatrix(modelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);

		glBindTexture(GL_TEXTURE_2D, cactus);

		//Define an array of vertex data
		float treeVertices[] = {
			-3.0, -1.0, //bottom left
			-2.25, -1.0, //bottom right
			-2.25, 0.8, //top right
			-2.25, 0.8, //top right
			-3.0, 0.8, //top left
			-3.0, -1.0  //bottom left
		};
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, treeVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float treeTexCoords[] = {
			0.0, 1.0, //bottom left
			1.0, 1.0, //bottom right
			1.0, 0.0, //top right
			1.0, 0.0, //top right
			0.0, 0.0, //top left
			0.0, 1.0 //bottom left
		};
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, treeTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);


		glBindTexture(GL_TEXTURE_2D, grass);

		float groundVertices[] = { 3.55, -1.0, -3.55, -1.0, -3.55, -2.0,
								   3.55, -1.0, -3.55, -2.0, 3.55, -2.0 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, groundVertices);
		glEnableVertexAttribArray(program.positionAttribute);
		float groundTexCoords[] = {
			0.0, -1.0,
			3.55, -1.0,
			3.55, 0.0,

			0.0, -1.0,
			3.55, 0.0,
			0.0, 0.0
		};
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, groundTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//CREATING THE SUN
		sunModelMatrix.Identity();
		glBindTexture(GL_TEXTURE_2D, sun);
		sunModelMatrix.Translate(-2.0, 0.0, 0.0);
		sunModelMatrix.Translate(dist, 0.0, 0.0);
		if (dist > 2.0) { 
			dist = 0; 
			sunModelMatrix.Translate(-2.0, 0.0, 0.0);
			sunModelMatrix.Translate(dist, 0.0, 0.0);
		}

		program.SetModelMatrix(sunModelMatrix);

		float sunVertices[] = { 3.0, 1.5, 3.0, 2.0, 2.5, 1.5,
								3.0, 2.0, 2.5, 2.0, 2.5, 1.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, sunVertices);
		glEnableVertexAttribArray(program.positionAttribute);
		
		float sunTexCoords[] = {
			1.0, -1.0, //bottom right
			1.0, 0.0, //top right
			0.0, -1.0, //bottom left
			
			1.0, 0.0, //top right
			0.0, 0.0,  //top left
			0.0, -1.0 //bottom left
		};

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, sunTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);



		//CREATING THE BUSH
		glBindTexture(GL_TEXTURE_2D, bush);

		float bushVertices[] = { 
			-1.0, -1.0, //bottom right
			-1.5, -1.0, //bottom left
 			-1.5, -0.5, //top left
			-1.5, -0.5, //top left
			-1.0, -0.5, //top right
			-1.0, -1.0  //bottom right
		};
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, bushVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float bushTexCoords[] = { 
			1.0, 1.0, //bottom right
			0.0, 1.0, //bottom left
			0.0, 0.0,  //top left 
			0.0, 0.0,  //top left 
			1.0, 0.0,  //top right
			1.0, 1.0  //bottom right
		};
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, bushTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);


		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
