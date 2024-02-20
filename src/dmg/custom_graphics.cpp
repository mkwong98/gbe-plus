// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : custom_gfx.cpp
// Date : July 23, 2015
// Description : Game Boy Enhanced custom graphics (DMG/GBC version)
//
// Handles dumping original BG and Sprite tiles
// Handles loading custom pixel data

#include <fstream>
#include <sstream>

#include "common/util.h"
#include "common/cgfx_common.h"
#include "lcd.h"
#include "common/config.h"
#include "midi_driver.h"

void GB_LCD::clear_manifest() {
	cgfx_stat.packVersion = "";

	for (u16 i = 0; i < cgfx_stat.imgs.size(); i++)
	{
		for (u16 j = 0; j < cgfx_stat.imgs[i].size(); j++)
		{
			SDL_FreeSurface(cgfx_stat.imgs[i][j]);
		}
		cgfx_stat.imgs[i].clear();
	}
	cgfx_stat.imgs.clear();

	for (u16 i = 0; i < cgfx_stat.himgs.size(); i++)
	{
		for (u16 j = 0; j < cgfx_stat.himgs[i].size(); j++)
		{
			SDL_FreeSurface(cgfx_stat.himgs[i][j]);
		}
		cgfx_stat.himgs[i].clear();
	}
	cgfx_stat.himgs.clear();
	cgfx_stat.conds.clear();
	cgfx_stat.tiles.clear();
	for (u8 i = 0; i < 60; i++)
	{
		cgfx_stat.bgs[i].clear();
	}

	SDL_FreeSurface(cgfx_stat.brightnessMod);
	SDL_FreeSurface(cgfx_stat.alphaCpy);
	SDL_FreeSurface(cgfx_stat.tempStrip);
}

u8 GB_LCD::getOpType(std::string op)
{
	if (op == "==") return pack_condition::EQ;
	if (op == "!=") return pack_condition::NE;
	if (op == ">") return pack_condition::GT;
	if (op == "<") return pack_condition::LS;
	if (op == ">=") return pack_condition::GE;
	if (op == "<=") return pack_condition::LE;
}

void GB_LCD::processConditionNames(std::string names, std::vector<pack_cond_app>* apps)
{
	std::string strRest = names;
	std::string token;
	std::size_t pos;
	pack_cond_app app;
	while (strRest.length() > 0)
	{
		pos = strRest.find("&");
		if (pos != std::string::npos)
		{
			token = util::trimfnc(strRest.substr(0, pos));
			strRest = strRest.substr(pos + 1, std::string::npos);
		}
		else
		{
			token = util::trimfnc(strRest);
			strRest = "";
		}
		pos = token.find("!");
		app.negate = (pos == 0);
		if (app.negate) 
		{
			token = token.substr(pos + 1, std::string::npos);
		}
		for (u16 i = 0; i < cgfx_stat.conds.size(); i++) {
			if (cgfx_stat.conds[i].name == token) {
				app.condIdx = i;
				apps->push_back(app);
			}
		}
	}
}

/****** Loads the manifest file ******/
bool GB_LCD::load_manifest(std::string filename)
{
	clear_manifest();

	std::ifstream file(filename.c_str(), std::ios::in);
	std::string input_line = "";
	std::string tagName = "";
	cgfx_stat.frameCnt = 0;

	//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
	//	"Loading HdPack",
	//	filename.c_str(),
	//	NULL);
	bool result = false;
	cgfx::scaling_factor = 1;

	//add build-in condition
	pack_condition pc;
	pc.type = pack_condition::HMIRROR;
	pc.name = "hmirror";
	cgfx_stat.conds.push_back(pc);
	pc.type = pack_condition::VMIRROR;
	pc.name = "vmirror";
	cgfx_stat.conds.push_back(pc);
	pc.type = pack_condition::BGPRIORITY;
	pc.name = "bgpriority";
	cgfx_stat.conds.push_back(pc);
	pc.type = pack_condition::PALETTE0;
	pc.name = "sppalette0";
	cgfx_stat.conds.push_back(pc);
	pc.type = pack_condition::PALETTE1;
	pc.name = "sppalette1";
	cgfx_stat.conds.push_back(pc);
	pc.type = pack_condition::PALETTE2;
	pc.name = "sppalette2";
	cgfx_stat.conds.push_back(pc);
	pc.type = pack_condition::PALETTE3;
	pc.name = "sppalette3";
	cgfx_stat.conds.push_back(pc);
	pc.type = pack_condition::PALETTE4;
	pc.name = "sppalette4";
	cgfx_stat.conds.push_back(pc);
	pc.type = pack_condition::PALETTE5;
	pc.name = "sppalette5";
	cgfx_stat.conds.push_back(pc);
	pc.type = pack_condition::PALETTE6;
	pc.name = "sppalette6";
	cgfx_stat.conds.push_back(pc);
	pc.type = pack_condition::PALETTE7;
	pc.name = "sppalette7";
	cgfx_stat.conds.push_back(pc);

	if (file.is_open())
	{
		std::vector<u16> imgIdxOffset;
		//Cycle through whole file, line-by-line
		while (getline(file, input_line))
		{
			input_line = util::trimfnc(input_line);
			//get tag content
			std::size_t found1 = input_line.find("<");
			std::size_t found2 = input_line.find(">");
			std::size_t found3 = input_line.find("[");
			std::size_t found4 = input_line.find("]");
			if (found1 != std::string::npos && found2 != std::string::npos && (found1 == 0 || found3 == 0))
			{
				std::string condNames;
				if (found3 == 0 && found4 > found3) {
					condNames = input_line.substr(found3 + 1, found4 - found3 - 1);
				}
				else
				{
					condNames = "";
				}
				if (found2 > found1)
				{
					tagName = input_line.substr(found1 + 1, found2 - found1 - 1);
				}
				std::string strRest = input_line.substr(found2 + 1);
				std::size_t pos;
				if (tagName == "ver") cgfx_stat.packVersion = util::trimfnc(strRest);
				else if (tagName == "scale") cgfx::scaling_factor = stoi(util::trimfnc(strRest));
				else if (tagName == "img")
				{
					//load image and make a h flipped copy
					std::vector<SDL_Surface*> imgs;
					std::vector<SDL_Surface*> himgs;
					SDL_Surface* img;
					pos = strRest.find(",");
					if (pos != std::string::npos)
					{
						std::string imgname = strRest.substr(0, pos);
						img = load_pack_image(imgname);
						imgs.push_back(img);
						img = h_flip_image(img);
						himgs.push_back(img);
						imgname = util::trimfnc(strRest.substr(pos + 1, std::string::npos));
						img = load_pack_image(imgname);
						imgs.push_back(img);
						img = h_flip_image(img);
						himgs.push_back(img);
					}
					else
					{
						img = load_pack_image(util::trimfnc(strRest));
						imgs.push_back(img);
						img = h_flip_image(img);
						himgs.push_back(img);
					}
					imgIdxOffset.push_back(cgfx_stat.imgs.size());
					cgfx_stat.imgs.push_back(imgs);
					cgfx_stat.himgs.push_back(himgs);
				}
				else if (tagName == "tile")
				{
					pack_tile pt;
					std::string token;
					u8 tokenCnt = 0;
					while (strRest.length() > 0)
					{
						pos = strRest.find(",");
						if (pos != std::string::npos)
						{
							token = util::trimfnc(strRest.substr(0, pos));
							strRest = strRest.substr(pos + 1, std::string::npos);
						}
						else
						{
							token = util::trimfnc(strRest);
							strRest = "";
						}

						switch (tokenCnt)
						{
						case 0:
							pt.imgIdx = imgIdxOffset[stoi(token)];
							break;
						case 1:
							pt.tileStr = token;
							for (u16 i = 0; i < 8; i++)
							{
								pt.tileData.line[i] = std::stoul(token.substr(i * 4, 4), nullptr, 16);
							}
							break;
						case 2:
							pt.palStr = token;
							if (token.length() == 2)
							{
								pt.palette[0] = std::stoul(token, nullptr, 16);
							}
							else
							{
								for (u16 i = 0; i < 4; i++)
								{
									pt.palette[i] = std::stoul(token.substr(i * 4, 4), nullptr, 16);
								}
							}
							break;
						case 3:
							pt.x = stoi(token);
							break;
						case 4:
							pt.y = stoi(token);
							break;
						case 5:
							pt.brightness = stof(token);
							break;
						case 6:
							pt.default = (token == "Y");
							break;

						default:
							break;
						}

						tokenCnt++;
					}
					if (condNames != "") {
						processConditionNames(condNames, &(pt.condApps));
					}
					cgfx_stat.tiles.push_back(pt);
				}
				else if (tagName == "condition")
				{
					std::string token;
					u8 tokenCnt = 0;
					while (strRest.length() > 0)
					{
						pos = strRest.find(",");
						if (pos != std::string::npos)
						{
							token = util::trimfnc(strRest.substr(0, pos));
							strRest = strRest.substr(pos + 1, std::string::npos);
						}
						else
						{
							token = util::trimfnc(strRest);
							strRest = "";
						}

						switch (tokenCnt)
						{
						case 0:
							pc.name = token;
							break;
						case 1:
							if (token == "tileNearby")
								pc.type = pack_condition::TILENEARBY;
							else if (token == "spriteNearby")
								pc.type = pack_condition::SPRITENEARBY;
							else if (token == "tileAtPosition")
								pc.type = pack_condition::TILEATPOSITION;
							else if (token == "spriteAtPosition")
								pc.type = pack_condition::SPRITEATPOSITION;
							else if (token == "memoryCheck")
								pc.type = pack_condition::MEMORYCHECK;
							else if (token == "memoryCheckConstant")
								pc.type = pack_condition::MEMORYCHECKCONSTANT;
							else if (token == "frameRange")
								pc.type = pack_condition::FRAMERANGE;
							else if (token == "bgPalValueMatch")
								pc.type = pack_condition::BGPALVAL;
							else if (token == "spPalValueMatch")
								pc.type = pack_condition::SPPALVAL;
							break;
						case 2:
							switch (pc.type)
							{
							case pack_condition::TILENEARBY:
							case pack_condition::SPRITENEARBY:
							case pack_condition::TILEATPOSITION:
							case pack_condition::SPRITEATPOSITION:
								pc.x = stoi(token);
								break;
							case pack_condition::MEMORYCHECK:
							case pack_condition::MEMORYCHECKCONSTANT:
								pc.address1 = std::stoul(token, nullptr, 16);
								pc.mask = 0xFF;
								break;
							case pack_condition::FRAMERANGE:
								pc.divisor = stoi(token);
								break;
							case pack_condition::BGPALVAL:
							case pack_condition::SPPALVAL:
								if (token.length() == 2)
								{
									pc.palette[0] = std::stoul(token, nullptr, 16);
								}
								else
								{
									for (u16 i = 0; i < 4; i++)
									{
										pc.palette[i] = std::stoul(token.substr(i * 4, 4), nullptr, 16);
									}
								}
								break;
							default:
								break;
							}
							break;
						case 3:
							switch (pc.type)
							{
							case pack_condition::TILENEARBY:
							case pack_condition::SPRITENEARBY:
							case pack_condition::TILEATPOSITION:
							case pack_condition::SPRITEATPOSITION:
								pc.y = stoi(token);
								break;
							case pack_condition::MEMORYCHECK:
							case pack_condition::MEMORYCHECKCONSTANT:
								pc.opType = getOpType(token);
								break;
							case pack_condition::FRAMERANGE:
								pc.compareVal = stoi(token);
								break;
							case pack_condition::BGPALVAL:
							case pack_condition::SPPALVAL:
								pc.palIdx = stoi(token);
								break;
							default:
								break;
							}
							break;
						case 4:
							switch (pc.type)
							{
							case pack_condition::TILENEARBY:
							case pack_condition::SPRITENEARBY:
							case pack_condition::TILEATPOSITION:
							case pack_condition::SPRITEATPOSITION:
								for (u16 i = 0; i < 8; i++)
								{
									pc.tileData.line[i] = std::stoul(token.substr(i * 4, 4), nullptr, 16);
								}
								break;
							case pack_condition::MEMORYCHECK:
								pc.address2 = std::stoul(token, nullptr, 16);
								break;
							case pack_condition::MEMORYCHECKCONSTANT:
								pc.value = std::stoul(token, nullptr, 16);
								break;
							default:
								break;
							}
							break;
						case 5:
							switch (pc.type)
							{
							case pack_condition::TILENEARBY:
							case pack_condition::SPRITENEARBY:
							case pack_condition::TILEATPOSITION:
							case pack_condition::SPRITEATPOSITION:
								if (token.length() == 2)
								{
									pc.palette[0] = std::stoul(token, nullptr, 16);
								}
								else
								{
									for (u16 i = 0; i < 4; i++)
									{
										pc.palette[i] = std::stoul(token.substr(i * 4, 4), nullptr, 16);
									}
								}
								break;
							case pack_condition::MEMORYCHECK:
							case pack_condition::MEMORYCHECKCONSTANT:
								pc.mask = std::stoul(token, nullptr, 16);
								break;
							default:
								break;
							}
							break;
						default:
							break;
						}

						tokenCnt++;
					}
					cgfx_stat.conds.push_back(pc);


				}
				else if (tagName == "background")
				{
					pack_background bg;
					bg.brightness = 1;
					bg.hscroll = 0;
					bg.vscroll = 0;
					bg.priority = 10;
					bg.offsetX = 0;
					bg.offsetY = 0;

					std::string token;
					u8 tokenCnt = 0;
					while (strRest.length() > 0)
					{
						pos = strRest.find(",");
						if (pos != std::string::npos)
						{
							token = util::trimfnc(strRest.substr(0, pos));
							strRest = strRest.substr(pos + 1, std::string::npos);
						}
						else
						{
							token = util::trimfnc(strRest);
							strRest = "";
						}

						switch (tokenCnt)
						{
						case 0:
							bg.imgIdx = imgIdxOffset[stoi(token)];
							break;
						case 1:
							bg.brightness = stof(token);
							break;
						case 2:
							bg.hscroll = stof(token);
							break;
						case 3:
							bg.vscroll = stof(token);
							break;
						case 4:
							bg.priority = stoi(token);
							break;
						case 5:
							bg.offsetX = stoi(token);
							break;
						case 6:
							bg.offsetY = stoi(token);
							break;

						default:
							break;
						}

						tokenCnt++;
					}
					if (condNames != "") 
					{
						processConditionNames(condNames, &(bg.condApps));
					}
					if (bg.brightness != 1)
					{
						u8 c;
						u32 brightnessC;
						SDL_Surface* brightnessMod = SDL_CreateRGBSurfaceWithFormat(0, cgfx_stat.imgs[bg.imgIdx][0]->w, cgfx_stat.imgs[bg.imgIdx][0]->h, 32, SDL_PIXELFORMAT_ARGB8888);
						SDL_Surface* alphaCpy = SDL_CreateRGBSurfaceWithFormat(0, cgfx_stat.imgs[bg.imgIdx][0]->w, cgfx_stat.imgs[bg.imgIdx][0]->h, 32, SDL_PIXELFORMAT_ARGB8888);
						SDL_Surface* img = SDL_CreateRGBSurfaceWithFormat(0, cgfx_stat.imgs[bg.imgIdx][0]->w, cgfx_stat.imgs[bg.imgIdx][0]->h, 32, SDL_PIXELFORMAT_ARGB8888);

						SDL_SetSurfaceBlendMode(cgfx_stat.imgs[bg.imgIdx][0], SDL_BLENDMODE_NONE);
						//make a copy of the alpha values with all pixels white
						SDL_BlitSurface(cgfx_stat.imgs[bg.imgIdx][0], NULL, alphaCpy, NULL);
						SDL_FillRect(brightnessMod, NULL, 0xFFFFFFFF);
						SDL_SetSurfaceBlendMode(brightnessMod, SDL_BLENDMODE_ADD);
						SDL_BlitSurface(brightnessMod, NULL, alphaCpy, NULL);

						//create pixel values
						if (bg.brightness < 1)
						{
							//create a darken copy
							c = 255 * (1.0 - bg.brightness);
							brightnessC = 0x00000000 | c << 24;
						}
						else 
						{
							//create a brighten copy
							c = 255 * (bg.brightness - 1.0);
							brightnessC = 0x00FFFFFF | c << 24;
						}
						SDL_FillRect(brightnessMod, NULL, brightnessC);
						SDL_SetSurfaceBlendMode(brightnessMod, SDL_BLENDMODE_BLEND);
						SDL_BlitSurface(cgfx_stat.imgs[bg.imgIdx][0], NULL, img, NULL);
						SDL_BlitSurface(brightnessMod, NULL, img, NULL);

						//merge pixel values to alpha
						SDL_SetSurfaceBlendMode(img, SDL_BLENDMODE_MOD);
						SDL_BlitSurface(img, NULL, alphaCpy, NULL);

						//clean up
						SDL_SetSurfaceBlendMode(cgfx_stat.imgs[bg.imgIdx][0], SDL_BLENDMODE_BLEND);
						SDL_FreeSurface(img);
						SDL_FreeSurface(brightnessMod);

						std::vector<SDL_Surface*> imgs;
						std::vector<SDL_Surface*> himgs;
						imgs.push_back(alphaCpy);
						img = h_flip_image(alphaCpy);
						himgs.push_back(img);

						bg.imgIdx = cgfx_stat.imgs.size();
						cgfx_stat.imgs.push_back(imgs);
						cgfx_stat.himgs.push_back(himgs);
					}
					cgfx_stat.bgs[bg.priority].push_back(bg);
				}
				else if (tagName == "midi") 
				{
					std::string token;
					u8 tokenCnt = 0;
					u8 sq;
					u8 duty;
					u8 instID;
					bool useHarmonic;
					while (strRest.length() > 0)
					{
						pos = strRest.find(",");
						if (pos != std::string::npos)
						{
							token = util::trimfnc(strRest.substr(0, pos));
							strRest = strRest.substr(pos + 1, std::string::npos);
						}
						else
						{
							token = util::trimfnc(strRest);
							strRest = "";
						}

						switch (tokenCnt)
						{
						case 0:
							sq = stoi(token);
							break;
						case 1:
							duty = stoi(token);
							break;
						case 2:
							instID = stoi(token);
							break;
						case 3:
							useHarmonic = (token == "Y" || token == "y");
							break;
						default:
							break;
						}

						tokenCnt++;
					}
					dmg_midi_driver::midi->addReplacement(sq, duty, instID, useHarmonic);
				}
				else if (tagName == "noise")
				{
					std::string token;
					u8 tokenCnt = 0;
					u8 nr43v;
					u8 instID;
					while (strRest.length() > 0)
					{
						pos = strRest.find(",");
						if (pos != std::string::npos)
						{
							token = util::trimfnc(strRest.substr(0, pos));
							strRest = strRest.substr(pos + 1, std::string::npos);
						}
						else
						{
							token = util::trimfnc(strRest);
							strRest = "";
						}

						switch (tokenCnt)
						{
						case 0:
							nr43v = std::stoul(token, nullptr, 16);
							break;
						case 1:
							instID = stoi(token);
							break;
						default:
							break;
						}

						tokenCnt++;
					}
					dmg_midi_driver::midi->addNoiseReplacement(nr43v, instID);
				}
				else if (tagName == "wave")
				{
					std::string token;
					u8 tokenCnt = 0;
					u8 waveForm[16];
					u8 instID;
					u8 vol;
					bool useHarmonic;
					while (strRest.length() > 0)
					{
						pos = strRest.find(",");
						if (pos != std::string::npos)
						{
							token = util::trimfnc(strRest.substr(0, pos));
							strRest = strRest.substr(pos + 1, std::string::npos);
						}
						else
						{
							token = util::trimfnc(strRest);
							strRest = "";
						}

						switch (tokenCnt)
						{
						case 0:
							for(u8 i = 0; i < 16; i++)
								waveForm[i] = std::stoul(token.substr(i * 2, 2), nullptr, 16);
							break;
						case 1:
							instID = stoi(token);
							break;
						case 2:
							useHarmonic = (token == "Y" || token == "y");
							break;
						case 3:
							vol = stoi(token);
							break;
						default:
							break;
						}

						tokenCnt++;
					}
					dmg_midi_driver::midi->addWaveReplacement(waveForm, instID, useHarmonic, vol);
				}
			}
		}
		file.close();
		result = true;
	}



	//Initialize HD buffer for CGFX greater that 1:1
	config::sys_width *= cgfx::scaling_factor;
	config::sys_height *= cgfx::scaling_factor;

	reset_buffers();

	cgfx_stat.brightnessMod = SDL_CreateRGBSurfaceWithFormat(0, cgfx::scaling_factor * 8, cgfx::scaling_factor, 32, SDL_PIXELFORMAT_ARGB8888);
	cgfx_stat.alphaCpy = SDL_CreateRGBSurfaceWithFormat(0, cgfx::scaling_factor * 8, cgfx::scaling_factor, 32, SDL_PIXELFORMAT_ARGB8888);
	cgfx_stat.tempStrip = SDL_CreateRGBSurfaceWithFormat(0, cgfx::scaling_factor * 8, cgfx::scaling_factor, 32, SDL_PIXELFORMAT_ARGB8888);

	srcrect.w = cgfx::scaling_factor;
	srcrect.h = cgfx::scaling_factor;
	hdrect.w = cgfx::scaling_factor * 8;
	hdrect.h = cgfx::scaling_factor;
	hdsrcrect = hdrect;

	return result;
}

SDL_Surface* GB_LCD::load_pack_image(std::string filename)
{
	std::string path = get_game_cgfx_folder() + filename;
	SDL_Surface* s = IMG_Load(path.c_str());
	SDL_Surface* r = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_ARGB8888, 0);
	SDL_FreeSurface(s);
	return r;
}

SDL_Surface* GB_LCD::h_flip_image(SDL_Surface* s)
{
	SDL_Surface* r = SDL_CreateRGBSurfaceWithFormat(0, s->w, s->h, 32, SDL_PIXELFORMAT_ARGB8888);
	SDL_LockSurface(s);
	SDL_LockSurface(r);
	u32* srcP = (u32*)(s->pixels);
	u32* dstP = (u32*)(r->pixels) + s->w;
	for (u16 y = 0; y < s->h; y++) {
		for (u16 x = 0; x < s->w; x++) {
			dstP--;
			*dstP = *srcP;
			srcP++;
		}
		dstP += s->w * 2;
	}
	SDL_UnlockSurface(r);
	SDL_UnlockSurface(s);

	return r;
}

void GB_LCD::new_rendered_screen_data() {

	//remove old, set new to old, update list id in vram list
	u16 i = 0;
	while (i < cgfx_stat.screen_data.rendered_tile.size())
	{
		if (cgfx_stat.screen_data.rendered_tile[i].isOld)
		{
			cgfx_stat.vram_tile_used[cgfx_stat.screen_data.rendered_tile[i].address] = 0;
			cgfx_stat.vram_tile_idx[cgfx_stat.screen_data.rendered_tile[i].address] = 0xFFFF;
			cgfx_stat.screen_data.rendered_tile.erase(cgfx_stat.screen_data.rendered_tile.begin() + i);
		}
		else
		{
			cgfx_stat.screen_data.rendered_tile[i].isOld = true;
			cgfx_stat.vram_tile_idx[cgfx_stat.screen_data.rendered_tile[i].address] = i;
			i++;
		}
	}

	cgfx_stat.screen_data.rendered_palette.clear();
	cgfx_stat.frameCnt++;
}

void GB_LCD::new_rendered_scanline_data(u8 lineNo)
{
	cgfx_stat.screen_data.scanline[lineNo].lcdc = mem->read_u8(REG_LCDC);
	cgfx_stat.screen_data.scanline[lineNo].rendered_bg.clear();
	cgfx_stat.screen_data.scanline[lineNo].rendered_win.clear();
	cgfx_stat.screen_data.scanline[lineNo].rendered_obj.clear();
}