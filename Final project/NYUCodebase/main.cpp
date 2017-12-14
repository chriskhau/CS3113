#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#include <SDL_mixer.h>
using namespace std;

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
enum GameState {GAME,MENU,WIN,GAME_OVER};
enum Difficulty {EASY,MEDIUM,HARD};
Difficulty difficulty;
GameState currState = MENU;
int counter = 0;

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
class Vector3 {
public:
	Vector3() {
		x = 0;
		y = 0;
		z = 0;
	}
	Vector3(float x, float y, float z) :x(x), y(y), z(z) {}
	float x;
	float y;
	float z;
};
void drawText(ShaderProgram& program, int fontTexture, string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;	
	vector<float> VertexData;
	vector<float> TexCoordData;

	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];

		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;

		VertexData.insert(VertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});

		TexCoordData.insert(TexCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}

	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, VertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, TexCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, VertexData.size()/2);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
	


}

class SheetSprite {
public:
	SheetSprite();
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float
		size) : textureID(textureID), u(u), v(v), width(width), height(height), size(size) {}

	void Draw(ShaderProgram *program);

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

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
		0.5f * size * aspect, -0.5f * size };

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

class Entity{
public:
	Entity() {};
	Entity(Vector3 position, Vector3 velocity, SheetSprite sprite, bool alive)
		: position(position), velocity(velocity), sprite(sprite), alive(alive) {}

	Vector3 position;
	Vector3 velocity;
	//Vector3 size;

	SheetSprite sprite;
	bool alive;

	bool collidedLeft = false;
	bool colliededRight = false;

	void draw(ShaderProgram& program);
	bool collisionDetection(Entity& x);
};
void Entity::draw(ShaderProgram& p) {
	Matrix modelview;
	modelview.Identity();
	modelview.Translate(position.x, position.y, 0);
	p.SetModelviewMatrix(modelview);
	sprite.Draw(&p);
}
bool Entity::collisionDetection(Entity& b) {
	float left = position.x - sprite.size * (sprite.width / sprite.height) / 2;
	float right = position.x + sprite.size * (sprite.width / sprite.height) / 2;
	float top = position.y + sprite.size / 2;
	float bottom = position.y - sprite.size / 2;
	float bleft = b.position.x - b.sprite.size *(b.sprite.width / b.sprite.height) / 2;
	float bright = b.position.x + b.sprite.size *(b.sprite.width / b.sprite.height) / 2;
	float btop = b.position.y + b.sprite.size / 2;
	float bbottom = b.position.y - b.sprite.size / 2;
	/*
	if (((bleft >= left && bright <= right) || (bright >= left && bright <= right)) &&
		((bbottom >= bottom && bbottom <= top) || (btop <= top && btop >= top)))
		{
		return true;
}*/
	if ((left <= bright) && (right >= bleft) && (bottom <= btop) && (top >= bbottom)) {
		return true;
	}
	else {
		return false;
	}
}

void renderGame(ShaderProgram& p, Entity& player1, Entity& bullet1, Entity& player2, Entity& bullet2,
	vector<Entity>& enemies, vector<Entity>& enemybullets, float elapsed) {
	if (player1.alive) {
		player1.draw(p);
	}
	if (player2.alive) {
		player2.draw(p);
	}
	bullet1.draw(p);
	bullet2.draw(p);
	for (int i = 0; i < enemies.size(); i++) {
		if (enemies[i].alive) {
			enemies[i].draw(p);
		}
	}
	for (int i = 0; i < enemybullets.size(); i++) {
		enemybullets[i].draw(p);
	}
}
void updateGame(ShaderProgram& p, Entity& player1, Entity& bullet1, Entity& player2, Entity& bullet2,
	vector<Entity>& enemies, vector<Entity>& enemybullets, float elapsed, Mix_Chunk *explosion, Mix_Chunk *playerExplosion) {
	if (bullet1.position.y <= 3) {
		bullet1.position.y += bullet1.velocity.y * elapsed;
	}
	if (bullet2.position.y <= 3) {
		bullet2.position.y += bullet2.velocity.y * elapsed;
	}
	for (int i = 0; i < enemybullets.size(); i++) {
		if (enemybullets[i].position.y >= -3) {
			enemybullets[i].position.y -= enemybullets[i].velocity.y *elapsed; //movement
		}
		else { enemybullets[i].alive = false; }
		//playerHitDetection
		if (player1.alive) {
			if (player1.collisionDetection(enemybullets[i])) {
				Mix_PlayChannel(-1, playerExplosion, 0);
				player1.alive = false;
				enemybullets[i].position.y = -3;
			}
		}
		if (player2.alive) {
			if (player2.collisionDetection(enemybullets[i])) {
				Mix_PlayChannel(-1, playerExplosion, 0);
				player2.alive = false;
				enemybullets[i].position.y = -3;
			}
		}
	}
	int winCounter = 0;
	for (int i = 0; i < enemies.size(); i++) {
		enemies[i].position.x += enemies[i].velocity.x * elapsed; //movement
		//enemyHitDetection
		if (enemies[i].alive) {
			winCounter += 1; //keeps track of how many enemies still alive
			if (enemies[i].collisionDetection(bullet1)) {
				Mix_PlayChannel(-1, explosion, 0);
				enemies[i].alive = false;
				bullet1.position.y = 3;
			}
			if (enemies[i].collisionDetection(bullet2)) {
				Mix_PlayChannel(-1, explosion, 0);
				enemies[i].alive = false;
				bullet2.position.y = 3;
			}
			//chance to shoot
			int r = rand() % 30000;
			if (r == 0) {
					if (enemybullets[counter].position.y <= -2.6) {
						if (difficulty == HARD) {
							enemybullets[counter].position.x = enemies[i].position.x - enemies[i].sprite.width ;
							enemybullets[counter].position.y = enemies[i].position.y - enemies[i].sprite.height / 2;
							counter++;
							enemybullets[counter].position.x = enemies[i].position.x + enemies[i].sprite.width ;
							enemybullets[counter].position.y = enemies[i].position.y - enemies[i].sprite.height / 2;
							counter++;
							if (counter > enemybullets.size() - 2) {
								counter = 0;
							}
						}
							if (difficulty == EASY || difficulty == MEDIUM) {
								enemybullets[counter].position.x = enemies[i].position.x;
								enemybullets[counter].position.y = enemies[i].position.y - enemies[i].sprite.height / 2;
								counter++;
								if (counter > enemybullets.size() - 1) {
									counter = 0;

								}
							}
						
				}
			}
		}
	}
	//if no enemies are alive winCounter should be zero
	if (winCounter == 0) {
		currState = WIN;
	}
	int a = abs(enemies[0].velocity.x); //to make sure enemies do not get stuck
	if (enemies[0].position.x <= -3.5) {
		if (enemies[0].position.y > 0.5) {
			for (int i = 0; i < enemies.size(); i++) {
				enemies[i].position.y -= 0.05;
			}
		}
		for (int i = 0; i < enemies.size(); i++) {
			enemies[i].velocity.x = a;
		}
	}
	else if (enemies[enemies.size() - 1].position.x >= 3.5) {
		if (enemies[0].position.y > 0.5) {
			for (int i = 0; i < enemies.size(); i++) {
				enemies[i].position.y -= 0.05;
			}
		}
		for (int i = 0; i < enemies.size(); i++) {
			enemies[i].velocity.x = -a;
		}
	}
	if (player1.alive == false && player2.alive == false) {
		currState = GAME_OVER;
	}
}
void processInput(Entity& player1, Entity& player2, Entity& bullet1, Entity& bullet2,
	vector<Entity>& enemies,vector<Entity>& enemybullets, float elapsed,bool& done,
	SheetSprite enemy1ship,SheetSprite enemy2ship, SheetSprite enemyshot, Mix_Chunk* shootSound) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	switch (currState) {
	case MENU:
		if (keys[SDL_SCANCODE_1]) {
			enemies.pop_back();
			for (int i = 0; i < 8; i++) {
				for (int j = 0; j < 3; j++) {
					enemies.push_back(Entity(Vector3(-3.5 + .4*i, 1.5 - .3*j, 0), Vector3(1.5, 0, 0), enemy1ship, true));
				}
			}
			difficulty = EASY;
			currState = GAME;
		}
		if (keys[SDL_SCANCODE_2]) {
			enemies.pop_back();
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 2; j++) {
					enemies.push_back(Entity(Vector3(-3.5 + .7*i, 1.5 - .3*j, 0), Vector3(1.75, 0, 0), enemy2ship, true));
				}
			}
			for (int i = 0; i < 7; i++) {
				for (int j = 0; j < 2; j++) {
					enemies.push_back(Entity(Vector3(-3.5 + .5*i, .9 - .3*j, 0), Vector3(1.75, 0, 0), enemy1ship, true));
				}
			}
			//load lvl 2
			//for (int )
			difficulty = MEDIUM;
			currState = GAME;
		}
		if (keys[SDL_SCANCODE_3]) {
			//load lvl 3
			enemies.pop_back();
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 4; j++) {
					enemies.push_back(Entity(Vector3(-3.5 + .7*i, 1.5 - .3*j, 0), Vector3(2, 0, 0), enemy2ship, true));
				}
			}
			for (int i = 0; i < 10; i++) {
				enemybullets.push_back(Entity(Vector3(-4, -4, 0), Vector3(0, 1.75, 0), enemyshot, true));
			}
			difficulty = HARD;
			currState = GAME;
		}
		if (keys[SDL_SCANCODE_ESCAPE]) {
			//quits game
			done = true;
		}
		break;
	case GAME:
		if (keys[SDL_SCANCODE_A] && player1.position.x >= -3.4) {
			player1.position.x -= 1.5 * elapsed;
		}
		if (keys[SDL_SCANCODE_D] && player1.position.x <= 3.4) {
			player1.position.x += 1.5 * elapsed;
		}
		if (keys[SDL_SCANCODE_S]) {
			if (bullet1.position.y > 2) {
				Mix_PlayChannel(-1, shootSound, 0);
				bullet1.position.x = player1.position.x;
				bullet1.position.y = player1.position.y;
			}
		}
		if (keys[SDL_SCANCODE_LEFT] && player2.position.x >= -3.4) {
			player2.position.x -= 1.5* elapsed;
		}
		if (keys[SDL_SCANCODE_RIGHT] && player2.position.x <= 3.4) {
			player2.position.x += 1.5*elapsed;
		}
		if (keys[SDL_SCANCODE_DOWN]) {
			if (bullet2.position.y > 2) {
				Mix_PlayChannel(-1, shootSound, 0);
				bullet2.position.x = player2.position.x;
				bullet2.position.y = player2.position.y;
			}
		}
		break;
	case GAME_OVER:
		if (keys[SDL_SCANCODE_ESCAPE]) {
			done = true;
		}
		break;
	case WIN:
		if (keys[SDL_SCANCODE_ESCAPE]) {
			done = true;
		}
	}


};
void renderTitle(ShaderProgram& p, GLuint& texture) {
	Matrix modelview;
	modelview.Identity();
	modelview.Translate(-2.4, 1.5, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "Almost Space Invaders", 0.5, -.25);
	modelview.Identity();
	modelview.Translate(-1.8, .75, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "Choose your difficulty with 1,2,3", 0.2, -.1);
	modelview.Identity();
	modelview.Translate(-.5, .50, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "1 - Easy", 0.2, -.1);
	modelview.Identity();
	modelview.Translate(-.5, .25, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "2 - Medium", 0.2, -.1);
	modelview.Identity();
	modelview.Translate(-.5, .0, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "3 - Hard", 0.2, -.1);
	modelview.Identity();
	modelview.Translate(-2.5, -.5, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "Player 1 Controls", 0.2, -.1);
	modelview.Identity();
	modelview.Translate(-2.5, -.75, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "A and D to move", 0.2, -.1);
	modelview.Identity();
	modelview.Translate(-2.5, -1, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "S to shoot", 0.2, -.1);
	modelview.Identity();
	modelview.Translate(1, -.5, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "Player 2 Controls", 0.2, -.1);
	modelview.Identity();
	modelview.Translate(1, -.75, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "Left and Right to move", 0.2, -.1);
	modelview.Identity();
	modelview.Translate(1, -1, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "Down to shoot", 0.2, -.1);

}
void renderGameOver(ShaderProgram& p, GLuint & texture) {
	Matrix modelview;
	modelview.Identity();
	modelview.Translate(-1.6, 1.2, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "Game Over", 0.5, -.2);
	modelview.Identity();
	modelview.Translate(-1.6, 0, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "Press esc to quit", 0.3, -.1);
}
void renderGameWin(ShaderProgram& p, GLuint & texture) {
	Matrix modelview;
	modelview.Identity();
	modelview.Translate(-1.6, 1.2, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "You Win!", 0.5, -.2);
	modelview.Identity();
	modelview.Translate(-1.6, 0, 0);
	p.SetModelviewMatrix(modelview);
	drawText(p, texture, "Press esc to quit", 0.3, -.1);

}

void Render(ShaderProgram& p, Entity& player1, Entity& bullet1, Entity& player2, Entity& bullet2,
	vector<Entity>& enemies, vector<Entity>& enemybullets, float elapsed,GLuint texture) {
	switch (currState) {
	case MENU:
		renderTitle(p, texture);
		break;
	case GAME:
		renderGame(p, player1, bullet1, player2, bullet2, enemies, enemybullets, elapsed);
		break;

	case GAME_OVER:
		renderGameOver(p, texture);
		break;
	case WIN:
		renderGameWin(p, texture);
		break;
	}
}


int main(int argc, char *argv[]) {

	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 360);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	GLuint spriteSheetTexture = LoadTexture(RESOURCE_FOLDER"sheet.png");
	GLuint textTexture = LoadTexture(RESOURCE_FOLDER"font1.png");

	//GLuint spriteSheetTexture = LoadTexture("sheet.png");
	//mySprite = SheetSprite(spriteSheetTexture, 425.0f / 1024.0f, 468.0f / 1024.0f, 93.0f / 1024.0f, 84.0f /
	//	1024.0f, 0.2f);

	SheetSprite player1ship(spriteSheetTexture, 346.0f / 1024.0f, 75.0f / 1024.0f, 98.0f / 1024.0f, 75.0f / 1024.0f, 0.3f);
	SheetSprite player2ship(spriteSheetTexture, 325.0f / 1024.0f, 0.0f / 1024.0f, 98.0f / 1024.0f, 75.0f / 1024.0f, 0.3f);
	SheetSprite player1shot(spriteSheetTexture, 854.0f / 1024.0f, 639.0f / 1024.0f, 9.0f / 1024.0f, 37.0f / 1024.0f, 0.2f);
	SheetSprite player2shot(spriteSheetTexture, 856.0f / 1024.0f, 602.0f / 1024.0f, 9.0f / 1024.0f, 37.0f / 1024.0f, 0.2f);

	SheetSprite enemy1ship(spriteSheetTexture, 425.0f / 1024.0f, 468.0f / 1024.0f, 93.0f / 1024.0f, 84.0f / 1024.0f, 0.3f);
	SheetSprite enemy2ship(spriteSheetTexture, 222.0f / 1024.0f, 0.0f / 1024.0f, 103.0f / 1024.0f, 84.0f / 1024.0f, 0.3f);
	SheetSprite enemyshot(spriteSheetTexture, 856.0f / 1024.0f, 57.0f / 1024.0f, 9.0f / 1024.0f, 37.0f / 1024.0f, 0.2f);

	SheetSprite meteor1(spriteSheetTexture, 651.0f / 1024.0f, 447.0f / 1024.0f, 43.0f / 1024.0f, 43.0f / 1024.0f, 0.3f);
	SheetSprite meteor2(spriteSheetTexture, 237.0f / 1024.0f, 452.0f / 1024.0f, 45.0f / 1024.0f, 40.0f / 1024.0f, 0.3f);


	Entity player1(Vector3(0.0, -1.8, 0), Vector3(0, 0, 0), player1ship, true);
	Entity bullet1(Vector3(5, 5, 0), Vector3(0, 1.5, 0), player1shot, true);
	Entity player2(Vector3(0.3, -1.8, 0), Vector3(0, 0, 0), player2ship, true);
	Entity bullet2(Vector3(5, 5, 0), Vector3(0, 1.5, 0), player2shot, true);
	vector<Entity> enemies;
	vector<Entity> enemybullets;
	vector<Entity> meteors;

	enemies.push_back(Entity(Vector3(-3.5, 1.5, 0), Vector3(1.5, 0, 0), enemy1ship, true));
	for (int i = 0; i < 10; i++) {
		enemybullets.push_back(Entity(Vector3(-4, -4, 0), Vector3(0, 1.75, 0), enemyshot, true));
	}
	for (int i = 0; i < 10; i++) {
		if (i % 2 == 0) {
			meteors.push_back(Entity(Vector3(-4, -4, 0), Vector3(0, 1.75, 0), meteor1, true));
		}
		if (i % 2 == 1) {
			meteors.push_back(Entity(Vector3(-4, -4, 0), Vector3(0, 1.3, 0), meteor2, true));
		}
	}

	Matrix projectionMatrix;
	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	Matrix modelviewMatrix;

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

	Mix_Chunk *shootSound;
	shootSound = Mix_LoadWAV("shoot.wav");
	Mix_Chunk *explosionSound;
	explosionSound = Mix_LoadWAV("explosion.wav");
	Mix_Chunk *playerExplosion;
	playerExplosion = Mix_LoadWAV("explosion_player.wav");

	Mix_Music *backgroundMusic;
	backgroundMusic = Mix_LoadMUS("BoxCat_Games_-_03_-_Battle_Special.mp3");

	Mix_PlayMusic(backgroundMusic, -1);


	float lastFrameTick = 0.0f;

	SDL_Event event;
	bool done = false;
	while (!done) {
		float ticks = (float)SDL_GetTicks() / 1000;
		float elapsed = ticks - lastFrameTick;
		lastFrameTick = ticks;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(program.programID);


		program.SetProjectionMatrix(projectionMatrix);
		program.SetModelviewMatrix(modelviewMatrix);

		processInput(player1, player2, bullet1, bullet2,enemies,enemybullets, elapsed,done,enemy1ship,enemy2ship,enemyshot,shootSound);
		Render(program, player1, bullet1, player2, bullet2, enemies, enemybullets, elapsed,textTexture);
		updateGame(program, player1, bullet1, player2, bullet2, enemies, enemybullets, elapsed,explosionSound,playerExplosion);
		SDL_GL_SwapWindow(displayWindow);
	}
	Mix_FreeChunk(shootSound);
	Mix_FreeChunk(explosionSound);
	Mix_FreeChunk(playerExplosion);
	Mix_FreeMusic(backgroundMusic);


	SDL_Quit();
	return 0;
}