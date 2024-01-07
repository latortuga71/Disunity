// Disunity.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <raylib.h>
// C++ LIBS
#include <unordered_map>
#include <vector>

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
	Vector2 scale;
	double rotation;
} Transformer;

// Health Components
typedef struct health_t {
	EntityId entityId;
	uint32_t currentHealth;
	uint32_t maxHealth;
} Health;


typedef struct sprite_t {
	Texture2D texture;
	Rectangle box;
	Vector2 pos;
	uint32_t frame;
	uint32_t frameMax;
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

typedef struct engine_t {
	// FPS And Window Config
	uint32_t windowHeight = 800;
	uint32_t windowWidth  = 450;
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


// Globals
Engine Disunity;

// Implementations Of Functions
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
			printf("Entity %d Position is now x %f y %f\n", (int)i, transformer.position.x, transformer.position.y);
		}
		catch (...) {
			Disunity.ErrorPrint("Entity ID Doesnt Satisfy Both Required Components");
			continue;
		}
		// Try Succeeded So Update Movement
	}
}

// Render System Requires { Transform, Sprite}
void UpdateRenderSystem(EntityManger* entities, ComponentRegistry* registry) {
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
			Sprite sprite = registry->SpriteComponents.at(i);
			Transformer transformer = registry->TransformComponents.at(i);
			DrawRectangle(transformer.position.x, transformer.position.y, sprite.box.width, sprite.box.height,RED);
		}
		catch (...) {
			Disunity.ErrorPrint("Entity ID Doesnt Satisfy Both Required Components");
			continue;
		}
		// Try Succeeded So Update Movement
	}
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
	// Create Entity And Assign Health Component
	EntityId tank = CreateEntity(&Disunity.entityManager);
	Disunity.DebugPrint("Created Entity");
	// Create data for tank
	RigidBody tankBody = { tank,{10.0,50.0} };
	Transformer tanktransformer = { tank,{10.0,30.0},{1.0,1.0},0.0};
	Sprite tankSprite = {};
	tankSprite.box.width = 50;
	tankSprite.box.height = 50;
	// add components to tank entity
	TransformerComponentAddEntity(&Disunity.components, tank, tanktransformer);
	RigidBodyComponentAddEntity(&Disunity.components, tank, tankBody);
	SpriteComponentAddEntity(&Disunity.components, tank, tankSprite);
	return true;
}

bool UninitEngine() {
	CloseWindow();
	return true;
}

void ProcessInput(){
	if (IsKeyDown(KEY_SPACE)) {
		DeleteEntity(&Disunity.entityManager, 1);
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
	// Delete Entities That Are Marked For Deletion
	PurgeEntities(&Disunity.entityManager);
	// Update All Systems Except Render System
	UpdateHealthSystem(&Disunity.entityManager, &Disunity.components);
	UpdateMovementSystem(&Disunity.entityManager, &Disunity.components,Disunity.deltaTime);
	
}

void Render(){
	BeginDrawing();
	ClearBackground(WHITE);
	// Draw Everything By Invoking Render System
	UpdateRenderSystem(&Disunity.entityManager, &Disunity.components);
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
	InitEngine();
	EngineLoop();
	UninitEngine();
	return 0;
}


// 
/*The main loop should iterate over systems, not entities.Each system iterates only over the entities that share the cross section of components that are relevant for that particular system.This prevents systems from wasting cycles on entities that have no relevance to the system.*/