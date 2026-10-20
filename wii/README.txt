PvZ Portable for Nintendo Wii - v1.1.0
======================================

This package does NOT include any Plants vs. Zombies game assets.
You need your own legally purchased copy of PvZ: GOTY Edition.

Installation
------------
1. Copy the "apps" folder to the root of your SD card.
   (It must be the SD card - the game always loads from sd:/apps/PvZPortable/,
   even when launched from a USB loader.)

2. From your PvZ GOTY install, copy these into sd:/apps/PvZPortable/ :
     main.pak
     properties/

3. (Optional) Music: on a computer, run
     tools/wii-music/render.sh /path/to/main.pak
   from the source repository (needs libopenmpt and libvorbis), then copy
   the resulting "prerendered" folder to:
     sd:/apps/PvZPortable/sounds/prerendered/

4. Launch "PvZ Portable" from the Homebrew Channel, WiiFlow (Homebrew view)
   or USB Loader GX.

Finished SD card layout
-----------------------
sd:/apps/PvZPortable/
    boot.dol
    meta.xml
    icon.png
    main.pak               <- from your copy of the game
    properties/            <- from your copy of the game
    sounds/prerendered/    <- optional, from render.sh
    save/                  <- created by the game

Controls
--------
Wii Remote: point at the screen and press A to click, B to cancel/right-click.
  Held sideways, the d-pad moves the cursor, 2 clicks and 1 cancels.
Nunchuk: stick moves the cursor, Z clicks, C cancels.
Classic Controller / GameCube controller: stick or d-pad moves the cursor,
  A clicks, B cancels.
+ / Start: pause menu.

Co-op (up to 4 players)
-----------------------
Every connected controller gets its own cursor (player 2 red, 3 green,
4 yellow; Wii Remote N or GameCube port N is player N). Any player can click
anything. Whoever picks up a seed packet, the shovel or a tool keeps control
until they use or drop it; meanwhile the others can still collect sun and
coins. Everyone can whack in Whack a Zombie.

Source: https://github.com/Mug-Costanza/PvZ-Portable-Wii-U
Based on: https://github.com/wszqkzqk/PvZ-Portable
