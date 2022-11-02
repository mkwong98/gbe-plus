// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : cgfx_common.h
// Date : August 12, 2015
// Description : GBE+ Global Custom Graphics
//
// Emulator-wide custom graphics assets

#include "cgfx_common.h"
#include "config.h"

namespace cgfx
{ 
	u8 gbc_bg_color_pal = 0;
	u8 gbc_bg_vram_bank = 0;
	u8 gbc_obj_color_pal = 0;
	u8 gbc_obj_vram_bank = 0;

	bool load_cgfx = false;
	bool loaded = false;
	bool ignore_blank_dumps = false;
	bool auto_obj_trans = false;

	u8 scaling_factor = 1;
	u32 transparency_color = 0x00000000;

	std::string cgfx_path = "HdPacks/";
	std::string dump_bg_path = "";
	std::string dump_obj_path = "";
	std::string dump_name = "";
	std::string meta_dump_name = "META";

	std::string last_hash = "";
	u32 last_vram_addr = 0;
	u8 last_type = 0;
	u8 last_palette = 0;
	bool last_added = false;
	bool last_saved = false;
	bool ignore_existing_hash = false;
}

std::string get_game_cgfx_folder()
{
	return cgfx::cgfx_path + get_rom_name() + "/";
}

std::string get_manifest_file() 
{
	return get_game_cgfx_folder() + "hires.txt";
}
