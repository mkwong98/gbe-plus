// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.h
// Date : May 15, 2014
// Description : Game Boy LCD emulation
//
// Draws background, window, and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#include <cmath>

#include "lcd.h"
#include "common/cgfx_common.h"
#include "common/util.h"

/****** LCD Constructor ******/
GB_LCD::GB_LCD()
{
	window = NULL;
	for (u8 i = 0; i < 5; i++) { buffers[i] = NULL; }
	finalscreen = NULL;
	tempscreen = NULL;
	reset();
}

/****** LCD Destructor ******/
GB_LCD::~GB_LCD()
{
	for (u8 i = 0; i < 5; i++) {
		if (buffers[i] != NULL) { SDL_FreeSurface(buffers[i]); }
	}
	if (finalscreen != NULL) { SDL_FreeSurface(finalscreen); }
	if (tempscreen != NULL) { SDL_FreeSurface(tempscreen); }

	SDL_DestroyWindow(window);
	std::cout<<"LCD::Shutdown\n";
}

/****** Reset LCD ******/
void GB_LCD::reset()
{
	mem = NULL;

	window = NULL;

	frame_start_time = 0;
	frame_current_time = 0;
	fps_count = 0;
	fps_time = 0;

	u16 max = (config::max_fps) ? config::max_fps : 60;
	for (u32 x = 0; x < 60; x++)
	{
		double frame_1 = ((1000.0 / max) * x);
		double frame_2 = ((1000.0 / max) * (x + 1));
		frame_delay[x] = (std::round(frame_2) - std::round(frame_1));
	}

	//Initialize various LCD status variables
	lcd_stat.lcd_control = 0;
	lcd_stat.lcd_enable = true;
	lcd_stat.window_enable = false;
	lcd_stat.bg_enable = false;
	lcd_stat.obj_enable = false;
	lcd_stat.window_map_addr = 0x9800;
	lcd_stat.bg_map_addr = 0x9800;
	lcd_stat.bg_tile_addr = 0x8800;
	lcd_stat.obj_size = 1;

	lcd_stat.lcd_mode = 2;
	lcd_stat.lcd_clock = 0;
	lcd_stat.vblank_clock = 0;

	lcd_stat.current_scanline = 0;
	lcd_stat.scanline_pixel_counter = 0;

	lcd_stat.bg_scroll_x = 0;
	lcd_stat.bg_scroll_y = 0;
	lcd_stat.window_x = 0;
	lcd_stat.window_y = 0;
	lcd_stat.last_y = 0;

	lcd_stat.oam_update = true;
	for (int x = 0; x < 40; x++) { lcd_stat.oam_update_list[x] = true; }

	lcd_stat.on_off = false;

	lcd_stat.update_bg_colors = false;
	lcd_stat.update_obj_colors = false;
	lcd_stat.hdma_in_progress = false;
	lcd_stat.hdma_line = false;
	lcd_stat.hdma_type = 0;

	lcd_stat.frame_delay = 0;

	//Clear GBC color palettes
	for (int x = 0; x < 4; x++)
	{
		for (int y = 0; y < 8; y++)
		{
			lcd_stat.obj_colors_raw[x][y] = 0;
			lcd_stat.obj_colors_final[x][y] = 0;
			lcd_stat.bg_colors_raw[x][y] = 0;
			lcd_stat.bg_colors_final[x][y] = 0;
		}
	}

	//Signed-to-unsigned tile lookup generation
	for (int x = 0; x < 256; x++)
	{
		u8 tile_number = x;

		if (tile_number <= 127)
		{
			tile_number += 128;
			lcd_stat.signed_tile_lut[x] = tile_number;
		}

		else
		{
			tile_number -= 128;
			lcd_stat.signed_tile_lut[x] = tile_number;
		}
	}

	//Unsigned-to-signed tile lookup generation
	for (int x = 0; x < 256; x++)
	{
		u8 tile_number = x;

		if (tile_number >= 127)
		{
			tile_number -= 128;
			lcd_stat.unsigned_tile_lut[x] = tile_number;
		}

		else
		{
			tile_number += 128;
			lcd_stat.unsigned_tile_lut[x] = tile_number;
		}
	}

	//8 pixel (horizontal+vertical) flipping lookup generation
	for (int x = 0; x < 8; x++) { lcd_stat.flip_8[x] = (7 - x); }

	//16 pixel (vertical) flipping lookup generation
	for (int x = 0; x < 16; x++) { lcd_stat.flip_16[x] = (15 - x); }

	//CGFX setup
	memset(cgfx_stat.vram_tile_used, 0, 768);
	memset(cgfx_stat.vram_tile_idx, 0xFF, 768 * 2);

	cgfx_stat.screen_data.rendered_tile.clear();
	cgfx_stat.screen_data.rendered_palette.clear();
	for (int x = 0; x < 144; x++)
	{
		cgfx_stat.screen_data.scanline[x].rendered_bg.clear();
		cgfx_stat.screen_data.scanline[x].rendered_win.clear();
		cgfx_stat.screen_data.scanline[x].rendered_obj.clear();
	}

	//Initialize system screen dimensions
	config::sys_width = 160;
	config::sys_height = 144;

	max_fullscreen_ratio = 2;

	reset_buffers();
	srcrect.w = cgfx::scaling_factor;
	srcrect.h = cgfx::scaling_factor;
	hdrect.w = cgfx::scaling_factor * 8;
	hdrect.h = cgfx::scaling_factor;
	hdsrcrect = hdrect;

	rawrect.w = 1;
	rawrect.h = 1;
	clear_buffers();
}

void GB_LCD::reset_buffers()
{
	for (u8 i = 0; i < 5; i++) {
		if (buffers[i] != NULL) { SDL_FreeSurface(buffers[i]); }
		buffers[i] = SDL_CreateRGBSurfaceWithFormat(0, config::sys_width, config::sys_height, 32, SDL_PIXELFORMAT_ARGB8888);
	}
	if (finalscreen != NULL) { SDL_FreeSurface(finalscreen); }
	finalscreen = SDL_CreateRGBSurfaceWithFormat(0, config::sys_width, config::sys_height, 32, SDL_PIXELFORMAT_ARGB8888);
	if (tempscreen != NULL) { SDL_FreeSurface(tempscreen); }
	tempscreen = SDL_CreateRGBSurfaceWithFormat(0, 160, 144, 32, SDL_PIXELFORMAT_ARGB8888);
}


void GB_LCD::clear_buffers()
{
	SDL_FillRect(buffers[0], NULL, 0xFFFFFFFF);
	for (u8 i = 1; i < 5; i++) {
		SDL_FillRect(buffers[i], NULL, 0x00000000);
	}
	SDL_FillRect(tempscreen, NULL, 0x00000000);
}


/****** Read LCD data from save state ******/
bool GB_LCD::lcd_read(u32 offset, std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);
	
	if(!file.is_open()) { return false; }

	//Go to offset
	file.seekg(offset);

	//Serialize LCD data from file stream
	file.read((char*)&lcd_stat, sizeof(lcd_stat));

	//Serialize OBJ data from file stream
	for(int x = 0; x < 40; x++)
	{
		file.read((char*)&obj[x], sizeof(obj[x]));
	}

	//Sanitize LCD data
	if(lcd_stat.current_scanline > 153) { lcd_stat.current_scanline = 0;  }
	if(lcd_stat.last_y > 153) { lcd_stat.last_y = 0; }
	if(lcd_stat.lcd_clock > 70224) { lcd_stat.lcd_clock = 0; }

	lcd_stat.lcd_mode &= 0x3;
	lcd_stat.hdma_type &= 0x1;
	
	file.close();

	//CGFX setup
	memset(cgfx_stat.vram_tile_used, 0, 768);
	memset(cgfx_stat.vram_tile_idx, 0xFF, 768 * 2);

	cgfx_stat.screen_data.rendered_tile.clear();
	cgfx_stat.screen_data.rendered_palette.clear();
	for (int x = 0; x < 144; x++)
	{
		cgfx_stat.screen_data.scanline[x].rendered_bg.clear();
		cgfx_stat.screen_data.scanline[x].rendered_win.clear();
		cgfx_stat.screen_data.scanline[x].rendered_obj.clear();
	}
	clear_buffers();

	return true;
}

/****** Read LCD data from save state ******/
bool GB_LCD::lcd_write(std::string filename)
{
	std::ofstream file(filename.c_str(), std::ios::binary | std::ios::app);
	
	if(!file.is_open()) { return false; }

	//Serialize LCD data to file stream
	file.write((char*)&lcd_stat, sizeof(lcd_stat));

	//Serialize OBJ data to file stream
	for(int x = 0; x < 40; x++)
	{
		file.write((char*)&obj[x], sizeof(obj[x]));
	}

	file.close();
	return true;
}

/****** Compares LY and LYC - Generates STAT interrupt ******/
void GB_LCD::scanline_compare()
{
	if(mem->memory_map[REG_LY] == mem->memory_map[REG_LYC]) 
	{ 
		mem->memory_map[REG_STAT] |= 0x4; 
		if(mem->memory_map[REG_STAT] & 0x40) { mem->memory_map[IF_FLAG] |= 2; }
	}
	else { mem->memory_map[REG_STAT] &= ~0x4; }
}

/****** Updates OAM entries when values in memory change ******/
void GB_LCD::update_oam()
{
	lcd_stat.oam_update = false;

	u16 oam_ptr = 0xFE00;
	u8 attribute = 0;

	for(int x = 0; x < 40; x++)
	{
		//Update if OAM entry has changed
		if(lcd_stat.oam_update_list[x])
		{
			lcd_stat.oam_update_list[x] = false;

			obj[x].height = 8;

			//Read and parse Attribute 0
			attribute = mem->memory_map[oam_ptr++];
			obj[x].y = (attribute - 16);

			//Read and parse Attribute 1
			attribute = mem->memory_map[oam_ptr++];
			obj[x].x = (attribute - 8);

			//Read and parse Attribute 2
			obj[x].tile_number = mem->memory_map[oam_ptr++];
			if(lcd_stat.obj_size == 16) { obj[x].tile_number &= ~0x1; }

			//Read and parse Attribute 3
			attribute = mem->memory_map[oam_ptr++];
			obj[x].color_palette_number = (attribute & 0x7);
			obj[x].vram_bank = (attribute & 0x8) ? 1 : 0;
			obj[x].palette_number = (attribute & 0x10) ? 1 : 0;
			obj[x].h_flip = (attribute & 0x20) ? true : false;
			obj[x].v_flip = (attribute & 0x40) ? true : false;
			obj[x].bg_priority = (attribute & 0x80) ? 1 : 0;
		}

		else { oam_ptr+= 4; }
	}	
}


SDL_Surface* GB_LCD::render_raw_layer(u8 layer)
{
	clear_buffers();
	for (u16 y = 0; y < 144; y++) {
		rawrect.y = y;
		switch (layer)
		{
		case 0:
			render_bg_scanline(true);
			break;
		case 1:
			render_win_scanline(true);
			break;
		case 2:
			render_obj_scanline(true);
			break;
		}
	}
	SDL_Rect r;
	r.x = 0;
	r.y = 0;
	r.w = 160;
	r.h = 144;
	for (u8 i = 0; i < 5; i++)
	{
		SDL_BlitSurface(buffers[i], &r, tempscreen, NULL);
	}
	return tempscreen;
}

void GB_LCD::render_HD_strip(SDL_Surface* src, SDL_Rect* srcr, SDL_Surface* dst, SDL_Rect* dstr, double brightness, bool vflip)
{
	if (brightness != 1) {
		SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_NONE);
		//make a copy of the alpha values with all pixels white
		SDL_BlitSurface(src, srcr, cgfx_stat.alphaCpy, NULL);
		SDL_FillRect(cgfx_stat.brightnessMod, NULL, 0xFFFFFF);
		SDL_SetSurfaceBlendMode(cgfx_stat.brightnessMod, SDL_BLENDMODE_ADD);
		SDL_BlitSurface(cgfx_stat.brightnessMod, NULL, cgfx_stat.alphaCpy, NULL);

		//create pixel values
		u8 c;
		u32 brightnessC;
		if (brightness < 1)
		{
			c = 255 * (1.0 - brightness);
			brightnessC = 0x00000000 | c << 24;
		}
		else
		{
			c = 255 * (brightness - 1.0);
			brightnessC = 0x00FFFFFF | c << 24;
		}

		SDL_FillRect(cgfx_stat.brightnessMod, NULL, brightnessC);
		SDL_SetSurfaceBlendMode(cgfx_stat.brightnessMod, SDL_BLENDMODE_BLEND);
		SDL_BlitSurface(src, NULL, cgfx_stat.tempStrip, NULL);
		SDL_BlitSurface(cgfx_stat.brightnessMod, NULL, cgfx_stat.tempStrip, NULL);

		//merge pixel values to alpha
		SDL_SetSurfaceBlendMode(cgfx_stat.tempStrip, SDL_BLENDMODE_MOD);
		SDL_BlitSurface(cgfx_stat.tempStrip, NULL, cgfx_stat.alphaCpy, NULL);

		//clean up
		SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_BLEND);

		if (vflip)
		{
			for (u8 i = 0; i < cgfx::scaling_factor; i++)
			{
				SDL_Rect r1;
				r1.w = cgfx_stat.alphaCpy->w;
				r1.h = 1;
				r1.x = 0;
				r1.y = 7 - i;
				SDL_Rect r2;
				r2.w = cgfx_stat.alphaCpy->w;
				r2.h = 1;
				r2.x = dstr->x;
				r2.y = dstr->y + i;
				SDL_BlitSurface(cgfx_stat.alphaCpy, &r1, dst, &r2);
			}
		}
		else
		{
			SDL_BlitSurface(cgfx_stat.alphaCpy, NULL, dst, dstr);
		}
	}
	else if (vflip)
	{
		for (u8 i = 0; i < cgfx::scaling_factor; i++)
		{
			SDL_Rect r1;
			r1.w = cgfx_stat.tempStrip->w;
			r1.h = 1;
			r1.x = srcr->x;
			r1.y = srcr->y + 7 - i;
			SDL_Rect r2;
			r2.w = cgfx_stat.tempStrip->w;
			r2.h = 1;
			r2.x = dstr->x;
			r2.y = dstr->y + i;
			SDL_BlitSurface(src, &r1, dst, &r2);
		}
	}
	else
	{
		SDL_BlitSurface(src, srcr, dst, dstr);
	}
}

void GB_LCD::clear_HD_strip(SDL_Surface* src, SDL_Rect* srcr, SDL_Surface* dst, SDL_Rect* dstr, bool vflip)
{
	//clear bg pixels
	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	for (u16 cy = 0; cy < cgfx::scaling_factor; cy++)
	{
		u16 cy2 = vflip ? cgfx::scaling_factor - 1 - cy : cy;
		if (dstr->y + cy < dst->h)
		{
			u8 pixelsToClear;
			u32 offset;
			u32 offset2;

			if (dstr->x < 0)
			{
				pixelsToClear = dstr->x + (8 * cgfx::scaling_factor);
				offset = ((dstr->y + cy) * dst->w) * 4;
				offset2 = ((srcr->y + cy2) * src->w + (srcr->x - dstr->x)) * 4;
			}
			else
			{
				if (dstr->x + (8 * cgfx::scaling_factor) < dst->w)
					pixelsToClear = 8 * cgfx::scaling_factor;
				else
					pixelsToClear = dst->w - dstr->x;
				offset = ((dstr->y + cy) * dst->w + dstr->x) * 4;
				offset2 = ((srcr->y + cy2) * src->w + srcr->x) * 4;
			}

			u8* dpixel = (u8*)(dst->pixels) + offset + 3;
			u8* spixel = (u8*)(src->pixels) + offset2 + 3;
			for (u8 ci = 0; ci < pixelsToClear; ci++)
			{
				if (*spixel >= *dpixel) *dpixel = 0;
				else *dpixel -= *spixel;
				dpixel += 4;
				spixel += 4;
			}
		}
	}
	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);
}

bool GB_LCD::check_screen_tile_at_loc(SDL_Rect* loc, pack_condition* c)
{
	bool matchResult = check_line_tile_at_loc(&(cgfx_stat.screen_data.scanline[loc->y].rendered_bg), loc, c);
	if (!matchResult)
		matchResult = check_line_tile_at_loc(&(cgfx_stat.screen_data.scanline[loc->y].rendered_win), loc, c);
	return matchResult;
}

bool GB_LCD::check_sprite_at_loc(SDL_Rect* loc, pack_condition* c)
{
	return check_line_tile_at_loc(&(cgfx_stat.screen_data.scanline[loc->y].rendered_obj), loc, c);
}

bool GB_LCD::check_line_tile_at_loc(std::vector <tile_strip>* scanline, SDL_Rect* loc, pack_condition* c)
{
	for (u8 i = 0; i < scanline->size(); i++) {
		tile_strip* testStr = &((*scanline)[i]);
		//check location
		if (testStr->x == loc->x && testStr->line == loc->h)
		{
			//check pattern
			if (memcmp(cgfx_stat.screen_data.rendered_tile[testStr->pattern_id].tile.line.data(), c->tileData.line.data(), 16) == 0)
			{
				//check palette
				if (check_tile_condition_palette_match(testStr, c))
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool DMG_LCD::check_tile_condition_palette_match(tile_strip* s, pack_condition* c)
{
	return cgfx_stat.screen_data.rendered_palette[s->palette_id].code == c->palette[0];
}

bool GBC_LCD::check_tile_condition_palette_match(tile_strip* s, pack_condition* c)
{
	return (cgfx_stat.screen_data.rendered_palette[s->palette_id].colour[0] == c->palette[0]
		&& cgfx_stat.screen_data.rendered_palette[s->palette_id].colour[1] == c->palette[1]
		&& cgfx_stat.screen_data.rendered_palette[s->palette_id].colour[2] == c->palette[2]
		&& cgfx_stat.screen_data.rendered_palette[s->palette_id].colour[3] == c->palette[3]);	
}


void GB_LCD::resolve_global_condition()
{
	SDL_Rect loc;
	u8 val1;
	u8 val2;

	for (u16 j = 0; j < cgfx_stat.conds.size(); j++)
	{
		pack_condition* c = &(cgfx_stat.conds[j]);
		c->latest_result = false;
		switch (c->type) {
		case pack_condition::TILEATPOSITION:
			loc.x = c->x;
			loc.y = c->y;
			loc.h = 0;
			c->latest_result = check_screen_tile_at_loc(&loc, c);
			break;

		case pack_condition::SPRITEATPOSITION:
			loc.x = c->x;
			loc.y = c->y;
			loc.h = 0;
			c->latest_result = check_sprite_at_loc(&loc, c);
			break;

		case pack_condition::MEMORYCHECK:
			val1 = mem->read_u8(c->address1) & c->mask;
			val2 = mem->read_u8(c->address2) & c->mask;
			switch (c->opType)
			{
			case pack_condition::EQ:
				c->latest_result = val1 == val2;
				break;
			case pack_condition::NE:
				c->latest_result = val1 != val2;
				break;
			case pack_condition::GT:
				c->latest_result = val1 > val2;
				break;
			case pack_condition::LS:
				c->latest_result = val1 < val2;
				break;
			case pack_condition::GE:
				c->latest_result = val1 >= val2;
				break;
			case pack_condition::LE:
				c->latest_result = val1 <= val2;
				break;
			default:
				break;
			}
			break;

		case pack_condition::MEMORYCHECKCONSTANT:
			val1 = mem->read_u8(c->address1) & c->mask;
			val2 = c->value & c->mask;
			switch (c->opType)
			{
			case pack_condition::EQ:
				c->latest_result = val1 == val2;
				break;
			case pack_condition::NE:
				c->latest_result = val1 != val2;
				break;
			case pack_condition::GT:
				c->latest_result = val1 > val2;
				break;
			case pack_condition::LS:
				c->latest_result = val1 < val2;
				break;
			case pack_condition::GE:
				c->latest_result = val1 >= val2;
				break;
			case pack_condition::LE:
				c->latest_result = val1 <= val2;
				break;
			default:
				break;
			}
			break;

		case pack_condition::FRAMERANGE:
			c->latest_result = cgfx_stat.frameCnt % c->divisor >= c->compareVal;
			break;
		default:
			break;
		}
	}
}


pack_tile* DMG_LCD::get_tile_match(tile_strip* s, u16 cscanline)
{
	pack_tile* hdTile;
	SDL_Rect loc;
	bool useDefault = false;
	
	do
	{
		for (u16 j = 0; j < cgfx_stat.screen_data.rendered_tile[s->pattern_id].pack_tile_match.size(); j++)
		{
			hdTile = &(cgfx_stat.tiles[cgfx_stat.screen_data.rendered_tile[s->pattern_id].pack_tile_match[j]]);
			if (hdTile->palette[0] == cgfx_stat.screen_data.rendered_palette[s->palette_id].code || (hdTile->default && useDefault))
			{
				//check for conditions
				bool allCondPassed = true;
				for (u16 i = 0; i < hdTile->condApps.size(); i++)
				{
					pack_condition* c = &(cgfx_stat.conds[hdTile->condApps[i].condIdx]);
					bool matchResult;
					switch (c->type) {
					case pack_condition::HMIRROR:
						matchResult = s->hflip;
						break;
					case pack_condition::VMIRROR:
						matchResult = s->vflip;
						break;
					case pack_condition::BGPRIORITY:
						matchResult = s->bg_priority;
						break;
					case pack_condition::PALETTE0:
						matchResult = (s->palette_sel == 0);
						break;
					case pack_condition::PALETTE1:
						matchResult = (s->palette_sel == 1);
						break;

					case pack_condition::TILENEARBY:
						loc.x = s->x + (s->hflip ? -c->x : c->x);
						loc.y = cscanline + (s->vflip ? -c->y : c->y);
						loc.h = s->line;
						matchResult = check_screen_tile_at_loc(&loc, c);
						break;

					case pack_condition::SPRITENEARBY:
						loc.x = s->x + (s->hflip ? -c->x : c->x);
						loc.y = cscanline + (s->vflip ? -c->y : c->y);
						loc.h = s->line;
						matchResult = check_sprite_at_loc(&loc, c);
						break;

					default:
						matchResult = c->latest_result;
						break;
					}
					if (hdTile->condApps[i].negate) matchResult = !matchResult;
					allCondPassed = allCondPassed && matchResult;
				}
				if (allCondPassed) return hdTile;
			}
		}
		useDefault = !useDefault;
	}while(useDefault);
	return NULL;
}

pack_tile* GBC_LCD::get_tile_match(tile_strip* s, u16 cscanline)
{
	pack_tile* hdTile;
	SDL_Rect loc;
	bool useDefault = false;

	do
	{
		for (u8 j = 0; j < cgfx_stat.screen_data.rendered_tile[s->pattern_id].pack_tile_match.size(); j++)
		{
			hdTile = &(cgfx_stat.tiles[cgfx_stat.screen_data.rendered_tile[s->pattern_id].pack_tile_match[j]]);
			if ((hdTile->palette[0] == cgfx_stat.screen_data.rendered_palette[s->palette_id].colour[0]
				&& hdTile->palette[1] == cgfx_stat.screen_data.rendered_palette[s->palette_id].colour[1]
				&& hdTile->palette[2] == cgfx_stat.screen_data.rendered_palette[s->palette_id].colour[2]
				&& hdTile->palette[3] == cgfx_stat.screen_data.rendered_palette[s->palette_id].colour[3]
				)
				|| (hdTile->default && useDefault))
			{
				//check for conditions
				bool allCondPassed = true;
				for (u16 i = 0; i < hdTile->condApps.size(); i++)
				{
					pack_condition* c = &(cgfx_stat.conds[hdTile->condApps[i].condIdx]);
					bool matchResult;
					switch (c->type) {
					case pack_condition::HMIRROR:
						matchResult = s->hflip;
						break;
					case pack_condition::VMIRROR:
						matchResult = s->vflip;
						break;
					case pack_condition::BGPRIORITY:
						matchResult = s->bg_priority;
						break;
					case pack_condition::PALETTE0:
						matchResult = (s->palette_sel == 0);
						break;
					case pack_condition::PALETTE1:
						matchResult = (s->palette_sel == 1);
						break;
					case pack_condition::PALETTE2:
						matchResult = (s->palette_sel == 2);
						break;
					case pack_condition::PALETTE3:
						matchResult = (s->palette_sel == 3);
						break;
					case pack_condition::PALETTE4:
						matchResult = (s->palette_sel == 4);
						break;
					case pack_condition::PALETTE5:
						matchResult = (s->palette_sel == 5);
						break;
					case pack_condition::PALETTE6:
						matchResult = (s->palette_sel == 6);
						break;
					case pack_condition::PALETTE7:
						matchResult = (s->palette_sel == 7);
						break;

					case pack_condition::TILENEARBY:
						loc.x = s->x + (s->hflip ? -c->x : c->x);
						loc.y = cscanline + (s->vflip ? -c->y : c->y);
						loc.h = s->line;
						matchResult = check_screen_tile_at_loc(&loc, c);
						break;

					case pack_condition::SPRITENEARBY:
						loc.x = s->x + (s->hflip ? -c->x : c->x);
						loc.y = cscanline + (s->vflip ? -c->y : c->y);
						loc.h = s->line;
						matchResult = check_sprite_at_loc(&loc, c);
						break;

					default:
						matchResult = c->latest_result;
						break;
					}
					if (hdTile->condApps[i].negate) matchResult = !matchResult;
					allCondPassed = allCondPassed && matchResult;
				}
				if (allCondPassed) return hdTile;
			}
		}
		useDefault = !useDefault;
	} while (useDefault);

	return NULL;
}



/****** Renders pixels for the BG (per-scanline) - DMG version ******/
void DMG_LCD::render_bg_scanline(bool raw)
{
	u16 scanline = raw ? rawrect.y : lcd_stat.renderY;
	SDL_Rect* r = raw ? &rawrect : &srcrect;

	pack_tile* hdTile;
	for (u8 i = 0; i < cgfx_stat.screen_data.scanline[scanline].rendered_bg.size(); i++) {
		u8 tile_pixel = 0;
		tile_strip p = cgfx_stat.screen_data.scanline[scanline].rendered_bg[i];
		s16 pixel_counter = p.x;
		if (pixel_counter >= 249) pixel_counter -= 256;
		srcrect.x = cgfx::scaling_factor * pixel_counter;
		rawrect.x = pixel_counter;

		//look for hd 
		if (!raw) hdTile = get_tile_match(&p, lcd_stat.renderY);
		else hdTile = NULL;

		if (hdTile)
		{
			hdrect.x = srcrect.x;
			hdsrcrect.x = hdTile->x;
			hdsrcrect.y = hdTile->y + p.graphicsLine * cgfx::scaling_factor;
			render_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][0], &hdsrcrect, buffers[2], &hdrect, hdTile->brightness, false);
			if (cgfx_stat.imgs[hdTile->imgIdx].size() > 1)
			{
				render_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][1], &hdsrcrect, buffers[0], &hdrect, hdTile->brightness, false);
			}
			pixel_counter += 8;
			srcrect.x += 8 * cgfx::scaling_factor;
			rawrect.x += 8;
		}
		else
		{
			for (int y = 7; y >= 0; y--)
			{
				//Calculate raw value of the tile's pixel
				tile_pixel = ((p.pattern_data >> 8) & (1 << y)) ? 2 : 0;
				tile_pixel |= (p.pattern_data & (1 << y)) ? 1 : 0;

				if (pixel_counter >= 0 && pixel_counter < 160)
				{
					if (tile_pixel != 0) {
						SDL_FillRect(buffers[2], r, config::DMG_BG_PAL[cgfx_stat.screen_data.rendered_palette[p.palette_id].colour[tile_pixel]]);
					}
					else {
						SDL_FillRect(buffers[0], r, config::DMG_BG_PAL[cgfx_stat.screen_data.rendered_palette[p.palette_id].colour[tile_pixel]]);
					}
				}
				pixel_counter++;
				srcrect.x += cgfx::scaling_factor;
				rawrect.x++;
			}
		}
	}
}

/****** Renders pixels for the BG (per-scanline) - GBC version ******/
void GBC_LCD::render_bg_scanline(bool raw)
{
	u16 scanline = raw ? rawrect.y : lcd_stat.renderY;
	SDL_Rect* r = raw ? &rawrect : &srcrect;
	bool lowP = (cgfx_stat.screen_data.scanline[scanline].lcdc & 0x01) == 0;
	u8 layer;
	pack_tile* hdTile;
	for (u8 i = 0; i < cgfx_stat.screen_data.scanline[scanline].rendered_bg.size(); i++) {
		u8 tile_pixel = 0;
		tile_strip p = cgfx_stat.screen_data.scanline[scanline].rendered_bg[i];
		s16 pixel_counter;

		//look for hd 
		if (!raw) hdTile = get_tile_match(&p, lcd_stat.renderY);
		else hdTile = NULL;

		if (hdTile)
		{
			if(p.x >= 249)
				hdrect.x = (p.x - 256) * cgfx::scaling_factor;
			else
				hdrect.x = p.x * cgfx::scaling_factor;
			hdsrcrect.y = hdTile->y + p.graphicsLine * cgfx::scaling_factor;
			if (p.hflip)
			{
				hdsrcrect.x = cgfx_stat.himgs[hdTile->imgIdx][0]->w - hdTile->x - hdsrcrect.w;
				if (cgfx_stat.himgs[hdTile->imgIdx].size() > 1)
				{
					render_HD_strip(cgfx_stat.himgs[hdTile->imgIdx][1], &hdsrcrect, buffers[0], &hdrect, hdTile->brightness, p.vflip);
				}
				render_HD_strip(cgfx_stat.himgs[hdTile->imgIdx][0], &hdsrcrect, lowP ? buffers[0] : (p.priority ? buffers[4] : buffers[2]), &hdrect, hdTile->brightness, p.vflip);
			}
			else
			{
				hdsrcrect.x = hdTile->x;
				if (cgfx_stat.imgs[hdTile->imgIdx].size() > 1)
				{
					render_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][1], &hdsrcrect, buffers[0], &hdrect, hdTile->brightness, p.vflip);
				}
				render_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][0], &hdsrcrect, lowP ? buffers[0] : (p.priority ? buffers[4] : buffers[2]), &hdrect, hdTile->brightness, p.vflip);
			}
		}
		else
		{
			for (int y = 7; y >= 0; y--)
			{
				//Calculate raw value of the tile's pixel
				tile_pixel = ((p.pattern_data >> 8) & (1 << y)) ? 2 : 0;
				tile_pixel |= (p.pattern_data & (1 << y)) ? 1 : 0;
				pixel_counter = (p.hflip ? p.x + y : p.x + 7 - y);
				if (pixel_counter >= 249) pixel_counter -= 256;
				if (pixel_counter >=0 && pixel_counter < 160)
				{
					srcrect.x = pixel_counter * cgfx::scaling_factor;
					rawrect.x = pixel_counter;

					if (lowP || tile_pixel == 0) layer = 0;
					else if (p.priority) layer = 4;
					else layer = 2;
					SDL_FillRect(buffers[layer], r, cgfx_stat.screen_data.rendered_palette[p.palette_id].renderColour[tile_pixel]);
				}
			}
		}
	}
}

//****** Renders pixels for the Window (per-scanline) - DMG version ******/
void DMG_LCD::render_win_scanline(bool raw)
{
	u16 scanline = raw ? rawrect.y : lcd_stat.renderY;
	SDL_Rect* r = raw ? &rawrect : &srcrect;

	pack_tile* hdTile;
	for (u8 i = 0; i < cgfx_stat.screen_data.scanline[scanline].rendered_win.size(); i++) {
		u8 tile_pixel = 0;
		tile_strip p = cgfx_stat.screen_data.scanline[scanline].rendered_win[i];
		u8 pixel_counter = p.x;
		srcrect.x = cgfx::scaling_factor * pixel_counter;
		rawrect.x = pixel_counter;

		//look for hd 
		if (!raw) hdTile = get_tile_match(&p, lcd_stat.renderY);
		else hdTile = NULL;

		if (hdTile)
		{
			hdrect.x = srcrect.x;
			hdsrcrect.x = hdTile->x;
			hdsrcrect.y = hdTile->y + p.graphicsLine * cgfx::scaling_factor;

			if (cgfx_stat.imgs[hdTile->imgIdx].size() > 1)
			{
				clear_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][1], &hdsrcrect, buffers[2], &hdrect, false);
				render_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][1], &hdsrcrect, buffers[0], &hdrect, hdTile->brightness, false);
			}
			render_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][0], &hdsrcrect, buffers[2], &hdrect, hdTile->brightness, false);
			
			if (pixel_counter >= 152) return;

			pixel_counter += 8;
			srcrect.x += 8 * cgfx::scaling_factor;
			rawrect.x += 8;
		}
		else
		{
			for (int y = 7; y >= 0; y--)
			{
				//Calculate raw value of the tile's pixel
				tile_pixel = ((p.pattern_data >> 8) & (1 << y)) ? 2 : 0;
				tile_pixel |= (p.pattern_data & (1 << y)) ? 1 : 0;

				if (pixel_counter < 160)
				{
					//clear bg pixels
					SDL_FillRect(buffers[2], r, 0x00000000);
					if (tile_pixel != 0) {
						SDL_FillRect(buffers[2], r, config::DMG_BG_PAL[cgfx_stat.screen_data.rendered_palette[p.palette_id].colour[tile_pixel]]);
					}
					else {
						SDL_FillRect(buffers[0], r, config::DMG_BG_PAL[cgfx_stat.screen_data.rendered_palette[p.palette_id].colour[tile_pixel]]);
					}
				}
				pixel_counter++;
				srcrect.x += cgfx::scaling_factor;
				rawrect.x++;
				if (pixel_counter == 160) return;
			}
		}
	}
}

/****** Renders pixels for the Window (per-scanline) - GBC version ******/
void GBC_LCD::render_win_scanline(bool raw)
{
	u16 scanline = raw ? rawrect.y : lcd_stat.renderY;
	SDL_Rect* r = raw ? &rawrect : &srcrect;
	bool lowP = (cgfx_stat.screen_data.scanline[scanline].lcdc & 0x01) == 0;
	u8 layer;

	pack_tile* hdTile;
	for (u8 i = 0; i < cgfx_stat.screen_data.scanline[scanline].rendered_win.size(); i++) {
		u8 tile_pixel = 0;
		tile_strip p = cgfx_stat.screen_data.scanline[scanline].rendered_win[i];
		u8 pixel_counter;

		//look for hd 
		if (!raw) hdTile = get_tile_match(&p, lcd_stat.renderY);
		else hdTile = NULL;

		if (hdTile)
		{
			hdrect.x = p.x * cgfx::scaling_factor;
			hdsrcrect.y = hdTile->y + p.graphicsLine * cgfx::scaling_factor;

			//clear bg pixels
			SDL_FillRect(buffers[2], r, 0x00000000);
			SDL_FillRect(buffers[4], r, 0x00000000);

			if (lowP || tile_pixel == 0) layer = 0;
			else if (p.priority) layer = 4;
			else layer = 2;

			if (p.hflip)
			{
				hdsrcrect.x = cgfx_stat.himgs[hdTile->imgIdx][0]->w - hdTile->x - hdsrcrect.w;
				if (cgfx_stat.himgs[hdTile->imgIdx].size() > 1)
				{
					clear_HD_strip(cgfx_stat.himgs[hdTile->imgIdx][1], &hdsrcrect, buffers[4], &hdrect, p.vflip);
					clear_HD_strip(cgfx_stat.himgs[hdTile->imgIdx][1], &hdsrcrect, buffers[2], &hdrect, p.vflip);
					render_HD_strip(cgfx_stat.himgs[hdTile->imgIdx][1], &hdsrcrect, buffers[0], &hdrect, hdTile->brightness, p.vflip);
				}
				if (layer < 4)
					clear_HD_strip(cgfx_stat.himgs[hdTile->imgIdx][0], &hdsrcrect, buffers[4], &hdrect, p.vflip);
				if (layer < 2)
					clear_HD_strip(cgfx_stat.himgs[hdTile->imgIdx][0], &hdsrcrect, buffers[2], &hdrect, p.vflip);
				render_HD_strip(cgfx_stat.himgs[hdTile->imgIdx][0], &hdsrcrect, buffers[layer], &hdrect, hdTile->brightness, p.vflip);
			}
			else
			{
				hdsrcrect.x = hdTile->x;
				if (cgfx_stat.imgs[hdTile->imgIdx].size() > 1)
				{
					clear_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][1], &hdsrcrect, buffers[4], &hdrect, p.vflip);
					clear_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][1], &hdsrcrect, buffers[2], &hdrect, p.vflip);
					render_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][1], &hdsrcrect, buffers[0], &hdrect, hdTile->brightness, p.vflip);
				}
				if (layer < 4)
					clear_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][0], &hdsrcrect, buffers[4], &hdrect, p.vflip);
				if (layer < 2)
					clear_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][0], &hdsrcrect, buffers[2], &hdrect, p.vflip);
				render_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][0], &hdsrcrect, buffers[layer], &hdrect, hdTile->brightness, p.vflip);
			}

			if(p.x >= 152) return;
		}
		else
		{
			for (int y = 7; y >= 0; y--)
			{
				//Calculate raw value of the tile's pixel
				tile_pixel = ((p.pattern_data >> 8) & (1 << y)) ? 2 : 0;
				tile_pixel |= (p.pattern_data & (1 << y)) ? 1 : 0;
				pixel_counter = (p.hflip ? p.x + y : p.x + 7 - y);
				if (pixel_counter < 160)
				{
					srcrect.x = pixel_counter * cgfx::scaling_factor;
					rawrect.x = pixel_counter;

					//clear bg pixels
					if (lowP || tile_pixel == 0)
					{
						layer = 0;
						SDL_FillRect(buffers[2], r, 0x00000000);
						SDL_FillRect(buffers[4], r, 0x00000000);
					}
					else if (p.priority) 
						layer = 4;
					else
					{
						layer = 2;
						SDL_FillRect(buffers[4], r, 0x00000000);
					}
					SDL_FillRect(buffers[layer], r, cgfx_stat.screen_data.rendered_palette[p.palette_id].renderColour[tile_pixel]);
				}
			}
		}
		if (p.x + 8 >= 160) return;
	}
}

/****** Renders pixels for OBJs (per-scanline) - DMG version ******/
void DMG_LCD::render_obj_scanline(bool raw)
{
	u16 scanline = raw ? rawrect.y : lcd_stat.renderY;
	SDL_Rect* r = raw ? &rawrect : &srcrect;

	pack_tile* hdTile;
	for (u8 i = 0; i < cgfx_stat.screen_data.scanline[scanline].rendered_obj.size(); i++) {
		u8 tile_pixel = 0;
		tile_strip p = cgfx_stat.screen_data.scanline[scanline].rendered_obj[i];
		u8 pixel_counter = p.x;

		//look for hd 
		if (!raw) hdTile = get_tile_match(&p, lcd_stat.renderY);
		else hdTile = NULL;

		if (hdTile)
		{
			hdrect.x = p.x * cgfx::scaling_factor;
			hdsrcrect.y = hdTile->y + (p.graphicsLine >= 8 ? p.graphicsLine - 8 : p.graphicsLine) * cgfx::scaling_factor;
			if (p.hflip)
			{
				hdsrcrect.x = cgfx_stat.himgs[hdTile->imgIdx][0]->w - hdTile->x - hdsrcrect.w;
				if (p.bg_priority == 1)
					clear_HD_strip(cgfx_stat.himgs[hdTile->imgIdx][0], &hdsrcrect, buffers[3], &hdrect, p.vflip);
				render_HD_strip(cgfx_stat.himgs[hdTile->imgIdx][0], &hdsrcrect, p.bg_priority == 0 ? buffers[3] : buffers[1], &hdrect, hdTile->brightness, p.vflip);
			}
			else
			{
				hdsrcrect.x = hdTile->x;
				if (p.bg_priority == 1)
					clear_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][0], &hdsrcrect, buffers[3], &hdrect, p.vflip);
				render_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][0], &hdsrcrect, p.bg_priority == 0 ? buffers[3] : buffers[1], &hdrect, hdTile->brightness, p.vflip);
			}
		}
		else
		{
			for (int y = 7; y >= 0; y--)
			{
				//Calculate raw value of the tile's pixel
				tile_pixel = ((p.pattern_data >> 8) & (1 << y)) ? 2 : 0;
				tile_pixel |= (p.pattern_data & (1 << y)) ? 1 : 0;
				pixel_counter = (p.hflip ? p.x + y : p.x + 7 - y);
				if (pixel_counter >= 0 && pixel_counter < 160)
				{
					if (tile_pixel != 0) {
						srcrect.x = pixel_counter * cgfx::scaling_factor;
						rawrect.x = pixel_counter;

						if (p.bg_priority == 0)
						{
							SDL_FillRect(buffers[3], r, config::DMG_OBJ_PAL[cgfx_stat.screen_data.rendered_palette[p.palette_id].colour[tile_pixel]][p.palette_sel]);
						}
						else
						{
							SDL_FillRect(buffers[1], r, config::DMG_OBJ_PAL[cgfx_stat.screen_data.rendered_palette[p.palette_id].colour[tile_pixel]][p.palette_sel]);
							SDL_FillRect(buffers[3], r, 0x00000000);
						}
					}
				}
				pixel_counter++;
			}
		}
	}
}

/****** Renders pixels for OBJs (per-scanline) - GBC version ******/
void GBC_LCD::render_obj_scanline(bool raw)
{
	u16 scanline = raw ? rawrect.y : lcd_stat.renderY;
	SDL_Rect* r = raw ? &rawrect : &srcrect;

	pack_tile* hdTile;
	rendered_screen::rendered_line* l = &(cgfx_stat.screen_data.scanline[scanline]);
	for (u8 i = 0; i < l->rendered_obj.size(); i++) {
		u8 tile_pixel = 0;
		tile_strip p = l->rendered_obj[i];
		u8 pixel_counter = p.x;

		//look for hd 
		if (!raw) hdTile = get_tile_match(&p, lcd_stat.renderY);
		else hdTile = NULL;

		if (hdTile)
		{
			hdrect.x = p.x * cgfx::scaling_factor;
			hdsrcrect.y = hdTile->y + (p.graphicsLine >= 8 ? p.graphicsLine - 8 : p.graphicsLine) * cgfx::scaling_factor;
			if (p.hflip)
			{
				hdsrcrect.x = cgfx_stat.himgs[hdTile->imgIdx][0]->w - hdTile->x - hdsrcrect.w;
				if (p.bg_priority == 1)
					clear_HD_strip(cgfx_stat.himgs[hdTile->imgIdx][0], &hdsrcrect, buffers[3], &hdrect, p.vflip);
				render_HD_strip(cgfx_stat.himgs[hdTile->imgIdx][0], &hdsrcrect, p.bg_priority == 0 ? buffers[3] : buffers[1], &hdrect, hdTile->brightness, p.vflip);
			}
			else
			{
				hdsrcrect.x = hdTile->x;
				if (p.bg_priority == 1)
					clear_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][0], &hdsrcrect, buffers[3], &hdrect, p.vflip);
				render_HD_strip(cgfx_stat.imgs[hdTile->imgIdx][0], &hdsrcrect, p.bg_priority == 0 ? buffers[3] : buffers[1], &hdrect, hdTile->brightness, p.vflip);
			}
		}
		else
		{
			for (int y = 7; y >= 0; y--)
			{
				//Calculate raw value of the tile's pixel
				tile_pixel = ((p.pattern_data >> 8) & (1 << y)) ? 2 : 0;
				tile_pixel |= (p.pattern_data & (1 << y)) ? 1 : 0;
				pixel_counter = (p.hflip ? p.x + y : p.x + 7 - y);
				if (pixel_counter < 160)
				{
					if (tile_pixel != 0) {
						srcrect.x = pixel_counter * cgfx::scaling_factor;
						rawrect.x = pixel_counter;

						if (p.bg_priority == 0)
						{
							SDL_FillRect(buffers[3], r, cgfx_stat.screen_data.rendered_palette[p.palette_id].renderColour[tile_pixel]);
						}
						else
						{
							SDL_FillRect(buffers[3], r, 0x00000000);
							SDL_FillRect(buffers[1], r, cgfx_stat.screen_data.rendered_palette[p.palette_id].renderColour[tile_pixel]);
						}
					}
				}
				pixel_counter++;
			}
		}
	}
}

/****** Update background color palettes on the GBC ******/
void GBC_LCD::update_bg_colors()
{
	u8 hi_lo = (mem->memory_map[REG_BCPS] & 0x1);
	u8 color = (mem->memory_map[REG_BCPS] >> 1) & 0x3;
	u8 palette = (mem->memory_map[REG_BCPS] >> 3) & 0x7;

	//Update lower-nibble of color
	if(hi_lo == 0) 
	{ 
		lcd_stat.bg_colors_raw[color][palette] &= 0xFF00;
		lcd_stat.bg_colors_raw[color][palette] |= mem->memory_map[REG_BCPD];
	}

	//Update upper-nibble of color
	else
	{
		lcd_stat.bg_colors_raw[color][palette] &= 0xFF;
		lcd_stat.bg_colors_raw[color][palette] |= (mem->memory_map[REG_BCPD] << 8);
	}

	//Auto update palette index
	if(mem->memory_map[REG_BCPS] & 0x80)
	{
		u8 new_index = mem->memory_map[REG_BCPS] & 0x3F;
		new_index = (new_index + 1) & 0x3F;
		mem->memory_map[REG_BCPS] = (0x80 | new_index);
	}

	//Convert RGB5 to 32-bit ARGB
	u16 color_bytes = lcd_stat.bg_colors_raw[color][palette];

	u8 red = color_bytes & 0x1F;
	red = ((red << 3) & 0xF8) | (red >> 2);

	u8 green = (color_bytes >> 5) & 0x1F;
	green = ((green << 3) & 0xF8) | (green >> 2);

	u8 blue = (color_bytes >> 10) & 0x1F;
	blue = ((blue << 3) & 0xF8) | (blue >> 2);

	u32 old_color = lcd_stat.bg_colors_final[color][palette];

	lcd_stat.bg_colors_final[color][palette] = 0xFF000000 | (red << 16) | (green << 8) | (blue);

	lcd_stat.update_bg_colors = false;
	scanlinePaletteUpdated = true;
}

/****** Update sprite color palettes on the GBC ******/
void GBC_LCD::update_obj_colors()
{
	u8 hi_lo = (mem->memory_map[REG_OCPS] & 0x1);
	u8 color = (mem->memory_map[REG_OCPS] >> 1) & 0x3;
	u8 palette = (mem->memory_map[REG_OCPS] >> 3) & 0x7;

	//Update lower-nibble of color
	if(hi_lo == 0) 
	{ 
		lcd_stat.obj_colors_raw[color][palette] &= 0xFF00;
		lcd_stat.obj_colors_raw[color][palette] |= mem->memory_map[REG_OCPD];
	}

	//Update upper-nibble of color
	else
	{
		lcd_stat.obj_colors_raw[color][palette] &= 0xFF;
		lcd_stat.obj_colors_raw[color][palette] |= (mem->memory_map[REG_OCPD] << 8);
	}

	//Auto update palette index
	if(mem->memory_map[REG_OCPS] & 0x80)
	{
		u8 new_index = mem->memory_map[REG_OCPS] & 0x3F;
		new_index = (new_index + 1) & 0x3F;
		mem->memory_map[REG_OCPS] = (0x80 | new_index);
	}

	//Convert RGB5 to 32-bit ARGB
	u16 color_bytes = lcd_stat.obj_colors_raw[color][palette];

	u8 red = color_bytes & 0x1F;
	red = ((red << 3) & 0xF8) | (red >> 2);

	u8 green = (color_bytes >> 5) & 0x1F;
	green = ((green << 3) & 0xF8) | (green >> 2);

	u8 blue = (color_bytes >> 10) & 0x1F;
	blue = ((blue << 3) & 0xF8) | (blue >> 2);

	u32 old_color = lcd_stat.obj_colors_final[color][palette];

	lcd_stat.obj_colors_final[color][palette] = 0xFF000000 | (red << 16) | (green << 8) | (blue);

	lcd_stat.update_obj_colors = false;
	scanlinePaletteUpdated = true;
}

/****** Execute LCD operations ******/
void GB_LCD::step(int cpu_clock)
{
	cpu_clock >>= config::oc_flags;
	cpu_clock = (cpu_clock == 0) ? 1 : cpu_clock;

	//Enable the LCD
	if ((lcd_stat.on_off) && (lcd_stat.lcd_enable))
	{
		lcd_stat.on_off = false;
		lcd_stat.lcd_mode = 2;

		scanline_compare();
	}

	//Disable the LCD (VBlank only?)
	else if ((lcd_stat.on_off) && (!lcd_stat.lcd_enable))
	{
		lcd_stat.on_off = false;

		//This should only happen in VBlank, but it's possible to do it in other modes
		//On real DMG HW, it creates a black line on the scanline the LCD turns off
		//Nintendo did NOT like this, as it could damage the LCD over repeated uses
		//Note: this is the same effect you see when hitting the power switch OFF
		if (lcd_stat.lcd_mode != 1) { std::cout << "LCD::Warning - Disabling LCD outside of VBlank\n"; }

		//Set LY to zero here, but DO NOT do a LYC test until LCD is turned back on
		//Mr. Do! requires the test to occur only when the LCD is turned on
		lcd_stat.current_scanline = mem->memory_map[REG_LY] = 0;
		lcd_stat.lcd_clock = 0;
		lcd_stat.lcd_mode = 0;
	}
	step_sub(cpu_clock);
}

void GB_LCD::step_sub(int cpu_clock) 
{
	//Perform LCD operations if LCD is enabled
	if(lcd_stat.lcd_enable) 
	{
		lcd_stat.lcd_clock += cpu_clock;

		//Modes 0, 2, and 3 - Outside of VBlank
		if(lcd_stat.lcd_clock < 65664)
		{
			//Mode 2 - OAM Read
			if((lcd_stat.lcd_clock % 456) < 80)
			{
				if(lcd_stat.lcd_mode != 2)
				{
					//Increment scanline when entering Mode 2, signifies the end of HBlank
					//When coming from VBlank, this must part must be ignored
					if(lcd_stat.lcd_mode != 1)
					{
						lcd_stat.current_scanline++;
						mem->memory_map[REG_LY] = lcd_stat.current_scanline;
						scanline_compare();
					}

					lcd_stat.lcd_mode = 2;

					//OAM STAT INT
					if(mem->memory_map[REG_STAT] & 0x20) { mem->memory_map[IF_FLAG] |= 2; }

					lcd_stat.hdma_line = false;
				}
			}

			//Mode 3 - VRAM Read
			else if((lcd_stat.lcd_clock % 456) < 252)
			{
				if(lcd_stat.lcd_mode != 3) { lcd_stat.lcd_mode = 3; }
			}

			//Mode 0 - HBlank
			else
			{
				if(lcd_stat.lcd_mode != 0)
				{
					lcd_stat.lcd_mode = 0;

					//Horizontal blanking DMA
					if((lcd_stat.hdma_in_progress) && (lcd_stat.hdma_type == 1) && (config::gb_type == 2)) { mem->hdma(); }

					//Update OAM
					if(lcd_stat.oam_update) update_oam();
			
					//Render scanline when first entering Mode 0
					collect_scanline_data();

					//HBlank STAT INT
					if(mem->memory_map[REG_STAT] & 0x08) { mem->memory_map[IF_FLAG] |= 2; }
				}
			}
		}

		//Mode 1 - VBlank
		else
		{
			//Entering VBlank
			if(lcd_stat.lcd_mode != 1)
			{
				render_full_screen();
				lcd_stat.lcd_mode = 1;

				//Restore Window parameters
				lcd_stat.last_y = 0;
				lcd_stat.window_y = mem->memory_map[REG_WY];
				lcd_stat.lock_window_y = false;

				//Unset frame delay
				lcd_stat.frame_delay = 0;

				//Increment scanline count
				lcd_stat.current_scanline++;
				mem->memory_map[REG_LY] = lcd_stat.current_scanline;
				scanline_compare();
				
				//Setup the VBlank clock to count 10 scanlines
				lcd_stat.vblank_clock = lcd_stat.lcd_clock - 65664;
					
				//VBlank STAT INT
				if(mem->memory_map[REG_STAT] & 0x10) { mem->memory_map[IF_FLAG] |= 2; }

				//Raise other STAT INTs on this line
				if(((mem->memory_map[IE_FLAG] & 0x1) == 0) && ((mem->memory_map[REG_STAT] & 0x20))) { mem->memory_map[IF_FLAG] |= 2; }

				//VBlank INT
				mem->memory_map[IF_FLAG] |= 1;

				//Display any OSD messages
				if(config::osd_count)
				{
					config::osd_count--;
					SDL_LockSurface(finalscreen);
					if (!cgfx::loaded) { draw_osd_msg(config::osd_message, (u32*)(finalscreen->pixels), 0, 0); }
					else { draw_osd_msg(config::osd_message, (u32*)(finalscreen->pixels), 0, 0); }
					SDL_UnlockSurface(finalscreen);
				}

				//Render final screen buffer
				if(lcd_stat.lcd_enable)
				{
					if (config::use_opengl)
					{
						config::render_external_hw(finalscreen);
					}
					else
					{
						SDL_LockSurface(finalscreen);
						config::render_external_sw((u32*)(finalscreen->pixels));
						SDL_UnlockSurface(finalscreen);
					}
				}

				//Limit framerate
				if(!config::turbo)
				{
					frame_current_time = SDL_GetTicks();
					int delay = frame_delay[fps_count % 60];
					if((frame_current_time - frame_start_time) < delay) { SDL_Delay(delay - (frame_current_time - frame_start_time));}
					frame_start_time = SDL_GetTicks();
				}

				//Update FPS counter + title
				fps_count++;

				//Process Gameshark cheats
				if(config::use_cheats) { mem->set_gs_cheats(); }

			}

			//Processing VBlank
			else
			{
				lcd_stat.vblank_clock += cpu_clock;

				//Increment scanline count
				if(lcd_stat.vblank_clock >= 456)
				{
					lcd_stat.vblank_clock -= 456;
					lcd_stat.current_scanline++;

					//By line 153, LCD has actually reached the top of the screen again
					//LY will read 153 for only a few cycles, then go to 0 for the rest of the scanline
					//Line 153 and Line 0 STAT-LYC IRQs should be triggered here
					if(lcd_stat.current_scanline == 153)
					{
						//Do a scanline compare for Line 153 now
						mem->memory_map[REG_LY] = lcd_stat.current_scanline;
						scanline_compare();

						//Set LY to 0, also trigger Line 0 STAT-LYC IRQ if necessary
						//Technically this should fire 8 cycles into the scanline
						lcd_stat.current_scanline = 0;
						mem->memory_map[REG_LY] = lcd_stat.current_scanline;
						scanline_compare();

						new_rendered_screen_data();
						clear_buffers();
					}

					//After Line 153 reset LCD clock, scanline count
					else if(lcd_stat.current_scanline == 1) 
					{
						lcd_stat.lcd_clock -= 70224;
						lcd_stat.current_scanline = 0;
						mem->memory_map[REG_LY] = lcd_stat.current_scanline;

					}

					//Process Lines 144-152 normally
					else
					{
						mem->memory_map[REG_LY] = lcd_stat.current_scanline;
						scanline_compare();
					}
				}
			}
		}
	}

	mem->memory_map[REG_STAT] = (mem->memory_map[REG_STAT] & ~0x3) | lcd_stat.lcd_mode;
}

void GBC_LCD::step_sub(int cpu_clock)
{
	//Update background color palettes on the GBC
	if (lcd_stat.update_bg_colors) { update_bg_colors(); }

	//Update sprite color palettes on the GBC
	if (lcd_stat.update_obj_colors) { update_obj_colors(); }

	//General Purpose DMA
	if ((lcd_stat.hdma_in_progress) && (lcd_stat.hdma_type == 0)) { mem->gdma(); }

	GB_LCD::step_sub(cpu_clock);
}

void DMG_LCD::collect_scanline_data()
{
	new_rendered_scanline_data(lcd_stat.current_scanline);
	collect_palette();
	if (lcd_stat.bg_enable) collect_bg_scanline();
    if (lcd_stat.bg_enable) collect_win_scanline();
	collect_obj_scanline();
}

void GBC_LCD::collect_scanline_data()
{
	new_rendered_scanline_data(lcd_stat.current_scanline);
	collect_palette();
	collect_bg_scanline();
	collect_win_scanline();
	collect_obj_scanline();
}

void DMG_LCD::collect_palette() {
	bgId = getUsedPaletteIdx(mem->memory_map[REG_BGP]);
	objId1 = getUsedPaletteIdx(mem->memory_map[REG_OBP0]);
	objId2 = getUsedPaletteIdx(mem->memory_map[REG_OBP1]);
}

u16 DMG_LCD::getUsedPaletteIdx(u8 pCode) {
	u16 idx;

	for (u16 i = 0; i < cgfx_stat.screen_data.rendered_palette.size(); i++) {
		if (cgfx_stat.screen_data.rendered_palette[i].code == pCode) {
			return i;
		}
	}
	palette p;
	p.code = pCode;
	p.colour[0] = p.code & 0x3;
	p.colour[1] = (p.code >> 2) & 0x3;
	p.colour[2] = (p.code >> 4) & 0x3;
	p.colour[3] = (p.code >> 6) & 0x3;
	idx = cgfx_stat.screen_data.rendered_palette.size();
	cgfx_stat.screen_data.rendered_palette.push_back(p);
	return idx;
}

void GBC_LCD::collect_palette() {
	if (lcd_stat.current_scanline == 0 || scanlinePaletteUpdated) {
		for (u8 i = 0; i < 8; i++) {
			bgId[i] = getUsedPaletteIdx((u16*)(&(lcd_stat.bg_colors_raw[0][i])), (u32*)(&(lcd_stat.bg_colors_final[0][i])));
			objId[i] = getUsedPaletteIdx((u16*)(&(lcd_stat.obj_colors_raw[0][i])), (u32*)(&(lcd_stat.obj_colors_final[0][i])));
		}
	}
	scanlinePaletteUpdated = false;
}

u16 GBC_LCD::getUsedPaletteIdx(u16* pal, u32* dcolor) {
	u16 idx;

	for (u16 i = 0; i < cgfx_stat.screen_data.rendered_palette.size(); i++) {
		if (cgfx_stat.screen_data.rendered_palette[i].colour[0] == *pal
			&& cgfx_stat.screen_data.rendered_palette[i].colour[1] == *(pal + 8)
			&& cgfx_stat.screen_data.rendered_palette[i].colour[2] == *(pal + 16)
			&& cgfx_stat.screen_data.rendered_palette[i].colour[3] == *(pal + 24)) {
			return i;
		}
	}
	palette p;
	p.colour[0] = *pal;
	p.colour[1] = *(pal + 8);
	p.colour[2] = *(pal + 16);
	p.colour[3] = *(pal + 24);
	p.renderColour[0] = *(dcolor);
	p.renderColour[1] = *(dcolor + 8);
	p.renderColour[2] = *(dcolor + 16);
	p.renderColour[3] = *(dcolor + 24);



	for (u8 i = 0; i < 4; i++) {
		u8 red = p.colour[i] & 0x1F;
		red = ((red << 3) & 0xF8) | (red >> 2);

		u8 green = (p.colour[i] >> 5) & 0x1F;
		green = ((green << 3) & 0xF8) | (green >> 2);

		u8 blue = (p.colour[i] >> 10) & 0x1F;
		blue = ((blue << 3) & 0xF8) | (blue >> 2);

		p.renderColour[i] = 0xFF000000 | (red << 16) | (green << 8) | (blue);
	}

	idx = cgfx_stat.screen_data.rendered_palette.size();
	cgfx_stat.screen_data.rendered_palette.push_back(p);
	return idx;
}


void GB_LCD::collect_bg_scanline() {
	//Determine where to start drawing
	u8 rendered_scanline = lcd_stat.current_scanline + lcd_stat.bg_scroll_y;
	lcd_stat.scanline_pixel_counter = (0x100 - lcd_stat.bg_scroll_x);

	//Determine which tiles we should generate to get the scanline data - integer division ftw :p
	u16 tile_lower_range = (rendered_scanline / 8) * 32;
	u16 tile_upper_range = tile_lower_range + 32;

	//Determine which line of the tiles to generate pixels for this scanline
	u8 tile_line = rendered_scanline % 8;

	collect_scanline_tiles(lcd_stat.bg_map_addr, tile_lower_range, tile_upper_range, tile_line, &(cgfx_stat.screen_data.scanline[lcd_stat.current_scanline].rendered_bg));
}

void GB_LCD::collect_win_scanline() {
	if (!lcd_stat.window_enable) return;
	//Determine if scanline is within window, if not abort rendering
	if ((lcd_stat.current_scanline < lcd_stat.window_y) || (lcd_stat.window_x >= 160)) { return; }

	//Determine where to start drawing
	u8 rendered_scanline = lcd_stat.current_scanline - lcd_stat.window_y;
	lcd_stat.scanline_pixel_counter = lcd_stat.window_x;

	if (!rendered_scanline) { lcd_stat.lock_window_y = true; }

	//Determine which tiles we should generate to get the scanline data - integer division ftw :p
	u16 tile_lower_range = (rendered_scanline / 8) * 32;
	u16 tile_upper_range = tile_lower_range + 32;

	//Determine which line of the tiles to generate pixels for this scanline
	u8 tile_line = rendered_scanline % 8;

	collect_scanline_tiles(lcd_stat.window_map_addr, tile_lower_range, tile_upper_range, tile_line, &(cgfx_stat.screen_data.scanline[lcd_stat.current_scanline].rendered_win));
}

void DMG_LCD::collect_scanline_tiles(u16 map_addr, u16 tile_lower_range, u16 tile_upper_range, u8 tile_line, std::vector <tile_strip>* strips) {
	//Generate background pixel data for selected tiles
	for (int x = tile_lower_range; x < tile_upper_range; x++)
	{
		u8 map_entry = mem->read_u8(map_addr + x);

		//Convert tile number to signed if necessary
		if (lcd_stat.bg_tile_addr == 0x8800) { map_entry = lcd_stat.signed_tile_lut[map_entry]; }

		//Calculate the address of the 8x1 pixel data based on map entry
		u16 tile_head = lcd_stat.bg_tile_addr + (map_entry << 4);
		if (lcd_stat.scanline_pixel_counter < 160 || lcd_stat.scanline_pixel_counter >= 249)
		{
			//add to screen data
			tile_strip t;
			t.hflip = 0;
			t.vflip = 0;
			t.line = tile_line;
			t.graphicsLine = tile_line;
			t.palette_id = bgId;
			t.pattern_id = getUsedTileIdx(tile_head);
			t.pattern_data = cgfx_stat.screen_data.rendered_tile[t.pattern_id].tile.line[tile_line];
			t.x = lcd_stat.scanline_pixel_counter;
			t.entity_id = x;
			strips->push_back(t);
		}
		lcd_stat.scanline_pixel_counter += 8;
	}
}

u16 DMG_LCD::getUsedTileIdx(u16 tile_head) {
	u16 bg_id = ((tile_head & ~0x8000) >> 4);
	tile_used p;
	if (!cgfx_stat.vram_tile_used[bg_id])
	{
		//add to rendered tile list
		p.address = bg_id;
		p.isOld = false;
		for (u8 i = 0; i < 8; i++)
		{
			p.tile.line[i] = mem->read_u16(tile_head + (i << 1));
		}
		for (u16 i = 0; i < cgfx_stat.tiles.size(); i++)
		{
			if (memcmp(cgfx_stat.tiles[i].tileData.line.data(), p.tile.line.data(), 16) == 0)
			{
				p.pack_tile_match.push_back(i);
			}
		}

		cgfx_stat.vram_tile_used[bg_id] = 1;
		cgfx_stat.vram_tile_idx[bg_id] = cgfx_stat.screen_data.rendered_tile.size();
		cgfx_stat.screen_data.rendered_tile.push_back(p);
	}
	else
	{
		cgfx_stat.screen_data.rendered_tile[cgfx_stat.vram_tile_idx[bg_id]].isOld = false;
	}
	return cgfx_stat.vram_tile_idx[bg_id];
}

void GBC_LCD::collect_scanline_tiles(u16 map_addr, u16 tile_lower_range, u16 tile_upper_range, u8 tile_line, std::vector <tile_strip>* strips) {
	u8 old_vram_bank = ((GBC_MMU*)mem)->vram_bank;

	//Generate background pixel data for selected tiles
	for (int x = tile_lower_range; x < tile_upper_range; x++)
	{
		if (lcd_stat.scanline_pixel_counter < 160 || lcd_stat.scanline_pixel_counter >= 249) {
			//Always read CHR data from Bank 0
			((GBC_MMU*)mem)->vram_bank = 0;

			u8 map_entry = mem->read_u8(map_addr + x);

			//Read BG Map attributes from Bank 1
			((GBC_MMU*)mem)->vram_bank = 1;
			u8 bg_map_attribute = mem->read_u8(map_addr + x);

			tile_strip t;

			//Convert tile number to signed if necessary
			if (lcd_stat.bg_tile_addr == 0x8800) { map_entry = lcd_stat.signed_tile_lut[map_entry]; }

			//Calculate the address of the 8x1 pixel data based on map entry
			u16 tile_head = lcd_stat.bg_tile_addr + (map_entry << 4);

			//Grab bytes from VRAM representing 8x1 pixel data
			t.pattern_id = getUsedTileIdx(tile_head, (bg_map_attribute & 0x8) ? 1 : 0);

			//add to screen data
			t.line = tile_line;
			t.palette_sel = bg_map_attribute & 0x7;
			t.palette_id = bgId[t.palette_sel];
			t.priority = (bg_map_attribute & 0x80) ? 1 : 0;

			t.hflip = (bg_map_attribute & 0x20) ? 1 : 0;
			t.vflip = (bg_map_attribute & 0x40) ? 1 : 0;

			t.graphicsLine = ((bg_map_attribute & 0x40) ? 7 - tile_line : tile_line);
			t.pattern_data = cgfx_stat.screen_data.rendered_tile[t.pattern_id].tile.line[t.graphicsLine];
			t.x = lcd_stat.scanline_pixel_counter;
			t.entity_id = x;
			strips->push_back(t);
		}

		lcd_stat.scanline_pixel_counter += 8;
	}

	((GBC_MMU*)mem)->vram_bank = old_vram_bank;
}

u16 GBC_LCD::getUsedTileIdx(u16 tile_head, u8 vbank) {
	u16 bg_id = ((tile_head & ~0x8000) >> 4) + (vbank ? 384 : 0);
	tile_used p;
	if (!cgfx_stat.vram_tile_used[bg_id])
	{
		//add to rendered tile list
		p.address = bg_id;
		p.isOld = false;
		((GBC_MMU*)mem)->vram_bank = vbank;
		for (u8 i = 0; i < 8; i++)
		{
			p.tile.line[i] = mem->read_u16(tile_head + (i << 1));
		}
		for (u16 i = 0; i < cgfx_stat.tiles.size(); i++)
		{
			if (memcmp(cgfx_stat.tiles[i].tileData.line.data(), p.tile.line.data(), 16) == 0)
			{
				p.pack_tile_match.push_back(i);
			}
		}
		cgfx_stat.vram_tile_used[bg_id] = 1;
		cgfx_stat.vram_tile_idx[bg_id] = cgfx_stat.screen_data.rendered_tile.size();
		cgfx_stat.screen_data.rendered_tile.push_back(p);
	}
	else
	{
		cgfx_stat.screen_data.rendered_tile[cgfx_stat.vram_tile_idx[bg_id]].isOld = false;
	}
	return cgfx_stat.vram_tile_idx[bg_id];
}

void DMG_LCD::collect_obj_scanline() {
	if (!lcd_stat.obj_enable) return;

	std::vector <tile_strip>* strips = &(cgfx_stat.screen_data.scanline[lcd_stat.current_scanline].rendered_obj);
	int searchF;
	int searchT;
	int compareIdx;
	//Update render list for DMG games
	//Cycle through all of the sprites
	for (int x = 0; x < 40; x++)
	{
		u8 test_top = ((obj[x].y + lcd_stat.obj_size) > 0x100) ? 0 : obj[x].y;
		u8 test_bottom = (obj[x].y + lcd_stat.obj_size);
		//Check to see if sprite is rendered on the current scanline
		if ((lcd_stat.current_scanline >= test_top) && (lcd_stat.current_scanline < test_bottom))
		{
			tile_strip t;
			t.x = obj[x].x;
			t.hflip = obj[x].h_flip;
			t.vflip = obj[x].v_flip;
			t.bg_priority = obj[x].bg_priority;
			t.line = (lcd_stat.current_scanline - obj[x].y);
			t.graphicsLine = t.line;
			if (t.line > 7) t.line -= 8;
			if (obj[x].v_flip) {
				t.graphicsLine = lcd_stat.obj_size - 1 - t.graphicsLine;
			}
			t.palette_id = (obj[x].palette_number == 1 ? objId2 : objId1);
			t.palette_sel = obj[x].palette_number;
			t.pattern_id = getUsedTileIdx(0x8000 + ((obj[x].tile_number + (t.graphicsLine > 7 ? 1 : 0)) << 4));
			t.pattern_data = cgfx_stat.screen_data.rendered_tile[t.pattern_id].tile.line[t.graphicsLine - (t.graphicsLine > 7 ? 8 : 0)];
			t.priority = (256 - obj[x].x) * 40 - x;
			t.entity_id = x;

			searchF = 0;
			searchT = strips->size();
			while (searchF != searchT) {
				compareIdx = (searchF + searchT) / 2;
				if ((*strips)[compareIdx].priority > t.priority) {
					searchT = compareIdx;
				}
				else {
					searchF = compareIdx + 1;
				}
			}
			strips->insert(strips->begin() + searchF, t);
		}

		if (strips->size() == 10) { break; }
	}
}



void GBC_LCD::collect_obj_scanline() {
	if (!lcd_stat.obj_enable) return;

	std::vector <tile_strip>* strips = &(cgfx_stat.screen_data.scanline[lcd_stat.current_scanline].rendered_obj);
	//Update render list for DMG games
	//Cycle through all of the sprites
	for (int x = 0; x < 40; x++)
	{
		u8 test_top = ((obj[x].y + lcd_stat.obj_size) > 0x100) ? 0 : obj[x].y;
		u8 test_bottom = (obj[x].y + lcd_stat.obj_size);
		//Check to see if sprite is rendered on the current scanline
		if ((lcd_stat.current_scanline >= test_top) && (lcd_stat.current_scanline < test_bottom))
		{
			tile_strip t;
			t.x = obj[x].x;
			t.hflip = obj[x].h_flip;
			t.vflip = obj[x].v_flip;
			t.bg_priority = obj[x].bg_priority;
			t.line = (lcd_stat.current_scanline - obj[x].y);
			t.graphicsLine = t.line;
			if (t.line > 7) t.line -= 8;
			if (obj[x].v_flip) {
				t.graphicsLine = lcd_stat.obj_size - 1 - t.graphicsLine;
			}
			t.palette_sel = obj[x].color_palette_number;
			t.palette_id = objId[obj[x].color_palette_number];
			t.pattern_id = getUsedTileIdx(0x8000 + ((obj[x].tile_number + (t.graphicsLine > 7 ? 1 : 0)) << 4), obj[x].vram_bank);
			t.pattern_data = cgfx_stat.screen_data.rendered_tile[t.pattern_id].tile.line[t.graphicsLine - (t.graphicsLine > 7 ? 8 : 0)];
			t.priority = 40 - x;
			t.entity_id = x;
			strips->push_back(t);
		}

		if (strips->size() == 10) { break; }
	}

}

void GB_LCD::render_full_screen() {
	//resolve global conditions
	resolve_global_condition();

	for (lcd_stat.renderY = 0; lcd_stat.renderY < 144; lcd_stat.renderY++)
	{
		srcrect.y = cgfx::scaling_factor * lcd_stat.renderY;
		hdrect.y = srcrect.y;

		//Draw background pixel data
		render_bg_scanline(false);
		//Draw window pixel data
		render_win_scanline(false);
		//Draw sprite pixel data
		render_obj_scanline(false);
	}

	SDL_FillRect(finalscreen, NULL, 0xFFFFFFFF);
	if (!lcd_stat.frame_delay)
	{
		for (u8 i = 0; i < 60; i++)
		{
			for (u16 n = 0; n < cgfx_stat.bgs[i].size(); n++)
			{
				//check for conditions
				bool allCondPassed = true;
				for (u16 j = 0; j < cgfx_stat.bgs[i][n].condApps.size(); j++)
				{
					pack_condition* c = &(cgfx_stat.conds[cgfx_stat.bgs[i][n].condApps[j].condIdx]);
					bool matchResult = c->latest_result;
					if (cgfx_stat.bgs[i][n].condApps[j].negate) matchResult = !matchResult;
					allCondPassed = allCondPassed && matchResult;
				}
				if (allCondPassed)
				{
					SDL_Rect srcR;
					srcR.x = cgfx_stat.bgs[i][n].offsetX + (lcd_stat.bg_scroll_x * cgfx_stat.bgs[i][n].hscroll);
					srcR.y = cgfx_stat.bgs[i][n].offsetY + (lcd_stat.bg_scroll_y * cgfx_stat.bgs[i][n].vscroll);
					srcR.w = cgfx_stat.imgs[cgfx_stat.bgs[i][n].imgIdx][0]->w;
					srcR.h = cgfx_stat.imgs[cgfx_stat.bgs[i][n].imgIdx][0]->h;

					SDL_BlitSurface(cgfx_stat.imgs[cgfx_stat.bgs[i][n].imgIdx][0], &srcR, finalscreen, NULL);
				}
			}
			if (i == 9)
				SDL_BlitSurface(buffers[0], NULL, finalscreen, NULL);
			else if (i == 19)
				SDL_BlitSurface(buffers[1], NULL, finalscreen, NULL);
			else if (i == 29)
				SDL_BlitSurface(buffers[2], NULL, finalscreen, NULL);
			else if (i == 39)
				SDL_BlitSurface(buffers[3], NULL, finalscreen, NULL);
			else if (i == 49)
				SDL_BlitSurface(buffers[4], NULL, finalscreen, NULL);
		}
	}
}



