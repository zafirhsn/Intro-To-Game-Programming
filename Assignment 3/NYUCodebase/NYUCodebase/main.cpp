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

//60 FPS (1 / 60) (update sixty times a second)
#define FIXED_TIMESTEP 0.01666666
#define MAX_TIMESTEPS 6

//Object Pool for bullets
#define MAX_BULLETS 10

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

//Simple vector class
class Vector3 {
public:
	Vector3() {};
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

	float x;
	float y;
	float z;
};

class Entity {
public:
	Entity() {};
	void Draw(ShaderProgram *program) {
		sprite.Draw(program);
	}

	void Update(float elapsed) {
		position.x += elapsed * velocity.x;
		position.y += elapsed * velocity.y;
		position.z += elapsed * velocity.z;
	}

	Vector3 position;
	Vector3 velocity;
	Vector3 size;
	bool dead; 

	float rotation;

	SheetSprite sprite;
};

class GameState {
public:
	Entity player;
	Entity enemy[32];
	Entity bullets[10];
	int score;
};

enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER };

GameMode mode;
GameState state;

//Player Globals
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;
Matrix playerModelMatrix;
Matrix enemyModelMatrix;
Matrix bulletModelMatrix;

Matrix titleModelMatrix;
Matrix commandModelMatrix;

GLuint textTexture;
GLuint spriteSheetTexture;

//Audio


int bulletIndex = 0;
void ShootBullet() {
	state.bullets[bulletIndex].position.x = state.player.position.x;
	state.bullets[bulletIndex].position.y = -2.25;
	state.bullets[bulletIndex].position.z = 0.0;
	state.bullets[bulletIndex].velocity.y = 6.0;
	state.bullets[bulletIndex].velocity.x = 0.0;
	state.bullets[bulletIndex].velocity.z = 0.0;
	bulletIndex++;
	if (bulletIndex > MAX_BULLETS - 1) {
		bulletIndex = 0;
		for (int i = 0; i < MAX_BULLETS - 1; i++) {
			state.bullets[i].dead = false;
		}

	}
}

//----------PROCESS INPUT FUNCTIONS------------

//Process regular player movement in game
void ProcessGameInput(float elapsed) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	if (keys[SDL_SCANCODE_RIGHT]) {
		state.player.position.x += elapsed * 2.5;
		playerModelMatrix.Translate(elapsed * 2.5, 0.0, 0.0);
		//OutputDebugString(std::to_string(3.14));
	}
	if (keys[SDL_SCANCODE_LEFT]) {
		state.player.position.x -= elapsed * 2.5;
		playerModelMatrix.Translate(elapsed * -2.5, 0.0, 0.0);
	}

}

//Process events for menu, only polling events
void ProcessMenuPollingInput(SDL_Event& event) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
			mode = STATE_GAME_LEVEL;
		}
	}
}

//Process polling events for game (shooting)
void ProcessGamePollingInput(SDL_Event& event, bool& prevPressed) {
	if (event.type == SDL_KEYDOWN && prevPressed == false) {
		if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
			ShootBullet();
			prevPressed = true;
		}
	}
	else if (event.type == SDL_KEYUP) {
		if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
			prevPressed = false;
		}
	}
}

//-----PROCESS INPUT FOR ENTIRE GAME------
void ProcessInput(float elapsed) {
	switch (mode) {
	case STATE_MAIN_MENU:
		break;
	case STATE_GAME_LEVEL:
		ProcessGameInput(elapsed);
		break;
	}
}

void Update(float elapsed) {
	int leftIndex = 0;
	float leftMostPos = 0;
	int rightIndex = 0;
	float rightMostPos = 0;
	for (int i = 0; i < 32; i++) {
		if (state.enemy[i].dead != true) {
			state.enemy[i].Update(elapsed);
		}

		if (state.enemy[i].position.x > rightMostPos) {
			rightIndex = i;
			rightMostPos = state.enemy[i].position.x;
		}
		if (state.enemy[i].position.x < leftMostPos) {
			leftIndex = i;
			leftMostPos = state.enemy[i].position.x;
		}
	}
	if (state.enemy[leftIndex].position.x < -4.5 || state.enemy[rightIndex].position.x > 4.5) {
		for (int i = 0; i < 32; i++) {
			state.enemy[i].velocity.x *= -1.05;
			state.enemy[i].position.y -= 0.10;
		}

	}


	for (int i = 0; i < MAX_BULLETS - 1; i++) {
		if (state.bullets[i].dead != true) {
			state.bullets[i].Update(elapsed);
		}

		for (int j = 0; j < 32; j++) {
			if (state.enemy[j].dead != true) {
				if (state.bullets[i].position.x < (state.enemy[j].position.x + state.enemy[j].size.x / 2) &&
					state.bullets[i].position.x >(state.enemy[j].position.x - state.enemy[j].size.x / 2)) {
					if (state.bullets[i].position.y < (state.enemy[j].position.y + state.enemy[j].size.y / 2) &&
						state.bullets[i].position.y >(state.enemy[j].position.y - state.enemy[j].size.y / 2)) {
						state.enemy[j].dead = true;
						state.enemy[j].position.x = 0;
						state.enemy[j].position.y = -200;
						state.enemy[j].position.z = 0.0;

						state.bullets[i].dead = true;
						state.bullets[i].position.x = 200.0;
						state.bullets[i].position.y = -2.25;
						state.bullets[i].position.z = 0.0;
					}
				}
			}
		}
	}

}

void RenderMenu(ShaderProgram *program) {
	program->SetModelMatrix(titleModelMatrix);
	DrawText(program, textTexture, "Space Invaders", 0.30, 0.05);
	program->SetModelMatrix(commandModelMatrix);
	DrawText(program, textTexture, "Press SPACE to Start", 0.20, -0.05);
}

void Render(ShaderProgram *program) {
	switch (mode) {
	case STATE_MAIN_MENU:
		RenderMenu(program);
		break;
	case STATE_GAME_LEVEL:
		program->SetModelMatrix(playerModelMatrix);
		state.player.Draw(program);

		for (int i = 0; i < 32; i++) {
			if (state.enemy[i].dead == false) {
				enemyModelMatrix.SetPosition(state.enemy[i].position.x, state.enemy[i].position.y, 0.0);
				program->SetModelMatrix(enemyModelMatrix);
				state.enemy[i].Draw(program);
			}
		}

		for (int i = 0; i < MAX_BULLETS - 1; i++) {
			if (state.bullets[i].dead == false) {
				bulletModelMatrix.SetPosition(state.bullets[i].position.x, state.bullets[i].position.y, 0.0);
				program->SetModelMatrix(bulletModelMatrix);
				state.bullets[i].Draw(program);
			}
		}

		break;
	}
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
	spriteSheetTexture = LoadTexture("sheet.png");
	//Setting up the text sheet
	textTexture = LoadTexture(RESOURCE_FOLDER"pixel_font.png");
	
	//playerShip2_red.png (line 224) 
	state.player.sprite = SheetSprite(spriteSheetTexture, 0/1024.0, 941.0/1024.0, 112.0/1024.0, 75.0/1024.0, 0.3);
	state.player.position.x = 0;
	state.player.position.y = 0;
	state.player.position.z = 0;
	state.player.size.x = 112.0 / 1024.0;
	state.player.size.z = 0;

	
	for (int i = 0; i < 32; i++) {
		if (i < 8) {
			//enemyBlack3.png (line 55)
			state.enemy[i].sprite = SheetSprite(spriteSheetTexture, 144.0 / 1024.0, 156.0 / 1024.0, 103.0 / 1024.0, 84.0 / 1024.0, 0.3);
		}
		else if (i < 16) {
			//enemyBlue4.png (line 4)
			state.enemy[i].sprite = SheetSprite(spriteSheetTexture, 518.0 / 1024.0, 409.0 / 1024.0, 82.0 / 1024.0, 84.0 / 1024.0, 0.3);
		}
		else if (i < 24) {
			//enemyGreen5.png (line 70)
			state.enemy[i].sprite = SheetSprite(spriteSheetTexture, 408.0 / 1024.0, 907.0 / 1024.0, 97.0 / 1024.0, 84.0 / 1024.0, 0.3);
		}
		
		else if (i < 32) {
			//enemyRed1.png (line 74)
			state.enemy[i].sprite = SheetSprite(spriteSheetTexture, 425.0 / 1024.0, 384.0 / 1024.0, 93.0 / 1024.0, 84.0 / 1024.0, 0.3);
		}
		state.enemy[i].size.x = 0.3;
		state.enemy[i].size.y = 0.3;
		state.enemy[i].size.z = 0.0;
		state.enemy[i].dead = false;
	}

	//Bullet sprite
	for (int i = 0; i < 10; i++) {
		state.bullets[i].sprite = SheetSprite(spriteSheetTexture, 843.0 / 1024.0, 426.0 / 1024.0, 13.0 / 1024.0, 54.0 / 1024.0, 0.3);
		state.bullets[i].size.x = 0.3;
		state.bullets[i].size.y = 0.3;
		state.bullets[i].size.z = 0.0;
		state.bullets[i].dead = false;
	}

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

	//Setup intial bullets position
	for (int i = 0; i < MAX_BULLETS - 1; i++) {
		state.bullets[i].position.x = 200.0;
		state.bullets[i].position.y = -2.25;
		state.bullets[i].position.z = 0.0;
	}

	//Setup initial enemy velocities
	for (int i = 0; i < 32; i++) {
		state.enemy[i].velocity.x = 0.5;
		state.enemy[i].velocity.y = 0.0;
		state.enemy[i].velocity.z = 0.0;

	}

	float j = -3.5;
	float k = 2.5;
	for (int i = 0; i < 32; i++) {
		state.enemy[i].position.x = j;
		state.enemy[i].position.y = k;
		state.enemy[i].position.z = 0.0;
		if (j < 3) { j++; }
		else { 
			j = -3.5;
			k -= 0.5;
		}
	}

	//Setup intial Menu text positions
	titleModelMatrix.Translate(-2.25, 0.75, 0.0);
	commandModelMatrix.Translate(-1.30, -0.25, 0.0);

	mode = STATE_MAIN_MENU;

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	Mix_Chunk *laserSound;
	laserSound = Mix_LoadWAV(RESOURCE_FOLDER"laser_gun.wav");

	Mix_PlayChannel(-1, laserSound, 0);

	SDL_Event event;
	bool done = false;
	bool spaceDown = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}

			switch (mode) {
			case STATE_MAIN_MENU:
				ProcessMenuPollingInput(event);
				spaceDown = true;
				break;
			case STATE_GAME_LEVEL: 
				ProcessGamePollingInput(event, spaceDown);
				break;
			}

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
			ProcessInput(FIXED_TIMESTEP);
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
