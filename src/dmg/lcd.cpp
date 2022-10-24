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
	reset();
}

/****** LCD Destructor ******/
GB_LCD::~GB_LCD()
{
	SDL_DestroyWindow(window);
	std::cout<<"LCD::Shutdown\n";
}

/****** Reset LCD ******/
void GB_LCD::reset()
{
	final_screen = NULL;
	mem = NULL;

	if((window != NULL) && (config::sdl_render)) { SDL_DestroyWindow(window); }
	window = NULL;

	screen_buffer.clear();
	scanline_buffer.clear();
	scanline_raw.clear();
	scanline_priority.clear();
	stretched_buffer.clear();

	screen_buffer.resize(0x5A00, 0);
	scanline_buffer.resize(0x100, 0);
	stretched_buffer.resize(0x100, 0);
	scanline_raw.resize(0x100, 0);
	scanline_priority.resize(0x100, 0);

	frame_start_time = 0;
	frame_current_time = 0;
	fps_count = 0;
	fps_time = 0;

	for(u32 x = 0; x < 60; x++)
	{
		u16 max = (config::max_fps) ? config::max_fps : 60;
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
	for(int x = 0; x < 40; x++) { lcd_stat.oam_update_list[x] = true; }

	lcd_stat.on_off = false;

	lcd_stat.update_bg_colors = false;
	lcd_stat.update_obj_colors = false;
	lcd_stat.hdma_in_progress = false;
	lcd_stat.hdma_line = false;
	lcd_stat.hdma_type = 0;

	lcd_stat.frame_delay = 0;

	//Clear GBC color palettes
	for(int x = 0; x < 4; x++)
	{
		for(int y =  0; y < 8; y++)
		{
			lcd_stat.obj_colors_raw[x][y] = 0;
			lcd_stat.obj_colors_final[x][y] = 0;
			lcd_stat.bg_colors_raw[x][y] = 0;
			lcd_stat.bg_colors_final[x][y] = 0;
		}
	}

	//Signed-to-unsigned tile lookup generation
	for(int x = 0; x < 256; x++)
	{
		u8 tile_number = x;

		if(tile_number <= 127)
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
	for(int x = 0; x < 256; x++)
	{
		u8 tile_number = x;

		if(tile_number >= 127)
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
	for(int x = 0; x < 8; x++) { lcd_stat.flip_8[x] = (7 - x); }

	//16 pixel (vertical) flipping lookup generation
	for(int x = 0; x < 16; x++) { lcd_stat.flip_16[x] = (15 - x); }

	//CGFX setup
	cgfx_stat.current_obj_hash.clear();
	cgfx_stat.current_obj_hash.resize(40, "");

	cgfx_stat.current_bg_hash.clear();
	cgfx_stat.current_bg_hash.resize(384, "");

	cgfx_stat.current_gbc_bg_hash.clear();
	cgfx_stat.current_gbc_bg_hash.resize(2048, "");

	cgfx_stat.bg_update_list.clear();
	cgfx_stat.bg_update_list.resize(384, false);

	cgfx_stat.bg_tile_update_list.clear();
	cgfx_stat.bg_tile_update_list.resize(256, false);

	cgfx_stat.bg_map_update_list.clear();
	cgfx_stat.bg_map_update_list.resize(2048, false);

	cgfx_stat.update_bg = false;
	cgfx_stat.update_map = false;

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

	for(int x = 0; x < 8; x++)
	{
		cgfx_stat.bg_pal_max[x] = 0;
		cgfx_stat.bg_pal_min[x] = 0;
		cgfx_stat.obj_pal_max[x] = 0;
		cgfx_stat.obj_pal_min[x] = 0;
	}

	//Initialize system screen dimensions
	config::sys_width = 160;
	config::sys_height = 144;

	//Initialize DMG/GBC on GBA stretching to normal mode
	config::resize_mode = 0;
	config::request_resize = false;

	max_fullscreen_ratio = 2;

	power_antenna_osd = false;
}

/****** Initialize LCD with SDL ******/
bool GB_LCD::init()
{
	//Initialize with SDL rendering software or hardware
	if(config::sdl_render)
	{
		//Initialize all of SDL
		if(SDL_Init(SDL_INIT_VIDEO) == -1)
		{
			std::cout<<"LCD::Error - Could not initialize SDL video\n";
			return false;
		}

		//Setup OpenGL rendering
		if(config::use_opengl)
		{
			if(!opengl_init())
			{
				std::cout<<"LCD::Error - Could not initialize OpenGL\n";
				return false;
			}
		}

		//Set up software rendering
		else
		{
			window = SDL_CreateWindow("GBE+", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, config::sys_width, config::sys_height, config::flags);
			SDL_GetWindowSize(window, &config::win_width, &config::win_height);
			config::scaling_factor = 1;

			final_screen = SDL_GetWindowSurface(window);
			if (final_screen == NULL) { return false; }
		}
		original_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, config::sys_width, config::sys_height, 32, 0, 0, 0, 0);
		SDL_SetWindowIcon(window, util::load_icon(config::data_path + "icons/gbe_plus.bmp"));
	}

	//Initialize with only a buffer for OpenGL (for external rendering)
	else if((!config::sdl_render) && (config::use_opengl))
	{
		original_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, config::sys_width, config::sys_height, 32, 0, 0, 0, 0);
	}
	if ((config::sdl_render || config::use_opengl) && original_screen == NULL) { return false; }

	std::cout<<"LCD::Initialized\n";

	return true;
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

			//CGFX - Update OBJ hashes
			if(cgfx::load_cgfx) 
			{
				update_obj_hash(x); 
			}
		}

		else { oam_ptr+= 4; }
	}	
}

/****** Updates a list of OBJs to render on the current scanline ******/
void DMG_LCD::update_obj_render_list()
{

}

void GBC_LCD::update_obj_render_list()
{
	obj_render_length = -1;

	u8 obj_x_sort[40];
	u8 obj_sort_length = 0;

	//Cycle through all of the sprites
	for (int x = 0; x < 40; x++)
	{
		u8 test_top = ((obj[x].y + lcd_stat.obj_size) > 0x100) ? 0 : obj[x].y;
		u8 test_bottom = (obj[x].y + lcd_stat.obj_size);

		//Check to see if sprite is rendered on the current scanline
		if ((lcd_stat.current_scanline >= test_top) && (lcd_stat.current_scanline < test_bottom))
		{
			obj_render_length++;
			obj_render_list[obj_render_length] = x;
		}

		//Enforce 10 sprite-per-scanline limit
		if (obj_render_length == 9) { break; }
	}
}

/****** Render pixels for a given scanline (per-scanline) ******/
void GB_LCD::run_render_scanline() 
{
	//Draw background pixel data
	render_bg_scanline();
	//Draw window pixel data
	render_win_scanline();
	//Draw sprite pixel data
	render_obj_scanline();

	//Ignore CGFX when greater than 1:1
	if ((cgfx::loaded) && (cgfx::scaling_factor > 1)) { return; }

	//Draw blank screen for 1 frame after LCD enabled
	if (lcd_stat.frame_delay)
	{
		for (int x = 0; x < 256; x++) { scanline_buffer[x] = 0xFFFFFFFF; }
	}

	//Push scanline buffer to screen buffer - Normal version
	else if ((config::resize_mode == 0) && (!config::request_resize))
	{
		for (int x = 0; x < 160; x++)
		{
			screen_buffer[(config::sys_width * lcd_stat.current_scanline) + x] = scanline_buffer[x];
			scanline_buffer[x] = 0xFFFFFFFF;
		}
	}
}

/****** Manually renders a given scanline - Used for external interfaces to grab screen data ******/
void GB_LCD::render_scanline(u8 line, u8 type)
{
	//Temporarily force current scanline
	u8 temp_line = lcd_stat.current_scanline;
	lcd_stat.current_scanline = line;

	//Temporarily disable CGFX if necessary
	bool cg_stat = cgfx::loaded;
	cgfx::loaded = false;
	
	//Clear scanline data before rendering
	for(u32 x = 0; x < 0x100; x++) { scanline_buffer[x] = 0xFFFFFFFF; }

	//Render based on specified type
	switch(type)
	{
		case 0x00: render_bg_scanline(); break;
		case 0x01: render_win_scanline(); break;
		case 0x02: update_obj_render_list(); render_obj_scanline(); break;
	}

	//Restore current scanline and CGFX
	lcd_stat.current_scanline = temp_line;
	cgfx::loaded = cg_stat;
}

/****** Manually retrieve a given pixel from scanline buffer - Used for external interfaces ******/
u32 GB_LCD::get_scanline_pixel(u8 pixel) { return scanline_buffer[pixel]; }

/****** Renders pixels for the BG (per-scanline) - DMG version ******/
void DMG_LCD::render_bg_scanline()
{
	for (u8 i = 0; i < cgfx_stat.screen_data.scanline[lcd_stat.current_scanline].rendered_bg.size(); i++) {
		u8 tile_pixel = 0;
		tile_strip p = cgfx_stat.screen_data.scanline[lcd_stat.current_scanline].rendered_bg[i];
		u8 pixel_counter = p.x;

		for(int y = 7; y >= 0; y--)
		{
			//Calculate raw value of the tile's pixel
			tile_pixel = ((p.pattern_data >> 8) & (1 << y)) ? 2 : 0;
			tile_pixel |= (p.pattern_data & (1 << y)) ? 1 : 0;

			if(pixel_counter < 160)
			{
				//Set the raw color of the BG
				scanline_raw[pixel_counter] = tile_pixel;
				scanline_buffer[pixel_counter] = config::DMG_BG_PAL[cgfx_stat.screen_data.rendered_palette[p.palette_id].colour[tile_pixel]];
			}
			pixel_counter++;
		}
	}
}

/****** Renders pixels for the BG (per-scanline) - DMG CGFX version ******/
void DMG_LCD::render_cgfx_bg_scanline(u16 bg_id, bool is_bg)
{
	//Determine where to start drawing
	u8 rendered_scanline = is_bg ? (lcd_stat.current_scanline + lcd_stat.bg_scroll_y) : (lcd_stat.current_scanline - lcd_stat.window_y);

	//Determine which line of the tiles to generate pixels for this scanline
	u8 tile_line = rendered_scanline % 8;
	u8 tile_start = tile_line * 8;

	//Grab the ID of this hash to pull custom pixel data
	u16 bg_tile_id = cgfx_stat.m_id[cgfx_stat.last_id];

	//Grab bytes from VRAM representing 8x1 pixel data - Used for drawing the raw_scanline
	u16 tile_data = mem->read_u16(0x8000 + (bg_id << 4) + (tile_line << 1));
	u8 tile_pixel = 0;

	for(int x = tile_start, y = 7; x < (tile_start + 8); x++, y--)
	{
		//Calculate raw value of the tile's pixel
		tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
		tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;

		//Set the raw color of the BG
		scanline_raw[lcd_stat.scanline_pixel_counter] = tile_pixel;

		//Render 1:1
		if(cgfx::scaling_factor <= 1)
		{
			scanline_buffer[lcd_stat.scanline_pixel_counter++] = cgfx_stat.bg_pixel_data[bg_tile_id][x];
		}

		//Render HD
		else
		{
			u32 pos = (lcd_stat.scanline_pixel_counter * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			u32 bg_pos = ((x - tile_start) * cgfx::scaling_factor) + (tile_line * cgfx::scale_squared * 8);
			
			if(lcd_stat.scanline_pixel_counter < 160)
			{

				for(int a = 0; a < cgfx::scaling_factor; a++)
				{
					for(int b = 0; b < cgfx::scaling_factor; b++)
					{
						hd_screen_buffer[pos + b] = cgfx_stat.bg_pixel_data[bg_tile_id][bg_pos + b];
					}
				
					pos += config::sys_width;
					bg_pos += (8 * cgfx::scaling_factor);
				}
			}

			lcd_stat.scanline_pixel_counter++;
		}
	}
}

/****** Renders pixels for the BG (per-scanline) - GBC version ******/
void GBC_LCD::render_bg_scanline()
{
	//Determine where to start drawing
	u8 rendered_scanline = lcd_stat.current_scanline + lcd_stat.bg_scroll_y;
	lcd_stat.scanline_pixel_counter = (0x100 - lcd_stat.bg_scroll_x);

	//Determine which tiles we should generate to get the scanline data - integer division ftw :p
	u16 tile_lower_range = (rendered_scanline / 8) * 32;
	u16 tile_upper_range = tile_lower_range + 32;

	//Generate background pixel data for selected tiles
	for(int x = tile_lower_range; x < tile_upper_range; x++)
	{
		//Always read CHR data from Bank 0
		u8 old_vram_bank = ((GBC_MMU*)mem)->vram_bank;
		((GBC_MMU*)mem)->vram_bank = 0;

		u8 map_entry = mem->read_u8(lcd_stat.bg_map_addr + x);
		u8 tile_pixel = 0;

		//Read BG Map attributes from Bank 1
		((GBC_MMU*)mem)->vram_bank = 1;
		u8 bg_map_attribute = mem->read_u8(lcd_stat.bg_map_addr + x);
		u8 bg_palette = bg_map_attribute & 0x7;
		u8 bg_priority = (bg_map_attribute & 0x80) ? 1 : 0;
		((GBC_MMU*)mem)->vram_bank = (bg_map_attribute & 0x8) ? 1 : 0;

		//Determine which line of the tiles to generate pixels for this scanline
		u8 tile_line = rendered_scanline % 8;
		if(bg_map_attribute & 0x40) { tile_line = lcd_stat.flip_8[tile_line]; }

		//Convert tile number to signed if necessary
		if(lcd_stat.bg_tile_addr == 0x8800) { map_entry = lcd_stat.signed_tile_lut[map_entry]; }

		//Calculate the address of the 8x1 pixel data based on map entry
		u16 tile_addr = (lcd_stat.bg_tile_addr + (map_entry << 4) + (tile_line << 1));

		//Grab bytes from VRAM representing 8x1 pixel data
		u16 tile_data = mem->read_u16(tile_addr);
		((GBC_MMU*)mem)->vram_bank = old_vram_bank;

		//Render CGFX
		u16 map_id = (lcd_stat.bg_map_addr + x) - 0x9800;
		u16 hash_addr = lcd_stat.bg_tile_addr + (map_entry << 4);
		
		if((cgfx::load_cgfx) && (has_hash(hash_addr, cgfx_stat.current_gbc_bg_hash[map_id])) && (cgfx_stat.m_types[cgfx_stat.last_id] == 20))
		{
			render_cgfx_bg_scanline(tile_data, bg_map_attribute, true);
		}

		//Render original pixel data
		else
		{
			for(int y = 7; y >= 0; y--)
			{
				//Calculate raw value of the tile's pixel
				if(bg_map_attribute & 0x20) 
				{
					tile_pixel = ((tile_data >> 8) & (1 << lcd_stat.flip_8[y])) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << lcd_stat.flip_8[y])) ? 1 : 0;
				}

				else
				{
					tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;
				}

				//Set the raw color of the BG
				scanline_raw[lcd_stat.scanline_pixel_counter] = tile_pixel;

				//Set the BG-to-OBJ priority
				scanline_priority[lcd_stat.scanline_pixel_counter] = bg_priority;

				//Set the final color of the BG
				scanline_buffer[lcd_stat.scanline_pixel_counter++] = lcd_stat.bg_colors_final[tile_pixel][bg_palette];

				u8 last_scanline_pixel = lcd_stat.scanline_pixel_counter - 1;

				//Render HD
				if((cgfx::loaded) && (cgfx::scaling_factor > 1) && (last_scanline_pixel < 160))
				{
					u32 pos = (last_scanline_pixel * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			
					for(int a = 0; a < cgfx::scaling_factor; a++)
					{
						for(int b = 0; b < cgfx::scaling_factor; b++)
						{
							hd_screen_buffer[pos + b] = scanline_buffer[last_scanline_pixel];
						}
	
						pos += config::sys_width;
					}
				}
			}
		}
	}
}

/****** Renders pixels for the BG (per-scanline) - GBC CGFX version ******/
void GBC_LCD::render_cgfx_bg_scanline(u16 tile_data, u8 bg_map_attribute, bool is_bg)
{
	//Determine where to start drawing
	u8 rendered_scanline = is_bg ? (lcd_stat.current_scanline + lcd_stat.bg_scroll_y) : (lcd_stat.current_scanline - lcd_stat.window_y);

	//Determine which line of the tiles to generate pixels for this scanline
	u8 tile_line = rendered_scanline % 8;
	if(bg_map_attribute & 0x40) { tile_line = lcd_stat.flip_8[tile_line]; }
	u8 tile_start = tile_line * 8;

	//Grab the ID of this hash to pull custom pixel data
	u16 bg_tile_id = cgfx_stat.m_id[cgfx_stat.last_id];

	//Grab the auto-bright property of this hash
	u8 auto_bright = cgfx_stat.m_auto_bright[cgfx_stat.last_id];

	u8 tile_pixel = 0;
	u8 bg_priority = (bg_map_attribute & 0x80) ? 1 : 0;

	//Account for horizontal flipping
	lcd_stat.scanline_pixel_counter = (bg_map_attribute & 0x20) ? (lcd_stat.scanline_pixel_counter + 7) : lcd_stat.scanline_pixel_counter;
	s16 counter = (bg_map_attribute & 0x20) ? -1 : 1;

	for(int x = tile_start, y = 7; x < (tile_start + 8); x++, y--)
	{
		//Calculate raw value of the tile's pixel
		if(bg_map_attribute & 0x20) 
		{
			tile_pixel = ((tile_data >> 8) & (1 << lcd_stat.flip_8[y])) ? 2 : 0;
			tile_pixel |= (tile_data & (1 << lcd_stat.flip_8[y])) ? 1 : 0;
		}

		else
		{
			tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
			tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;
		}

		//Set the raw color of the BG
		scanline_raw[lcd_stat.scanline_pixel_counter] = tile_pixel;

		//Set the BG-to-OBJ priority
		scanline_priority[lcd_stat.scanline_pixel_counter] = bg_priority;

		//Render 1:1
		if(cgfx::scaling_factor <= 1)
		{
			//Adjust pixel brightness if EXT_AUTO_BRIGHT is set
			if(auto_bright) 
			{
				u32 custom_color = cgfx_stat.bg_pixel_data[bg_tile_id][x];
				scanline_buffer[lcd_stat.scanline_pixel_counter] = adjust_pixel_brightness(custom_color, (bg_map_attribute & 0x7), 0);
			}

			else { scanline_buffer[lcd_stat.scanline_pixel_counter] = cgfx_stat.bg_pixel_data[bg_tile_id][x]; }
		}

		//Render HD
		else
		{
			u32 pos = (lcd_stat.scanline_pixel_counter * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			u32 bg_pos = ((x - tile_start) * cgfx::scaling_factor) + (tile_line * cgfx::scale_squared * 8);
			u32 c = 0;
			u32 d = 0;
			
			if(lcd_stat.scanline_pixel_counter < 160)
			{
				for(int a = 0; a < cgfx::scaling_factor; a++)
				{
					//Calculate vertical flipping offset
					d = (bg_map_attribute & 0x40) ? ((cgfx::scaling_factor - a - 1) * config::sys_width) :  (a * config::sys_width);

					for(int b = 0; b < cgfx::scaling_factor; b++)
					{
						//Calculate horizontal flipping offset
						c = (bg_map_attribute & 0x20) ? (cgfx::scaling_factor - b - 1) : b;

						//Adjust pixel brightness if EXT_AUTO_BRIGHT is set
						if(auto_bright)
						{
							u32 custom_color = cgfx_stat.bg_pixel_data[bg_tile_id][bg_pos + c];
							hd_screen_buffer[pos + b + d] = adjust_pixel_brightness(custom_color, (bg_map_attribute & 0x7), 0);
						}

						else { hd_screen_buffer[pos + b + d] = cgfx_stat.bg_pixel_data[bg_tile_id][bg_pos + c]; }
					}
				
					bg_pos += (8 * cgfx::scaling_factor);
				}
			}
		}

		lcd_stat.scanline_pixel_counter += counter;
	}

	if(bg_map_attribute & 0x20) { lcd_stat.scanline_pixel_counter += 9; }
}

/****** Renders pixels for the Window (per-scanline) - DMG version ******/
void DMG_LCD::render_win_scanline()
{
	for (u8 i = 0; i < cgfx_stat.screen_data.scanline[lcd_stat.current_scanline].rendered_win.size(); i++) {
		u8 tile_pixel = 0;
		tile_strip p = cgfx_stat.screen_data.scanline[lcd_stat.current_scanline].rendered_win[i];
		u8 pixel_counter = p.x;

		for (int y = 7; y >= 0; y--)
		{
			//Calculate raw value of the tile's pixel
			tile_pixel = ((p.pattern_data >> 8) & (1 << y)) ? 2 : 0;
			tile_pixel |= (p.pattern_data & (1 << y)) ? 1 : 0;

			if (pixel_counter < 160)
			{
				//Set the raw color of the BG
				scanline_raw[pixel_counter] = tile_pixel;
				scanline_buffer[pixel_counter] = config::DMG_BG_PAL[cgfx_stat.screen_data.rendered_palette[p.palette_id].colour[tile_pixel]];
			}
			pixel_counter++;
			if (pixel_counter == 160) return;
		}
	}
}

/****** Renders pixels for the Window (per-scanline) - GBC version ******/
void GBC_LCD::render_win_scanline()
{
	if (!lcd_stat.window_enable) return;

	//Determine if scanline is within window, if not abort rendering
	if((lcd_stat.current_scanline < lcd_stat.window_y) || (lcd_stat.window_x >= 160)) { return; }

	//Determine where to start drawing
	u8 rendered_scanline = lcd_stat.current_scanline - lcd_stat.window_y;
	lcd_stat.scanline_pixel_counter = lcd_stat.window_x;

	if(!rendered_scanline) { lcd_stat.lock_window_y = true; }

	//Determine which tiles we should generate to get the scanline data - integer division ftw :p
	u16 tile_lower_range = (rendered_scanline / 8) * 32;
	u16 tile_upper_range = tile_lower_range + 32;

	//Generate background pixel data for selected tiles
	for(int x = tile_lower_range; x < tile_upper_range; x++)
	{
		//Always read CHR data from Bank 0
		u8 old_vram_bank = ((GBC_MMU*)mem)->vram_bank;
		((GBC_MMU*)mem)->vram_bank = 0;

		u8 map_entry = mem->read_u8(lcd_stat.window_map_addr + x);
		u8 tile_pixel = 0;

		//Read BG Map attributes from Bank 1
		((GBC_MMU*)mem)->vram_bank = 1;
		u8 bg_map_attribute = mem->read_u8(lcd_stat.window_map_addr + x);
		u8 bg_palette = bg_map_attribute & 0x7;
		u8 bg_priority = (bg_map_attribute & 0x80) ? 1 : 0;
		((GBC_MMU*)mem)->vram_bank = (bg_map_attribute & 0x8) ? 1 : 0;

		//Determine which line of the tiles to generate pixels for this scanline
		u8 tile_line = rendered_scanline % 8;
		if(bg_map_attribute & 0x40) { tile_line = lcd_stat.flip_8[tile_line]; }

		//Convert tile number to signed if necessary
		if(lcd_stat.bg_tile_addr == 0x8800) { map_entry = lcd_stat.signed_tile_lut[map_entry]; }

		//Calculate the address of the 8x1 pixel data based on map entry
		u16 tile_addr = (lcd_stat.bg_tile_addr + (map_entry << 4) + (tile_line << 1));

		//Grab bytes from VRAM representing 8x1 pixel data
		u16 tile_data = mem->read_u16(tile_addr);
		((GBC_MMU*)mem)->vram_bank = old_vram_bank;

		//Render CGFX
		u16 map_id = (lcd_stat.window_map_addr + x) - 0x9800;
		u16 hash_addr = lcd_stat.bg_tile_addr + (map_entry << 4);
		
		if((cgfx::load_cgfx) && (has_hash(hash_addr, cgfx_stat.current_gbc_bg_hash[map_id])) && (cgfx_stat.m_types[cgfx_stat.last_id] == 20))
		{
			render_cgfx_bg_scanline(tile_data, bg_map_attribute, false);
		}

		//Render original pixel data
		else
		{
			for(int y = 7; y >= 0; y--)
			{
				//Calculate raw value of the tile's pixel
				if(bg_map_attribute & 0x20) 
				{
					tile_pixel = ((tile_data >> 8) & (1 << lcd_stat.flip_8[y])) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << lcd_stat.flip_8[y])) ? 1 : 0;
				}

				else 
				{
					tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;
				}

				//Set the raw color of the BG
				scanline_raw[lcd_stat.scanline_pixel_counter] = tile_pixel;

				//Set the BG-to-OBJ priority
				scanline_priority[lcd_stat.scanline_pixel_counter] = bg_priority;

				//Set the final color of the BG
				scanline_buffer[lcd_stat.scanline_pixel_counter++] = lcd_stat.bg_colors_final[tile_pixel][bg_palette];

				u8 last_scanline_pixel = lcd_stat.scanline_pixel_counter - 1;

				//Render HD
				if((cgfx::loaded) && (cgfx::scaling_factor > 1) && (last_scanline_pixel < 160))
				{
					u32 pos = (last_scanline_pixel * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			
					for(int a = 0; a < cgfx::scaling_factor; a++)
					{
						for(int b = 0; b < cgfx::scaling_factor; b++)
						{
							hd_screen_buffer[pos + b] = scanline_buffer[last_scanline_pixel];
						}
	
						pos += config::sys_width;
					}
				}

				//Abort rendering if next pixel is off-screen
				if(lcd_stat.scanline_pixel_counter == 160) { return; }
			}
		}
	}
}

/****** Renders pixels for OBJs (per-scanline) - DMG version ******/
void DMG_LCD::render_obj_scanline()
{
	u32 line[160];
	for (u8 x = 0; x < 160; x++) line[x] = 0;

	for (u8 i = 0; i < cgfx_stat.screen_data.scanline[lcd_stat.current_scanline].rendered_obj.size(); i++) {
		u8 tile_pixel = 0;
		tile_strip p = cgfx_stat.screen_data.scanline[lcd_stat.current_scanline].rendered_obj[i];
		u8 pixel_counter = p.x;

		for (int y = 7; y >= 0; y--)
		{
			//Calculate raw value of the tile's pixel
			tile_pixel = ((p.pattern_data >> 8) & (1 << y)) ? 2 : 0;
			tile_pixel |= (p.pattern_data & (1 << y)) ? 1 : 0;
			pixel_counter = (p.hflip ? p.x + y : p.x + 7 - y);
			if (pixel_counter < 160)
			{
				if (tile_pixel != 0) {
					line[pixel_counter] = 0;
					if (p.bg_priority == 0 || scanline_raw[pixel_counter] == 0) {
						line[pixel_counter] = config::DMG_BG_PAL[cgfx_stat.screen_data.rendered_palette[p.palette_id].colour[tile_pixel]];
					}
				}
			}
			pixel_counter++;
		}

	}
	//fill actual buffer
	for (u8 i = 0; i < 160; i++) {
		if (line[i] != 0) {
			scanline_buffer[i] = line[i];
		}
	}
}

/****** Renders pixels for OBJs (per-scanline) - DMG CGFX version ******/
void DMG_LCD::render_cgfx_obj_scanline(u8 sprite_id)
{
	//Set the current pixel to start obj rendering
	lcd_stat.scanline_pixel_counter = obj[sprite_id].x;
		
	//Determine which line of the tiles to generate pixels for this scanline		
	u8 tile_line = (lcd_stat.current_scanline - obj[sprite_id].y);
	if(obj[sprite_id].v_flip) { tile_line = (lcd_stat.obj_size == 8) ? lcd_stat.flip_8[tile_line] : lcd_stat.flip_16[tile_line]; }

	//Grab the ID of this hash to pull custom pixel data
	u16 obj_id = cgfx_stat.m_id[cgfx_stat.last_id];

	u16 tile_pixel = (8 * tile_line);
	u32 custom_color = 0;

	//Account for horizontal flipping
	lcd_stat.scanline_pixel_counter = obj[sprite_id].h_flip ? (lcd_stat.scanline_pixel_counter + 7) : lcd_stat.scanline_pixel_counter;
	s16 counter = obj[sprite_id].h_flip ? -1 : 1;

	//Output 8x1 line of custom pixel data
	for(int x = tile_pixel; x < (tile_pixel + 8); x++)
	{
		//Render 1:1
		if(cgfx::scaling_factor <= 1)
		{
			custom_color = cgfx_stat.obj_pixel_data[obj_id][x];

			if(custom_color == cgfx::transparency_color) { }
			else if((obj[sprite_id].bg_priority == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { }
			else { scanline_buffer[lcd_stat.scanline_pixel_counter] = cgfx_stat.obj_pixel_data[obj_id][x]; }
		}

		//Render HD
		else
		{
			u32 pos = (lcd_stat.scanline_pixel_counter * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			u32 obj_pos = ((x - tile_pixel) * cgfx::scaling_factor) + (tile_line * cgfx::scale_squared * 8);
			u32 c = 0;
			u32 d = 0;

			if(lcd_stat.scanline_pixel_counter < 160)
			{
				for(int a = 0; a < cgfx::scaling_factor; a++)
				{
					//Calculate vertical flipping offset
					d = obj[sprite_id].v_flip ? ((cgfx::scaling_factor - a - 1) * config::sys_width) :  (a * config::sys_width);

					for(int b = 0; b < cgfx::scaling_factor; b++)
					{
						//Calculate horizontal flipping offset
						c = obj[sprite_id].h_flip ? (cgfx::scaling_factor - b - 1) : b;
						custom_color = cgfx_stat.obj_pixel_data[obj_id][obj_pos + c];

						if(custom_color == cgfx::transparency_color) { }
						else if((obj[sprite_id].bg_priority == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { }
						else { hd_screen_buffer[pos + b + d] = cgfx_stat.obj_pixel_data[obj_id][obj_pos + c]; }
					}

					obj_pos += (8 * cgfx::scaling_factor);
				}
			}
		}

		lcd_stat.scanline_pixel_counter += counter;
	}
}	

/****** Renders pixels for OBJs (per-scanline) - GBC version ******/
void GBC_LCD::render_obj_scanline()
{
	if (!lcd_stat.obj_enable) return;
	//If no sprites are rendered on this line, quit now
	if(obj_render_length < 0) { return; }

	//Cycle through all sprites that are rendering on this pixel, draw them according to their priority
	for(int x = obj_render_length; x >= 0; x--)
	{
		u8 sprite_id = obj_render_list[x];

		//Render CGFX
		u16 hash_addr = 0x8000 + (obj[sprite_id].tile_number << 4);
		
		if((cgfx::load_cgfx) && (has_hash(hash_addr, cgfx_stat.current_obj_hash[sprite_id])) && (cgfx_stat.m_types[cgfx_stat.last_id] == 2))
		{
			render_cgfx_obj_scanline(sprite_id);
		}

		//Render original pixel data
		else
		{
			//Set the current pixel to start obj rendering
			lcd_stat.scanline_pixel_counter = obj[sprite_id].x;
		
			//Determine which line of the tiles to generate pixels for this scanline		
			u8 tile_line = (lcd_stat.current_scanline - obj[sprite_id].y);
			if(obj[sprite_id].v_flip) { tile_line = (lcd_stat.obj_size == 8) ? lcd_stat.flip_8[tile_line] : lcd_stat.flip_16[tile_line]; }

			u8 tile_pixel = 0;

			//Calculate the address of the 8x1 pixel data based on map entry
			u16 tile_addr = (0x8000 + (obj[sprite_id].tile_number << 4) + (tile_line << 1));

			//Grab bytes from VRAM representing 8x1 pixel data
			u8 old_vram_bank = ((GBC_MMU*)mem)->vram_bank;
			((GBC_MMU*)mem)->vram_bank = obj[sprite_id].vram_bank;
			u16 tile_data = mem->read_u16(tile_addr);
			((GBC_MMU*)mem)->vram_bank = old_vram_bank;

			for(int y = 7; y >= 0; y--)
			{
				bool draw_obj_pixel = true;

				//Calculate raw value of the tile's pixel
				if(obj[sprite_id].h_flip) 
				{
					tile_pixel = ((tile_data >> 8) & (1 << lcd_stat.flip_8[y])) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << lcd_stat.flip_8[y])) ? 1 : 0;
				}

				else 
				{
					tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;
				}

				//If Bit 0 of LCDC is clear, always give sprites priority
				if(!lcd_stat.bg_enable) { scanline_priority[lcd_stat.scanline_pixel_counter] = 0; }

				//If raw color is zero, this is the sprite's transparency, abort rendering this pixel
				if(tile_pixel == 0) { draw_obj_pixel = false; }

				//If sprite is below BG and BG raw color is non-zero, abort rendering this pixel
				else if((obj[sprite_id].bg_priority == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { draw_obj_pixel = false; }

				//If sprite is above BG but BG has priority and BG raw color is non-zero, abort rendering this pixel
				else if((obj[sprite_id].bg_priority == 0) && (scanline_priority[lcd_stat.scanline_pixel_counter] == 1) 
				&& (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { draw_obj_pixel = false; }
				
				//Render sprite pixel
				if(draw_obj_pixel)
				{
					scanline_buffer[lcd_stat.scanline_pixel_counter++] = lcd_stat.obj_colors_final[tile_pixel][obj[sprite_id].color_palette_number];

					u8 last_scanline_pixel = lcd_stat.scanline_pixel_counter - 1;

					//Render HD
					if((cgfx::loaded) && (cgfx::scaling_factor > 1) && (last_scanline_pixel < 160))
					{
						u32 pos = (last_scanline_pixel * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			
						for(int a = 0; a < cgfx::scaling_factor; a++)
						{
							for(int b = 0; b < cgfx::scaling_factor; b++)
							{
								hd_screen_buffer[pos + b] = scanline_buffer[last_scanline_pixel];
							}
	
							pos += config::sys_width;
						}
					}
				}

				//Move onto next pixel in scanline to see if sprite rendering occurs
				else { lcd_stat.scanline_pixel_counter++; }
			}
		}
	}
}

/****** Renders pixels for OBJs (per-scanline) - GBC CGFX version ******/
void GBC_LCD::render_cgfx_obj_scanline(u8 sprite_id)
{
	//Set the current pixel to start obj rendering
	lcd_stat.scanline_pixel_counter = obj[sprite_id].x;
		
	//Determine which line of the tiles to generate pixels for this scanline		
	u8 tile_line = (lcd_stat.current_scanline - obj[sprite_id].y);
	if(obj[sprite_id].v_flip) { tile_line = (lcd_stat.obj_size == 8) ? lcd_stat.flip_8[tile_line] : lcd_stat.flip_16[tile_line]; }

	//Grab the ID of this hash to pull custom pixel data
	u16 obj_id = cgfx_stat.m_id[cgfx_stat.last_id];

	//Grab the auto-bright property of this hash
	u8 auto_bright = cgfx_stat.m_auto_bright[cgfx_stat.last_id];

	u16 tile_pixel = (8 * tile_line);
	u32 custom_color = 0;

	//Account for horizontal flipping
	lcd_stat.scanline_pixel_counter = obj[sprite_id].h_flip ? (lcd_stat.scanline_pixel_counter + 7) : lcd_stat.scanline_pixel_counter;
	s16 counter = obj[sprite_id].h_flip ? -1 : 1;

	//Output 8x1 line of custom pixel data
	for(int x = tile_pixel; x < (tile_pixel + 8); x++)
	{
		if(!lcd_stat.bg_enable) { scanline_priority[lcd_stat.scanline_pixel_counter] = 0; }

		//Render 1:1
		if(cgfx::scaling_factor <= 1)
		{
			custom_color = cgfx_stat.obj_pixel_data[obj_id][x];

			//Adjust pixel brightness if EXT_AUTO_BRIGHT is set
			if((auto_bright) && (custom_color != cgfx::transparency_color)) 
			{
				custom_color = adjust_pixel_brightness(custom_color, obj[sprite_id].color_palette_number, 1);
			}

			if(custom_color == cgfx::transparency_color) { }
			else if((obj[sprite_id].bg_priority == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { }
			else if((obj[sprite_id].bg_priority == 0) && (scanline_priority[lcd_stat.scanline_pixel_counter] == 1) 
			&& (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { }
			else { scanline_buffer[lcd_stat.scanline_pixel_counter] = custom_color; }
		}

		//Render HD
		else
		{
			u32 pos = (lcd_stat.scanline_pixel_counter * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			u32 obj_pos = ((x - tile_pixel) * cgfx::scaling_factor) + (tile_line * cgfx::scale_squared * 8);
			u32 c = 0;
			u32 d = 0;
			
			if(lcd_stat.scanline_pixel_counter < 160)
			{
				for(int a = 0; a < cgfx::scaling_factor; a++)
				{
					//Calculate vertical flipping offset
					d = obj[sprite_id].v_flip ? ((cgfx::scaling_factor - a - 1) * config::sys_width) :  (a * config::sys_width);

					for(int b = 0; b < cgfx::scaling_factor; b++)
					{
						c = obj[sprite_id].h_flip ? (cgfx::scaling_factor - b - 1) : b;
						custom_color = cgfx_stat.obj_pixel_data[obj_id][obj_pos + c];

						//Adjust pixel brightness if EXT_AUTO_BRIGHT is set
						if((auto_bright) && (custom_color != cgfx::transparency_color)) 
						{
							custom_color = adjust_pixel_brightness(custom_color, obj[sprite_id].color_palette_number, 1);
						}

						if(custom_color == cgfx::transparency_color) { }
						else if((obj[sprite_id].bg_priority == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { }
						else if((obj[sprite_id].bg_priority == 0) && (scanline_priority[lcd_stat.scanline_pixel_counter] == 1) 
						&& (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { }
						else { hd_screen_buffer[pos + b + d] = custom_color; }
					}

					obj_pos += (8 * cgfx::scaling_factor);
				}
			}
		}

		lcd_stat.scanline_pixel_counter += counter;
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

	u8 red = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 green = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 blue = ((color_bytes & 0x1F) * 8);

	u32 old_color = lcd_stat.bg_colors_final[color][palette];

	lcd_stat.bg_colors_final[color][palette] = 0xFF000000 | (red << 16) | (green << 8) | (blue);
	lcd_stat.bg_colors_raw[color][palette] = lcd_stat.bg_colors_raw[color][palette];

	//Update DMG BG palette when using GBC BIOS
	if(mem->in_bios)
	{
		config::DMG_BG_PAL[0] = lcd_stat.bg_colors_final[0][0];
		config::DMG_BG_PAL[1] = lcd_stat.bg_colors_final[1][0];
		config::DMG_BG_PAL[2] = lcd_stat.bg_colors_final[2][0];
		config::DMG_BG_PAL[3] = lcd_stat.bg_colors_final[3][0];
	}

	lcd_stat.update_bg_colors = false;

	//CGFX - Update BG hashes
	if((cgfx::load_cgfx) && (old_color != lcd_stat.bg_colors_final[color][palette]))
	{
		u8 temp_vram_bank = ((GBC_MMU*)mem)->vram_bank;
		((GBC_MMU*)mem)->vram_bank = 1;

		for(u16 x = 0; x < 2048; x++)
		{
			u8 bg_pal = (mem->read_u8(0x9800 + x) & 0x7);

			if(bg_pal == palette)
			{
				cgfx_stat.update_map = true;
				cgfx_stat.bg_map_update_list[x] = true;
			}
		}

		((GBC_MMU*)mem)->vram_bank = temp_vram_bank;
	}

	//CGFX - Find max-min palette brightness
	if(cgfx::load_cgfx)
	{
		cgfx_stat.bg_pal_max[palette] = cgfx_stat.bg_pal_min[palette] = util::get_brightness_fast(lcd_stat.bg_colors_final[0][palette]);

		for(u32 x = 0; x < 4; x++)
		{
			u8 brightness = util::get_brightness_fast(lcd_stat.bg_colors_final[x][palette]);

			if(brightness > cgfx_stat.bg_pal_max[palette]) { cgfx_stat.bg_pal_max[palette] = brightness; }
			if(brightness < cgfx_stat.bg_pal_min[palette]) { cgfx_stat.bg_pal_min[palette] = brightness; }
		}
	}
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

	u8 red = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 green = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 blue = ((color_bytes & 0x1F) * 8);

	u32 old_color = lcd_stat.obj_colors_final[color][palette];

	lcd_stat.obj_colors_final[color][palette] = 0xFF000000 | (red << 16) | (green << 8) | (blue);
	lcd_stat.obj_colors_raw[color][palette] = lcd_stat.obj_colors_raw[color][palette];

	//Update DMG OBJ palettes when using GBC BIOS
	if(mem->in_bios)
	{
		config::DMG_OBJ_PAL[0][0] = lcd_stat.obj_colors_final[0][0];
		config::DMG_OBJ_PAL[1][0] = lcd_stat.obj_colors_final[1][0];
		config::DMG_OBJ_PAL[2][0] = lcd_stat.obj_colors_final[2][0];
		config::DMG_OBJ_PAL[3][0] = lcd_stat.obj_colors_final[3][0];

		config::DMG_OBJ_PAL[0][1] = lcd_stat.obj_colors_final[0][1];
		config::DMG_OBJ_PAL[1][1] = lcd_stat.obj_colors_final[1][1];
		config::DMG_OBJ_PAL[2][1] = lcd_stat.obj_colors_final[2][1];
		config::DMG_OBJ_PAL[3][1] = lcd_stat.obj_colors_final[3][1];
	}

	lcd_stat.update_obj_colors = false;

	//CGFX - Update OBJ hashes
	if((cgfx::load_cgfx) && (old_color != lcd_stat.obj_colors_final[color][palette]))
	{
		for(int x = 0; x < 40; x++)
		{
			if(obj[x].color_palette_number == palette) { update_obj_hash(x); }
		}
	}

	//CGFX - Find max-min palette brightness
	if(cgfx::load_cgfx)
	{
		cgfx_stat.obj_pal_max[palette] = cgfx_stat.obj_pal_min[palette] = util::get_brightness_fast(lcd_stat.obj_colors_final[1][palette]);

		for(u32 x = 1; x < 4; x++)
		{
			u8 brightness = util::get_brightness_fast(lcd_stat.obj_colors_final[x][palette]);

			if(brightness > cgfx_stat.obj_pal_max[palette]) { cgfx_stat.obj_pal_max[palette] = brightness; }
			if(brightness < cgfx_stat.obj_pal_min[palette]) { cgfx_stat.obj_pal_min[palette] = brightness; }
		}
	}
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
					update_obj_render_list();

					//CGFX - Update BG Hashes
					if(cgfx_stat.update_bg)
					{
						update_all_bg_hash();
						cgfx_stat.update_bg = false;
					}

					if(cgfx_stat.update_map)
					{
						//Update specific map entries
						for(int x = 0; x < 2048; x++)
						{
							if(cgfx_stat.bg_map_update_list[x]) 
							{
								update_bg_hash(0x9800 + x);
								cgfx_stat.bg_map_update_list[x] = false;
							}
						}
						
						cgfx_stat.update_map = false;
					}
					
					//Render scanline when first entering Mode 0
					collect_scanline_data();
					run_render_scanline();

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

				//Check for screen resize - DMG/GBC stretch or sewing subscreen
				if((config::request_resize) && (config::resize_mode > 0))
				{
					//DMG/GBC stretch
					if(config::sio_device != 14)
					{
						config::sys_width = 240;
						config::sys_height = 160;
						screen_buffer.clear();
						screen_buffer.resize(0x9600, 0xFFFFFFFF);
					}

					else
					{
						config::resize_mode = 0;
						config::sys_width = 160;
						config::sys_height = 288;
						screen_buffer.clear();
						screen_buffer.resize(0xB400, 0xFFFFFFFF);
						mem->sub_screen_buffer.clear();
						mem->sub_screen_buffer.resize(0x5A00, 0xFFFFFFFF);
						mem->g_pad->con_update = true;
					}
					
					if((window != NULL) && (config::sdl_render)) { SDL_DestroyWindow(window); }
					init();
					
					if(config::sdl_render) { config::request_resize = false; }
				}

				//Check for screen resize - Normal DMG/GBC screen
				else if(config::request_resize)
				{
					config::sys_width = 160;
					config::sys_height = 144;
					screen_buffer.clear();
					screen_buffer.resize(0x5A00, 0xFFFFFFFF);
					mem->sub_screen_buffer.clear();

					if((window != NULL) && (config::sdl_render)) { SDL_DestroyWindow(window); }
					init();
					
					if(config::sdl_render) { config::request_resize = false; }
				}

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
					if(!cgfx::loaded) { draw_osd_msg(config::osd_message, screen_buffer, 0, 0); }
					else { draw_osd_msg(config::osd_message, screen_buffer, 0, 0); }
				}

				//Process Power Antenna
				if(power_antenna_osd)
				{
					u8 x_offset = (config::sys_width / 8) - 3;
					u8 y_offset = (config::sys_height / 8) - 1;
					draw_osd_msg(std::string("***"), screen_buffer, x_offset, y_offset);
				}

				//Process sewing machines
				if(mem->g_pad->con_flags & 0x800) { mem->g_pad->con_update = true; }

				//Render final screen buffer
				if(lcd_stat.lcd_enable)
				{
					//Copy sub-screen to screen buffer
					if(mem->sub_screen_buffer.size())
					{
						for(u32 x = 0; x < 0x5A00; x++)
						{
							screen_buffer[0x5A00 + x] = mem->sub_screen_buffer[x];
						}
					}
					if (config::sdl_render || config::use_opengl)
					{
						//Lock source surface
						if (SDL_MUSTLOCK(original_screen)) { SDL_LockSurface(original_screen); }
						u32* out_pixel_data = (u32*)original_screen->pixels;

						//Regular 1:1 framebuffer rendering
						if ((!cgfx::loaded) || (cgfx::scaling_factor <= 1))
						{
							for (int a = 0; a < screen_buffer.size(); a++) { out_pixel_data[a] = screen_buffer[a]; }
						}

						//HD CGFX framebuffer rendering
						else if ((cgfx::loaded) && (cgfx::scaling_factor > 1))
						{
							for (int a = 0; a < hd_screen_buffer.size(); a++) { out_pixel_data[a] = hd_screen_buffer[a]; }
						}

						//Unlock source surface
						if (SDL_MUSTLOCK(original_screen)) { SDL_UnlockSurface(original_screen); }

						//Use SDL
						if (config::sdl_render)
						{
							//If using SDL and no OpenGL, manually stretch for fullscreen via SDL
							if ((config::flags & SDL_WINDOW_FULLSCREEN_DESKTOP) && (!config::use_opengl))
							{
								//Blit the original surface to the final stretched one
								SDL_Rect dest_rect;
								dest_rect.w = config::sys_width * max_fullscreen_ratio;
								dest_rect.h = config::sys_height * max_fullscreen_ratio;
								dest_rect.x = ((config::win_width - dest_rect.w) >> 1);
								dest_rect.y = ((config::win_height - dest_rect.h) >> 1);
								SDL_BlitScaled(original_screen, NULL, final_screen, &dest_rect);

								if (SDL_UpdateWindowSurface(window) != 0) { std::cout << "LCD::Error - Could not blit\n"; }
							}

							//Otherwise, render normally (SDL 1:1, OpenGL handles its own stretching)
							else
							{
								//Display final screen buffer - OpenGL
								if (config::use_opengl) { opengl_blit(); }

								//Display final screen buffer - SDL
								else
								{
									if (SDL_UpdateWindowSurface(window) != 0) { std::cout << "LCD::Error - Could not blit\n"; }
								}
							}
						}
						else 
						{
							config::render_external_hw(original_screen);
						}
					}
					else
					{
						//Regular 1:1 framebuffer rendering
						if ((!cgfx::loaded) || (cgfx::scaling_factor <= 1)) { config::render_external_sw(screen_buffer); }

						//HD CGFX framebuffer rendering
						else if ((cgfx::loaded) && (cgfx::scaling_factor > 1)) { config::render_external_sw(hd_screen_buffer); }
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
				if(((SDL_GetTicks() - fps_time) >= 1000) && (config::sdl_render)) 
				{ 
					fps_time = SDL_GetTicks();
					config::title.str("");
					config::title << "GBE+ " << fps_count << "FPS";
					SDL_SetWindowTitle(window, config::title.str().c_str());
					fps_count = 0; 
				}

				//Process gyroscope
				if(mem->cart.mbc_type == GB_MMU::MBC7) { mem->g_pad->process_gyroscope(); }

				//Process Gameshark cheats
				if(config::use_cheats) { mem->set_gs_cheats(); }

				//Process Constant IR Light - Interactive Mode
				if((config::ir_device == 5) && (config::ir_db_index == 1))
				{
					if(mem->g_pad->ir_delay)
					{
						mem->memory_map[REG_RP] &= ~0x2;
						mem->g_pad->ir_delay--;
					}

					else { mem->memory_map[REG_RP] |= 0x2; }
				}	
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
					}

					//After Line 153 reset LCD clock, scanline count
					else if(lcd_stat.current_scanline == 1) 
					{
						lcd_stat.lcd_clock -= 70224;
						lcd_stat.current_scanline = 0;
						mem->memory_map[REG_LY] = lcd_stat.current_scanline;

						new_rendered_screen_data();
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

void DMG_LCD::update_all_bg_hash()
{
	for (int x = 0; x < 384; x++)
	{
		if (cgfx_stat.bg_update_list[x])
		{
			update_bg_hash(x);
			cgfx_stat.bg_update_list[x] = false;
		}
	}
}

void GBC_LCD::update_all_bg_hash()
{
	//Check for updates based on tile data
	for (int tile_number = 0; tile_number < 256; tile_number++)
	{
		//Cycle through all tile numbers that were updated
		if (cgfx_stat.bg_tile_update_list[tile_number])
		{
			//Scan BG map for all tiles that use this tile number
			for (int x = 0; x < 2048; x++)
			{
				if (((GBC_MMU*)mem)->video_ram[0][0x1800 + x] == tile_number) { update_bg_hash(0x9800 + x); }
			}

			cgfx_stat.bg_tile_update_list[tile_number] = false;
		}
	}
}

void GB_LCD::collect_scanline_data()
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


void DMG_LCD::collect_bg_scanline() {
	if (!lcd_stat.bg_enable) return;

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

u16 GB_LCD::getUsedTileIdx(u16 tile_head) {
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


void GBC_LCD::collect_bg_scanline() {
}

void DMG_LCD::collect_win_scanline() {
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


void GBC_LCD::collect_win_scanline() {
}

void DMG_LCD::collect_scanline_tiles(u16 map_addr, u16 tile_lower_range, u16 tile_upper_range, u8 tile_line, std::vector <tile_strip>* strips) {
	//Generate background pixel data for selected tiles
	for (int x = tile_lower_range; x < tile_upper_range; x++)
	{
		u8 map_entry = mem->read_u8(map_addr + x);
		u8 tile_pixel = 0;

		//Convert tile number to signed if necessary
		if (lcd_stat.bg_tile_addr == 0x8800) { map_entry = lcd_stat.signed_tile_lut[map_entry]; }

		//Calculate the address of the 8x1 pixel data based on map entry
		u16 tile_head = lcd_stat.bg_tile_addr + (map_entry << 4);
		if (lcd_stat.scanline_pixel_counter < 160 || lcd_stat.scanline_pixel_counter >= 250)
		{
			//add to screen data
			tile_strip t;
			t.line = tile_line;
			t.palette_id = bgId;
			t.pattern_id = getUsedTileIdx(tile_head);
			t.pattern_data = cgfx_stat.screen_data.rendered_tile[t.pattern_id].tile.line[tile_line];
			t.x = lcd_stat.scanline_pixel_counter;
			strips->push_back(t);
		}
		lcd_stat.scanline_pixel_counter += 8;
	}
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
			if (obj[x].v_flip) {
				t.line = lcd_stat.obj_size - 1 - t.line;
			}
			t.palette_id = (obj[x].palette_number == 1 ? objId2 : objId1);
			t.pattern_id = getUsedTileIdx(0x8000 + ((obj[x].tile_number + (t.line > 7 ? 1 : 0)) << 4));
			t.pattern_data = cgfx_stat.screen_data.rendered_tile[t.pattern_id].tile.line[t.line - (t.line > 7 ? 8 : 0)];
			t.priority = (256 - obj[x].x) * 40 - x;

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

void GBC_LCD::collect_palette() {
}

void GBC_LCD::collect_obj_scanline() {
}

void GB_LCD::render_full_screen() {
}

void DMG_LCD::render_scanline() {
}

void GBC_LCD::render_scanline() {
}

