#include "raylib.h"
#include "math.h"
#include <stdlib.h> // For abs()

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

// #define UNIT_TEST 1

//----------------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------------
#define SCREEN_WIDTH            800
#define SCREEN_HEIGHT           600

#define PLAYER_SPEED            5.0f
#define PLAYER_BULLET_SPEED     7.0f
#define ALIEN_BULLET_SPEED      4.0f

#define ALIENS_ROWS             5
#define ALIENS_COLS             11
#define NUM_ALIENS              (ALIENS_ROWS * ALIENS_COLS)
#define MAX_ALIEN_BULLETS       10 // Max simultaneous alien bullets

#define NUM_SHIELDS             4
#define SHIELD_SEGMENTS_X       8
#define SHIELD_SEGMENTS_Y       5

#define ALIEN_MOVE_WAIT_TIME_START 0.8f // Initial time between alien moves (seconds)
#define ALIEN_MOVE_SPEEDUP_FACTOR  0.97f // Multiplier applied to wait time when an alien is killed
#define ALIEN_SHOOT_INTERVAL_MIN   0.5f // Minimum time between alien shots
#define ALIEN_SHOOT_INTERVAL_MAX   2.0f // Maximum time between alien shots

#define UFO_SPEED               55.0f
#define UFO_POINTS              200

      
#define UFO_SPAWN_INTERVAL_MIN  30.0f // Minimum seconds until UFO appears
#define UFO_SPAWN_INTERVAL_MAX  240.0f // Maximum seconds until UFO appears

    

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum GameScreen { LOGO, TITLE, GAMEPLAY, GAME_OVER } GameScreen;
typedef enum AlienType { ALIEN_TYPE_1 = 0, ALIEN_TYPE_2, ALIEN_TYPE_3 } AlienType; // Type 3 top, Type 1 bottom

typedef struct Player {
    Vector2 position;
    Texture2D texture;
    Rectangle textureRect; // Actual source rect
    Vector2 size;          // Scaled size for drawing/collision
    int lives;
    bool shotActive;
    Vector2 shotPosition;
    Texture2D shotTexture;
    Rectangle shotTextureRect;
    Vector2 shotSize;
    float explosionTimer; // Timer for player explosion effect
} Player;

typedef struct Alien {
    Vector2 position;
    Vector2 basePosition; // Original grid position
    AlienType type;
    Texture2D texture1;
    Texture2D texture2;
    Rectangle textureRect;
    Vector2 size;
    bool active;
    bool currentFrame; // false = frame 1, true = frame 2
    Color color;       // Tint, can be removed if using specific textures per type
    int points;
} Alien;

typedef struct Bullet {
    Vector2 position;
    bool active;
    float speed;
    Texture2D texture;
    Rectangle textureRect;
    Vector2 size;
} Bullet;

typedef struct Shield {
    Vector2 position;
    RenderTexture2D renderTexture; // Use a render texture for destructibility
    Texture2D baseTexture;
    bool active;
    Rectangle bounds;
} Shield;

typedef struct UFO {
    Vector2 position;
    Texture2D texture;
    Rectangle textureRect;
    Vector2 size;
    bool active;
    float speed;
    float spawnTimer;
    float timeActive; // For sound pitch
    bool exploding;
    float explosionTimer;
    Texture2D explosionTexture;
    Rectangle explosionTextureRect;
} UFO;

typedef struct Explosion {
    Vector2 position;
    Texture2D texture;
    Rectangle textureRect;
    Vector2 size;
    float timer;
    bool active;
} Explosion;



#define MAX_EXPLOSIONS 10

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static GameScreen currentScreen = LOGO; // Change to TITLE if no logo screen needed
static int framesCounter = 0;
static bool gameOver = false;
static bool gamePaused = false;
static int score = 0;
static int hiScore = 0; // Basic high score persistence needed for web (localStorage JS?)

// Gameplay specific
static Player player = { 0 };
static Alien aliens[NUM_ALIENS] = { 0 };
static Bullet alienBullets[MAX_ALIEN_BULLETS] = { 0 };
static Shield shields[NUM_SHIELDS] = { 0 };
static UFO ufo = { 0 };
static Explosion explosions[MAX_EXPLOSIONS] = { 0 };

static int aliensAlive = 0;
static float alienMoveTimer = 0;
static float alienMoveWaitTime = ALIEN_MOVE_WAIT_TIME_START;
static int alienDirection = 1; // 1 = right, -1 = left
static float alienHorizontalMove = 3.0f;
static float alienVerticalMove = 2.0f;
static bool alienSwitchFrame = false; // Flag to switch animation frame
static bool moveDown = false; // Flag for aliens to move down
static float alienShootTimer = 0;
static int alienMoveSoundIndex = 0; // 0 to 3 for the fastinvader sounds
static int currentWave = 1;

// Resources
static Texture2D alienTexture1_1, alienTexture1_2;
static Texture2D alienTexture2_1, alienTexture2_2;
static Texture2D alienTexture3_1, alienTexture3_2;
static Texture2D playerTexture;
static Texture2D playerShotTexture;
static Texture2D alienShotTexture; // Using 'rolling' for alien shots - pick one or animate
static Texture2D rollingTexture1, rollingTexture2, rollingTexture3, rollingTexture4; // For alien shot anim
static Texture2D plungerTexture1, plungerTexture2, plungerTexture3, plungerTexture4; // Alt alien shot anim
static Texture2D squigTexture1, squigTexture2, squigTexture3, squigTexture4;         // Alt alien shot anim
static Texture2D shieldTexture;
static Texture2D ufoTexture;
static Texture2D alienExplosionTexture;
static Texture2D playerExplosionTexture; // Use alien_exploding? or specific one? Using alien_exploding for now
static Texture2D shotExplosionTexture; // player_shot_exploding
static Texture2D ufoExplosionTexture;  // saucer_exploding

static Sound shootSound;
static Sound invaderKilledSound;
static Sound explosionSound; // Player explosion
static Sound fastInvaderSound1, fastInvaderSound2, fastInvaderSound3, fastInvaderSound4;
static Sound ufoHighSound, ufoLowSound;
static Sound alienExplosionSound; // Use invaderkilled or a different one? Using invaderkilled
static Sound ufoExplosionSound; // Use explosion or invaderkilled? Using explosion

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);         // Initialize game
static void UpdateGame(void);       // Update game (one frame)
static void DrawGame(void);         // Draw game (one frame)
static void UnloadGame(void);       // Unload game
static void UpdateDrawFrame(void); // Update and Draw (web loop)

static void LoadResources(void);
static void UnloadResources(void);
static void InitAliens(void);
static void InitShields(void);
static void DamageShield(int shieldIndex, Vector2 hitPosition);
static void UpdateAliens(float delta);
static void UpdateBullets(float delta);
static void UpdateUFO(float delta);
static void UpdateExplosions(float delta);
static void CheckCollisions(void);
static void SpawnPlayerShot(void);
static void SpawnAlienShot(Vector2 position);
static void SpawnUFO(void);
static void SpawnExplosion(Vector2 position, Texture2D tex, Rectangle texRect, Vector2 size);
static void ResetLevel(bool playerDied);
static void NextLevel(void);
static Vector2 WorldToShieldTexCoords(int shieldIndex, Vector2 worldPos);



//typedef struct Shield Shield;  // lets the compiler know the type
//extern Shield shields[];       // tells it that the array exists later


#ifdef UNIT_TEST
#include <stdio.h>
#include <assert.h>

// Helper: expose the core of "alien bullet vs shield" collision so we can call it from main()
bool TestAlienBulletShieldCollision(int bi, int si, Vector2 *outWorldHit, Color *outPx)
{
    // build bullet rect
    Rectangle bulletRect = {
        alienBullets[bi].position.x,
        alienBullets[bi].position.y,
        alienBullets[bi].size.x,
        alienBullets[bi].size.y
    };

    // quick AABB check
    if (!shields[si].active
     || !CheckCollisionRecs(bulletRect, shields[si].bounds))
        return false;

    // compute world‐space hit point (bottom of bullet)
    Vector2 worldHit = {
        bulletRect.x + bulletRect.width * 0.5f,
        bulletRect.y + bulletRect.height
    };
    *outWorldHit = worldHit;

    // convert into shield‐texture coordinates
    Vector2 texHit = WorldToShieldTexCoords(si, worldHit);

    printf("[UNIT_TEST] Bullet[%d] vs Shield[%d]: worldHit=(%.1f,%.1f) texHit=(%.1f,%.1f)\n",
           bi, si, worldHit.x, worldHit.y, texHit.x, texHit.y);

    // sample the pixel alpha
    Image texImg = LoadImageFromTexture(shields[si].renderTexture.texture);
    Color px = GetImageColor(texImg, (int)texHit.x, (int)texHit.y);
    UnloadImage(texImg);
    *outPx = px;

    printf("[UNIT_TEST] sample alpha = %d → %s\n",
           px.a, (px.a > 10) ? "HIT" : "MISS");

    return (px.a > 10);
}

// Helper: expose the core of "player shot vs shield" collision for testing
bool TestPlayerBulletShieldCollision(int si, Vector2 shotPos, Vector2 shotSize, Vector2 *outCollisionPoint, Color *outPx)
{
    Rectangle shotRect = { shotPos.x, shotPos.y, shotSize.x, shotSize.y };

    if (!shields[si].active || !CheckCollisionRecs(shotRect, shields[si].bounds))
        return false;

    // collision point at top center of shot
    Vector2 collisionPoint = {
        shotRect.x + shotRect.width * 0.5f,
        shotRect.y
    };
    *outCollisionPoint = collisionPoint;

    // convert to shield texture coords
    Vector2 localHit = { collisionPoint.x - shields[si].position.x,
                         collisionPoint.y - shields[si].position.y };
    localHit.x /= (shields[si].bounds.width  / shields[si].renderTexture.texture.width);
    localHit.y /= (shields[si].bounds.height / shields[si].renderTexture.texture.height);

    printf("[UNIT_TEST] Player shot vs shield[%d]: collision=(%.1f,%.1f) tex=(%.1f,%.1f)\n",
           si, collisionPoint.x, collisionPoint.y, localHit.x, localHit.y);

    Image texImg = LoadImageFromTexture(shields[si].renderTexture.texture);
    Color px = GetImageColor(texImg, (int)localHit.x, (int)localHit.y);
    UnloadImage(texImg);
    *outPx = px;

    printf("[UNIT_TEST] alpha=%d → %s\n", px.a, (px.a>10)?"HIT":"MISS");
    return (px.a > 10);
}
#endif


// -----------------------------------------------------------------------------
static Vector2 WorldToShieldTexCoords(int shieldIndex, Vector2 worldPos)
{
    // Convert world position to the *render‑texture* pixel we are going to read
    float scaleX = (float)shields[shieldIndex].renderTexture.texture.width  /
                   shields[shieldIndex].bounds.width;
    float scaleY = (float)shields[shieldIndex].renderTexture.texture.height /
                   shields[shieldIndex].bounds.height;

    Vector2 local;
    local.x = (worldPos.x - shields[shieldIndex].position.x) * scaleX;
    local.y = (worldPos.y - shields[shieldIndex].position.y) * scaleY;

    // Destination is drawn with a negative source‑height → Y axis is flipped.
    local.y = shields[shieldIndex].renderTexture.texture.height - local.y;

    // Clamp to valid pixel range so we never read OOB
    if (local.x < 0) local.x = 0; 
    if (local.y < 0) local.y = 0;
    if (local.x > shields[shieldIndex].renderTexture.texture.width  - 1) 
        local.x = shields[shieldIndex].renderTexture.texture.width  - 1;
    if (local.y > shields[shieldIndex].renderTexture.texture.height - 1) 
        local.y = shields[shieldIndex].renderTexture.texture.height - 1;
    return local;
}
// -----------------------------------------------------------------------------
// ★★★ MODIFIED DamageShield() ★★★

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------


int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "raylib - Space Invaders");
    InitAudioDevice();

    LoadResources();
    InitGame();

#if defined(PLATFORM_WEB)
    // Required argument is a function pointer, so pass the function name directly
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------
    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }
#endif

    UnloadGame();
    UnloadResources();
    CloseAudioDevice();
    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition - Resource Management
//----------------------------------------------------------------------------------
void LoadResources(void) {
    // Textures - Use standard resolution first
    alienTexture1_1 = LoadTexture("resources/inv11.png");
    alienTexture1_2 = LoadTexture("resources/inv12.png");
    alienTexture2_1 = LoadTexture("resources/inv21.png");
    alienTexture2_2 = LoadTexture("resources/inv22.png");
    alienTexture3_1 = LoadTexture("resources/inv31.png");
    alienTexture3_2 = LoadTexture("resources/inv32.png");
    playerTexture = LoadTexture("resources/play.png"); // Assuming 'play.png' is the player ship
    playerShotTexture = LoadTexture("resources/player_shot.png");
    shieldTexture = LoadTexture("resources/shield.png");
    ufoTexture = LoadTexture("resources/saucer.png");

    // Decide on alien shot graphic - Using 'rolling' for now
    alienShotTexture = LoadTexture("resources/rolling1.png"); // Placeholder, could animate
    rollingTexture1 = LoadTexture("resources/rolling1.png");
    rollingTexture2 = LoadTexture("resources/rolling2.png");
    rollingTexture3 = LoadTexture("resources/rolling3.png");
    rollingTexture4 = LoadTexture("resources/rolling4.png");
    // Load others if needed: plunger1-4, squig1-4

    alienExplosionTexture = LoadTexture("resources/alien_exploding.png");
    playerExplosionTexture = LoadTexture("resources/explosion.wav"); // Using explosion sound, need player explosion image. Use alien one for now.
    playerExplosionTexture = LoadTexture("resources/alien_exploding.png"); // Fallback
    shotExplosionTexture = LoadTexture("resources/player_shot_exploding.png");
    ufoExplosionTexture = LoadTexture("resources/saucer_exploding.png");

    // Sounds
    shootSound = LoadSound("resources/shoot.wav");
    invaderKilledSound = LoadSound("resources/invaderkilled.wav");
    explosionSound = LoadSound("resources/explosion.wav"); // Player death
    fastInvaderSound1 = LoadSound("resources/fastinvader1.wav");
    fastInvaderSound2 = LoadSound("resources/fastinvader2.wav");
    fastInvaderSound3 = LoadSound("resources/fastinvader3.wav");
    fastInvaderSound4 = LoadSound("resources/fastinvader4.wav");
    ufoHighSound = LoadSound("resources/ufo_highpitch.wav");
    ufoLowSound = LoadSound("resources/ufo_lowpitch.wav"); // Maybe alternate or use one

    // Assign other sounds (can reuse)
    alienExplosionSound = invaderKilledSound; // Reuse kill sound for alien explosion sound
    ufoExplosionSound = explosionSound; // Reuse player explosion for UFO explosion sound

    // Set looping for UFO sound if desired (handled manually by restarting)
    // SetSoundLoop(ufoHighSound, true);
}

void UnloadResources(void) {
    // Textures
    UnloadTexture(alienTexture1_1); UnloadTexture(alienTexture1_2);
    UnloadTexture(alienTexture2_1); UnloadTexture(alienTexture2_2);
    UnloadTexture(alienTexture3_1); UnloadTexture(alienTexture3_2);
    UnloadTexture(playerTexture);
    UnloadTexture(playerShotTexture);
    UnloadTexture(alienShotTexture);
    UnloadTexture(rollingTexture1); UnloadTexture(rollingTexture2); UnloadTexture(rollingTexture3); UnloadTexture(rollingTexture4);
    UnloadTexture(shieldTexture);
    UnloadTexture(ufoTexture);
    UnloadTexture(alienExplosionTexture);
    UnloadTexture(playerExplosionTexture);
    UnloadTexture(shotExplosionTexture);
    UnloadTexture(ufoExplosionTexture);

    // Sounds
    UnloadSound(shootSound);
    UnloadSound(invaderKilledSound);
    UnloadSound(explosionSound);
    UnloadSound(fastInvaderSound1); UnloadSound(fastInvaderSound2);
    UnloadSound(fastInvaderSound3); UnloadSound(fastInvaderSound4);
    UnloadSound(ufoHighSound); UnloadSound(ufoLowSound);
    // No need to unload alienExplosionSound/ufoExplosionSound if they alias others
}


//----------------------------------------------------------------------------------
// Module Functions Definition - Game Initialization
//----------------------------------------------------------------------------------
void InitGame(void)
{
    framesCounter = 0;
    gameOver = false;
    gamePaused = false;
    score = 0;
    currentWave = 1;
    // hiScore = LoadHighScore(); // Need mechanism for this

    // Init Player
    player.texture = playerTexture;
    player.textureRect = (Rectangle){ 0, 0, (float)player.texture.width, (float)player.texture.height };
    player.size = (Vector2){ player.textureRect.width * 1.5f, player.textureRect.height * 1.5f }; // Scale slightly
    player.position = (Vector2){ SCREEN_WIDTH / 2.0f - player.size.x / 2.0f, SCREEN_HEIGHT - player.size.y - 20.0f };
    player.lives = 3;
    player.shotActive = false;
    player.shotTexture = playerShotTexture;
    player.shotTextureRect = (Rectangle){ 0, 0, (float)player.shotTexture.width, (float)player.shotTexture.height };
    player.shotSize = (Vector2){ player.shotTextureRect.width * 1.5f, player.shotTextureRect.height * 1.5f };
    player.explosionTimer = 0.0f;

    // Init Alien Bullets
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        alienBullets[i].active = false;
        alienBullets[i].texture = rollingTexture1; // Start with first frame
        alienBullets[i].textureRect = (Rectangle){0, 0, (float)rollingTexture1.width, (float)rollingTexture1.height};
        alienBullets[i].size = (Vector2){alienBullets[i].textureRect.width * 1.5f, alienBullets[i].textureRect.height * 1.5f};
        alienBullets[i].speed = ALIEN_BULLET_SPEED;
    }

    // Init UFO
    ufo.texture = ufoTexture;
    ufo.textureRect = (Rectangle){ 0, 0, (float)ufo.texture.width, (float)ufo.texture.height };
    ufo.size = (Vector2){ ufo.textureRect.width * 1.5f, ufo.textureRect.height * 1.5f };
    ufo.active = false;
    //ufo.spawnTimer = GetRandomValue(600, 1200); // Frames until first potential spawn
    ufo.spawnTimer = GetRandomValue((int)(UFO_SPAWN_INTERVAL_MIN * 100), (int)(UFO_SPAWN_INTERVAL_MAX * 100)) / 100.0f; 
    ufo.explosionTexture = ufoExplosionTexture;
    ufo.explosionTextureRect = (Rectangle){0, 0, (float)ufo.explosionTexture.width, (float)ufo.explosionTexture.height};


    // Init Explosions
    for(int i=0; i<MAX_EXPLOSIONS; ++i) explosions[i].active = false;


    InitAliens();
    InitShields();

    currentScreen = TITLE; // Go to title screen after init
}

void InitAliens(void) {
    aliensAlive = 0;
    float startX = 80.0f;
    float startY = 80.0f;
    float spacingX = 45.0f;
    float spacingY = 35.0f;

    for (int r = 0; r < ALIENS_ROWS; r++) {
        for (int c = 0; c < ALIENS_COLS; c++) {
            int i = r * ALIENS_COLS + c;
            aliens[i].basePosition = (Vector2){ startX + c * spacingX, startY + r * spacingY };
            aliens[i].position = aliens[i].basePosition;
            aliens[i].active = true;
            aliens[i].currentFrame = false;

            if (r == 0) { // Top row
                aliens[i].type = ALIEN_TYPE_3;
                aliens[i].texture1 = alienTexture3_1;
                aliens[i].texture2 = alienTexture3_2;
                aliens[i].points = 30;
                 aliens[i].color = WHITE; // Or specific colors: PINK
            } else if (r < 3) { // Middle rows
                aliens[i].type = ALIEN_TYPE_2;
                aliens[i].texture1 = alienTexture2_1;
                aliens[i].texture2 = alienTexture2_2;
                aliens[i].points = 20;
                 aliens[i].color = WHITE; // Or specific colors: YELLOW
            } else { // Bottom rows
                aliens[i].type = ALIEN_TYPE_1;
                aliens[i].texture1 = alienTexture1_1;
                aliens[i].texture2 = alienTexture1_2;
                aliens[i].points = 10;
                 aliens[i].color = WHITE; // Or specific colors: LIME
            }
            aliens[i].textureRect = (Rectangle){ 0, 0, (float)aliens[i].texture1.width, (float)aliens[i].texture1.height };
            aliens[i].size = (Vector2){ aliens[i].textureRect.width * 1.5f, aliens[i].textureRect.height * 1.5f };

            aliensAlive++;
        }
    }

    alienMoveWaitTime = ALIEN_MOVE_WAIT_TIME_START / (1.0f + (currentWave - 1) * 0.2f); // Faster start on later waves
    alienMoveTimer = alienMoveWaitTime;
    alienDirection = 1;
    moveDown = false;
    alienMoveSoundIndex = 0;
    alienShootTimer = GetRandomValue(ALIEN_SHOOT_INTERVAL_MIN * 100, ALIEN_SHOOT_INTERVAL_MAX * 100) / 100.0f;
}

void InitShields(void) {
    float shieldSpacing = (SCREEN_WIDTH - (NUM_SHIELDS * shieldTexture.width * 2.0f)) / (NUM_SHIELDS + 1); // Scaled width
    float shieldY = SCREEN_HEIGHT - 120.0f;

    for (int i = 0; i < NUM_SHIELDS; i++) {
        shields[i].baseTexture = shieldTexture;
        // Create a render texture for each shield
        shields[i].renderTexture = LoadRenderTexture(shields[i].baseTexture.width, shields[i].baseTexture.height);
        shields[i].position = (Vector2){ shieldSpacing + i * (shields[i].baseTexture.width * 2.0f + shieldSpacing), shieldY };
        shields[i].active = true;
        shields[i].bounds = (Rectangle){ shields[i].position.x, shields[i].position.y, shields[i].baseTexture.width * 2.0f, shields[i].baseTexture.height * 2.0f }; // Scaled bounds

        // Initial draw of the shield onto its render texture
        BeginTextureMode(shields[i].renderTexture);
            ClearBackground(BLANK); // Important: Clear with transparent
            DrawTexture(shields[i].baseTexture, 0, 0, WHITE);
        EndTextureMode();
    }
}

//----------------------------------------------------------------------------------
// Module Functions Definition - Game Update
//----------------------------------------------------------------------------------
void UpdateGame(void)
{
    if (gameOver) {
        if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP)) {
            InitGame(); // Restart
        }
        return;
    }

    if (IsKeyPressed(KEY_P)) gamePaused = !gamePaused;

    if (gamePaused) {
        // Add pause logic if needed (e.g., handle unpause)
        if (IsKeyPressed(KEY_P)) gamePaused = !gamePaused; // Unpause
        return; // Skip update if paused
    }

    float delta = GetFrameTime();

    // Player Control
    if (player.explosionTimer <= 0) { // Only allow control if not exploding
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) player.position.x -= PLAYER_SPEED;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) player.position.x += PLAYER_SPEED;

        // Touch controls (simple half-screen) - Rely on Mouse Button Down for touch
        // IsGestureDown(GESTURE_DRAG) is not a standard raylib function
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) { // Mouse button down maps to touch press on web
             Vector2 touchPos = GetMousePosition();
             if (touchPos.x < SCREEN_WIDTH / 2) player.position.x -= PLAYER_SPEED;
             else player.position.x += PLAYER_SPEED;
        }


        // Keep player on screen
        if (player.position.x < 0) player.position.x = 0;
        if (player.position.x > SCREEN_WIDTH - player.size.x) player.position.x = SCREEN_WIDTH - player.size.x;

        // Player Shooting
        if (IsKeyPressed(KEY_SPACE) || IsGestureDetected(GESTURE_TAP)) { // Keep GESTURE_TAP for shooting
            SpawnPlayerShot();
        }
    } else {
        player.explosionTimer -= delta;
        if (player.explosionTimer <= 0) {
            player.lives--;
            if (player.lives <= 0) {
                gameOver = true;
                // SaveHighScore(score);
            } else {
                // Reset player position for respawn
                 player.position = (Vector2){ SCREEN_WIDTH / 2.0f - player.size.x / 2.0f, SCREEN_HEIGHT - player.size.y - 20.0f };
                 // Add brief invincibility? (optional)
            }
        }
    }

    UpdateAliens(delta);
    UpdateBullets(delta);
    UpdateUFO(delta);
    UpdateExplosions(delta);
    CheckCollisions();

     // Check Win Condition (All aliens destroyed)
    if (aliensAlive <= 0 && !ufo.active && player.explosionTimer <= 0) {
        NextLevel();
    }

    // Check Lose Condition (Aliens reach bottom)
    for (int i = 0; i < NUM_ALIENS; i++) {
        if (aliens[i].active) {
            if (aliens[i].position.y + aliens[i].size.y >= player.position.y) {
                 gameOver = true;
                 PlaySound(explosionSound); // Player dies even if not shot
                 // SaveHighScore(score);
                 break;
            }
             // Check if aliens reached shield level
            for(int s=0; s<NUM_SHIELDS; ++s) {
                if (shields[s].active && CheckCollisionRecs((Rectangle){aliens[i].position.x, aliens[i].position.y, aliens[i].size.x, aliens[i].size.y}, shields[s].bounds)) {
                   // Damage shield significantly or destroy it if aliens touch it
                   DamageShield(s, (Vector2){aliens[i].position.x + aliens[i].size.x/2, aliens[i].position.y + aliens[i].size.y});
                   DamageShield(s, (Vector2){aliens[i].position.x + aliens[i].size.x/2, aliens[i].position.y + aliens[i].size.y});
                   DamageShield(s, (Vector2){aliens[i].position.x + aliens[i].size.x/2, aliens[i].position.y + aliens[i].size.y});
                   // Maybe deactivate alien? Or let them pass through destroyed shields?
                }
            }
        }
    }
}

void UpdateAliens(float delta) {
    alienMoveTimer -= delta;

    if (alienMoveTimer <= 0) {
        moveDown = false;
        float leftmost = SCREEN_WIDTH;
        float rightmost = 0;

        // Check bounds and find edges
        for (int i = 0; i < NUM_ALIENS; i++) {
            if (aliens[i].active) {
                if (aliens[i].position.x < leftmost) leftmost = aliens[i].position.x;
                if (aliens[i].position.x + aliens[i].size.x > rightmost) rightmost = aliens[i].position.x + aliens[i].size.x;
            }
        }

        // Check if edge hit
        if ((rightmost + alienHorizontalMove * alienDirection > SCREEN_WIDTH && alienDirection > 0) ||
            (leftmost + alienHorizontalMove * alienDirection < 0 && alienDirection < 0)) {
            alienDirection *= -1;
            moveDown = true;
        }

        // Move aliens
        for (int i = 0; i < NUM_ALIENS; i++) {
            if (aliens[i].active) {
                if (moveDown) {
                    aliens[i].position.y += alienVerticalMove;
                } else {
                    aliens[i].position.x += alienHorizontalMove * alienDirection;
                }
                // Switch animation frame
                aliens[i].currentFrame = !aliens[i].currentFrame;
            }
        }

        // Play move sound
        switch(alienMoveSoundIndex) {
            case 0: PlaySound(fastInvaderSound1); break;
            case 1: PlaySound(fastInvaderSound2); break;
            case 2: PlaySound(fastInvaderSound3); break;
            case 3: PlaySound(fastInvaderSound4); break;
        }
        alienMoveSoundIndex = (alienMoveSoundIndex + 1) % 4;

        // Reset timer
        alienMoveTimer = alienMoveWaitTime;
    }

    // Alien Shooting Logic
    alienShootTimer -= delta;
    if (alienShootTimer <= 0 && aliensAlive > 0) {
        int tries = 0;
        bool shotFired = false;
        while(tries < NUM_ALIENS && !shotFired) { // Limit tries to avoid infinite loop if logic fails
            int shooterIndex = GetRandomValue(0, NUM_ALIENS - 1);

            if (aliens[shooterIndex].active) {
                // Check if this alien is clear below (simplification: just fire)
                // More complex: Check if another alien is directly below it.
                bool clearBelow = true;
                /* // Optional: Check if blocked below
                Rectangle shooterRect = { aliens[shooterIndex].position.x, aliens[shooterIndex].position.y, aliens[shooterIndex].size.x, aliens[shooterIndex].size.y };
                for (int i = 0; i < NUM_ALIENS; i++) {
                    if (i == shooterIndex || !aliens[i].active) continue;
                    Rectangle otherRect = { aliens[i].position.x, aliens[i].position.y, aliens[i].size.x, aliens[i].size.y };
                    // Check if 'other' is below and horizontally overlapping 'shooter'
                    if (otherRect.y > shooterRect.y &&
                        otherRect.x < shooterRect.x + shooterRect.width &&
                        otherRect.x + otherRect.width > shooterRect.x) {
                        clearBelow = false;
                        break;
                    }
                }
                */

                if (clearBelow) {
                    Vector2 shotPos = { aliens[shooterIndex].position.x + aliens[shooterIndex].size.x / 2 - alienBullets[0].size.x / 2,
                                        aliens[shooterIndex].position.y + aliens[shooterIndex].size.y };
                    SpawnAlienShot(shotPos);
                    shotFired = true;
                }
            }
             tries++;
        }

        // Reset shoot timer with some randomness, scaling with fewer aliens
        float shootIntervalMultiplier = ((float)aliensAlive / NUM_ALIENS) * 0.5f + 0.5f; // Becomes faster (0.5x to 1.0x interval) as aliens die
        alienShootTimer = (GetRandomValue(ALIEN_SHOOT_INTERVAL_MIN * 100, ALIEN_SHOOT_INTERVAL_MAX * 100) / 100.0f) * shootIntervalMultiplier;
        if (alienShootTimer < 0.1f) alienShootTimer = 0.1f; // Minimum interval cap
    }
}


void UpdateBullets(float delta) {
    // Player Bullet
    if (player.shotActive) {
        player.shotPosition.y -= PLAYER_BULLET_SPEED;
        if (player.shotPosition.y + player.shotSize.y < 0) {
            player.shotActive = false;
        }
    }

    // 5. Alien Shots vs Shields
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++)
    {
        if (!alienBullets[i].active) continue;
        Rectangle bulletRect = {alienBullets[i].position.x, alienBullets[i].position.y,
                                alienBullets[i].size.x, alienBullets[i].size.y};
        for (int s = 0; s < NUM_SHIELDS; s++)
        {
#ifdef UNIT_TEST
            {
                Vector2 wh; Color px;
                if (TestAlienBulletShieldCollision(i, s, &wh, &px))
                {
                    assert(px.a > 10 && "UNIT_TEST: expected opaque pixel → damage");
                    alienBullets[i].active = false;
                    printf("[UNIT_TEST] calling DamageShield(%d)\n", s);
                    DamageShield(s, wh);
                }
            }
            break;
#else
            if (shields[s].active && CheckCollisionRecs(bulletRect, shields[s].bounds))
            {
                Vector2 worldHit = {bulletRect.x + bulletRect.width * 0.5f,
                                     bulletRect.y + bulletRect.height};
                Vector2 texHit = WorldToShieldTexCoords(s, worldHit);

                Image texImg = LoadImageFromTexture(shields[s].renderTexture.texture);
                Color px = GetImageColor(texImg, (int)texHit.x, (int)texHit.y);
                UnloadImage(texImg);

                if (px.a > 10)
                {
                    alienBullets[i].active = false;
                    DamageShield(s, worldHit);
                    SpawnExplosion(
                        (Vector2){bulletRect.x + bulletRect.width*0.5f, bulletRect.y + bulletRect.height},
                        shotExplosionTexture,
                        (Rectangle){0,0,(float)shotExplosionTexture.width,(float)shotExplosionTexture.height},
                        (Vector2){shotExplosionTexture.width, shotExplosionTexture.height}
                    );
                }
                break;
            }
#endif
        }
    }


    // Alien Bullets
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (alienBullets[i].active) {
            alienBullets[i].position.y += alienBullets[i].speed; // Use individual speed if needed

            // Animate alien bullet (simple example using rolling textures)
            int frame = ((int)(GetTime() * 10.0f)) % 4; // Cycle through 4 frames based on time
            switch(frame) {
                case 0: alienBullets[i].texture = rollingTexture1; break;
                case 1: alienBullets[i].texture = rollingTexture2; break;
                case 2: alienBullets[i].texture = rollingTexture3; break;
                case 3: alienBullets[i].texture = rollingTexture4; break;
            }
             alienBullets[i].textureRect = (Rectangle){0, 0, (float)alienBullets[i].texture.width, (float)alienBullets[i].texture.height};


            if (alienBullets[i].position.y > SCREEN_HEIGHT) {
                alienBullets[i].active = false;
            }
        }
    }
}

void UpdateUFO(float delta) {
    if (!ufo.active) {
        ufo.spawnTimer -= delta;
#if 0        
        if (ufo.spawnTimer <= 0) {
             // Check chance every frame once timer is up
             if (GetRandomValue(1, (int)(1.0f/UFO_SPAWN_CHANCE)) == 1) {
                 SpawnUFO();
             }
             // Reset timer slightly so it doesn't check *every* frame forever
              ufo.spawnTimer = 0.1f;
        }
#endif
        if (ufo.spawnTimer <= 0) {
            SpawnUFO();
            // The timer will be reset automatically when this spawned UFO goes off-screen or is destroyed.
        }
        return;
    }

    // UFO is active
    ufo.position.x += ufo.speed * delta;
    ufo.timeActive += delta;

    // Play UFO sound (restart periodically for classic effect)
    if (fmodf(ufo.timeActive, 0.5f) < delta) { // Play roughly every 0.5 seconds
         PlaySound(ufoLowSound); // Or alternate ufoLowSound
    }


    // Check if off screen
    if (ufo.speed > 0 && ufo.position.x > SCREEN_WIDTH) {
        ufo.active = false;
         StopSound(ufoLowSound); // Stop sound when offscreen
         ufo.spawnTimer = GetRandomValue(600, 1800) * delta; // Reset spawn timer
    } else if (ufo.speed < 0 && ufo.position.x + ufo.size.x < 0) {
        ufo.active = false;
         StopSound(ufoLowSound);
         ufo.spawnTimer = GetRandomValue(600, 1800) * delta;
    }

    // Update explosion if UFO was hit
    if (ufo.exploding) {
        ufo.explosionTimer -= delta;
        if (ufo.explosionTimer <= 0) {
            ufo.exploding = false;
            ufo.active = false; // Deactivate fully after explosion
            ufo.spawnTimer = GetRandomValue((int)(UFO_SPAWN_INTERVAL_MIN * 100), (int)(UFO_SPAWN_INTERVAL_MAX * 100)) / 100.0f; // Correct time-based reset
        }
    }

}

void UpdateExplosions(float delta) {
    for(int i=0; i<MAX_EXPLOSIONS; ++i) {
        if (explosions[i].active) {
            explosions[i].timer -= delta;
            if (explosions[i].timer <= 0) {
                explosions[i].active = false;
            }
        }
    }
}

void SpawnExplosion(Vector2 position, Texture2D tex, Rectangle texRect, Vector2 size) {
    for(int i=0; i<MAX_EXPLOSIONS; ++i) {
        if (!explosions[i].active) {
            explosions[i].active = true;
            explosions[i].position = (Vector2){position.x - size.x/2, position.y - size.y/2}; // Center explosion
            explosions[i].texture = tex;
            explosions[i].textureRect = texRect;
            explosions[i].size = size;
            explosions[i].timer = 0.3f; // Duration of explosion display
            // Maybe play a generic small explosion sound here?
            return; // Spawn only one
        }
    }
}

void DamageShield(int shieldIndex, Vector2 hitPosition)
{
    if (!shields[shieldIndex].active) return;
    
    Vector2 localHit = WorldToShieldTexCoords(shieldIndex, hitPosition);
    const float damageRadius = 5.0f;
    
    // Get the texture image data
    Image shieldImage = LoadImageFromTexture(shields[shieldIndex].renderTexture.texture);
    
    // Create a damage pattern in the image
    for (int y = 0; y < shieldImage.height; y++) {
        for (int x = 0; x < shieldImage.width; x++) {
            float distance = sqrtf((x - localHit.x) * (x - localHit.x) + 
                                  (y - localHit.y) * (y - localHit.y));
            
            if (distance <= damageRadius) {
                // Make pixels transparent within the damage radius
                Color* pixel = &((Color*)shieldImage.data)[y * shieldImage.width + x];
                pixel->a = 0; // Set alpha to transparent
            }
        }
    }
    
    // Update the texture with the modified image data
    UpdateTexture(shields[shieldIndex].renderTexture.texture, shieldImage.data);
    
    // Free the image data
    UnloadImage(shieldImage);
    
    // Print for debugging
    //printf("[UNIT_TEST] DamageShield(%d) hit=(%.1f,%.1f) tex=(%.1f,%.1f)\n",
    //       shieldIndex, hitPosition.x, hitPosition.y, localHit.x, localHit.y);
}

#if 0
// Gemini version
void DamageShield(int shieldIndex, Vector2 hitPosition)
{
    if (!shields[shieldIndex].active) return;

    Vector2 localHit = WorldToShieldTexCoords(shieldIndex, hitPosition);

    const float damageRadius = 5.0f;
    const float damageSize   = damageRadius * 2.0f;

    // Print for debugging
    printf("[UNIT_TEST] DamageShield(%d) hit=(%.1f,%.1f) tex=(%.1f,%.1f)\n",
           shieldIndex, hitPosition.x, hitPosition.y, localHit.x, localHit.y);

    BeginTextureMode(shields[shieldIndex].renderTexture);
        DrawRectangleRec((Rectangle){ localHit.x - damageRadius,
                                      localHit.y - damageRadius,
                                      damageSize, damageSize },
                         BLANK);
        // Draw a black circle for debugging
        DrawCircle((int)localHit.x, (int)localHit.y, damageRadius, BLACK); 
    EndTextureMode();
}
    #endif

#if 0
void DamageShield(int shieldIndex, Vector2 hitPosition) {
    if (!shields[shieldIndex].active) return;

    // Calculate hit position relative to the shield's render texture
    Vector2 localHit;
    localHit.x = hitPosition.x - shields[shieldIndex].position.x;
    localHit.y = hitPosition.y - shields[shieldIndex].position.y;

    // Scale local hit position to the original texture coordinates (since we scale drawing)
    localHit.x /= (shields[shieldIndex].bounds.width / shields[shieldIndex].renderTexture.texture.width);
    localHit.y /= (shields[shieldIndex].bounds.height / shields[shieldIndex].renderTexture.texture.height);


    // Define the damage radius/size
    float damageRadius = 5.0f; // Size of the hole to make (half-width of rectangle)
    float damageSize = damageRadius * 2.0f; // Full width/height of rectangle

    // Draw transparent rectangle onto the render texture at the hit location
    BeginTextureMode(shields[shieldIndex].renderTexture);
        // DrawRectangleBlend or DrawCircleBlend not standard, use DrawRectangleRec with BLANK
        DrawRectangleRec(
            (Rectangle){localHit.x - damageRadius, localHit.y - damageRadius, damageSize, damageSize},
            BLANK // BLANK color makes the pixels transparent (Color){ 0, 0, 0, 0 }
        );
    EndTextureMode();
    // Optional: Check if shield is destroyed (e.g., count transparent pixels - expensive!)
    // Simpler: Deactivate after N hits (needs a counter per shield)
}
#endif


//----------------------------------------------------------------------------------
// Module Functions Definition - Collision Detection
//----------------------------------------------------------------------------------
void CheckCollisions() {

    // --- Player Shot Collisions ---

    if (player.shotActive) {
        Rectangle playerShotRect = { player.shotPosition.x, player.shotPosition.y, player.shotSize.x, player.shotSize.y };

        // 1. Player Shot vs Aliens
        for (int i = 0; i < NUM_ALIENS; i++) {
            if (aliens[i].active) {
                Rectangle alienRect = { aliens[i].position.x, aliens[i].position.y, aliens[i].size.x, aliens[i].size.y };
                if (CheckCollisionRecs(playerShotRect, alienRect)) {
                    player.shotActive = false;
                    aliens[i].active = false;
                    aliensAlive--;
                    score += aliens[i].points;
                    if (score > hiScore) hiScore = score;

                    SpawnExplosion(
                        (Vector2){aliens[i].position.x + aliens[i].size.x/2, aliens[i].position.y + aliens[i].size.y/2},
                        alienExplosionTexture,
                        (Rectangle){0,0, (float)alienExplosionTexture.width, (float)alienExplosionTexture.height},
                        (Vector2){(float)alienExplosionTexture.width * 1.5f, (float)alienExplosionTexture.height * 1.5f}
                    );
                    PlaySound(invaderKilledSound);
                    alienMoveWaitTime *= ALIEN_MOVE_SPEEDUP_FACTOR;
                    if (alienMoveWaitTime < 0.05f) alienMoveWaitTime = 0.05f;
                    goto next_collision_check; // Exit alien loop once shot hits
                }
            }
        }

        // 2. Player Shot vs UFO
        if (ufo.active && !ufo.exploding) {
            Rectangle ufoRect = { ufo.position.x, ufo.position.y, ufo.size.x, ufo.size.y };
            if (CheckCollisionRecs(playerShotRect, ufoRect)) {
                player.shotActive = false;
                ufo.exploding = true;
                ufo.explosionTimer = 0.5f;
                score += UFO_POINTS; // Using defined constant
                if (score > hiScore) hiScore = score;
                StopSound(ufoLowSound);
                PlaySound(ufoExplosionSound);
                goto next_collision_check; // Exit checks for this shot
            }
        }

        // 3. Player Shot vs Shields
        for (int i = 0; i < NUM_SHIELDS; i++) {
            if (shields[i].active && CheckCollisionRecs(playerShotRect, shields[i].bounds)) {
                Vector2 worldHit = { playerShotRect.x + playerShotRect.width * 0.5f, playerShotRect.y }; // Top of bullet
                Vector2 texHit = WorldToShieldTexCoords(i, worldHit);

                // Optimization: Avoid LoadImageFromTexture every frame if possible
                // For now, keeping the precise check:
                Image texImg = LoadImageFromTexture(shields[i].renderTexture.texture);
                Color px = GetImageColor(texImg, (int)texHit.x, (int)texHit.y);
                UnloadImage(texImg);

                if (px.a > 10) { // Opaque pixel hit
                    player.shotActive = false;
                    DamageShield(i, worldHit);
                    SpawnExplosion(worldHit, shotExplosionTexture,
                                   (Rectangle){0, 0, (float)shotExplosionTexture.width, (float)shotExplosionTexture.height},
                                   (Vector2){shotExplosionTexture.width * 1.5f, shotExplosionTexture.height * 1.5f});
                    goto next_collision_check; // Exit shield loop and checks for this shot
                }
                // If transparent, bullet passes through
            }
        }
    }

next_collision_check: // Label used by goto to skip further checks for the same player shot

    // --- Alien Shot Collisions ---

    // 4. Alien Shots vs Player   <--- THIS WAS THE MISSING PART
    if (player.explosionTimer <= 0) { // Player can only be hit if not already exploding
        Rectangle playerRect = { player.position.x, player.position.y, player.size.x, player.size.y };
        for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
            if (alienBullets[i].active) {
                Rectangle bulletRect = { alienBullets[i].position.x, alienBullets[i].position.y, alienBullets[i].size.x, alienBullets[i].size.y };
                if (CheckCollisionRecs(bulletRect, playerRect)) {
                    alienBullets[i].active = false;
                    player.explosionTimer = 1.0f; // Start player explosion timer
                    PlaySound(explosionSound);    // Play player death sound
                    // Lives are decremented in UpdateGame when the timer runs out
                    break; // Player hit, no need to check other bullets against player this frame
                }
            }
        }
    } // End of Alien Shots vs Player Check


    // 5. Alien Shots vs Shields
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (!alienBullets[i].active) continue; // Skip inactive bullets or bullets that just hit the player

        Rectangle bulletRect = { alienBullets[i].position.x, alienBullets[i].position.y, alienBullets[i].size.x, alienBullets[i].size.y };
        for (int s = 0; s < NUM_SHIELDS; s++) {
            if (shields[s].active && CheckCollisionRecs(bulletRect, shields[s].bounds)) {
                Vector2 worldHit = { bulletRect.x + bulletRect.width * 0.5f, bulletRect.y + bulletRect.height }; // Bottom of bullet
                Vector2 texHit = WorldToShieldTexCoords(s, worldHit);

                Image texImg = LoadImageFromTexture(shields[s].renderTexture.texture);
                Color px = GetImageColor(texImg, (int)texHit.x, (int)texHit.y);
                UnloadImage(texImg);

                if (px.a > 10) { // Opaque pixel hit
                    alienBullets[i].active = false; // Deactivate bullet
                    DamageShield(s, worldHit);
                    SpawnExplosion(worldHit, shotExplosionTexture, // Use shot explosion for bullet hitting shield
                                   (Rectangle){0, 0, (float)shotExplosionTexture.width, (float)shotExplosionTexture.height},
                                   (Vector2){shotExplosionTexture.width, shotExplosionTexture.height}); // Maybe smaller explosion
                    goto next_alien_bullet; // Stop checking this bullet against other shields
                }
                // If transparent, bullet passes through
            }
        }
    next_alien_bullet:; // Label used by goto
    } // End of Alien Bullet Loop


    // --- Alien vs Shield Collision (When aliens reach them) ---
    // This logic is currently in UpdateGame() where aliens move down. It can stay there.

} // End of CheckCollisions


void SpawnPlayerShot() {
    if (!player.shotActive && player.explosionTimer <= 0) {
        player.shotActive = true;
        player.shotPosition.x = player.position.x + player.size.x / 2 - player.shotSize.x / 2;
        player.shotPosition.y = player.position.y - player.shotSize.y;
        PlaySound(shootSound);
    }
}

void SpawnAlienShot(Vector2 position) {
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (!alienBullets[i].active) {
            alienBullets[i].active = true;
            alienBullets[i].position = position;
            // Add randomness to bullet type/speed later if needed
            return; // Spawn only one
        }
    }
}

void SpawnUFO() {
    ufo.active = true;
    ufo.exploding = false;
    ufo.explosionTimer = 0.0f;
    ufo.timeActive = 0.0f;
    // Random direction
    if (GetRandomValue(0, 1) == 0) { // From left
        ufo.position = (Vector2){ -ufo.size.x, 50.0f };
        ufo.speed = UFO_SPEED;
    } else { // From right
        ufo.position = (Vector2){ SCREEN_WIDTH, 50.0f };
        ufo.speed = -UFO_SPEED;
    }
     PlaySound(ufoLowSound); // Start sound
}

void ResetLevel(bool playerDied) {
     // Reset aliens to top
    InitAliens();
    // Reset player position
    player.position = (Vector2){ SCREEN_WIDTH / 2.0f - player.size.x / 2.0f, SCREEN_HEIGHT - player.size.y - 20.0f };
    // Deactivate all bullets
    player.shotActive = false;
    for(int i=0; i<MAX_ALIEN_BULLETS; ++i) alienBullets[i].active = false;
    // Deactivate UFO
    ufo.active = false;
    StopSound(ufoLowSound);
    // Reset Shields (only if starting new game or new wave, not on player death?) - Classic game keeps shield damage.
    // If we want to reset shields on death/new wave:
    // for(int i=0; i<NUM_SHIELDS; ++i) UnloadRenderTexture(shields[i].renderTexture);
    // InitShields();

    // If player died, potentially keep score, decrease life (already handled by explosion timer end)
    // If new wave, reset alien speed multiplier? No, it should persist within a game.
}

void NextLevel() {
    currentWave++;
    // Increase base speed slightly for the new wave? (Handled by InitAliens)
    // Reset aliens
    InitAliens();
    // Reset player position
    player.position = (Vector2){ SCREEN_WIDTH / 2.0f - player.size.x / 2.0f, SCREEN_HEIGHT - player.size.y - 20.0f };
     // Deactivate bullets
    player.shotActive = false;
    for(int i=0; i<MAX_ALIEN_BULLETS; ++i) alienBullets[i].active = false;
    // Reset UFO spawn timer potentially faster
    ufo.spawnTimer = GetRandomValue((int)(UFO_SPAWN_INTERVAL_MIN * 100), (int)(UFO_SPAWN_INTERVAL_MAX * 100)) / 100.0f; 
    // Reset shields? (Classic game keeps damage)
    // If resetting shields:
    for(int i=0; i<NUM_SHIELDS; ++i) UnloadRenderTexture(shields[i].renderTexture); // Unload old ones first
    InitShields();

    // Add brief "Wave X" message?
}


//----------------------------------------------------------------------------------
// Module Functions Definition - Game Drawing
//----------------------------------------------------------------------------------
void DrawGame(void)
{
    BeginDrawing();
        ClearBackground(BLACK);

        if (gameOver) {
            DrawText("GAME OVER", SCREEN_WIDTH/2 - MeasureText("GAME OVER", 40)/2, SCREEN_HEIGHT/2 - 40, 40, RED);
            DrawText(TextFormat("FINAL SCORE: %d", score), SCREEN_WIDTH/2 - MeasureText(TextFormat("FINAL SCORE: %d", score), 20)/2, SCREEN_HEIGHT/2 + 10, 20, RAYWHITE);
            DrawText("PRESS [ENTER] or TAP TO RESTART", SCREEN_WIDTH/2 - MeasureText("PRESS [ENTER] or TAP TO RESTART", 20)/2, SCREEN_HEIGHT/2 + 40, 20, LIGHTGRAY);
        } else {
            // Draw Shields
            for (int i = 0; i < NUM_SHIELDS; i++) {
                if (shields[i].active) {
                    // Draw the render texture, scaled up
                    #if 1
                    DrawTexturePro(shields[i].renderTexture.texture,
                                   (Rectangle){ 0, 0, (float)shields[i].renderTexture.texture.width, (float)-shields[i].renderTexture.texture.height }, // Source rect, Y flipped!
                                   shields[i].bounds, // Destination rect (already scaled)
                                   (Vector2){ 0, 0 }, // Origin
                                   0.0f, WHITE);
                    #endif
                }
            }

             // Draw Aliens
            for (int i = 0; i < NUM_ALIENS; i++) {
                if (aliens[i].active) {
                    DrawTexturePro(aliens[i].currentFrame ? aliens[i].texture2 : aliens[i].texture1,
                                   aliens[i].textureRect,
                                   (Rectangle){ aliens[i].position.x, aliens[i].position.y, aliens[i].size.x, aliens[i].size.y },
                                   (Vector2){ 0, 0 }, 0.0f, aliens[i].color);
                }
            }

            // Draw Player
            if (player.explosionTimer > 0) {
                // Draw explosion centered on player pos
                 DrawTexturePro(playerExplosionTexture,
                               (Rectangle){0,0, (float)playerExplosionTexture.width, (float)playerExplosionTexture.height},
                               (Rectangle){ player.position.x + player.size.x/2 - playerExplosionTexture.width, // Center explosion roughly
                                            player.position.y + player.size.y/2 - playerExplosionTexture.height,
                                            (float)playerExplosionTexture.width * 2.0f, (float)playerExplosionTexture.height * 2.0f },
                               (Vector2){ 0, 0 }, 0.0f, WHITE);
            } else if (player.lives > 0) {
                DrawTexturePro(player.texture, player.textureRect,
                               (Rectangle){ player.position.x, player.position.y, player.size.x, player.size.y },
                               (Vector2){ 0, 0 }, 0.0f, WHITE);
            }


            // Draw Player Shot
            if (player.shotActive) {
                 DrawTexturePro(player.shotTexture, player.shotTextureRect,
                               (Rectangle){ player.shotPosition.x, player.shotPosition.y, player.shotSize.x, player.shotSize.y },
                               (Vector2){ 0, 0 }, 0.0f, WHITE);
            }

            // Draw Alien Shots
            for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
                if (alienBullets[i].active) {
                    DrawTexturePro(alienBullets[i].texture, alienBullets[i].textureRect,
                                   (Rectangle){ alienBullets[i].position.x, alienBullets[i].position.y, alienBullets[i].size.x, alienBullets[i].size.y },
                                   (Vector2){ 0, 0 }, 0.0f, WHITE);
                }
            }

            // Draw UFO
            if (ufo.active) {
                 if (ufo.exploding) {
                     // Draw UFO explosion centered
                     DrawTexturePro(ufo.explosionTexture, ufo.explosionTextureRect,
                                   (Rectangle){ ufo.position.x + ufo.size.x/2 - ufo.explosionTexture.width*1.5f/2, // Center explosion
                                                ufo.position.y + ufo.size.y/2 - ufo.explosionTexture.height*1.5f/2,
                                                ufo.explosionTexture.width * 1.5f, ufo.explosionTexture.height * 1.5f },
                                   (Vector2){ 0, 0 }, 0.0f, WHITE);
                 } else {
                      DrawTexturePro(ufo.texture, ufo.textureRect,
                                   (Rectangle){ ufo.position.x, ufo.position.y, ufo.size.x, ufo.size.y },
                                   (Vector2){ 0, 0 }, 0.0f, RED); // UFO is often red
                 }
            }

             // Draw Explosions
             for(int i=0; i<MAX_EXPLOSIONS; ++i) {
                if (explosions[i].active) {
                    DrawTexturePro(explosions[i].texture, explosions[i].textureRect,
                                   (Rectangle){ explosions[i].position.x, explosions[i].position.y, explosions[i].size.x, explosions[i].size.y },
                                   (Vector2){ 0, 0 }, 0.0f, WHITE);
                }
             }


            // Draw UI
            DrawText(TextFormat("SCORE: %04d", score), 10, 10, 20, RAYWHITE);
            DrawText(TextFormat("HI-SCORE: %04d", hiScore), SCREEN_WIDTH / 2 - MeasureText("HI-SCORE: 0000", 20)/2, 10, 20, RAYWHITE);
            DrawText(TextFormat("WAVE: %d", currentWave), SCREEN_WIDTH - 100, SCREEN_HEIGHT - 30, 20, LIGHTGRAY);

            // Draw Lives
            for (int i = 0; i < player.lives; i++) {
                DrawTextureEx(player.texture, (Vector2){ (float)(SCREEN_WIDTH - 110 + i * (player.texture.width * 0.7f + 5)), 10.0f }, 0.0f, 0.7f, WHITE);
            }
            if (player.lives > 0) DrawText("LIVES:", SCREEN_WIDTH - 110 - MeasureText("LIVES: ", 20), 10, 20, RAYWHITE);


            if (gamePaused) {
                DrawText("PAUSED", SCREEN_WIDTH/2 - MeasureText("PAUSED", 40)/2, SCREEN_HEIGHT/2 - 20, 40, GRAY);
            }
        }

        // Draw FPS (optional)
        //DrawFPS(SCREEN_WIDTH - 90, 10);

    EndDrawing();
}

//----------------------------------------------------------------------------------
// Module Functions Definition - Game Unloading
//----------------------------------------------------------------------------------
void UnloadGame(void)
{
    // Unload render textures
    for(int i=0; i<NUM_SHIELDS; ++i) {
        if (shields[i].renderTexture.id != 0) UnloadRenderTexture(shields[i].renderTexture);
    }
    // Resource unloading is handled separately in UnloadResources()
}

//----------------------------------------------------------------------------------
// Module Functions Definition - Update and Draw Frame Loop
//----------------------------------------------------------------------------------
void UpdateDrawFrame(void)
{
    framesCounter++;

    switch(currentScreen)
    {
        case LOGO:
        {
            // TODO: Implement splash screen logic
            if (framesCounter > 120) { // e.g., wait 2 seconds
                 currentScreen = TITLE;
                 framesCounter = 0; // Reset counter for next screen
            }
             BeginDrawing();
                ClearBackground(BLACK);
                DrawText("LOGO SCREEN", 20, 20, 40, LIGHTGRAY);
                DrawText("WAIT for 2 SECONDS...", 290, 220, 20, GRAY);
            EndDrawing();

        } break;
        case TITLE:
        {
            if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP)) {
                 // Reset necessary game state before starting gameplay
                 InitAliens(); // Re-init aliens for first wave
                 // Reset player lives and score if restarting from Title
                 player.lives = 3;
                 score = 0;
                 currentWave = 1;
                 // Reset shields completely
                 for(int i=0; i<NUM_SHIELDS; ++i) UnloadRenderTexture(shields[i].renderTexture);
                 InitShields();

                 currentScreen = GAMEPLAY;
            }
             BeginDrawing();
                ClearBackground(BLACK);
                DrawText("SPACE INVADERS", SCREEN_WIDTH/2 - MeasureText("SPACE INVADERS", 40)/2, SCREEN_HEIGHT/2 - 80, 40, GREEN);
                DrawText("PRESS [ENTER] or TAP to START", SCREEN_WIDTH/2 - MeasureText("PRESS [ENTER] or TAP to START", 20)/2, SCREEN_HEIGHT/2 - 10, 20, RAYWHITE);
                DrawText("CONTROLS:", 20, SCREEN_HEIGHT - 60, 20, LIGHTGRAY);
                DrawText("ARROW KEYS / A / D / TOUCH SIDES: MOVE", 20, SCREEN_HEIGHT - 40, 20, LIGHTGRAY);
                DrawText("SPACE / TAP: SHOOT", 20, SCREEN_HEIGHT - 20, 20, LIGHTGRAY);

            EndDrawing();

        } break;
        case GAMEPLAY:
        {
            UpdateGame();
            DrawGame();
             if (gameOver) {
                currentScreen = GAME_OVER;
                framesCounter = 0; // Reset timer for game over screen
            }
        } break;
        case GAME_OVER:
        {
             // Update logic for game over screen (e.g., flashing text)
            framesCounter++;

            // Draw Game Over Screen (already done in DrawGame when gameOver is true, but can add overlays here)
            DrawGame(); // Keep drawing the final state

             // Add specific Game Over overlays if needed
            // if ((framesCounter/30)%2) // Flashing text example
            // {
            //     DrawText("PRESS [ENTER] or TAP TO RESTART", GetScreenWidth()/2 - MeasureText("PRESS [ENTER] or TAP TO RESTART", 20)/2, GetScreenHeight()/2 + 40, 20, YELLOW);
            // }

             if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
            {
                InitGame(); // Reinitialize everything
                currentScreen = TITLE; // Go back to title
            }
        } break;
        default: break;
    }
}

