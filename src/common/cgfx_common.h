// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : cgfx.h
// Date : August 12, 2015
// Description : GBE+ Global Custom Graphics
//
// Emulator-wide custom graphics assets

#ifndef GBE_CGFX
#define GBE_CGFX

#include <string>

#include "common.h"

std::string get_game_cgfx_folder();
std::string get_manifest_file();

namespace cgfx
{ 
	extern bool load_cgfx;
	extern bool loaded;

	extern u8 scaling_factor;

	extern std::string cgfx_path;
	extern std::string meta_dump_name;
}

#endif // GBE_CGFX 
