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
#include <vector>
#include <windows.h>

//60 FPS (1 / 60) (update sixty times a second)
#define FIXED_TIMESTEP 0.01666666
#define MAX_TIMESTEPS 6

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif


SDL_Window* displayWindow;

//Function to load texture as unsigned int
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

//Sprite class to make objects out of sprite sheets
class SheetSprite {
public:
	SheetSprite() {};
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size) : textureID(textureID), u(u), v(v), width(width), height(height), size(size) {}

	void Draw(ShaderProgram *program);

	float size;
	unsigned int textureID;
	float u;
	float v;
	float height;
	float width;
};

//Draw, bind texture to sprite and draw it
void SheetSprite::Draw(ShaderProgram *program) {
	glBindTexture(GL_TEXTURE_2D, textureID);

	GLfloat texCoords[] = {
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
	};

	float aspect = width / height; 
	float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, -0.5f * size
	};

	//draw our arrays
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

//Simple vector class
class Vector3 {
public:
	Vector3() {};
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

	float x;
	float y;
	float z;
};

void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
			});
	}
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	// draw this data (use the .data() method of std::vector to get pointer to data)
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6 * text.size());

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);

}

class Entity {
public:
	Entity() {};
	void Draw(ShaderProgram *program) {
		sprite.Draw(program);
	}

	void Update(float elapsed) {

	}
	Vector3 position;
	Vector3 velocity;
	Vector3 size;

	float rotation;

	SheetSprite sprite;
};

void Render(ShaderProgram *program) {


}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	displayWindow = SDL_CreateWindow("Assignment 3: Space Invaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640 * 2, 360 * 2, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	//Set the size and offset of rendering area (in pixels)	
	glViewport(0, 0, 640 * 2, 360 * 2);

	//Load the shader program
	ShaderProgram program;
	//WE'RE GONNA WANT THE TEXTURED VERTEX SHADER AND FRAGMENT SHADER
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	//Setting up the Sprite sheet
	GLuint spriteSheetTexture = LoadTexture("sheet.png");
	//playerShip2_red.png (line 224)
	Entity player; 
	player.sprite = SheetSprite(spriteSheetTexture, 0/1024.0, 941.0/1024.0, 112.0/1024.0, 75.0/1024.0, 0.3);
	/*
	player.position.x = 0;
	player.position.y = 0;
	player.position.z = 0;
	player.size.x = 112.0 / 1024.0;
	player.size.y = 75.0 / 1024.0;
	player.size.z = 0;
	*/

	//enemyBlack3.png (line 55)
	SheetSprite enemy1 = SheetSprite(spriteSheetTexture, 144.0 / 1024.0, 156.0 / 1024.0, 103.0 / 1024.0, 84.0 / 1024.0, 0.3);

	//enemyBlue4.png (line 4)
	SheetSprite enemy2 = SheetSprite(spriteSheetTexture, 518.0 / 1024.0, 409.0 / 1024.0, 82.0 / 1024.0, 84.0 / 1024.0, 0.3);

	//enemyGreen5.png (line 70)
	SheetSprite enemy3 = SheetSprite(spriteSheetTexture, 408.0/ 1024.0, 907.0 / 1024.0, 97.0/ 1024.0, 84.0/ 1024.0, 0.3);

	//enemyRed1.png (line 74)
	SheetSprite enemy4 = SheetSprite(spriteSheetTexture, 425.0 / 1024.0, 384.0 / 1024.0, 93.0/ 1024.0, 84.0 / 1024.0, 0.3);

	//Load the matrices
	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;
	Matrix playerModelMatrix;
	Matrix enemyModelMatrix;

	//Sets an orthographic projection in a matrix
	projectionMatrix.SetOrthoProjection(-5.33f, 5.33f, -3.0f, 3.0f, -1.0f, 1.0f);

	//Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//This is how we will keep track of time
	float lastFrameTicks = 0.0;
	float angle = 0.0; 
	float accumulator = 0.0;

	//Set the color for untextured polygons
	program.SetColor(1.0f, 1.0f, 1.0f, 1.0f);

	//Set clear color of screen
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		
	//Setup initial player position
	playerModelMatrix.Translate(0, -2.25, 0);
	program.SetModelMatrix(playerModelMatrix);

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

		float ticks = (float)SDL_GetTicks() / 1000.0;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		//Keeping time with a fixed timestep
		/*elapsed += accumulator;
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}
		while (elapsed >= FIXED_TIMESTEP) {
			//Update(FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;
		//Render();
		*/

		//Pass the matrices to our program
		program.SetModelMatrix(modelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);

		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		//Player 1 moves the left paddle
		if (keys[SDL_SCANCODE_RIGHT]) {
			playerModelMatrix.Translate(elapsed * 2.5, 0.0, 0.0);
		}
		if (keys[SDL_SCANCODE_LEFT]) {
			playerModelMatrix.Translate(elapsed * -2.5, 0.0, 0.0);
		}

		program.SetModelMatrix(playerModelMatrix);

		player.Draw(&program);

		program.SetModelMatrix(modelMatrix); 

		enemy1.Draw(&program);
		enemy2.Draw(&program);
		enemy3.Draw(&program);
		enemy4.Draw(&program);


		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
