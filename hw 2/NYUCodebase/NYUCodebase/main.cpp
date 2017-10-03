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
#include <iostream>
using namespace std;

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

class Paddle {
public:
	Paddle(float xPos, float yPos) {
		x = xPos;
		y = yPos;
		width = 0.25f;
		height = 1.0f;
	};
	void Draw(ShaderProgram *p);

	float x;
	float y;

	float width;
	float height;

};
void Paddle::Draw(ShaderProgram *p) {
	float vertices[] = { x, y, x + width, y, x + width, y + height,
		x,y,x + width,y + height,x,y + height };
	glVertexAttribPointer(p->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(p->positionAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(p->positionAttribute);
}
class Ball{
public:
	Ball(float xPos, float yPos) {
		x = xPos;
		y = yPos;
		width = 0.3f;
		height = 0.3f;
		velocity = 1;
		x_vel = 0;
		y_vel = 0;
	};

	void Draw(ShaderProgram *p);
	void updateMovement(Paddle& p1, Paddle& p2);
	float x;
	float y;
	float width;
	float height;

	float velocity;
	float x_vel;
	float y_vel;
};
void Ball::Draw(ShaderProgram *p) {

	float vertices[] = { x, y, x + width, y, x + width, y + height,
		x,y,x + width,y + height,x,y + height };
	glVertexAttribPointer(p->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(p->positionAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(p->positionAttribute);
}
void Ball::updateMovement(Paddle& p1,Paddle& p2) {
	if (x <= -3.55) {
		cout << "Right side Wins!" << endl;
		glClearColor(0.0f, 0.0f, 1.0f, 1);
		//right wins
		x_vel = 0;
		y_vel = 0;
	}
	if ((x + width) >= 3.55) {
		cout << "Left side Wins!" << endl;
		glClearColor(1.0f, 0.0f, 0.0f, 1);
		x_vel = 0;
		y_vel = 0;
	}
	if (y+height >= 2.0 || y <= -2.0) { //bounce of walls
		y_vel = -y_vel;
	}
	//collision detection for left paddle
	if ((x <= (p1.x+p1.width)) && (x >= p1.x) &&(y <= (p1.y + p1.height)) && (y + height >= p1.y)) {
		x_vel = -x_vel;
	}
	if ((x+width >= p2.x) &&(x+width <= (p2.x+p2.width))&&(y + height >= p2.y) && (y <= (p2.y + p2.height))) {
		x_vel = -x_vel;
	}
	x += x_vel;
	y += y_vel;
}


int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	glViewport(0, 0, 640, 360);

	ShaderProgram program(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");

	Matrix projectionMatrix;
	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	Matrix modelviewMatrix;

	Paddle paddleLeft(-3.5f,-0.5f);
	Paddle paddleRight(3.25, -0.5);
	Ball ball(-.15,-.15);
	int randomDirection = rand();

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
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					glClear(GL_COLOR_BUFFER_BIT);
					ball.x_vel = ball.velocity*cos(rand()%360)*elapsed;
					ball.y_vel = ball.velocity*sin(rand()%360)*elapsed;
				}
				}
			}


		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program.programID);

		program.SetProjectionMatrix(projectionMatrix);
		program.SetModelviewMatrix(modelviewMatrix);
		
		paddleLeft.Draw(&program);
		paddleRight.Draw(&program);
		ball.Draw(&program);
		ball.updateMovement(paddleLeft,paddleRight);

		
		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		if (keys[SDL_SCANCODE_W]) {
			if (paddleLeft.y <= (2.0f - paddleLeft.height))
				paddleLeft.y += elapsed *2.0f;
		}
		if (keys[SDL_SCANCODE_S]) {
			if (paddleLeft.y >= -2.0f)
				paddleLeft.y -= elapsed *2.0f;
		}
		if (keys[SDL_SCANCODE_UP]) {
			if (paddleRight.y <= (2.0f - paddleRight.height))
				paddleRight.y += elapsed *2.0f;
		}		
		if (keys[SDL_SCANCODE_DOWN]) {
			if (paddleRight.y >= -2.0f)
				paddleRight.y -= elapsed *2.0f;
		}

		if (keys[SDL_SCANCODE_R]) {
			ball.x = -.15;
			ball.y = -.15;
			ball.x_vel = 0;
			ball.y_vel = 0;
			glClear(GL_COLOR_BUFFER_BIT);
		}

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
