// Disunity.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <raylib.h>
#include<raymath.h>
// C++ LIBS
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <iostream>
#include <deque>

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
	Vector2 direction; // TODO Extract This Out To keyboard componenet
	float scale;
	double rotation;
} Transformer;

// Health Components
typedef struct health_t {
	EntityId entityId;
	uint32_t currentHealth;
	uint32_t maxHealth;
} Health;

// Animation Component
typedef struct animation_t {
	uint32_t numFrames;
	uint32_t currentFrame;
	float frameRateSpeed; // 1.0/16.f 16 frames per second
	float runningTime;
	bool shouldLoop;
} Animation;

// Sprite Component
typedef struct sprite_t {
	std::string assetId;
	Rectangle box;
	uint32_t zIndex;
} Sprite;

// BoxCollider Component
typedef struct box_collider_t {
	uint32_t width;
	uint32_t height;
	Vector2 offset;
} BoxCollider;

// Components Registry 
typedef struct componentRegistry_t {
	std::deque<EntityId> FreeEntityIds;
	std::unordered_map<EntityId, Health> HealthComponents;
	std::unordered_map<EntityId, Transformer> TransformComponents;
	std::unordered_map<EntityId, RigidBody> RigidBodyComponents;
	std::unordered_map<EntityId, Sprite> SpriteComponents;
	std::unordered_map<EntityId, Animation> AnimationComponents;
	std::unordered_map<EntityId, BoxCollider> BoxColliderComponents;
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

// Collision Event 
typedef struct collisionEvent_t {
	EntityId a;
	EntityId b;
} CollisionEvent;

typedef struct keyboardEvent_t {
	KeyboardKey symbol;
} KeyBoardEvent;

// Event Type Enum
typedef enum eventType_t {
	COLLISION,
	KEYBOARD
} EventType;

// Event Callback declaration
typedef struct eventCallback_t;
typedef void (EventCallback)(void* data);

// Event Manager
typedef struct eventManager_t {
	std::unordered_map<EventType, std::vector<EventCallback*>>Subscribers;
} EventManager;


typedef struct engine_t {
	// FPS And Window Config
	uint32_t windowHeight = 1600;
	uint32_t windowWidth  = 800;
	uint8_t fps = FPS;
	double previousFrameTime = GetTime();
	double currentFrameTime = 0.0;
	double deltaTime = 0.0 ;
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
	// Event Manager
	EventManager eventManager;
} Engine;

// Function Declarations

// Engine Function Declarations
bool InitEngine();
bool UninitEngine();

void ProcessInput();
void Render();
void Update();

// Entity Functions
EntityId CreateEntity(EntityManger* entities, ComponentRegistry* registry);
void DeleteEntity(EntityManger * entities, EntityId entity);
void PurgeEntities(EntityManger* entities, ComponentRegistry* registry);

// Add Entity To Components
void HealthComponentAddEntity(ComponentRegistry* registry, EntityId entityId, Health health);
void TransformerComponentAddEntity(ComponentRegistry* registry, EntityId entityId, Transformer trans);
void RigidBodyComponentAddEntity(ComponentRegistry * registry, EntityId entityId, RigidBody body);
void SpriteComponentAddEntity(ComponentRegistry* registry, EntityId entityId, Sprite sprite);
void AnimationComponentAddEntity(ComponentRegistry * registry, EntityId entityId, Animation animation);
void BoxColliderComponentAddEntity(ComponentRegistry * registry, EntityId entityId, BoxCollider boxCollider);

// System Functions
void UpdateHealthSystem(EntityManger* entities, ComponentRegistry* registry);;
void UpdateMovementSystem(EntityManger* entities, ComponentRegistry* registry, double deltaTime);
void UpdateRenderSystem(EntityManger* entities, ComponentRegistry* registry, AssetManager* assetManager);
void UpdateAnimationSystem(EntityManger* entities, ComponentRegistry* registry, double deltaTime);
void UpdateBoxCollisionSystem(EntityManger* entities, ComponentRegistry* registry,EventManager* eventManager);
void UpdateDebugBoxCollisionsSystem(EntityManger* entities, ComponentRegistry* registry);
void UpdateKeyboardControlSystem(EntityManger* entities, ComponentRegistry* registry,EventManager* eventManager);

// SystemEventCallbacks

void HealthSystemEventCallback(void* thedata);

// Asset Manager Functions 
void AddTexture(AssetManager* assets,const std::string& assetId, const std::string& filePath);
void ClearAssets(AssetManager* assets);
Texture GetTexture(AssetManager*assets,const std::string& assetId);

// EventManger Functions
void ClearEvents(EventManager* eventManager);
void SubscribeToEvent(EventManager* eventManager, EventType etype, EventCallback* callback);
void EmitEvent(EventManager* eventManager, EventType etype, void* data);

// UtilityFunctions
bool CheckAABBCollision(double aX, double aY, double aW, double aH, double bX, double bY, double bW, double bH);

void ClearEvents(EventManager* eventManager) {
	for (auto i = eventManager->Subscribers.begin(); i != eventManager->Subscribers.end();  i++) {
		i->second.clear();
	}
	eventManager->Subscribers.clear();
}

void SubscribeToEvent(EventManager* eventManager, EventType etype, EventCallback* callback) {
	eventManager->Subscribers[etype].push_back(callback);
}

void EmitEvent(EventManager* eventManager, EventType etype, void* eventData) {
	try {
		std::vector<EventCallback*> subscribers = eventManager->Subscribers[etype];
		for (auto &callback : subscribers) {
			(*callback)(eventData); // perform the callback
		}
	}
	catch (...) {
		printf("Unknown Event Type Emitted!\n");
	}
}

//Event Callback Functions
void HealthSystemEventCallback(void* thedata) {
	printf("HealthSystemEvenCalback Called");
}
void KeyboardControlSystemEventCallback(void* data) {
	KeyBoardEvent* evt = (KeyBoardEvent*)data;
	printf("KeyboardControlSystemEventCallback Called\n");
}

bool CheckAABBCollision(double aX, double aY, double aW, double aH, double bX, double bY, double bW, double bH) {
	return (
		aX < bX + bW &&
		aX + aW > bX &&
		aY < bY + bH &&
		aY + aH > bY
		);
}


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
void AnimationComponentAddEntity(ComponentRegistry* registry,EntityId entityId, Animation animation) {
	registry->AnimationComponents.insert({entityId,animation});
}
void BoxColliderComponentAddEntity(ComponentRegistry* registry, EntityId entityId, BoxCollider boxCollider) {
	registry->BoxColliderComponents.insert({entityId,boxCollider});
}
// Entity Creation
EntityId CreateEntity(EntityManger* entities,ComponentRegistry* registry) {
	if (registry->FreeEntityIds.empty()) {
		entities->EntityCounter++;
		entities->Entities.insert({ entities->EntityCounter,false });
		printf("Entity %d Created\n", entities->EntityCounter);
		return entities->EntityCounter;
	}
	else {
		EntityId id = registry->FreeEntityIds.front();
		registry->FreeEntityIds.pop_front();
		printf(" REUsingEntity %d \n", id);
	}
}

// Entity Deletion 
void DeleteEntity(EntityManger* entities,EntityId entity) {
	entities->Entities[entity] = true;
	return;
}

// Entity Deletion
void PurgeEntities(EntityManger* entities,ComponentRegistry* registry) {
	std::vector<EntityId>ids;
	for (auto it = entities->Entities.begin(); it != entities->Entities.end(); it++) {
		if (it->second) {
			ids.push_back(it->first);
		}
	}
	for (EntityId i : ids) {
		entities->Entities.erase(i);
		registry->HealthComponents.erase(i);
		registry->RigidBodyComponents.erase(i);
		registry->TransformComponents.erase(i);
		registry->AnimationComponents.erase(i);
		registry->SpriteComponents.erase(i);
		registry->BoxColliderComponents.erase(i);
		registry->FreeEntityIds.push_back(i);
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
			Health& h = registry->HealthComponents.at(i);
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
			RigidBody& rigidBody = registry->RigidBodyComponents.at(i);
			Transformer transformer = registry->TransformComponents.at(i);
			// TODO FIX THIS IN FUTURE PIKUMA KEYBOARD CONTROLLER CHAPTER
			if (Vector2Length(transformer.direction) != 0) {
				transformer.position = Vector2Subtract(transformer.position, Vector2Scale(Vector2Normalize(transformer.direction), rigidBody.velocity.x));
				transformer.direction.x = 0;
				transformer.direction.y = 0;
				registry->TransformComponents.at(i) = transformer;
			}
			//transformer.position.x += rigidBody.velocity.x * deltaTime;
			//transformer.position.y += rigidBody.velocity.y * deltaTime;
			printf("Entity %d Position is now x %f y %f\n", (int)i, transformer.position.x, transformer.position.y);
		}
		catch (...) {
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
				Sprite& sprite = registry->SpriteComponents.at(it->first);
				Transformer& transformer = registry->TransformComponents.at(it->first);
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
			Transformer& transformer = registry->TransformComponents.at(entity);
			Texture t = GetTexture(assetManager, sprite.assetId);
			Rectangle r;
			r.x = transformer.position.x;
			r.y = transformer.position.y;
			r.width = sprite.box.width * transformer.scale;
			r.height = sprite.box.height * transformer.scale;
			Vector2 origin = { sprite.box.width / 2,sprite.box.height / 2 };  // we want our sprite to be centered or on the bottom
			// https://tradam.itch.io/raylib-drawtexturepro-interactive-demo <- understand scale
			// https://www.youtube.com/watch?v=AKTLg1SWfG0 <-- understand origin offsets
			DrawTexturePro(t, sprite.box, r, origin, 0.0, WHITE);
		}
	}
	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
	std::cout << duration.count() << std::endl;
}

void UpdateAnimationSystem(EntityManger* entities, ComponentRegistry* registry,double deltaTime) {
	auto start = std::chrono::high_resolution_clock::now();
	std::map<uint32_t,std::vector<EntityId>>ids;
	// Get Alive Entity Ids That Have Sprite And Transform Component
	for (auto it = entities->Entities.begin(); it != entities->Entities.end(); it++) {
		if (!it->second) {
			try {
				Sprite& sprite = registry->SpriteComponents.at(it->first);
				Animation& animation = registry->AnimationComponents.at(it->first);
				if (animation.shouldLoop) {
					animation.runningTime += deltaTime;
					if (animation.runningTime >= animation.frameRateSpeed) {
						animation.currentFrame++;
						animation.runningTime = 0.f;
						if (animation.currentFrame > animation.numFrames) animation.currentFrame = 1; // always start on frame 1
						// Update Sprite Component Rectangle this moves the pixels to the left on the texture to get the next frame of the animation
						sprite.box.x = animation.currentFrame * sprite.box.width;
					}
				}
			}
			catch (...) {
				continue;
			}
		}
	}
	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
	std::cout << duration.count() << std::endl;
}

// Box Collision System
void UpdateBoxCollisionSystem(EntityManger* entities, ComponentRegistry* registry,EventManager* eventManager) {
	auto start = std::chrono::high_resolution_clock::now();
	std::map<uint32_t,std::vector<EntityId>>ids;
	std::vector<EntityId>collidableEntities;
	// Get Alive Entity Ids That Have Transform Component And BoxCollider
	for (auto it = entities->Entities.begin(); it != entities->Entities.end(); it++) {
		if (!it->second) {
			try {
				BoxCollider boxCollider = registry->BoxColliderComponents.at(it->first);
				Transformer transformer = registry->TransformComponents.at(it->first);
				collidableEntities.push_back(it->first);
			}
			catch (...) {
				continue;
			}
		}
	}
	// loop over each entity use iterators  
	for (auto i = collidableEntities.begin(); i != collidableEntities.end(); i++) {
		EntityId a = *i;
		Transformer aTransform = registry->TransformComponents.at(a);
		BoxCollider aCollider = registry->BoxColliderComponents.at(a);
		for (auto j = i; j != collidableEntities.end(); j++) {
			EntityId b = *j;
			if (a == b) continue;
			Transformer bTransform = registry->TransformComponents.at(b);
			BoxCollider bCollider = registry->BoxColliderComponents.at(b);
			// TODO Check If Collision Between A And B
			bool collision = CheckAABBCollision(aTransform.position.x + aCollider.offset.x, aTransform.position.y + aCollider.offset.y, aCollider.width, aCollider.height, bTransform.position.x + bCollider.offset.x, bTransform.position.y + bCollider.offset.y, bCollider.width, bCollider.height);
			if (collision) {
				// Example Of Collision System
				printf("COLLISION! EMITTING EVENT\n");
				CollisionEvent evt = { a,b };
				EmitEvent(eventManager, COLLISION, &evt);
			}
		}
	}
	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
	std::cout << duration.count() << std::endl;
}

void UpdateDebugBoxCollisionsSystem(EntityManger* entities, ComponentRegistry* registry) {
	auto start = std::chrono::high_resolution_clock::now();
	std::map<uint32_t,std::vector<EntityId>>ids;
	std::vector<EntityId>collidableEntities;
	// Get Alive Entity Ids That Have Transform Component And BoxCollider
	for (auto it = entities->Entities.begin(); it != entities->Entities.end(); it++) {
		if (!it->second) {
			try {
				BoxCollider boxCollider = registry->BoxColliderComponents.at(it->first);
				Transformer transformer = registry->TransformComponents.at(it->first);
				Rectangle rectangle;
				rectangle.x = transformer.position.x;
				rectangle.y = transformer.position.y;
				rectangle.width = boxCollider.width;
				rectangle.height = boxCollider.height;
				DrawRectangleLines(transformer.position.x + boxCollider.offset.x, transformer.position.y + boxCollider.offset.y, boxCollider.width, boxCollider.height, RED);
			}
			catch (...) {
				continue;
			}
		}
	}
	// loop over each entity use iterators  
	for (auto i = collidableEntities.begin(); i != collidableEntities.end(); i++) {
		EntityId a = *i;
		Transformer aTransform = registry->TransformComponents.at(a);
		BoxCollider aCollider = registry->BoxColliderComponents.at(a);
	}
	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
	std::cout << duration.count() << std::endl;
}

void UpdateKeyboardControlSystem(EntityManger* entities, ComponentRegistry* registry, EventManager* eventManager){
	// TODO Currently Doesnt Do Anything
}

void LoadTileMap(Engine* Disunity,const std::string tilePath,uint32_t imageWidth,uint32_t imageHeight){
	// Ok so we load 1 tilemap texture as 1 entity, we do not have each singular tile as an entity.
	AddTexture(&Disunity->assetManager, "tile-map-image", tilePath);
	EntityId tile = CreateEntity(&Disunity->entityManager,&Disunity->components);
	// Add Components
	Transformer tileTransformer = { tile, {0.0,0.0},{0,0}, 4.0, 0.0 };
	Sprite tileSprite = {};
	tileSprite.box.width = imageWidth;
	tileSprite.box.height = imageHeight;
	tileSprite.box.x = 0; // box is zero because we are showing the whole tileset at once.
	tileSprite.box.y = 0;
	tileSprite.assetId = "tile-map-image";
	tileSprite.zIndex = 0;
	TransformerComponentAddEntity(&Disunity->components, tile, tileTransformer);
	SpriteComponentAddEntity(&Disunity->components, tile, tileSprite);
}


void LoadLevel(uint32_t level, Engine* Disunity){
	LoadTileMap(Disunity, "C:\\temp\\assets\\nature_tileset\\OpenWorldMap24x24.png",768,768);
	EntityId tank = CreateEntity(&Disunity->entityManager,&Disunity->components);
	EntityId truck = CreateEntity(&Disunity->entityManager,&Disunity->components);
	EntityId knight = CreateEntity(&Disunity->entityManager,&Disunity->components);
	Transformer knightPos = { knight,{500.0,500.0},{0.f,0.f},3.4,0.0};
	Vector2 knightVelocity = { 5.0,5.0 };
	RigidBody knightBody = { knight,knightVelocity};
	Sprite knightSprite = {};
	Animation knightAnimation = {6, 1, 1.f/12.f, 0, true};
	BoxCollider knightCollider = { 32,32,{0,0} };
	knightSprite.assetId = "knight-image";
	knightSprite.box.width = 16;
	knightSprite.box.height = 16;
	knightSprite.zIndex = 1;
	TransformerComponentAddEntity(&Disunity->components, knight, knightPos);
	RigidBodyComponentAddEntity(&Disunity->components,knight, knightBody);
	SpriteComponentAddEntity(&Disunity->components, knight, knightSprite);
	AnimationComponentAddEntity(&Disunity->components, knight, knightAnimation);
	BoxColliderComponentAddEntity(&Disunity->components, knight, knightCollider);
	AddTexture(&Disunity->assetManager, "knight-image", "C:\\temp\\assets\\characters\\knight_idle_spritesheet.png");
	Disunity->DebugPrint("Created Entity");
	// Create data for tank sprite
	float tankScale = 3.4;
	Transformer tanktransformer = { tank,{10.0,30.0},{0.f,0.f},tankScale,0.0};
	Vector2 tankVelocity = { 100.0,20.0 };
	RigidBody tankBody = { tank,tankVelocity};
	Sprite tankSprite = {};
	BoxCollider tankCollider = { 64,64, {0,0} };
	// Usually Have Box Double The Size Of the Sprite Seems to work.
	tankSprite.assetId = "tank-image";
	//tankSprite.texture = GetTexture(&Disunity.assetManager, tankSprite.assetId);
	tankSprite.box.width = 32;
	tankSprite.box.height = 32;
	tankSprite.zIndex = 1;
	// add components to tank entity
	TransformerComponentAddEntity(&Disunity->components, tank, tanktransformer);
	RigidBodyComponentAddEntity(&Disunity->components, tank, tankBody);
	SpriteComponentAddEntity(&Disunity->components, tank, tankSprite);
	BoxColliderComponentAddEntity(&Disunity->components, tank, tankCollider);
	// create data for truck sprite
	Transformer truckTransformer = { truck,{50.0,100.0},{0,0},3.0,45.0};
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
	LoadLevel(1,&Disunity);
	AddTexture(&Disunity.assetManager, "truck-image", "C:\\temp\\assets\\images\\truck-ford-right.png");
	AddTexture(&Disunity.assetManager, "tank-image", "C:\\temp\\assets\\images\\tank-panther-right.png");
	return true;
}

bool UninitEngine() {
	CloseWindow();
	return true;
}

void ProcessInput(){
	if (IsKeyDown(KEY_W)) {
		// Example Emit Keyboard Event
		KeyBoardEvent evt = { (KeyboardKey)KEY_W };
		EmitEvent(&Disunity.eventManager, KEYBOARD, &evt);
		Transformer* pos = &Disunity.components.TransformComponents.at(4);
		pos->direction.y+=1.0;
		//DeleteEntity(&Disunity.entityManager, 4);
		//Animation* animation = &Disunity.components.AnimationComponents.at(4);
		//animation->shouldLoop = true;
	}
	if (IsKeyDown(KEY_A)) {
		Transformer* pos = &Disunity.components.TransformComponents.at(4);
		pos->direction.x += 1.0;
		//DeleteEntity(&Disunity.entityManager, 4);
		//Animation* animation = &Disunity.components.AnimationComponents.at(4);
		//animation->shouldLoop = true;
	}
	if (IsKeyDown(KEY_S)) {
		Transformer* pos = &Disunity.components.TransformComponents.at(4);
		pos->direction.y -= 1.0;
		//DeleteEntity(&Disunity.entityManager, 4);
		//Animation* animation = &Disunity.components.AnimationComponents.at(4);
		//animation->shouldLoop = true;
	}
	if (IsKeyDown(KEY_D)) {
		Transformer* pos = &Disunity.components.TransformComponents.at(4);
		pos->direction.x -= 1.0;
	}
	if (IsKeyDown(KEY_SPACE)) {
		DeleteEntity(&Disunity.entityManager, 2);
		//Animation* animation = &Disunity.components.AnimationComponents.at(4);
		//animation->shouldLoop = true;
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
	// Clear events
	ClearEvents(&Disunity.eventManager);
	// Delete Entities That Are Marked For Deletion
	PurgeEntities(&Disunity.entityManager,&Disunity.components);
	// Register Event Callbacks For Systems
	SubscribeToEvent(&Disunity.eventManager, COLLISION, HealthSystemEventCallback);
	SubscribeToEvent(&Disunity.eventManager, KEYBOARD, KeyboardControlSystemEventCallback);
	// Update All Systems Except Render System
	UpdateMovementSystem(&Disunity.entityManager, &Disunity.components,Disunity.deltaTime);
	UpdateHealthSystem(&Disunity.entityManager, &Disunity.components);
	UpdateAnimationSystem(&Disunity.entityManager, &Disunity.components,Disunity.deltaTime);
	UpdateKeyboardControlSystem(&Disunity.entityManager, &Disunity.components,&Disunity.eventManager);
}

void Render(){
	BeginDrawing();
	ClearBackground(WHITE);
	// Draw Everything By Invoking Render System
	//DrawTextureEx(GetTexture(&Disunity.assetManager, "tile-map-image"), { 0.0,0.0 }, 0.0, 4.0, WHITE);
	UpdateRenderSystem(&Disunity.entityManager, &Disunity.components,&Disunity.assetManager);
	// Debugging BoxCollision By Drawing Boxes
	UpdateDebugBoxCollisionsSystem(&Disunity.entityManager, &Disunity.components);
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