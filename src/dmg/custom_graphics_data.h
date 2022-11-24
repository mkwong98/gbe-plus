// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : custom_graphics_data.h
// Date : July 23, 2015
// Description : Custom graphics data
//
// Defines the data structure that hold info about custom DMG-GBC graphics
// Only the functions in the dmg_cgfx namespace should use this

#ifndef GB_CGFX_DATA
#define GB_CGFX_DATA

#include <vector>
#include <string>
#include <map>

#include "common/common.h"

struct pack_condition
{
	static const u8 HMIRROR = 0;
	static const u8 VMIRROR = 1;
	static const u8 BGPRIORITY = 2;
	static const u8 PALETTE0 = 3;
	static const u8 PALETTE1 = 4;
	static const u8 PALETTE2 = 5;
	static const u8 PALETTE3 = 6;
	static const u8 PALETTE4 = 7;
	static const u8 PALETTE5 = 8;
	static const u8 PALETTE6 = 9;
	static const u8 PALETTE7 = 10;
	static const u8 TILENEARBY = 11;
	static const u8 SPRITENEARBY = 12;
	static const u8 TILEATPOSITION = 13;
	static const u8 SPRITEATPOSITION = 14;
	static const u8 MEMORYCHECK = 15;
	static const u8 MEMORYCHECKCONSTANT = 16;
	static const u8 FRAMERANGE = 17;

	static const u8 EQ = 0;
	static const u8 NE = 1;
	static const u8 GT = 2;
	static const u8 LS = 3;
	static const u8 GE = 4;
	static const u8 LE = 5;

	std::string name;
	u8 type;

	//find tile
	s16 x;
	s16 y;
	tile_pattern tileData;
	u16 palette[4];

	//mem compare
	u16 address1;
	u16 address2;
	u8 opType;
	u8 value;
	u8 mask;

	//frame range
	u32 divisor;
	u32 compareVal;

	bool latest_result;
	
};

struct pack_cond_app
{
	u16 condIdx;
	bool negate;
};

struct pack_tile
{
	std::vector <pack_cond_app> condApps;
	u16 imgIdx;
	tile_pattern tileData;
	std::string tileStr;
	u16 palette[4];
	std::string palStr;
	u16 x;
	u16 y;
	float brightness;
	bool default;
};

struct pack_background
{
	std::vector <pack_cond_app> condApps;
	s16 imgIdx;
	float brightness;
	float hscroll;
	float vscroll;
	u16 offsetX;
	u16 offsetY;
	u8 priority;
};



struct dmg_cgfx_data
{ 
	std::string packVersion;
	std::vector <std::vector <SDL_Surface*>> imgs;
	std::vector <std::vector <SDL_Surface*>> himgs;
	std::vector <pack_condition> conds;
	std::vector <pack_tile> tiles;
	std::array < std::vector <pack_background>, 60> bgs;
	SDL_Surface* brightnessMod;
	SDL_Surface* tempStrip;
	u32 frameCnt;

	//record how the screen is generated
	u8 vram_tile_used[768];
	u16 vram_tile_idx[768];
	rendered_screen screen_data;

};

#endif // GB_CGFX_DATA
