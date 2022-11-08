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
	const u8 HMIRROR = 0;
	const u8 VMIRROR = 1;
	const u8 BGPRIORITY = 2;
	const u8 PALETTE0 = 3;
	const u8 PALETTE1 = 4;
	const u8 PALETTE2 = 5;
	const u8 PALETTE3 = 6;
	const u8 PALETTE4 = 7;
	const u8 PALETTE5 = 8;
	const u8 PALETTE6 = 9;
	const u8 PALETTE7 = 10;
	const u8 TILENEARBY = 11;
	const u8 SPRITENEARBY = 12;
	const u8 TILEATPOSITION = 13;
	const u8 SPRITEATPOSITION = 14;
	const u8 MEMORYCHECK = 15;
	const u8 MEMORYBANKCHECK = 16;
	const u8 FRAMERANGE = 17;

	std::string name;
	u8 type;

	//find tile
	s16 x;
	s16 y;
	u16 tileData[8];
	u16 palette[4];

	//mem compare
	u16 address1;
	u16 address2;
	u8 bank;
	u8 opType;
	u8 value;
	u8 mask;

	//frame range
	u32 divisor;
	u32 compareVal;
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
	u16 palette[4];
	u16 x;
	u16 y;
	float brightness;
	bool default;
};

struct pack_background
{
	std::vector <pack_cond_app> condApps;
	SDL_Surface* img;
	float brightness;
	float hscroll;
	u16 offsetX;
	u16 offsetY;
	u8 priority;
};



struct dmg_cgfx_data
{ 
	std::string packVersion;
	u16 packScale;
	std::vector <std::vector <SDL_Surface*>> imgs;
	std::vector <std::vector <SDL_Surface*>> himgs;
	std::vector <pack_condition> conds;
	std::vector <pack_tile> tiles;
	std::vector <pack_background> bgs;
	SDL_Surface* brightnessMod;
	SDL_Surface* tempStrip;

	std::vector <std::string> manifest;
	std::vector <u8> manifest_entry_size;
	
	//Data pulled from manifest file - Regular entries
	//Hashes - Actual hash data. Duplicated abd sorted into separate OBJ + BG lists
	//Hashes Raw - So called raw hash data (no prepended palette data). May match multiple tiles.
	//Files - Location of the image file used for this hash
	//Types - Determines what system a hash belongs to (DMG, GBC, GBA) and if it's an OBJ or BG
	//ID - Keeps track of which manifest entry corresponds to which OBJ or BG entry
	//VRAM Address - If not zero, this value is computed with the hash
	//Auto Bright - Boolean to auto adjust CGFX assets to the original GBC or GBA palette brightness
	std::map <std::string, u32> m_hashes;
	std::map <std::string, u32> m_hashes_raw;
	std::vector <std::string> m_files;
	std::vector <u8> m_types;
	std::vector <u16> m_id;
	std::vector < std::map <std::string, u32> > m_vram_addr;
	std::vector <u16> m_auto_bright;

	//Data pulled from manifest file - Metatile entries
	//Files - Location of the metatile image file
	//Names - Base name for parsing entries
	//Forms - 1 byte number to identify base type and size
	//ID - Keeps track of where to look for pixel data when generating corresponding normal entries
	std::vector <std::string> m_meta_files;
	std::vector <std::string> m_meta_names;
	std::vector <u8> m_meta_forms;
	std::vector <u32> m_meta_width;
	std::vector <u32> m_meta_height;

	u32 last_id;

	//Working hash list of graphics in VRAM
	std::vector <std::string> current_obj_hash;
	std::vector <std::string> current_bg_hash;
	std::vector <std::string> current_gbc_bg_hash;

	//List of all computed hashes
	std::vector <std::string> obj_hash_list;
	std::vector <std::string> bg_hash_list;

	//Pixel data for all computed hashes (when loading CGFX)
	std::vector< std::vector<u32> > obj_pixel_data;
	std::vector< std::vector<u32> > bg_pixel_data;
	std::vector< std::vector<u32> > meta_pixel_data;

	//List of all tiles that have been updated
	//NOTE - OBJs don't need a list, since the LCD keeps track of OAM updates
	//The LCD does not keep track of BG updates, however.
	std::vector <bool> bg_update_list;
	bool update_bg;
	bool update_map;

	std::vector <bool> bg_tile_update_list;
	std::vector <bool> bg_map_update_list;

	//Palette brightness max and min
	u8 bg_pal_max[8];
	u8 bg_pal_min[8];
	u8 obj_pal_max[8];
	u8 obj_pal_min[8];

	//record how the screen is generated
	u8 vram_tile_used[768];
	u16 vram_tile_idx[768];
	rendered_screen screen_data;

};

#endif // GB_CGFX_DATA
