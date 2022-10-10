// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd_data.h
// Date : May 14, 2015
// Description : Core LCD data
//
// Defines the LCD data structure that the MMU will update whenever values are written in memory
// Only the LCD should read values from this structure. Only the MMU should write values to this structure.

#ifndef GB_LCD_DATA
#define GB_LCD_DATA

#include <vector>

#include "common.h" 

struct dmg_lcd_data
{
	u8 lcd_control;
	bool lcd_enable;
	bool window_enable;
	bool bg_enable;
	bool obj_enable;
	u16 window_map_addr;
	u16 bg_map_addr;
	u16 bg_tile_addr;
	u8 obj_size;

	u8 lcd_mode;
	u32 lcd_clock;
	u32 vblank_clock;

	u8 current_scanline;
	u8 scanline_pixel_counter;

	u8 bgp[4];
	u8 obp[4][2];

	u8 bg_scroll_x;
	u8 bg_scroll_y;
	u8 window_x;
	u8 window_y;
	u8 last_y;

	u8 signed_tile_lut[256];
	u8 unsigned_tile_lut[256];
	u8 flip_8[8];
	u8 flip_16[16];

	bool oam_update;
	bool oam_update_list[40];

	bool on_off;

	bool lock_window_y;

	bool update_bg_colors;
	bool update_obj_colors;
	bool hdma_in_progress;
	bool hdma_line;
	u8 hdma_type;

	u16 obj_colors_raw[4][8];
	u16 bg_colors_raw[4][8];

	u32 obj_colors_final[4][8];
	u32 bg_colors_final[4][8];

	u8 frame_delay;
};

struct tile_pattern
{
	u16 line[8];
};

struct tile_used
{
	tile_pattern tile;
	u16 address;
	bool isOld;
};

struct palette
{
	u16 colour[4];
};

struct palette_used
{
	palette pal;
	u8 idx;
	bool isOld;
};

struct tile_strip
{
	u8 x;
	bool hflip;
	bool vflip;
	u8 priority;
	u16 pattern_id;
	u8 palette_id;
	u8 line;
	u16 pattern_data;
};

struct rendered_screen
{
	std::vector <tile_used> rendered_tile;
	std::vector <palette_used> rendered_palette;
	struct rendered_line
	{
		u8 lcdc;
		std::vector <tile_strip> rendered_bg;
		std::vector <tile_strip> rendered_win;
		std::vector <tile_strip> rendered_obj;
	}scanline[144];
};

#endif // GB_LCD_DATA
