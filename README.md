## Vibe coding Invaders

### Description

Here is an article about why I wrote this repo.
https://olof-astrand.medium.com/vibe-coding-an-invaders-game-for-the-web-7f18ad739d0a


## Try it online

https://ebiroll.github.io/invaders/invaders.html


### Features

Sound
### Controls

Keyboard/Mouse:
left arrow/right arrow , space

### Screenshots

_TODO: Show your game to the world, animated GIFs recommended!._

### Developers

To build
make -e PLATFORM=PLATFORM_WEB -B

To test
python -m http.server 8000

Coders
Gemini 2.5 Pro Preview 03-25
Anthropic Claude 3.7
Open AI 03


### Gemini Pro

Resource Loading: Added LoadResources and UnloadResources to handle all the specified textures and sounds. It assumes standard filenames (e.g., inv11.png, not @2x). You might need to adjust filenames if you intended to use the @2x versions.

Game States: Implemented basic LOGO, TITLE, GAMEPLAY, GAME_OVER states.

Structs: Defined appropriate structs for Player, Alien, Bullet, Shield, UFO, Explosion.

Alien Grid & Movement:

    InitAliens creates the classic 5x11 grid.

    UpdateAliens handles the block movement: side-to-side, dropping down at edges.

    Uses the four fastinvaderX.wav sounds in sequence for movement.

    Alien animation flips between texture1 and texture2.

Gradual Speed Increase:

    alienMoveWaitTime decreases slightly (ALIEN_MOVE_SPEEDUP_FACTOR) every time an alien is killed.

    Alien shooting frequency also increases as fewer aliens remain (shootIntervalMultiplier).

    Initial speed increases slightly per wave in InitAliens.

Shields & Damage:

    Uses RenderTexture2D for each shield.

    InitShields draws the base shield texture onto the render texture.

    DamageShield calculates the hit position relative to the shield and draws a BLANK rectangle onto the render texture using BeginTextureMode/EndTextureMode, effectively erasing part of it.

    Collision checks against shields (CheckCollisions) now include a check for the alpha value of the pixel at the collision point on the shield's render texture (LoadImageFromTexture, GetImageColor, UnloadImage) to ensure bullets only damage visible parts. Note: LoadImageFromTexture can be slow; this is a common way but optimization might be needed for many hits per frame.

Shooting:

    Player can only have one shot active (player.shotActive).

    Aliens shoot randomly from the active pool, with increasing frequency. Multiple alien shots (MAX_ALIEN_BULLETS) are possible.

    Alien shots have basic animation using the rollingX.png textures (can be swapped for plunger or squig).

UFO: Spawns periodically, flies across the top, plays its sound (ufo_highpitch.wav), and gives points when shot.

Collisions: CheckCollisions handles:

    Player Shot <-> Alien/UFO/Shield

    Alien Shot <-> Player/Shield

    (Implicit) Alien <-> Player Level (Game Over)

    (Implicit) Alien <-> Shield Level (Damages shield)

Explosions: A simple Explosion struct and system (SpawnExplosion, UpdateExplosions) shows explosion textures briefly for aliens, player shots, and the UFO. Player death also triggers an explosion effect.

Sound: Plays sounds for shooting, alien death, player death, alien movement, and UFO.

Game Flow: Handles starting, pausing (P key), game over, restarting (Enter/Tap), lives, score, high score (display only), and wave progression (NextLevel).

Web Build: Includes the #if defined(PLATFORM_WEB) block and emscripten_set_main_loop.

This is not needed if you use the makefile instead
emcc invaders.c -o invaders.html \
     -I path/to/raylib/src -L path/to/raylib/src -lraylib \
     -s USE_GLFW=3 -s ASYNCIFY \
     -s TOTAL_MEMORY=67108864 --preload-file resources \
     -DPLATFORM_WEB --shell-file path/to/raylib/src/shell.html \
     -O2 # Or -O3 for more optimization




### Links


### License

This project sources are licensed under an 

*Copyright (c) 2024 Ebiroll *
