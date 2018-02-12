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
	//Doesn't seem that we need this
	//glViewport(0, 0, 800, 400);

	#ifdef _WINDOWS
		glewInit();
	#endif
	
	//Load the shader program
	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	
	//Load the matrices
	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	//Sets an orthographic projection in a matrix
	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	//Use the specified program ID
	glUseProgram(program.programID);

	GLuint grass = LoadTexture(RESOURCE_FOLDER"grass.png");




	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.2f, 0.5f, 0.80f, 0.9f);

		program.SetColor(0.2f, 0.8f, 0.4f, 1.0f);

		//Pass the matrices to our program
		program.SetModelMatrix(modelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);

		glBindTexture(GL_TEXTURE_2D, grass);

		//Define an array of vertex data
		float vertices[] = { -3.0, -1.0, -2.75, -1.0, -2.75, 0.8,   
							 -2.75, 0.8, -3.0, 0.8, -3.0, -1.0,   
							 3.55, -1.0, -3.55, -1.0, -3.55, -2.0,
							 3.55, -1.0, -3.55, -2.0, 3.55, -2.0
		};
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords[] = { 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		
		glDrawArrays(GL_TRIANGLES, 0, 12);

		//glVertexAttribPointer(program.positionAttribute, 3, GL_FLOAT, false, 0, triangle);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
