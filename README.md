gbe-hd is a heavily cut version of gbe+ with the goal of replacing the HD pack feature with one which resembles the HD pack in Mesen. Many features are cut or broken in the process. This implementation of the HD pack feature is not compatible with existing GBE+ HD packs. Output to midi is added to the HD pack feature.

From the original:
# gbe-plus

GB Enhanced+ is a DMG/GBC/GBA emulator and experimental NDS emulator. It is the successor to the original GB Enhanced project with a focus on enhancements and emulating as much of the Game Boy as possible.

## License
GB Enhanced+ as an emulator is Free Open Source Software available under the GPLv2. See license.txt for full details.

Technical documentation and Dan Docs are Public Domain.

## Overview

GB Enhanced+ (GBE+ for short) aims to be a fully functional Game Boy emulator. The goal is to create a highly portable emulator using C++ and SDL, document the Game Boy's functions through clear code, and add as many enhancements (scaling filters, cheats, custom graphics) as reasonably possible. Additionally GBE+ strives to emulate everything about the Game Boy, including obscure accessories.

## Compiling GBE+

The only requirements at this moment are SDL 2.0, OpenGL 3.3 or greater, a C++ compiler, and CMake. Qt (for the GUI) and SDL2_net (for netplay) are optional dependencies. GLEW is a Windows-only mandatory dependency.

## READ THE MANUAL

GBE+ comes with very nice PDF and ODT manuals explaining everything about its operation, and devotes extensive time to covering complex topics such as Custom Graphics (CGFX). PLEASE READ THE MANUAL if you have any questions regarding this emulator.
