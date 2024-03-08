#ifndef GB_CUSTOM_SOUND
#define GB_CUSTOM_SOUND

#include <vector>
#include <SDL2/SDL_mixer.h>
#include "common.h"

#include <fstream>
#include <sstream>


struct dmg_custom_sound_address {
	u16 address;
	u8 function;
	u8 type;
	u8 value;
	u8 valueMask;
};

struct dmg_custom_sound_file {
	u8 id;
	std::string fileName;
};

class dmg_custom_sound
{
public:
	static const u8 FUNCTION_W_NONE = 0;
	static const u8 FUNCTION_W_PLAY_BGM = 1;
	static const u8 FUNCTION_W_SET_LOOPING = 2;
	static const u8 FUNCTION_W_SET_NOT_LOOPING = 3;
	static const u8 FUNCTION_W_STOP_BGM = 4;
	static const u8 FUNCTION_W_PAUSE_BGM = 5;
	static const u8 FUNCTION_W_RESUME_BGM = 6;
	static const u8 FUNCTION_W_BGM_VOLUME = 7;
	static const u8 FUNCTION_W_PLAY_SFX = 8;
	static const u8 FUNCTION_W_STOP_SFX = 9;
	static const u8 FUNCTION_W_SFX_VOLUME = 10;

	static const u8 REPLACE_NO_CHANGE = 0;
	static const u8 REPLACE_CONST_VALUE = 1;
	static const u8 REPLACE_SKIP_WRITE = 2;

	static const u8 FUNCTION_R_OLD_VAL = 0;
	static const u8 FUNCTION_R_CONST = 1;
	static const u8 FUNCTION_R_BGM_ID = 2;
	static const u8 FUNCTION_R_PLAYING = 3;
	static const u8 FUNCTION_R_LOOPING = 4;
	static const u8 FUNCTION_R_PAUSED = 5;
	static const u8 FUNCTION_R_BGM_VOL = 6;
	static const u8 FUNCTION_R_SFX_VOL = 7;

	static const u8 RETURN_OVERWRITE = 0;
	static const u8 RETURN_OR_VALUE = 1;
	static const u8 RETURN_AND_VALUE = 2;
	static const u8 RETURN_NOT_VALUE = 3;

	static dmg_custom_sound* soundex;

	dmg_custom_sound();
	~dmg_custom_sound();

	void addWAddress(u16 address, u8 function, u8 replaceType, u8 replaceValue, u8 valueMask);
	bool handleWrite(u16 address, u8* value);
	void addRAddress(u16 address, u8 function, u8 returnType, u8 returnValue, u8 valueMask);
	bool handleRead(u16 address, u8* value);
	void addBgm(u8 id, std::string fileName);
	void addSfx(u8 id, std::string fileName);
	void loadFiles();
	void updateVolume(u8 v);

private:
	u8 current_bgm;
	bool looping;
	bool paused;
	u8 bgm_vol;
	u8 sfx_vol;
	double bgm_volume;
	double sfx_volume;

	std::vector <dmg_custom_sound_address> waddresses;
	std::vector <dmg_custom_sound_address> raddresses;

	Mix_Music* bgm[256];
	Mix_Chunk* sfx[256];

	std::vector<dmg_custom_sound_file> bgmFileName;
	std::vector<dmg_custom_sound_file> sfxFileName;

	bool handleReplace(dmg_custom_sound_address a, u8* value);
	void handleReturn(dmg_custom_sound_address a, u8* value, u8 newVal);
};

#endif // GB_CUSTOM_SOUND


