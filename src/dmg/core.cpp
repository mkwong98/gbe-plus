// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : core.cpp
// Date : May 17, 2015
// Description : Emulator core
//
// The "Core" is an abstraction for all of emulated components
// It controls the large-scale behavior of the CPU, LCD/GPU, MMU, and APU/DSP
// Can start, stop, and reset emulator

#include <iomanip>
#include <ctime>
#include <sstream>

#include "common/util.h"
#include "common/cgfx_common.h"

#include "core.h"

/****** Core Constructor ******/
DMG_core::DMG_core()
{
	core_cpu = new DMG_Z80();
	core_mmu = new DMG_MMU();
	init_cpu();
	std::cout << "GBE::Launching DMG core\n";

	//OSD
	config::osd_message = "DMG CORE INIT";
	config::osd_count = 180;
}

GBC_core::GBC_core()
{
	core_cpu = new GBC_Z80();
	core_mmu = new GBC_MMU();
	init_cpu();
	std::cout << "GBE::Launching GBC core\n";

	//OSD
	config::osd_message = "GBC CORE INIT";
	config::osd_count = 180;
}

DMG_core::~DMG_core()
{
	delete core_cpu;
	delete core_mmu;
}

GBC_core::~GBC_core()
{
	delete core_cpu;
	delete core_mmu;
}

void GB_core::init_cpu()
{
	//Link CPU and MMU
	core_cpu->mem = core_mmu;

	//Link LCD and MMU
	core_cpu->controllers.video->mem = core_mmu;
	core_mmu->set_lcd_data(&core_cpu->controllers.video->lcd_stat);
	core_mmu->set_cgfx_data(&core_cpu->controllers.video->cgfx_stat);

	//Link APU and MMU
	core_cpu->controllers.audio.mem = core_mmu;
	core_mmu->set_apu_data(&core_cpu->controllers.audio.apu_stat);

	//Link MMU and GamePad
	core_cpu->mem->g_pad = &core_pad;
}



/****** Start the core ******/
void GB_core::start()
{
	running = true;
	core_cpu->running = true;

	//Initialize video output
	if(!core_cpu->controllers.video->init())
	{
		running = false;
		core_cpu->running = false;
	}

	//Initialize audio output
	if(!core_cpu->controllers.audio.init())
	{
		running = false;
		core_cpu->running = false;
	}

	//Initialize the GamePad
	core_pad.init();
}

/****** Stop the core ******/
void GB_core::stop()
{
	running = false;
	core_cpu->running = false;
}

/****** Shutdown core's components ******/
void GB_core::shutdown()
{
	core_mmu->GB_MMU::~GB_MMU();
	core_cpu->Z80::~Z80();
}

/****** Reset the core ******/
void GB_core::reset()
{
	bool can_reset = true;

	core_cpu->reset();
	core_cpu->controllers.video->reset();
	core_cpu->controllers.audio.reset();
	core_mmu->reset();

	//Link CPU and MMU
	core_cpu->mem = core_mmu;

	//Link LCD and MMU
	core_cpu->controllers.video->mem = core_mmu;

	//Link APU and MMU
	core_cpu->controllers.audio.mem = core_mmu;

	//Link MMU and GamePad
	core_cpu->mem->g_pad = &core_pad;

	//Re-read specified ROM file
	if(!core_mmu->read_file(get_rom_file())) { can_reset = false; }

	//Re-read BIOS file
	if((config::use_bios) && (!read_bios(config::bios_file))) { can_reset = false; }

	//Start everything all over again
	if(can_reset) { start(); }
	else { running = false; }
}

/****** Loads a save state ******/
void GB_core::load_state(u8 slot)
{
	std::string id = (slot > 0) ? util::to_str(slot) : "";

	std::string state_file = get_rom_state();
	state_file += id;

	u32 offset = 0;

	//Check if save state is accessible
	std::ifstream test(state_file.c_str());
	
	if(!test.good())
	{
		config::osd_message = "INVALID SAVE STATE " + util::to_str(slot);
		config::osd_count = 180;
		return;
	}

	//Offset 0, size 43
	if(!core_cpu->cpu_read(offset, state_file)) { return; }
	offset += core_cpu->size();	

	//Offset 43, size 213047
	if(!core_mmu->mmu_read(offset, state_file)) { return; }
	offset += core_mmu->size();

	//Offset 213090, size 320
	if(!core_cpu->controllers.audio.apu_read(offset, state_file)) { return; }
	offset += core_cpu->controllers.audio.size();

	//Offset 213410
	if(!core_cpu->controllers.video->lcd_read(offset, state_file)) { return; }

	std::cout<<"GBE::Loaded state " << state_file << "\n";

	//Invalidate current CGFX
	if(cgfx::load_cgfx) { core_cpu->controllers.video->invalidate_cgfx(); }

	//OSD
	config::osd_message = "LOADED STATE " + util::to_str(slot);
	config::osd_count = 180;
}

/****** Saves a save state ******/
void GB_core::save_state(u8 slot)
{
	std::string id = (slot > 0) ? util::to_str(slot) : "";

	std::string state_file = get_rom_state();
	state_file += id;

	if(!core_cpu->cpu_write(state_file)) { return; }
	if(!core_mmu->mmu_write(state_file)) { return; }
	if(!core_cpu->controllers.audio.apu_write(state_file)) { return; }
	if(!core_cpu->controllers.video->lcd_write(state_file)) { return; }

	std::cout<<"GBE::Saved state " << state_file << "\n";

	//OSD
	config::osd_message = "SAVED STATE " + util::to_str(slot);
	config::osd_count = 180;
}

/****** Run the core in a loop until exit ******/
void GB_core::run_core()
{
	//Begin running the core
	while(running)
	{
		//Handle SDL Events
		if(core_cpu->controllers.video->lcd_stat.current_scanline == 144)
		{
			if(SDL_PollEvent(&event))
			{
				//X out of a window
				if(event.type == SDL_QUIT) { stop(); SDL_Quit(); }

				//Process gamepad or hotkey
				else if((event.type == SDL_KEYDOWN) || (event.type == SDL_KEYUP) 
				|| (event.type == SDL_JOYBUTTONDOWN) || (event.type == SDL_JOYBUTTONUP)
				|| (event.type == SDL_JOYAXISMOTION) || (event.type == SDL_JOYHATMOTION))
				{
					core_pad.handle_input(event);
					handle_hotkey(event);

					//Trigger Joypad Interrupt if necessary
					if(core_pad.joypad_irq) { core_mmu->memory_map[IF_FLAG] |= 0x10; }
				}

				//Hotplug joypad
				else if((event.type == SDL_JOYDEVICEADDED) && (!core_pad.joy_init)) { core_pad.init(); }
			}
			
			//Perform reset for GB Memory Cartridge
			if((config::cart_type == DMG_GBMEM) && (core_mmu->cart.flash_stat == 0xF0)) { reset(); }
		}

		//Run the CPU
		if(core_cpu->running)
		{
			core_cpu->cycles = 0;

			//Handle Interrupts
			core_cpu->handle_interrupts();

			//Halt CPU if necessary
			if(core_cpu->halt == true)
			{
				//Normal HALT mode
				if(core_cpu->interrupt || !core_cpu->skip_instruction) { core_cpu->cycles += 4; }

				//HALT bug
				else if(core_cpu->skip_instruction)
				{
					//Exit HALT mode
					core_cpu->halt = false;
					core_cpu->skip_instruction = false;

					//Execute next opcode, but do not increment PC
					core_cpu->opcode = core_mmu->read_u8(core_cpu->reg.pc);
					core_cpu->exec_op(core_cpu->opcode);
				}
			}

			//Process Opcodes
			else 
			{
				core_cpu->opcode = core_mmu->read_u8(core_cpu->reg.pc++);
				core_cpu->exec_op(core_cpu->opcode);
			}

			//Update LCD
			core_cpu->controllers.video->step(core_cpu->get_cycles());

			//Update DIV timer - Every 4 M clocks
			core_cpu->div_counter += core_cpu->cycles;
		
			if(core_cpu->div_counter >= 256) 
			{
				core_cpu->div_counter -= 256;
				core_mmu->memory_map[REG_DIV]++;
			}

			//Update TIMA timer
			if(core_mmu->memory_map[REG_TAC] & 0x4) 
			{
				if(core_mmu->div_reset)
				{
					core_mmu->div_reset = false;
					core_cpu->tima_counter = 0;
				}

				core_cpu->tima_counter += core_cpu->cycles;

				switch(core_mmu->memory_map[REG_TAC] & 0x3)
				{
					case 0x00: core_cpu->tima_speed = 1024; break;
					case 0x01: core_cpu->tima_speed = 16; break;
					case 0x02: core_cpu->tima_speed = 64; break;
					case 0x03: core_cpu->tima_speed = 256; break;
				}
	
				if(core_cpu->tima_counter >= core_cpu->tima_speed)
				{
					core_mmu->memory_map[REG_TIMA]++;
					core_cpu->tima_counter -= core_cpu->tima_speed;

					if(core_mmu->memory_map[REG_TIMA] == 0)
					{
						core_mmu->memory_map[IF_FLAG] |= 0x04;
						core_mmu->memory_map[REG_TIMA] = core_mmu->memory_map[REG_TMA];
					}	

				}
			}

		}

		//Stop emulation
		else { stop(); }
	}
	
	//Shutdown core
	shutdown();
}

void GBC_core::run_core()
{
	core_cpu->reg.a = 0x11;
	GB_core::run_core();
}


/****** Manually run core for 1 instruction ******/
void GB_core::step()
{
	//Run the CPU
	if(core_cpu->running)
	{
		core_cpu->cycles = 0;

		//Handle Interrupts
		core_cpu->handle_interrupts();
	
		//Halt CPU if necessary
		if(core_cpu->halt == true)
		{
			//Normal HALT mode
			if(core_cpu->interrupt || !core_cpu->skip_instruction) { core_cpu->cycles += 4; }

			//HALT bug
			else if(core_cpu->skip_instruction)
			{
				//Exit HALT mode
				core_cpu->halt = false;
				core_cpu->skip_instruction = false;

				//Execute next opcode, but do not increment PC
				core_cpu->opcode = core_mmu->read_u8(core_cpu->reg.pc);
				core_cpu->exec_op(core_cpu->opcode);
			}
		}

		//Process Opcodes
		else 
		{
			core_cpu->opcode = core_mmu->read_u8(core_cpu->reg.pc++);
			core_cpu->exec_op(core_cpu->opcode);
		}

		//Update LCD
		core_cpu->controllers.video->step(core_cpu->get_cycles());

		//Update DIV timer - Every 4 M clocks
		core_cpu->div_counter += core_cpu->cycles;
		
		if(core_cpu->div_counter >= 256) 
		{
			core_cpu->div_counter -= 256;
			core_mmu->memory_map[REG_DIV]++;
		}

		//Update TIMA timer
		if(core_mmu->memory_map[REG_TAC] & 0x4) 
		{	
			core_cpu->tima_counter += core_cpu->cycles;

			switch(core_mmu->memory_map[REG_TAC] & 0x3)
			{
				case 0x00: core_cpu->tima_speed = 1024; break;
				case 0x01: core_cpu->tima_speed = 16; break;
				case 0x02: core_cpu->tima_speed = 64; break;
				case 0x03: core_cpu->tima_speed = 256; break;
			}
	
			if(core_cpu->tima_counter >= core_cpu->tima_speed)
			{
				core_mmu->memory_map[REG_TIMA]++;
				core_cpu->tima_counter -= core_cpu->tima_speed;

				if(core_mmu->memory_map[REG_TIMA] == 0)
				{
					core_mmu->memory_map[IF_FLAG] |= 0x04;
					core_mmu->memory_map[REG_TIMA] = core_mmu->memory_map[REG_TMA];
				}	

			}
		}

	}
}
	
/****** Process hotkey input ******/
void GB_core::handle_hotkey(SDL_Event& event)
{
	//Disallow key repeats
	if(event.key.repeat) { return; }

	//Quit on Q or ESC
	if((event.type == SDL_KEYDOWN) && ((event.key.keysym.sym == SDLK_q) || (event.key.keysym.sym == SDLK_ESCAPE)))
	{
		running = false; 
		SDL_Quit();
	}

	//Mute or unmute sound on M
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == config::hotkey_mute) && (!config::use_external_interfaces))
	{
		if(config::volume == 0)
		{
			update_volume(config::old_volume);
		}

		else
		{
			config::old_volume = config::volume;
			update_volume(0);
		}
	}

	//Quick save state on F1
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F1)) 
	{
		save_state(0);
	}

	//Quick load save state on F2
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F2)) 
	{
		load_state(0);
	}

	//Screenshot on F9
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F9)) 
	{
		std::stringstream save_stream;
		std::string save_name = config::ss_path;

		//Prefix SDL Ticks to screenshot name
		save_stream << SDL_GetTicks();
		save_name += save_stream.str();
		save_stream.str(std::string());

		//Append random number to screenshot name
		srand(SDL_GetTicks());
		save_stream << rand() % 1024 << rand() % 1024 << rand() % 1024;
		save_name += save_stream.str() + ".bmp";
	
		SDL_SaveBMP(core_cpu->controllers.video->original_screen, save_name.c_str());

		//OSD
		config::osd_message = "SAVED SCREENSHOT";
		config::osd_count = 180;
	}

	//Toggle Fullscreen on F12
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F12))
	{
		//Unset fullscreen
		if(config::flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
		{
			config::flags &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
			config::scaling_factor = config::old_scaling_factor;
		}

		//Set fullscreen
		else
		{
			config::flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
			config::old_scaling_factor = config::scaling_factor;
		}

		//Destroy old window
		SDL_DestroyWindow(core_cpu->controllers.video->window);

		//Initialize new window - SDL
		if(!config::use_opengl)
		{
			core_cpu->controllers.video->window = SDL_CreateWindow("GBE+", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, config::sys_width, config::sys_height, config::flags);
			core_cpu->controllers.video->final_screen = SDL_GetWindowSurface(core_cpu->controllers.video->window);
			SDL_GetWindowSize(core_cpu->controllers.video->window, &config::win_width, &config::win_height);

			//Find the maximum fullscreen dimensions that maintain the original aspect ratio
			if(config::flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
			{
				double max_width, max_height, ratio = 0.0;

				max_width = (double)config::win_width / config::sys_width;
				max_height = (double)config::win_height / config::sys_height;

				if(max_width <= max_height) { ratio = max_width; }
				else { ratio = max_height; }

				core_cpu->controllers.video->max_fullscreen_ratio = ratio;
			}
		}

		//Initialize new window - OpenGL
		else
		{
			core_cpu->controllers.video->opengl_init();
		}
	}

	//Pause emulation
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_PAUSE))
	{
		config::pause_emu = true;
		SDL_PauseAudio(1);
		std::cout<<"EMU::Paused\n";

		//Delay until pause key is hit again
		while(config::pause_emu)
		{
			SDL_Delay(50);
			if((SDL_PollEvent(&event)) && (event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_PAUSE))
			{
				config::pause_emu = false;
				SDL_PauseAudio(0);
				std::cout<<"EMU::Unpaused\n";
			}
		}
	}

	//Toggle turbo on
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == config::hotkey_turbo))
	{
		config::turbo = true;
		if((config::sdl_render) && (config::use_opengl)) { SDL_GL_SetSwapInterval(0); }
	}

	//Toggle turbo off
	else if((event.type == SDL_KEYUP) && (event.key.keysym.sym == config::hotkey_turbo))
	{
		config::turbo = false;
		if((config::sdl_render) && (config::use_opengl)) { SDL_GL_SetSwapInterval(1); }
	}
		
	//Reset emulation on F8
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F8))
	{
		//If running GB Memory Cartridge, make sure this is a true reset, i.e. boot to the menu program
		if(core_mmu->cart.flash_stat == 0x40)
		{
			core_mmu->cart.flash_stat = 0;
			core_mmu->cart.flash_io_bank = 0;
			config::gb_type = core_mmu->cart.flash_cnt;
		}

		reset();
	}

	//GB Camera load/unload external picture into VRAM
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == config::hotkey_camera))
	{
		//Set data into VRAM
		if((core_mmu->cart.mbc_type == GB_MMU::GB_CAMERA) && (!core_mmu->cart.cam_lock))
		{
			if(core_mmu->cam_load_snapshot(config::external_camera_file)) { std::cout<<"GBE::Loaded external camera file\n"; }
			core_mmu->cart.cam_lock = true;
		}

		//Clear data from VRAM
		else if((core_mmu->cart.mbc_type == GB_MMU::GB_CAMERA) && (core_mmu->cart.cam_lock))
		{
			//Clear VRAM - 0x9000 to 0x9800
			for(u32 x = 0x9000; x < 0x9800; x++) { core_mmu->write_u8(x, 0x0); }

			//Clear VRAM - 0x8800 to 0x8900
			for(u32 x = 0x8800; x < 0x8900; x++) { core_mmu->write_u8(x, 0x0); }

			//Clear VRAM - 0x8000 to 0x8500
			for(u32 x = 0x8000; x < 0x8500; x++) { core_mmu->write_u8(x, 0x0); }

			//Clear SRAM
			for(u32 x = 0; x < core_mmu->cart.cam_buffer.size(); x++) { core_mmu->random_access_bank[0][0x100 + x] = 0x0; }
			
			std::cout<<"GBE::Erased external camera file from VRAM\n";

			core_mmu->cart.cam_lock = false;
		}
	}


}

/****** Process hotkey input - Use exsternally when not using SDL ******/
void GB_core::handle_hotkey(int input, bool pressed)
{
	//Toggle turbo on
	if((input == config::hotkey_turbo) && (pressed)) { config::turbo = true; }

	//Toggle turbo off
	else if((input == config::hotkey_turbo) && (!pressed)) { config::turbo = false; }

	//GB Camera load/unload external picture into VRAM
	else if((input == config::hotkey_camera) && (pressed))
	{
		//Set data into VRAM
		if((core_mmu->cart.mbc_type == GB_MMU::GB_CAMERA) && (!core_mmu->cart.cam_lock))
		{
			if(core_mmu->cam_load_snapshot(config::external_camera_file)) { std::cout<<"GBE::Loaded external camera file\n"; }
			core_mmu->cart.cam_lock = true;
		}

		//Clear data from VRAM
		else if((core_mmu->cart.mbc_type == GB_MMU::GB_CAMERA) && (core_mmu->cart.cam_lock))
		{
			//Clear VRAM - 0x9000 to 0x9800
			for(u32 x = 0x9000; x < 0x9800; x++) { core_mmu->write_u8(x, 0x0); }

			//Clear VRAM - 0x8800 to 0x8900
			for(u32 x = 0x8800; x < 0x8900; x++) { core_mmu->write_u8(x, 0x0); }

			//Clear VRAM - 0x8000 to 0x8500
			for(u32 x = 0x8000; x < 0x8500; x++) { core_mmu->write_u8(x, 0x0); }

			//Clear SRAM
			for(u32 x = 0; x < core_mmu->cart.cam_buffer.size(); x++) { core_mmu->random_access_bank[0][0x100 + x] = 0x0; }
			
			std::cout<<"GBE::Erased external camera file from VRAM\n";

			core_mmu->cart.cam_lock = false;
		}
	}

	//Reset emulation on F8
	//Only done when using GB Memory Cartridge via GUI
	else if((input == SDLK_F8) && (pressed) && (config::cart_type == DMG_GBMEM) && (config::use_external_interfaces))
	{
		//If running GB Memory Cartridge, make sure this is a true reset, i.e. boot to the menu program
		if(core_mmu->cart.flash_stat == 0x40)
		{
			core_mmu->cart.flash_stat = 0;
			config::gb_type = core_mmu->cart.flash_cnt;
		}

		reset();
	}
}

/****** Updates the core's volume ******/
void GB_core::update_volume(u8 volume)
{
	config::volume = volume;
	core_cpu->controllers.audio.apu_stat.channel_master_volume = (config::volume >> 2);
}

/****** Feeds key input from an external source (useful for TAS) ******/
void GB_core::feed_key_input(int sdl_key, bool pressed)
{
	core_pad.process_keyboard(sdl_key, pressed);
	handle_hotkey(sdl_key, pressed);
}

/****** Return a CPU register ******/
u32 GB_core::ex_get_reg(u8 reg_index)
{
	switch(reg_index)
	{
		case 0x0: return core_cpu->reg.a;
		case 0x1: return core_cpu->reg.b;
		case 0x2: return core_cpu->reg.c;
		case 0x3: return core_cpu->reg.d;
		case 0x4: return core_cpu->reg.e;
		case 0x5: return core_cpu->reg.h;
		case 0x6: return core_cpu->reg.l;
		case 0x7: return core_cpu->reg.f;
		case 0x8: return core_cpu->reg.sp;
		case 0x9: return core_cpu->reg.pc;
		default: return 0;
	}
}

/****** Read binary file to memory ******/
bool GB_core::read_file(std::string filename) { return core_mmu->read_file(filename); }

/****** Read BIOS file into memory ******/
bool GB_core::read_bios(std::string filename) { return core_mmu->read_bios(config::bios_file); }

/****** Read firmware file into memory (not applicable) ******/
bool GB_core::read_firmware(std::string filename) { return true; }

/****** Returns a byte from core memory ******/
u8 GB_core::ex_read_u8(u16 address) { return core_mmu->read_u8(address); }

/****** Writes a byte to core memory ******/
void GB_core::ex_write_u8(u16 address, u8 value) { core_mmu->write_u8(address, value); }

void GB_core::read_cgfx() {	cgfx::loaded = core_cpu->controllers.video->load_manifest(get_manifest_file());}

/****** Dumps selected OBJ to a file ******/
void GB_core::dump_obj(int obj_index)
{
	core_cpu->controllers.video->dump_obj(obj_index);
}

/****** Dumps selected BG tile to a file ******/
void GB_core::dump_bg(int bg_index)
{
	core_cpu->controllers.video->dump_bg(bg_index);
}

/****** Grabs the OBJ palette ******/
u32* GB_core::get_obj_palette(int pal_index)
{
	return &core_cpu->controllers.video->lcd_stat.obj_colors_final[0][pal_index];
}

/****** Grabs the BG palette ******/
u32* GB_core::get_bg_palette(int pal_index)
{
	return &core_cpu->controllers.video->lcd_stat.bg_colors_final[0][pal_index];
}

/****** Grabs the hash for a specific tile ******/
std::string GB_core::get_hash(u32 addr, u8 gfx_type)
{
	return core_cpu->controllers.video->get_hash(addr, gfx_type);
}


/****** Returns miscellaneous data from the core ******/
u32 GB_core::get_core_data(u32 core_index)
{
	u32 result = 0;

	switch(core_index & 0xFF)
	{
		//Joypad state
		case 0x0:
			result = ~((core_pad.p15 << 4) | core_pad.p14);
			result &= 0xFF;
			break;

		//Invalidate CGFX
		case 0x2:
			core_cpu->controllers.video->invalidate_cgfx();
			result = 1;
			break;

		//Grab current scanline pixel
		case 0x3:
			//Use bits 8-15 as index
			result = core_cpu->controllers.video->get_scanline_pixel((core_index >> 8) & 0xFF);
			break;

		//Render BG Scanline
		case 0x4:
			//Use bits 8-15 as index
			core_cpu->controllers.video->render_scanline(((core_index >> 8) & 0xFF), 0);
			result = 1;
			break;

		//Render Window Scanline
		case 0x5:
			//Use bits 8-15 as index
			core_cpu->controllers.video->render_scanline(((core_index >> 8) & 0xFF), 1);
			result = 1;
			break;

		//Render OBJ Scanline
		case 0x6:
			//Use bits 8-15 as index
			core_cpu->controllers.video->render_scanline(((core_index >> 8) & 0xFF), 2);
			result = 1;
			break;
	}

	return result;
}
