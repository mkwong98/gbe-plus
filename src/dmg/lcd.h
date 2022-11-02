// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.h
// Date : May 15, 2014
// Description : Game Boy LCD emulation
//
// Draws background, window, and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#ifndef GB_DISPLAY
#define GB_DISPLAY

#include "SDL2/SDL.h"
#include "SDL2/SDL_opengl.h"
#include "mmu.h"
#include "custom_graphics_data.h"


class GB_LCD
{
	public:
	
	//Link to memory map
	GB_MMU* mem;

	//Core Functions
	GB_LCD();
	~GB_LCD();

	void step(int cpu_clock);
	virtual void step_sub(int cpu_clock);

	void reset();
	bool opengl_init();

	//Custom GFX functions
	bool load_manifest(std::string filename);
	void clear_manifest();
	SDL_Surface* load_pack_image(std::string filename);

	bool load_image_data();
	bool load_meta_data();
	bool find_meta_data();

	virtual void dump_obj(u8 obj_index) = 0;
	virtual void dump_bg(u16 obj_index) = 0;

	virtual void update_obj_hash(u8 obj_index) = 0;
	virtual void update_all_bg_hash() = 0;
	virtual void update_bg_hash(u16 bg_index) = 0;

	bool has_hash(u16 addr, std::string hash);
	virtual std::string get_hash(u16 addr, u8 gfx_type) = 0;
	u32 adjust_pixel_brightness(u32 color, u8 palette_id, u8 gfx_type);
	void invalidate_cgfx();

	void new_rendered_screen_data();
	void new_rendered_scanline_data(u8 lineNo);

	SDL_Surface* render_raw_layer(u8 layer);
	//Serialize data for save state loading/saving
	bool lcd_read(u32 offset, std::string filename);
	bool lcd_write(std::string filename);

	//Screen data
	SDL_Window *window;
	SDL_Surface* finalscreen;

	//OpenGL data
	#ifdef GBE_OGL
	
	SDL_GLContext gl_context;
	GLuint lcd_texture;
	GLuint program_id;
	GLuint vertex_buffer_object, vertex_array_object, element_buffer_object;
	GLfloat ogl_x_scale, ogl_y_scale;
	GLfloat ext_data_1, ext_data_2;
	u32 external_data_usage;
	
	#endif	

	dmg_lcd_data lcd_stat;
	dmg_cgfx_data cgfx_stat;

	int max_fullscreen_ratio;

	protected:

	struct oam_entries
	{
		//X-Y Coordinates
		u8 x;
		u8 y;
	
		//Horizonal and vertical flipping options
		bool h_flip;
		bool v_flip;

		//Dimensions
		u8 height;

		//Misc properties
		u8 tile_number;
		u8 bg_priority;
		u8 bit_depth;
		u8 palette_number;
		u8 vram_bank;
		u8 color_palette_number;
		
	} obj[40];

	u8 obj_render_list[10];
	int obj_render_length;

	//Screen pixel buffer
	//layers: white or low bg, low obj, bg, obj, high bg
	SDL_Surface* buffers[5];
	void clear_buffers();
	void reset_buffers();
	SDL_Rect srcrect;
	SDL_Rect rawrect;
	SDL_Surface* tempscreen;

	int frame_start_time;
	int frame_current_time;
	int fps_count;
	int fps_time;
	int frame_delay[60];

	//OAM updates
	void update_oam();
	virtual void update_obj_render_list() = 0;

	//Per-scanline rendering
	virtual void collect_scanline_data() = 0;
	virtual void collect_palette() = 0;
	void collect_bg_scanline();
	void collect_win_scanline();
	virtual void collect_scanline_tiles(u16 map_addr, u16 tile_lower_range, u16 tile_upper_range, u8 tile_line, std::vector <tile_strip>* strips) = 0;
	virtual void collect_obj_scanline() = 0;
	void render_full_screen();

	void run_render_scanline();
	virtual void render_bg_scanline(bool raw) = 0;
	virtual void render_win_scanline(bool raw) = 0;
	virtual void render_obj_scanline(bool raw) = 0;

	void scanline_compare();

};

class DMG_LCD : public GB_LCD
{
public:
	void dump_obj(u8 obj_index);
	void dump_bg(u16 obj_index);
	void update_obj_hash(u8 obj_index);
	void update_all_bg_hash();
	void update_bg_hash(u16 bg_index);
	std::string get_hash(u16 addr, u8 gfx_type);

protected:
	void update_obj_render_list();

	void collect_scanline_data();
	u16 getUsedPaletteIdx(u8 p);
	void collect_palette();
	void collect_scanline_tiles(u16 map_addr, u16 tile_lower_range, u16 tile_upper_range, u8 tile_line, std::vector <tile_strip>* strips);
	void collect_obj_scanline();
	u16 getUsedTileIdx(u16 tile_head);

	void render_bg_scanline(bool raw);
	void render_win_scanline(bool raw);
	void render_obj_scanline(bool raw);

private:
	u16 bgId;
	u16 objId1;
	u16 objId2;
};

class GBC_LCD : public GB_LCD
{
public:
	void step_sub(int cpu_clock);
	void dump_obj(u8 obj_index);
	void dump_bg(u16 obj_index);
	void update_obj_hash(u8 obj_index);
	void update_all_bg_hash();
	void update_bg_hash(u16 bg_index);
	std::string get_hash(u16 addr, u8 gfx_type);

protected:
	void update_obj_render_list();

	//GBC color palette updates
	void update_bg_colors();
	void update_obj_colors();

	void collect_scanline_data();
	void collect_palette();
	u16 getUsedPaletteIdx(u16* pal, u32* dcolor);
	void collect_scanline_tiles(u16 map_addr, u16 tile_lower_range, u16 tile_upper_range, u8 tile_line, std::vector <tile_strip>* strips);
	void collect_obj_scanline();
	u16 getUsedTileIdx(u16 tile_head, u8 vbank);

	void render_bg_scanline(bool raw);
	void render_win_scanline(bool raw);
	void render_obj_scanline(bool raw);

private:
	u16 bgId[8];
	u16 objId[8];
	bool scanlinePaletteUpdated;
};

#endif // GB_DISPLAY 
