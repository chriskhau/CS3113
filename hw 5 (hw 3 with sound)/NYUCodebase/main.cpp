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
GLuint LoadTexture(const char);


class Bullet {
public:
	Bullet() {
		x = 4.0;
		y = 0;
		vel = 0.0;
		aLive = 0;
		width = 0.05;
		height = 0.2;
	}

	void Draw(ShaderProgram*p, GLuint image);

	float x;
	float y;
	float width;
	float height;

	float vel;
	float aLive; //0 = not shot, 1 = shot
	float side;

};
class Player {
public:
	Player(float xPos, float yPos) {
		x = xPos;
		y = yPos;
		width = 0.3;
		height = 0.3;
		aLive = 1;
	}
	void Draw(ShaderProgram *p, GLuint image);
	bool hitDetection(Bullet b[]);

	float x;
	float y;
	float width;
	float height;

	int aLive;
};
class Enemy {
public:
	Enemy() {}
	Enemy(float xPos, float yPos) {
		x = xPos;
		y = yPos;
		height = 0.25;
		width = 0.25;
		aLive = 1;
		xVel = 0.2;
	}
	void Draw(ShaderProgram *p, GLuint image);
	bool hitDetection(Bullet b,Mix_Chunk *explosion);
	void ChanceToShoot(Bullet b[], int& counter);

	float x;
	float y;
	float width;
	float height;

	float xVel;

	int aLive;
};
bool Player::hitDetection(Bullet b[]) {
	for (int i = 0; i < 14; i++) {
		if (((b[i].x >= x && b[i].x <= x + width) || (b[i].x + b[i].width >= x && b[i].x + b[i].width <= x + width)) &&
			((b[i].y >= y && b[i].y <= y + height) || (b[i].y + b[i].height <= y + height && b[i].y + b[i].height >= y))
			&& aLive == 1) {
			aLive = 0;
			return true;
		}
		return false;
		
	}
}
void Enemy::ChanceToShoot(Bullet b[], int& counter) {
	int cts = rand() % 100;
	if (cts == 0 && aLive == 1) {
		if (b[counter].aLive == 0) {
			b[counter].x = x + width / 2;
			b[counter].y = y;
			b[counter].vel = -1;
			b[counter].aLive = 1;
			if (counter == 14) {
				counter = 0;
			}
			else {
				counter++;
			}
		}
	}

}
bool Enemy::hitDetection(Bullet b,Mix_Chunk *explosion) {
	if (((b.x >= x && b.x <= x + width) || (b.x + b.width >= x && b.x + b.width <= x + width)) &&
		((b.y >= y && b.y <= y + height) || (b.y + b.height <= y + height && b.y + b.height >= y))
		&& aLive == 1) {
		aLive = 0;
		Mix_PlayChannel(-1, explosion, 0);
		return true;
	}
	else {
		return false;
	}
	/*
	if (b.x<=x + width && b.x + b.width >= x && b.x+b.height >= y && b.x+b.height <= y+height) {
	aLive = 0;
	}*/
}
void Bullet::Draw(ShaderProgram*p, GLuint image) {
	glBindTexture(GL_TEXTURE_2D, image);

	float vertices[] = { x, y, x + width, y, x + width, y + height,
		x,y,x + width,y + height,x,y + height };
	glVertexAttribPointer(p->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(p->positionAttribute);

	float texCoords[] = { 0.0f,1.0f, 1.0f,1.0f, 1.0f,0.0f,
		0.0f,1.0f, 1.0f,0.0f, 0.0f,0.0f };
	glVertexAttribPointer(p->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(p->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(p->positionAttribute);
	glDisableVertexAttribArray(p->texCoordAttribute);
}
void Player::Draw(ShaderProgram*p, GLuint image) {
	if (aLive == 1) {
		glBindTexture(GL_TEXTURE_2D, image);

		float vertices[] = { x, y, x + width, y, x + width, y + height,
			x,y,x + width,y + height,x,y + height };
		glVertexAttribPointer(p->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(p->positionAttribute);

		float texCoords[] = { 0.0f,1.0f, 1.0f,1.0f, 1.0f,0.0f,
			0.0f,1.0f, 1.0f,0.0f, 0.0f,0.0f };
		glVertexAttribPointer(p->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(p->texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(p->positionAttribute);
		glDisableVertexAttribArray(p->texCoordAttribute);
	}
}
void Enemy::Draw(ShaderProgram*p, GLuint image) {
	if (aLive == 1) {
		glBindTexture(GL_TEXTURE_2D, image);

		float vertices[] = { x, y, x + width, y, x + width, y + height,
			x,y,x + width,y + height,x,y + height };
		glVertexAttribPointer(p->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(p->positionAttribute);

		float texCoords[] = { 0.0f,1.0f, 1.0f,1.0f, 1.0f,0.0f,
			0.0f,1.0f, 1.0f,0.0f, 0.0f,0.0f };
		glVertexAttribPointer(p->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(p->texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(p->positionAttribute);
		glDisableVertexAttribArray(p->texCoordAttribute);
	}

}


class SheetSprite {
public:
	SheetSprite();
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float
		size);

	void Draw(ShaderProgram *p);

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};
void SheetSprite::Draw(ShaderProgram *p) {
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
		-0.5f * size * aspect, -0.5f * size ,
		0.5f * size * aspect, -0.5f * size };
	// draw our arrays
	glVertexAttribPointer(p->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(p->positionAttribute);

	glVertexAttribPointer(p->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(p->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(p->positionAttribute);
	glDisableVertexAttribArray(p->texCoordAttribute);
}

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

int main(int argc, char *argv[]) {

	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 360);

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	//GLuint spriteSheetTexture = LoadTexture("sheet.png");
	//mySprite = SheetSprite(spriteSheetTexture, 425.0f / 1024.0f, 468.0f / 1024.0f, 93.0f / 1024.0f, 84.0f /
	//	1024.0f, 0.2f);

	GLuint enemy1 = LoadTexture(RESOURCE_FOLDER"enemyBlue1.png");
	GLuint playerModel = LoadTexture(RESOURCE_FOLDER"_playerShip.png");
	GLuint pBullet = LoadTexture(RESOURCE_FOLDER"_playerBullet.png");
	GLuint eBullet = LoadTexture(RESOURCE_FOLDER"_enemyBullet.png");

	Matrix projectionMatrix;
	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	Matrix modelviewMatrix;

	Enemy enemyArray[10][4];
	Player playerOne(-0.15f, -1.8f);
	Bullet playerBullet;
	Bullet enemyBullets[15];
	int ebCounter = 0;

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

	Mix_Chunk *shootSound;
	shootSound = Mix_LoadWAV("shoot.wav");
	Mix_Chunk *explosionSound;
	explosionSound = Mix_LoadWAV("explosion.wav");

	Mix_Music *backgroundMusic;
	backgroundMusic = Mix_LoadMUS("BoxCat_Games_-_03_-_Battle_Special.mp3");

	Mix_PlayMusic(backgroundMusic, -1);


	float lastFrameTick = 0.0f;

	for (int i = 0; i < 15; i++) {
		Bullet eBullet;
		enemyBullets[i] = eBullet;
	}

	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 4; j++) {
			Enemy enemy(-3.5 + .4*i, 1.5 - .4*j);
			enemyArray[i][j] = (enemy);
		}
	}

	//Enemy enemy(-3.5, 1.5);

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
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					if (playerBullet.aLive == 0) {
						playerBullet.x = playerOne.x + playerOne.width / 2;
						playerBullet.y = playerOne.y + playerOne.height;
						playerBullet.vel = 1.0f;
						playerBullet.aLive = 1;
						Mix_PlayChannel(-1, shootSound, 0);
					}
				}
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(program.programID);

		program.SetProjectionMatrix(projectionMatrix);
		program.SetModelviewMatrix(modelviewMatrix);

		//draw player
		playerOne.Draw(&program, playerModel);
		playerBullet.Draw(&program, pBullet);


		//draw enemies
		for (int i = 0; i < 10; i++) {
			for (int j = 0; j < 4; j++) {

				enemyArray[i][j].Draw(&program, enemy1);
			}
		}


		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		if (keys[SDL_SCANCODE_LEFT] && playerOne.x >= -3.55) {
			playerOne.x -= 1.0f * elapsed;
		}
		if (keys[SDL_SCANCODE_RIGHT] && playerOne.x + 0.3 <= 3.55) {
			playerOne.x += 1.0f * elapsed;
		}

		if (playerBullet.y >= 2.00) {
			playerBullet.aLive = 0;
		}

		playerBullet.y += playerBullet.vel * elapsed;

		playerOne.hitDetection(enemyBullets);

		for (int i = 0; i < 15; i++) {
			enemyBullets[i].Draw(&program, eBullet);
			enemyBullets[i].y += enemyBullets[i].vel * elapsed;
			if (enemyBullets[i].y + enemyBullets[i].height <= -2.0) {
				enemyBullets[i].aLive = 0;
			}
		}


		//enemy movement
		for (int i = 0; i < 10; i++) {
			for (int j = 0; j < 4; j++) {
				int ChanceToShoot = rand() % 10;
				enemyArray[i][j].x += enemyArray[i][j].xVel * elapsed;
				if (enemyArray[9][1].x + .25 >= 3.55) {
					enemyArray[i][j].xVel = -enemyArray[i][j].xVel;
					enemyArray[i][j].y -= 0.05;
				}
				else if (enemyArray[0][1].x <= -3.55) {
					enemyArray[i][j].xVel = -enemyArray[i][j].xVel;
					enemyArray[i][j].y -= 0.05;
				}
				if (enemyArray[i][j].hitDetection(playerBullet,explosionSound)) {
					playerBullet.y = 2.0;
				}
				enemyArray[i][j].ChanceToShoot(enemyBullets, ebCounter);

			}
		}

		SDL_GL_SwapWindow(displayWindow);
	}
	Mix_FreeChunk(shootSound);
	Mix_FreeChunk(explosionSound);
	Mix_FreeMusic(backgroundMusic);

	SDL_Quit();
	return 0;
}