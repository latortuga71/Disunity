// Disunity.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <raylib.h>
// C++ LIBS
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <iostream>

const uint8_t FPS = 60;

void LogDebugMessage(const char* message) {
	fprintf(stderr, "DISUNITY:::DEBUG::: %s\n",message);
}

void LogErrorMessage(const char* message) {
	fprintf(stderr, "DISUNITY:::ERROR::: %s\n",message);
}

Transform t;
typedef uint64_t EntityId;

// RigidBody Component
typedef struct rigidBody_t {
	EntityId entityId;
	Vector2 velocity;
} RigidBody;

// Transform Component -> Raylib has Transform type
typedef struct transform_t {
	EntityId entityId;
	Vector2 position;
	//Vector2 scale;
	float scale;
	double rotation;
} Transformer;

// Health Components
typedef struct health_t {
	EntityId entityId;
	uint32_t currentHealth;
	uint32_t maxHealth;
} Health;


typedef struct sprite_t {
	std::string assetId;
	Rectangle box;
	uint32_t zIndex;
} Sprite;

// Components Registry 
typedef struct componentRegistry_t {
	std::unordered_map<EntityId, Health> HealthComponents;
	std::unordered_map<EntityId, Transformer> TransformComponents;
	std::unordered_map<EntityId, RigidBody> RigidBodyComponents;
	std::unordered_map<EntityId, Sprite> SpriteComponents;
} ComponentRegistry;

// Entity HashMap
typedef struct entityManger_t {
	std::unordered_map<EntityId, bool> Entities;
	EntityId EntityCounter = 0;
} EntityManger;

// Asset Manager
typedef struct assetManager_t {
	std::unordered_map<std::string, Texture>Textures;
} AssetManager;

typedef struct engine_t {
	// FPS And Window Config
	uint32_t windowHeight = 1600;
	uint32_t windowWidth  = 800;
	uint8_t fps = FPS;
	double previousFrameTime = GetTime();
	double currentFrameTime = 0.0;
	double deltaTime = 0.0;
	bool isRunning = false;
	// Member Functions For Debugging Etc
	void(*DebugPrint)(const char* message);
	void(*ErrorPrint)(const char* message);
	// Entity List Basically Handles Removing/Adding Entities
	EntityManger entityManager;
	// Components HashMap That Contains Each Component
	ComponentRegistry components;
	// Asset Manager
	AssetManager assetManager;
} Engine;

// Function Declarations

// Engine Function Declarations
bool InitEngine();
bool UninitEngine();

void ProcessInput();
void Render();
void Update();

// Entity Functions
EntityId CreateEntity(EntityManger* entities);
void DeleteEntity(EntityManger * entities, EntityId entity);
void PurgeEntities(EntityManger* entities);
// Add Entity To Components
void HealthComponentAddEntity(ComponentRegistry* registry, EntityId entityId, Health health);
void TransformerComponentAddEntity(ComponentRegistry* registry, EntityId entityId, Transformer trans);
void RigidBodyComponentAddEntity(ComponentRegistry * registry, EntityId entityId, RigidBody body);

// System Functions
void UpdateHealthSystem(EntityManger* entities, ComponentRegistry* registry);
void UpdateMovementSystem(EntityManger* entities, ComponentRegistry* registry, double deltaTime);
void UpdateRenderSystem(EntityManger* entities, ComponentRegistry* registry, double deltaTime);


// Asset Manager Functions 
void AddTexture(AssetManager* assets,const std::string& assetId, const std::string& filePath);
void ClearAssets(AssetManager* assets);
Texture GetTexture(AssetManager*assets,const std::string& assetId);

// Globals
Engine Disunity;

// Implementations Of Functions

void AddTexture(AssetManager* assets,const std::string& assetId, const std::string& filePath) {
	// I think LoadTexture stored data on the heap....or in GPU memory....So I think below is ok.
	Texture texture = LoadTexture(filePath.c_str());
	assets->Textures.insert({assetId,texture});
}

void ClearAssets(AssetManager* assets) {
}

Texture GetTexture(AssetManager* assets,const std::string& assetId) {
	try {
		return assets->Textures.at(assetId);
	}
	catch (...) {
		return Texture{ 0 };
	}
}


// Add EntityToComponents
void HealthComponentAddEntity(ComponentRegistry* registry,EntityId entityId, Health health) {
	registry->HealthComponents.insert({entityId,health});
}
void RigidBodyComponentAddEntity(ComponentRegistry* registry,EntityId entityId, RigidBody body) {
	registry->RigidBodyComponents.insert({entityId,body});
}
void TransformerComponentAddEntity(ComponentRegistry* registry,EntityId entityId, Transformer trans) {
	registry->TransformComponents.insert({entityId,trans});
}
void SpriteComponentAddEntity(ComponentRegistry* registry,EntityId entityId, Sprite sprite) {
	registry->SpriteComponents.insert({entityId,sprite});
}
// Entity Creation
EntityId CreateEntity(EntityManger* entities) {
	entities->EntityCounter++;
	entities->Entities.insert({ entities->EntityCounter,false });
	printf("Entity %d Created\n", entities->EntityCounter);
	return entities->EntityCounter;
}

// Entity Deletion 
void DeleteEntity(EntityManger* entities,EntityId entity) {
	entities->Entities[entity] = true;
	return;
}

// Entity Deletion
void PurgeEntities(EntityManger* entities) {
	std::vector<EntityId>ids;
	for (auto it = entities->Entities.begin(); it != entities->Entities.end(); it++) {
		if (it->second) {
			ids.push_back(it->first);
		}
	}
	for (EntityId i : ids) {
		entities->Entities.erase(i);
	}
}


// Systems
void UpdateHealthSystem(EntityManger* entities,ComponentRegistry* registry) {
	std::vector<EntityId>ids;
	// Get Alive Entity Ids
	for (auto it = entities->Entities.begin(); it != entities->Entities.end(); it++) {
		if (!it->second) {
			ids.push_back(it->first);
		}
	}
	// Check For Entities That Have This Component That Are Not Scheduled For Delete
	for (EntityId i : ids) {
		if (registry->HealthComponents.find(i) != registry->HealthComponents.end()) {
			// Update Entity Health Component
			Health h = registry->HealthComponents.at(i);
			if (h.currentHealth == 0) {
				printf("Your health is zero!");
				// Mark it as purged in entity Manager
				entities->Entities[i] = true;
			}
		}
	}
}

// Movement System Requires { Transform, RigidBody }
void UpdateMovementSystem(EntityManger* entities, ComponentRegistry* registry,double deltaTime) {
	std::vector<EntityId>ids;
	// Get Alive Entity Ids
	for (auto it = entities->Entities.begin(); it != entities->Entities.end(); it++) {
		if (!it->second) {
			ids.push_back(it->first);
		}
	}
	// Check For Entities That Have This Component That Are Not Scheduled For Delete
	for (EntityId i : ids) {
		try {
			RigidBody rigidBody = registry->RigidBodyComponents.at(i);
			Transformer transformer = registry->TransformComponents.at(i);
			transformer.position.x += rigidBody.velocity.x * deltaTime;
			transformer.position.y += rigidBody.velocity.y * deltaTime;
			registry->TransformComponents.at(i) = transformer;
			//printf("Entity %d Position is now x %f y %f\n", (int)i, transformer.position.x, transformer.position.y);
		}
		catch (...) {
			//Disunity.ErrorPrint("Entity ID Doesnt Satisfy Both Required Components");
			continue;
		}
		// Try Succeeded So Update Movement
	}
}

// Render System Requires { Transform, Sprite}
void UpdateRenderSystem(EntityManger* entities, ComponentRegistry* registry, AssetManager* assetManager) {
	auto start = std::chrono::high_resolution_clock::now();
	std::map<uint32_t,std::vector<EntityId>>ids;
	// Get Alive Entity Ids That Have Sprite And Transform Component
	for (auto it = entities->Entities.begin(); it != entities->Entities.end(); it++) {
		if (!it->second) {
			try {
				Sprite sprite = registry->SpriteComponents.at(it->first);
				Transformer transformer = registry->TransformComponents.at(it->first);
				if (ids.find(sprite.zIndex) == ids.end()) {
					std::vector<EntityId> tmp;
					tmp.push_back(it->first);
					ids.insert({ sprite.zIndex,tmp });
				}
				else {
					ids[sprite.zIndex].push_back(it->first);
				}
			}
			catch (...) {
				continue;
			}
		}
	}
	for (auto it = ids.begin(); it != ids.end(); it++) {
		for (auto& entity : it->second) {
			Sprite sprite = registry->SpriteComponents.at(entity);
			Transformer transformer = registry->TransformComponents.at(entity);
			Texture t = GetTexture(assetManager, sprite.assetId);
			Rectangle r;
			r.x = transformer.position.x;
			r.y = transformer.position.y;
			r.width = sprite.box.width * transformer.scale;
			r.height = sprite.box.height * transformer.scale;
			Vector2 origin = { 0,0 };
			// https://tradam.itch.io/raylib-drawtexturepro-interactive-demo <- understand scale
			DrawTexturePro(t, sprite.box, r, origin, 0.0, WHITE);
		}
		//RLAPI void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint); // Draw a part of a texture defined by a rectangle with 'pro' parameters
	}
	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
	std::cout << duration.count() << std::endl;
}


void LoadTileMap(Engine* Disunity,const std::string tilePath,uint32_t imageWidth,uint32_t imageHeight){
	// Ok so we load 1 tilemap texture as 1 entity, we do not have each singular tile as an entity.
	AddTexture(&Disunity->assetManager, "tile-map-image", tilePath);
	EntityId tile = CreateEntity(&Disunity->entityManager);
	// Add Components
	Transformer tileTransformer = { tile, {0.0,0.0}, 4.0, 0.0};
	Sprite tileSprite = {};
	tileSprite.box.width = imageWidth;
	tileSprite.box.height = imageHeight;
	tileSprite.box.x = 0; // box is zero because we are showing the whole tileset at once.
	tileSprite.box.y = 0;
	// TODO CHANGE STRING BELOW TO INCREMENT WITH EACH TILE OR USE THE POSITION
	tileSprite.assetId = "tile-map-image";
	tileSprite.zIndex = 0;
	TransformerComponentAddEntity(&Disunity->components, tile, tileTransformer);
	SpriteComponentAddEntity(&Disunity->components, tile, tileSprite);
}


void LoadLevel(uint32_t level, Engine* Disunity){
	LoadTileMap(Disunity, "C:\\temp\\assets\\nature_tileset\\OpenWorldMap24x24.png",768,768);
	//LoadTileMap(Disunity, "C:\\Users\\lator\\Desktop\\GameDevelopment\\Assets\\Disunity.png", 384, 384);
	// Load How to Configure The TileMap
	// Create Entity And Assign Health Component
	EntityId tank = CreateEntity(&Disunity->entityManager);
	EntityId truck = CreateEntity(&Disunity->entityManager);
	Disunity->DebugPrint("Created Entity");
	// Create data for tank sprite
	Transformer tanktransformer = { tank,{10.0,30.0},3.4,0.0};
	Vector2 tankVelocity = { 100.0,20.0 };
	RigidBody tankBody = { tank,tankVelocity};
	Sprite tankSprite = {};
	tankSprite.assetId = "tank-image";
	//tankSprite.texture = GetTexture(&Disunity.assetManager, tankSprite.assetId);
	tankSprite.box.width = 32;
	tankSprite.box.height = 32;
	tankSprite.zIndex = 1;
	// add components to tank entity
	TransformerComponentAddEntity(&Disunity->components, tank, tanktransformer);
	RigidBodyComponentAddEntity(&Disunity->components, tank, tankBody);
	SpriteComponentAddEntity(&Disunity->components, tank, tankSprite);
	// create data for truck sprite
	Transformer truckTransformer = { truck,{50.0,100.0},3.0,45.0};
	RigidBody truckBody = { truck,{10.0,50.0} };
	Sprite truckSprite = {};
	truckSprite.assetId = "truck-image";
	//truckSprite.texture = GetTexture(&Disunity.assetManager, truckSprite.assetId);
	truckSprite.box.width = 32;
	truckSprite.box.height = 32;
	truckSprite.zIndex = 1;
	// add components to truck entity
	TransformerComponentAddEntity(&Disunity->components, truck, truckTransformer);
	RigidBodyComponentAddEntity(&Disunity->components, truck, truckBody);
	SpriteComponentAddEntity(&Disunity->components, truck, truckSprite);
}

bool InitEngine() {
	Disunity.DebugPrint = LogDebugMessage;
	Disunity.ErrorPrint = LogErrorMessage;
	Disunity.fps = FPS;
	Disunity.isRunning = true;
	Disunity.windowHeight = 800;
	Disunity.windowWidth = 800;
	InitWindow(Disunity.windowWidth, Disunity.windowHeight, "Disunity");
	SetTargetFPS(Disunity.fps);
	Disunity.DebugPrint("Initialized Engine");
	// Add Assets To Asset Manager
	AddTexture(&Disunity.assetManager, "truck-image", "C:\\temp\\assets\\images\\truck-ford-right.png");
	AddTexture(&Disunity.assetManager, "tank-image", "C:\\temp\\assets\\images\\tank-panther-right.png");
	LoadLevel(1,&Disunity);
	return true;
}

bool UninitEngine() {
	CloseWindow();
	return true;
}

void ProcessInput(){
	if (IsKeyDown(KEY_SPACE)) {
		DeleteEntity(&Disunity.entityManager, 2);
	}
}


void Update() {
	// Add pending entities to systems. if we decide to do it like that.
	// End Drawing Handle Framerate Waiting For Us
	// So incrementing by 1 means 1 pixel per second
	Disunity.deltaTime = (GetTime() - Disunity.previousFrameTime) / 1.0;
	Disunity.previousFrameTime = GetTime();
	// Velocity * deltaTime
	//test.x += 50 * Disunity.deltaTime;
	// TODO Add Entries That Are Waiting To Be Added -> Difficult because each could need to have different variables initialized for the component
	// would need a function that takes the flags of what components the entity needs then or the flags in a loop and initialize it that way?
	PurgeEntities(&Disunity.entityManager);
	// Update All Systems Except Render System
	UpdateHealthSystem(&Disunity.entityManager, &Disunity.components);
	UpdateMovementSystem(&Disunity.entityManager, &Disunity.components,Disunity.deltaTime);
	
}

void Render(){
	BeginDrawing();
	ClearBackground(WHITE);
	// Draw Everything By Invoking Render System
	//DrawTextureEx(GetTexture(&Disunity.assetManager, "tile-map-image"), { 0.0,0.0 }, 0.0, 4.0, WHITE);
	UpdateRenderSystem(&Disunity.entityManager, &Disunity.components,&Disunity.assetManager);
	EndDrawing();
}

void EngineLoop() {
	while (!WindowShouldClose()) {
		ProcessInput();
		Update();
		Render();
	}
}
//https://gamedev.stackexchange.com/questions/152080/how-do-components-access-one-another-in-a-component-based-entity-system/152093#152093
//https://gamedev.stackexchange.com/questions/172584/how-could-i-implement-an-ecs-in-c

int main(int argc, char** argv)
{
	// Stopped At Managing Assets In Course Displaying Textures
	InitEngine();
	EngineLoop();
	UninitEngine();
	return 0;
}


// 
/*The main loop should iterate over systems, not entities.Each system iterates only over the entities that share the cross section of components that are relevant for that particular system.This prevents systems from wasting cycles on entities that have no relevance to the system.*/