// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mmu.h
// Date : May 09, 2015
// Description : Game Boy (Color) memory manager unit
//
// Handles reading and writing bytes to memory locations

#ifndef GB_MEM
#define GB_MEM

#include <fstream>
#include <string>
#include <vector>
#include <iostream>

#include "common.h"
#include "common/config.h"
#include "common/util.h"
#include "gamepad.h"
#include "lcd_data.h"
#include "custom_graphics_data.h"
#include "apu_data.h"
#include "midi_driver.h"
#include "custom_sound.h"

class GB_MMU
{
	public:

	//Memory bank controller-types enumeration
	enum mbc_types
	{ 
		ROM_ONLY, 
		MBC1, 
		MBC2, 
		MBC3, 
		MBC5,
		MBC6,
		MBC7,
		HUC1,
		HUC3,
		MMM01,
		GB_CAMERA,
		TAMA5,
	};

	std::vector <u8> memory_map;

	//Memory Banks
	std::vector< std::vector<u8> > read_only_bank;
	std::vector< std::vector<u8> > random_access_bank;

	//Flash memory - MBC6 only
	std::vector< std::vector<u8> > flash;

	//Bank controls
	u16 rom_bank;
	u8 ram_bank;
	u8 bank_bits;
	u8 bank_mode;
	bool ram_banking_enabled;

	//Cartridge data structure
	struct cart_data
	{
		//General MBC attributes
		u32 rom_size;
		u32 ram_size;
		mbc_types mbc_type;
		bool battery;
		bool ram;
		bool rumble;

		//MBC3 RTC
		bool rtc;
		bool rtc_enabled;
		bool rtc_latched;
		u8 rtc_latch_1, rtc_latch_2, rtc_reg[5];
		int rtc_last_time[9];
		u8 latch_reg[5];

		//MBC6
		u8 flash_cnt;
		u8 flash_cmd;
		u8 flash_stat;
		u8 flash_io_bank;
		bool flash_get_id;

		//MBC7
		bool idle;
		u8 internal_value;
		u8 internal_state;
		u8 cs;
		u8 sk;
		u8 buffer_length;
		u8 command_code;
		u16 addr;
		u16 buffer;

		//TAMA5
		u8 tama_reg[16];
		u8 tama_ram[256];
		u8 tama_cmd;
		u8 tama_out;


	} cart;

	u8 ir_signal;
	bool ir_send;
	u8 ir_trigger;
	s32 ir_counter;

	bool div_reset;

	dmg_core_pad* g_pad;

	std::vector<u32> sub_screen_buffer;
	u32 sub_screen_update;
	bool sub_screen_lock;

	GB_MMU();
	~GB_MMU();

	virtual void reset();
	void grab_time();

	virtual u8 read_u8(u16 address) = 0;
	u8 read_u8_sub(u16 address);
	u16 read_u16(u16 address);
	s8 read_s8(u16 address);

	virtual void write_u8(u16 address, u8 value);
	void write_u16(u16 address, u16 value);

	//GBC DMAs
	void hdma();
	void gdma();

	bool read_file(std::string filename);
	virtual void init_io_reg() = 0;
	bool save_backup(std::string filename);
	bool load_backup(std::string filename);

	bool patch_ips(std::string filename);
	bool patch_ups(std::string filename);

	//Memory Bank Controller dedicated read/write operations
	void mbc_write(u16 address, u8 value);
	u8 mbc_read(u16 address);

	void mbc1_write(u16 address, u8 value);
	u8 mbc1_read(u16 address);

	void mbc2_write(u16 address, u8 value);
	u8 mbc2_read(u16 address);

	void mbc3_write(u16 address, u8 value);
	u8 mbc3_read(u16 address);

	void mbc5_write(u16 address, u8 value);
	u8 mbc5_read(u16 address);

	void mbc6_write(u16 address, u8 value);
	u8 mbc6_read(u16 address);

	void mbc7_write(u16 address, u8 value);
	void mbc7_write_ram(u8 value);
	u8 mbc7_read(u16 address);

	void huc1_write(u16 address, u8 value);
	u8 huc1_read(u16 address);

	void huc3_write(u16 address, u8 value);
	u8 huc3_read(u16 address);

	void mmm01_write(u16 address, u8 value);
	u8 mmm01_read(u16 address);

	void tama5_write(u16 address, u8 value);
	u8 tama5_read(u16 address);
	void grab_tama5_time(u8 index);

	void set_gs_cheats();
	void set_gg_cheats();

	void set_lcd_data(dmg_lcd_data* ex_lcd_stat);
	void set_cgfx_data(dmg_cgfx_data* ex_cgfx_stat);
	void set_apu_data(dmg_apu_data* ex_apu_stat);

	//Serialize data for save state loading/saving
	bool mmu_read(u32 offset, std::string filename);
	virtual void mmu_read_content(std::ifstream* file);
	bool mmu_write(std::string filename);
	virtual void mmu_write_content(std::ofstream* file);
	virtual u32 size();

	protected:
	u8 previous_value;

	//Only the MMU and LCD should communicate through this structure
	dmg_lcd_data* lcd_stat;

	//Only the MMU and the LCD should communicate through this structure
	dmg_cgfx_data* cgfx_stat;

	//Only the MMU and APU should communicate through this structure
	dmg_apu_data* apu_stat;
};

class DMG_MMU : public GB_MMU
{
public:
	u8 read_u8(u16 address);
	void write_u8(u16 address, u8 value);
	void init_io_reg();
};

class GBC_MMU : public GB_MMU
{
public:
	//Working RAM Banks - GBC only
	std::vector< std::vector<u8> > video_ram;
	u8 vram_bank;

	std::vector< std::vector<u8> > working_ram_bank;
	u8 wram_bank;

	GBC_MMU();
	void reset();

	u8 read_u8(u16 address);
	void write_u8(u16 address, u8 value);
	void init_io_reg();

	void mmu_read_content(std::ifstream* file);
	void mmu_write_content(std::ofstream* file);
	u32 size();
};

#endif // GB_MEM
