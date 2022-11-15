// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : config.h
// Date : September 20, 2014
// Description : GBE+ configuration options
//
// Parses command-line arguments to configure GBE options

#ifndef EMU_CONFIG
#define EMU_CONFIG


#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <sstream>

#include "common.h"

void reset_dmg_colors();
void set_dmg_colors(u8 color_type);
u8 get_system_type_from_file(std::string filename);

void set_rom_file(std::string filename);
std::string get_rom_file();
std::string get_rom_name();
std::string get_rom_save();
std::string get_rom_state();


bool parse_ini_file();
bool parse_cheats_file(bool add_cheats);
bool save_ini_file();
bool save_cheats_file();

bool load_osd_font();
void draw_osd_msg(std::string osd_text, u32* osd_surface, u8 x_offset, u8 y_offset);

namespace config
{ 
	extern std::string save_path;
	extern std::string ss_path;
	extern std::string cfg_path;
	extern std::string data_path;
	extern std::string cheats_path;
	extern std::vector <std::string> recent_files;

	extern u32 gbe_key_a, gbe_key_b, gbe_key_start, gbe_key_select, gbe_key_up, gbe_key_down, gbe_key_left, gbe_key_right;
	extern u32 gbe_joy_a, gbe_joy_b, gbe_joy_start, gbe_joy_select, gbe_joy_up, gbe_joy_down, gbe_joy_left, gbe_joy_right;

	extern u32 hotkey_turbo;
	extern u32 hotkey_mute;
	extern int dead_zone;
	extern int joy_id;
	extern int joy_sdl_id;
	extern bool use_haptics;

	extern u32 flags;
	extern bool pause_emu;
	extern bool ignore_illegal_opcodes;

	extern bool use_opengl;
	extern bool turbo;
	extern u8 scaling_factor;
	extern u8 old_scaling_factor;
	extern std::stringstream title;
	extern u8 gb_type;
	extern u8 dmg_gbc_pal;
	extern u32 utp_steps;

	extern bool use_cheats;
	extern std::vector <u32> gs_cheats;
	extern std::vector <std::string> gg_cheats;
	extern std::vector <std::string> cheats_info;
	extern bool use_patches;

	extern u8 volume;
	extern u8 old_volume;
	extern u32 sample_size;
	extern double sample_rate;
	extern bool mute;
	extern bool use_stereo;
	extern bool use_microphone;
	
	extern u32 sys_width;
	extern u32 sys_height;
	extern s32 win_width;
	extern s32 win_height;

	extern std::string vertex_shader;
	extern std::string fragment_shader;

	extern bool maintain_aspect_ratio;
	extern u8 lcd_config;
	extern u16 max_fps;

	extern u32 DMG_BG_PAL[4];
	extern u32 DMG_OBJ_PAL[4][2];

	extern u16 rtc_offset[6];
	extern u32 oc_flags;

	extern u8 turbo_file_options;

	extern bool auto_gen_am3_id;

	extern bool use_osd;
	extern std::vector <u32> osd_font;
	extern std::string osd_message;
	extern u32 osd_count;

	extern bool use_external_interfaces;

	//Function pointer for external software rendering
	//This function is provided by frontends that will not rely on SDL
	extern void (*render_external_sw)(u32*);

	//Function pointer for external rendering
	//This function is provided by frontends that will not rely on SDL+OGL
	extern void (*render_external_hw)(SDL_Surface*);

}

#endif // EMU_CONFIG
