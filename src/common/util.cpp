// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : util.h
// Date : August 06, 2015
// Description : Misc. utilites
//
// Provides miscellaneous utilities for the emulator

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <iomanip>

#include "util.h"

namespace util
{

//CRC32 Polynomial
u32 poly32 = 0x04C11DB7;

//CRC lookup table
u32 crc32_table[256];

/****** Mirrors bits ******/
u32 reflect(u32 src, u8 bit)
{
	//2nd parameter 'bit' defines which to stop mirroring, e.g. generally Bit 7, Bit 15, or Bit 31 (count from zero)

	u32 out = 0;

	for(int x = 0; x <= bit; x++)
	{
		if(src & 0x1) { out |= (1 << (bit - x)); }
		src >>= 1;
	}

	return out;
}

/****** Sets up the CRC lookup table ******/
void init_crc32_table()
{
	for(int x = 0; x < 256; x++)
	{
		crc32_table[x] = (reflect(x, 7) << 24);

		for(int y = 0; y < 8; y++)
		{
			crc32_table[x] = (crc32_table[x] << 1) ^ (crc32_table[x] & (1 << 31) ? poly32 : 0);
		}

		crc32_table[x] = reflect(crc32_table[x], 31);
	}
}

/****** Return CRC32 for given data ******/
u32 get_crc32(u8* data, u32 length)
{
	init_crc32_table();

	u32 crc32 = 0xFFFFFFFF;

	for(int x = 0; x < length; x++)
	{
		crc32 = (crc32 >> 8) ^ crc32_table[(crc32 & 0xFF) ^ (*data)];
		data++;
	}

	return (crc32 ^ 0xFFFFFFFF);
}

/****** Return Addler32 for given data ******/
u32 get_addler32(u8* data, u32 length)
{
	u16 a = 1;
	u16 b = 0;

	for(int x = 0; x < length; x++)
	{
		a += (*data);
		b += a;
	}

	a = a % 65521;
	b = b % 65521;

	u32 result = (b *= 65536) + a;
	return result;
}

/****** Switches endianness of a 32-bit integer ******/
u32 switch_endian32(u32 input)
{	
	u8 byte_1 = ((input >> 24) & 0xFF);
	u8 byte_2 = ((input >> 16) & 0xFF);
	u8 byte_3 = ((input >> 8) & 0xFF);
	u8 byte_4 = (input & 0xFF);

	return ((byte_4 << 24) | (byte_3 << 16) | (byte_2 << 8) | byte_1);
}

/****** Convert a number into 2 digit hex as a C++ string ******/
std::string to_hex_strXX(u16 input)
{
	std::stringstream temp;

	//Auto fill with '0's
	if (input < 0x10) { temp << "0" << std::hex << std::uppercase << input; }
	else { temp << std::hex << std::uppercase << input; }
	return temp.str();
}

/****** Convert a number into 4 digit hex as a C++ string ******/
std::string to_hex_strXXXX(u16 input)
{
	std::stringstream temp;

	//Auto fill with '0's
	if (input < 0x10) { temp << "000" << std::hex << std::uppercase << input; }
	else if (input < 0x100) { temp << "00" << std::hex << std::uppercase << input; }
	else if (input < 0x1000) { temp << "0" << std::hex << std::uppercase << input; }
	else { temp << std::hex << std::uppercase << input; }

	return temp.str();
}

/****** Convert a number into hex as a C++ string ******/
std::string to_hex_str(u32 input)
{
	std::stringstream temp;

	//Auto fill with '0's
	if(input < 0x10) { temp << "0x0" << std::hex << std::uppercase << input; }
	else if((input < 0x1000) && (input >= 0x100)) { temp << "0x0" << std::hex << std::uppercase << input; }
	else if((input < 0x100000) && (input >= 0x10000)) { temp << "0x0" << std::hex << std::uppercase << input; }
	else if((input < 0x10000000) && (input >= 0x1000000)) { temp << "0x0" << std::hex << std::uppercase << input; }
	else { temp << "0x" << std::hex << std::uppercase << input; }

	return temp.str();
}

/****** Convert a number into hex as C++ string - Full 8, 16, 24, or 32-bit representations ******/
std::string to_hex_str(u32 input, u8 bit_level)
{
	std::stringstream temp;
	std::string result = "";
	u32 num = (input & 0xFF);

	//Limit to 32-bit max
	if(bit_level > 4) { bit_level = 4; }

	for(u32 x = 0; x < bit_level; x++)
	{
		temp << std::hex << std::uppercase << num;
		result += temp.str();
		if(num < 0x10) { result = "0" + result; }
		num = ((input >> ((x+1) * 8)) & 0xFF);
		temp.str(std::string());
	}

	result = "0x" + result;
	return result;
}

/****** Converts C++ string representing a hex number into an integer value ******/
bool from_hex_str(std::string input, u32 &result)
{
	//This function expects the hex string to contain only hexadecimal numbers and letters
	//E.g. it expects "8000" rather than "0x8000" or "$8000"
	//Returns false + result = 0 if it encounters any unexpected characters

	result = 0;
	u32 hex_size = (input.size() - 1);
	std::string hex_char = "";

	//Convert hex string into usable u32
	for(int x = hex_size, y = 0; x >= 0; x--, y+=4)
	{
		hex_char = input[x];

		if(hex_char == "0") { result += 0; }
		else if(hex_char == "1") { result += (1 << y); }
		else if(hex_char == "2") { result += (2 << y); }
		else if(hex_char == "3") { result += (3 << y); }
		else if(hex_char == "4") { result += (4 << y); }
		else if(hex_char == "5") { result += (5 << y); }
		else if(hex_char == "6") { result += (6 << y); }
		else if(hex_char == "7") { result += (7 << y); }
		else if(hex_char == "8") { result += (8 << y); }
		else if(hex_char == "9") { result += (9 << y); }
		else if(hex_char == "A") { result += (10 << y); }
		else if(hex_char == "a") { result += (10 << y); }
		else if(hex_char == "B") { result += (11 << y); }
		else if(hex_char == "b") { result += (11 << y); }
		else if(hex_char == "C") { result += (12 << y); }
		else if(hex_char == "c") { result += (12 << y); }
		else if(hex_char == "D") { result += (13 << y); }
		else if(hex_char == "d") { result += (13 << y); }
		else if(hex_char == "E") { result += (14 << y); }
		else if(hex_char == "e") { result += (14 << y); }
		else if(hex_char == "F") { result += (15 << y); }
		else if(hex_char == "f") { result += (15 << y); }
		else { result = 0; return false; }
	}

	return true;
}

/****** Convert an unsigned integer into a C++ string ******/
std::string to_str(u32 input)
{
	std::stringstream temp;
	temp << input;
	return temp.str();
}

/****** Convert a signed integer into a C++ string ******/
std::string to_sstr(s32 input)
{
	std::stringstream temp;
	temp << input;
	return temp.str();
}

/****** Converts a string into an integer value ******/
bool from_str(std::string input, u32 &result)
{
	result = 0;
	u32 size = (input.size() - 1);
	std::string value_char = "";

	if(input.size() == 0) { return false; }

	//Convert string into usable u32
	for(int x = size, y = 1; x >= 0; x--, y *= 10)
	{
		value_char = input[x];

		if(value_char == "0") { result += 0; }
		else if(value_char == "1") { result += (1 * y); }
		else if(value_char == "2") { result += (2 * y); }
		else if(value_char == "3") { result += (3 * y); }
		else if(value_char == "4") { result += (4 * y); }
		else if(value_char == "5") { result += (5 * y); }
		else if(value_char == "6") { result += (6 * y); }
		else if(value_char == "7") { result += (7 * y); }
		else if(value_char == "8") { result += (8 * y); }
		else if(value_char == "9") { result += (9 * y); }
		else if(value_char == "-") { result *= -1; }
		else { result = 0; return false; }
	}

	return true;
}

/****** Converts a series of bytes to ASCII ******/
std::string data_to_str(u8* data, u32 length)
{
	std::string temp = "";

	for(u32 x = 0; x < length; x++)
	{
		char ascii = *data;
		temp += ascii;
		data++;
	}

	return temp;
}

/****** Converts an ASCII string to a series of bytes ******/
void str_to_data(u8* data, std::string input)
{
	for(u32 x = 0; x < input.size(); x++)
	{
		char ascii = input[x];
		*data = ascii;
		data++;
	}
}

/****** Swaps unprintable characters in an ASCII string for spaces ******/
std::string make_ascii_printable(std::string input)
{
	std::string result = "";

	for(u32 x = 0; x < input.size(); x++)
	{
		char ascii = input[x];

		if((ascii >= 0x20) && (ascii <= 0x7E)) { result += ascii; }
		else { result += " "; }
	}

	return result;
}

/****** Converts a string IP address to an integer value ******/
bool ip_to_u32(std::string ip_addr, u32 &result)
{
	u8 digits[4] = { 0, 0, 0, 0 };
	u8 dot_count = 0;
	std::string temp = "";
	std::string current_char = "";
	u32 str_end = ip_addr.length() - 1;

	//IP address comes in the form 123.456.678.901
	//Grab each character between the dots and convert them into integer
	for(u32 x = 0; x < ip_addr.length(); x++)
	{
		current_char = ip_addr[x];

		//Check to make sure the character is valid for an IP address
		bool check_char = false;

		if(current_char == "0") { check_char = true; }
		else if(current_char == "1") { check_char = true; }
		else if(current_char == "2") { check_char = true; }
		else if(current_char == "3") { check_char = true; }
		else if(current_char == "4") { check_char = true; }
		else if(current_char == "5") { check_char = true; }
		else if(current_char == "6") { check_char = true; }
		else if(current_char == "7") { check_char = true; }
		else if(current_char == "8") { check_char = true; }
		else if(current_char == "9") { check_char = true; }
		else if(current_char == ".") { check_char = true; }

		//Quit now if invalid character
		if(!check_char)
		{
			result = 0;
			return false;
		}

		//Convert characters into u32 when a "." is encountered
		if((current_char == ".") || (x == str_end))
		{
			if(x == str_end) { temp += current_char; }

			//If somehow parsing more than 3 dots, string is malformed
			if(dot_count == 4)
			{
				result = 0;
				return false;
			}

			u32 digit = 0;

			//If the string can't be converted into a digit, quit now
			if(!from_str(temp, digit))
			{
				result = 0;
				return false;
			}

			//If the string is longer than 3 characters or zero, quit now
			if((temp.size() > 3) || (temp.size() == 0))
			{
				result = 0;
				return false;
			}


			digits[dot_count++] = digit & 0xFF;
			temp = "";
		}

		else { temp += current_char; }
	}

	//If the dot count is less than three, something is wrong with the IP address
	if(dot_count != 4)
	{
		result = 0;
		return false;
	}

	//Encode result in network byte order aka big endian
	result = (digits[3] << 24) | (digits[2] << 16) | (digits[1] << 8) | digits[0];

	return true;
}

/****** Converts an integers IP address to a string value ******/
std::string ip_to_str(u32 ip_addr)
{
	u32 mask = 0x000000FF;
	u32 shift = 0;
	std::string temp = "";

	for(u32 x = 0; x < 4; x++)
	{
		u32 digit = (ip_addr & mask) >> shift;
		temp += to_str(digit);
		
		if(x != 3) { temp += "."; }

		shift += 8;
		mask <<= 8;
	}

	return temp;
}

/****** Gets the filename from a full or partial path ******/
std::string get_filename_from_path(std::string path)
{
	std::size_t match = path.find_last_of("/\\");

	if(match != std::string::npos) { return path.substr(match + 1); }
	else { return path; }
}

/****** Loads icon into SDL Surface ******/
SDL_Surface* load_icon(std::string filename)
{
	SDL_Surface* source = SDL_LoadBMP(filename.c_str());

	if(source == NULL)
	{
		std::cout<<"GBE::Error - Could not load icon file " << filename << ". Check file path or permissions. \n";
		return NULL;
	}

	SDL_Surface* output = SDL_CreateRGBSurface(SDL_SWSURFACE, source->w, source->h, 32, 0, 0, 0, 0);

	//Cycle through all pixels, then set the alpha of all green pixels to zero
	u8* in_pixel_data = (u8*)source->pixels;
	u32* out_pixel_data = (u32*)output->pixels;

	for(int a = 0, b = 0; a < (source->w * source->h); a++, b+=3)
	{
		out_pixel_data[a] = (0xFF000000 | (in_pixel_data[b+2] << 16) | (in_pixel_data[b+1] << 8) | (in_pixel_data[b]));

		SDL_SetColorKey(output, SDL_TRUE, 0xFF00FF00);
	}

	return output;
}

/****** Converts an integer into a BCD ******/
u32 get_bcd(u32 input)
{
	//Convert to a string
	std::string temp = to_str(input);

	//Convert string back into an int
	from_hex_str(temp, input);

	return input;
}

/****** Converts a BCD into an integer ******/
u32 get_bcd_int(u32 input)
{
	//Convert to a string
	std::string temp = to_hex_str(input);
	temp = temp.substr(2);

	//Convert string back into an int
	from_str(temp, input);

	return input;
}

/****** Byte swaps a 32-bit value ******/
u32 bswap(u32 input)
{
	u32 result = (input >> 24);
	result |= (((input >> 16) & 0xFF) << 8);
	result |= (((input >> 8) & 0xFF) << 16);
	result |= ((input & 0xFF) << 24);

	return result;
}

std::string trimfnc(std::string str) {
	const char* typeOfWhitespaces = " \t\n\r\f\v";
	str.erase(str.find_last_not_of(typeOfWhitespaces) + 1);
	str.erase(0, str.find_first_not_of(typeOfWhitespaces));
	return str;
}

std::string timeStr() {
	std::stringstream temp;

	const std::chrono::time_point<std::chrono::system_clock> now =
		std::chrono::system_clock::now();

	const std::time_t t_c = std::chrono::system_clock::to_time_t(now);

	temp << std::put_time(std::localtime(&t_c), "%Y%m%d%H%M%S") << std::flush;
	return temp.str();
}

} //Namespace
