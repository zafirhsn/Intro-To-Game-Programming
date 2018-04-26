#define STB_IMAGE_IMPLEMENTATION
#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#include "stb_image.h"
#include <vector>
#include <windows.h>
#include <iostream>
#include <string>
#include <cmath>
#include <vector>

//60 FPS (1 / 60) (update sixty times a second)
#define FIXED_TIMESTEP 0.01666666
#define MAX_TIMESTEPS 6

//Level Width and Height
#define LEVEL_WIDTH 5
#define LEVEL_HEIGHT 5
#define SPRITE_COUNT_X 30
#define SPRITE_COUNT_Y 30
#define TILE_SIZE float(0.5)

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

unsigned int levelData[LEVEL_HEIGHT][LEVEL_WIDTH] = 
{
	{1,4,1,1,1},
	{1,1,1,1,1},
	{ 1,1,16,1,1 },
	{ 1,7,10,1,1 },
	{ 1,9,1,1,1 }
};

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

//Uniform Sprite class to make objects out of sprite sheets
class SheetSprite {
public:
	SheetSprite() : textureID(0), size(0), index(0), spriteCountX(0), spriteCountY(0) {};
	SheetSprite(unsigned int textureID, int index, float size) : textureID(textureID), index(index), size(size), spriteCountX(SPRITE_COUNT_X), spriteCountY(SPRITE_COUNT_Y) {};												
	void Draw(ShaderProgram* program) const;
	float size;
	unsigned int textureID;
	int index;
	int spriteCountX;
	int spriteCountY;
};

//Draw, bind texture to sprite and draw it
void SheetSprite::Draw(ShaderProgram* program) const {
	if (index >= 0) {
		glBindTexture(GL_TEXTURE_2D, textureID);
		float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
		float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
		float spriteWidth = 1.0 / (float)spriteCountX;
		float spriteHeight = 1.0 / (float)spriteCountY;

		float texCoords[] = {
			u, v + spriteHeight,
			u + spriteWidth, v,
			u, v,
			u + spriteWidth, v,
			u, v + spriteHeight,
			u + spriteWidth, v + spriteHeight
		};
		float vertices[] = {
			-0.5f*size, -0.5f*size,
			0.5f*size, 0.5f*size,
			-0.5f*size, 0.5f*size,
			0.5f*size, 0.5f*size,
			-0.5f*size,-0.5f*size,
			0.5f*size, -0.5f*size };

		// draw this data
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

}

//Simple vector class
class Vector3 {
public:
	Vector3():  x(0), y(0), z(0) {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

	float x;
	float y;
	float z;
};

void DrawMap(ShaderProgram *program, int texture) {
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	for (int y = 0; y < LEVEL_HEIGHT; y++) {
		for (int x = 0; x < LEVEL_WIDTH; x++) {
			if (levelData[y][x] != 0) {
				float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
				float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
				float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
				vertexData.insert(vertexData.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
					});
				texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),
					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
					});
			}
		}
	}
	//glBindTexture(GL_TEXTURE_2D, texture);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	//Draw this data 
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);

}

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



float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t * v1;
}

enum EntityType { ENTITY_PLAYER, ENTITY_ENEMY, ENTITY_COIN };

class Entity {
public:
	Entity() {};
	void Draw(ShaderProgram *program) {
		sprite.Draw(program);
	}

	void Update(float elapsed) {
		velocity.x = lerp(velocity.x, 0.0, elapsed * 0.1);
		velocity.y = lerp(velocity.y, 0.0, elapsed * 0.1);
		velocity.z = 0.0;

		velocity.y += acceleration.y * elapsed;
		velocity.x += acceleration.x * elapsed; 
		velocity.z = 0.0;

		position.y += velocity.y * elapsed;

		position.x += velocity.x * elapsed;
 
		position.z += velocity.z * elapsed;
	}

	bool CollidesWith(Entity *entity) {
		//R1 bottom > R2 top
		if ((position.y - (size.y / 2)) > (entity->position.y + (entity->size.y / 2)) ||
			//R1 top < R2 bottom
			(position.y + (size.y / 2)) < (entity->position.y - (entity->size.y / 2)) ||
			//R1 left > R2 right
			(position.x - (size.x / 2)) > (entity->position.x + (entity->size.x / 2)) ||
			//R1 right < R2 left
			(position.x + (size.x / 2)) < (entity->position.x - (entity->size.x / 2))) {
			collidedBottom = false;
			collidedTop = false;
			collidedLeft = false;
			collidedRight = false;
			return false;
		}
		else {
			if ((position.y - (size.y / 2)) <= (entity->position.y + (entity->size.y / 2))) {
				collidedBottom = true;
			}
			if ((position.y + (size.y / 2)) >= (entity->position.y - (entity->size.y / 2))) {
				collidedTop = true;
			}
			if ((position.x - (size.x / 2)) <= (entity->position.x + (entity->size.x / 2))) {
				collidedRight = true;
			}
			if ((position.x + (size.x / 2)) >= (entity->position.x - (entity->size.x / 2))) {
				collidedLeft = true;
			}
			return true;
		}

	}

	Vector3 position;
	Vector3 velocity;
	Vector3 acceleration;
	Vector3 size;

	float rotation;

	SheetSprite sprite;

	bool isStatic;
	EntityType enityType;

	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
};

class GameState {
public:
	Entity player;
	Entity enemies[12];
	Entity bullets[10];
	int score;
};

enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER };

GameMode mode;
GameState state;

GLuint spriteSheetTexture;
GLuint textTex;
GLuint tileTexture;

//Load the matrices
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;
Matrix playerModelMatrix;
Matrix enemyModelMatrix;
Matrix titleModelMatrix;
Matrix tileModelMatrix;

//Process polling events for game (shooting)
void ProcessGamePollingInput(SDL_Event& event, bool& prevPressed) {
	if (event.type == SDL_KEYDOWN && prevPressed == false) {
		if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
			state.player.velocity.y = 1.5;
			prevPressed = true;
		}
	}
	else if (event.type == SDL_KEYUP) {
		if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
			prevPressed = false;
		}
	}
}

//Process regular player movement in game
void ProcessGameInput(float elapsed) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	if (keys[SDL_SCANCODE_RIGHT]) {
		//state.player.position.x -= cos(90 * elapsed);
		playerModelMatrix.Rotate(elapsed * (90 * (3.1415926/180)) * -1);
	}
	if (keys[SDL_SCANCODE_LEFT]) {
		//state.player.position.x += cos(90 * elapsed);
		playerModelMatrix.Rotate(elapsed * (90 * (3.1415926 / 180)));
	}
}

void Update(float elapsed) {
	state.player.Update(elapsed);

}


void Render(ShaderProgram *program) {
	playerModelMatrix.SetPosition(state.player.position.x, state.player.position.y, 0.0);
	program->SetModelMatrix(playerModelMatrix);
	state.player.Draw(program);

	program->SetModelMatrix(tileModelMatrix);
	DrawMap(program, tileTexture);
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	displayWindow = SDL_CreateWindow("Assignment 4: Platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640 * 2, 360 * 2, SDL_WINDOW_OPENGL);
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
	spriteSheetTexture = LoadTexture("spritesheet_rgba.png");
	textTex = LoadTexture("font1.png");

	//playerShip2_red.png (line 224)
	state.player.sprite = SheetSprite(spriteSheetTexture, 19, 0.5);
	state.player.acceleration.x = 0.0;
	state.player.size.x = 0.5;
	state.player.size.y = 0.5;
	state.player.isStatic = false;
	state.player.enityType = ENTITY_PLAYER;

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

	mode = STATE_GAME_LEVEL;

	

	SDL_Event event;
	bool done = false;
	bool spaceDown = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}

			ProcessGamePollingInput(event, spaceDown);
		}

		glClear(GL_COLOR_BUFFER_BIT);

		//Use the specified program ID
		glUseProgram(program.programID);

		//Pass the matrices to our program
		program.SetModelMatrix(modelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);

		float ticks = (float)SDL_GetTicks() / 1000.0;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		//Keeping time with a fixed timestep
		elapsed += accumulator;
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}
		while (elapsed >= FIXED_TIMESTEP) {
			ProcessGameInput(FIXED_TIMESTEP);
			Update(FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;

		Render(&program);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
