// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : util.h
// Date : August 06, 2015
// Description : Misc. utilites
//
// Provides miscellaneous utilities for the emulator

#ifndef GBE_UTIL
#define GBE_UTIL

#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "common.h"

namespace util
{
	u32 reflect(u32 src, u8 bit);
	void init_crc32_table();
	u32 get_crc32(u8* data, u32 length);

	u32 get_addler32(u8* data, u32 length);

	u32 switch_endian32(u32 input);

	std::string to_hex_strXX(u16 input);
	std::string to_hex_strXXXX(u16 input);
	std::string to_hex_str(u32 input);
	std::string to_hex_str(u32 input, u8 bit_level);
	bool from_hex_str(std::string input, u32 &result);

	std::string to_str(u32);
	std::string to_sstr(s32);
	bool from_str(std::string input, u32 &result);

	std::string ip_to_str(u32 ip_addr);
	bool ip_to_u32(std::string ip_addr, u32 &result);

	std::string data_to_str(u8* data, u32 length);
	void str_to_data(u8* data, std::string input);

	std::string make_ascii_printable(std::string input);

	std::string get_filename_from_path(std::string path);

	u32 get_bcd(u32 input);
	u32 get_bcd_int(u32 input);

	u32 bswap(u32 input);

	SDL_Surface* load_icon(std::string filename);

	extern u32 crc32_table[256];
	extern u32 poly32;

	std::string trimfnc(std::string str);
	std::string timeStr();
}

#endif // GBE_UTIL 
