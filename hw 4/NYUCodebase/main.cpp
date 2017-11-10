#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ShaderProgram.h"
#include "Matrix.h"
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

#define TILE_SIZE 0.3f
#define SPRITE_COUNT_X 16
#define SPRITE_COUNT_Y 8


using namespace std;

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

int mapWidth;
int mapHeight;
int** levelData;
vector<float> vertexData;
vector<float> texCoordData;

enum EntityType{ENTITY_PLAYER,ENTITY_COIN};

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

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	*gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(-worldY / TILE_SIZE);
}
float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t*v1;
}

class SheetSprite {
public:
	SheetSprite() {};
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size) {
		textureID = textureID;
		u = u;
		v = v;
		width = width;
		height = height;
		size = size;
	}
	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;

	void Draw(ShaderProgram *p);
};void SheetSprite::Draw(ShaderProgram *p) {
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

class Vector3 {
public:
	Vector3() {};
	Vector3(float x, float y, float z) {
		x = x;
		y = y;
		z = z;
	}
	float x;
	float y;
	float z;
};

class Entity {
public:
	Entity() {
		size.x = TILE_SIZE;
		size.y = TILE_SIZE;
		size.z = 0;
	}
	SheetSprite sprite;
	Vector3 position;
	Vector3 size;
	Vector3 velocity;
	Vector3 acceleration;

	bool isStatic;
	EntityType entityType;

	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;

	void Update(float elapsed);
	void render(ShaderProgram &program) { sprite.Draw(&program); }
	bool CollidesWith(const Entity &entity);
};
Entity player;
Entity coin;

void Entity::Update(float elasped) {

}
bool readHeader(ifstream &stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height") {
			mapHeight = atoi(value.c_str());
		}
	}
	if (mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else { // allocate our map data
		levelData = new int*[mapHeight];
		for(int i =0; i < mapHeight; ++i) {
			levelData[i] = new int[mapWidth];
		}
		return true;
	}
}
bool readLayerData(std::ifstream &stream) {
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					int val = atoi(tile.c_str());
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						levelData[y][x] = val - 1;
					}
					else {
						levelData[y][x] = 0;
					}
				}
			}
		}
	}
	return true;
}
void placeEntity(string type, float x, float y,GLuint& sprites) {

	if (type == "Player") {
		float u = (float) (99 % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
		float v = (float)(99 / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
		player.entityType = ENTITY_PLAYER;
		player.position = Vector3(0, 0, 0);
		player.velocity = Vector3(0, 0, 0);
		player.acceleration = Vector3(0, -1, 0);
		player.sprite = SheetSprite(sprites, 0, 0, 1.0 / SPRITE_COUNT_X, 1.0 / SPRITE_COUNT_Y, TILE_SIZE);
	}
	else if (type == "Coin") {
		float u = (float)(79 % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
		float v = (float)(79 / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
		coin.entityType = ENTITY_COIN;
		coin.position = Vector3(x, y, 0);
		coin.velocity = Vector3(0, 0, 0);
		coin.acceleration = Vector3(0, 0, 0);
		coin.sprite = SheetSprite(sprites, u, v, 1.0 / SPRITE_COUNT_X, 1.0 / SPRITE_COUNT_Y, TILE_SIZE);
	}
}
bool readEntityData(ifstream &stream,GLuint& sprites) {
	string line;
	string type;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "type") {
			type = value;
		}
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');
			float placeX = atoi(xPosition.c_str())*TILE_SIZE;
			float placeY = atoi(yPosition.c_str())*-TILE_SIZE;
			placeEntity(type, placeX, placeY,sprites);
		}
	}
	return true;
}
void renderMap(ShaderProgram* p,GLuint& sprites) {
	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {
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
	glBindTexture(GL_TEXTURE_2D, sprites);
	glVertexAttribPointer(p->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(p->positionAttribute);
	glVertexAttribPointer(p->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(p->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, vertexData.size()/2);
	glDisableVertexAttribArray(p->positionAttribute);
	glDisableVertexAttribArray(p->texCoordAttribute);
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
	SDL_Event event;
	
	glViewport(0, 0, 640, 360);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	GLuint sprites = LoadTexture(RESOURCE_FOLDER"arne_sprites.png");

	Matrix projectionMatrix;
	Matrix modelviewMatrix;
	projectionMatrix.SetOrthoProjection(-10.0f, 10.0f, -4.0f, 4.0f, -1.0f, 1.0f);


	float lastFrameTick = 0.0f;

	ifstream infile(RESOURCE_FOLDER"Platformer_map.txt");
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				assert(false);
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
		else if (line == "[Object Layer 1]") {
			readEntityData(infile,sprites);
		}
	}
	infile.close();


	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		float ticks = (float)SDL_GetTicks() / 1000;
		float elapsed = ticks - lastFrameTick;
		lastFrameTick = ticks;

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(program.programID);
		modelviewMatrix.Identity();
		program.SetProjectionMatrix(projectionMatrix);
		program.SetModelviewMatrix(modelviewMatrix);

		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		
		renderMap(&program,sprites);
		player.sprite.Draw(&program);
		coin.sprite.Draw(&program);
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
