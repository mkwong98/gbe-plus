#include "custom_sound.h"
#include "common\config.h"
#include "common\util.h"

dmg_custom_sound* dmg_custom_sound::soundex = 0;

dmg_custom_sound::dmg_custom_sound() {
	for (u16 i = 0; i < 256; i++) {
		bgm[i] = 0;
		sfx[i] = 0;
	}
	current_bgm = 0;
	looping = false;
	bgm_vol = 0xFF;
	bgm_volume = 1;
	sfx_vol = 0xFF;
	sfx_volume = 1;
}

dmg_custom_sound::~dmg_custom_sound() {
	if (Mix_PlayingMusic()) Mix_HaltMusic();
	for (u16 i = 0; i < 256; i++) {
		if (bgm[i]) Mix_FreeMusic(bgm[i]);
		if (sfx[i]) Mix_FreeChunk(sfx[i]);
	}
}

/********register an write address **********/
void dmg_custom_sound::addWAddress(u16 address, u8 function, u8 replaceType, u8 replaceValue, u8 valueMask) {
	dmg_custom_sound_address a;
	a.address = address;
	a.function = function;
	a.type = replaceType;
	a.value = replaceValue;
	a.valueMask = valueMask;

	waddresses.push_back(a);
}

/********write to address **********/
bool dmg_custom_sound::handleWrite(u16 address, u8* value) {

	bool skipWrite = false;
	for (const dmg_custom_sound_address& a : waddresses) {
		bool workDone;
		if (a.address == address) {
			workDone = false;
			double range;
			double diff;

			switch (a.function) {
			case FUNCTION_W_NONE:
				workDone = true;
				break;
			case FUNCTION_W_PLAY_BGM:
				if (Mix_PlayingMusic() && (current_bgm != *value)) {
					Mix_HaltMusic();
				}
				if (bgm[*value]) {
					if (Mix_PlayingMusic() && !paused) {
						if (current_bgm != *value) {
							Mix_HaltMusic();
							Mix_PlayMusic(bgm[*value], (looping ? -1 : 0));
						}
					} 
					else {
						Mix_PlayMusic(bgm[*value], (looping ? -1 : 0));
					}
					paused = false;
					workDone = true;
				}
				current_bgm = *value;
				break;
			case FUNCTION_W_SET_LOOPING:
				if ((*value & a.valueMask) == a.value) {
					looping = true;
					workDone = true;
				}
				break;
			case FUNCTION_W_SET_NOT_LOOPING:
				if ((*value & a.valueMask) == a.value) {
					looping = false;
					workDone = true;
				}
				break;
			case FUNCTION_W_STOP_BGM:
				if ((*value & a.valueMask) == a.value) {
					Mix_HaltMusic();
					workDone = true;
				}
				break;
			case FUNCTION_W_PAUSE_BGM:
				if ((*value & a.valueMask) == a.value) {
					Mix_PauseMusic();
					paused = true;
					workDone = true;
				}
				break;
			case FUNCTION_W_RESUME_BGM:
				if ((*value & a.valueMask) == a.value) {
					Mix_ResumeMusic();
					paused = false;
					workDone = true;
				}
				break;
			case FUNCTION_W_BGM_VOLUME:
				range = a.valueMask - a.value;
				diff = (*value) - a.value;
				bgm_vol = *value;
				bgm_volume = diff / range;
				Mix_VolumeMusic((config::volume >> 1) * bgm_volume);
				workDone = true;
				break;
			case FUNCTION_W_PLAY_SFX:
				if (sfx[*value]) {
					Mix_PlayChannel(-1, sfx[*value], 0);
					workDone = true;
				}
				break;
			case FUNCTION_W_STOP_SFX:
				if ((*value & a.valueMask) == a.value) {
					Mix_HaltChannel(-1);
					workDone = true;
				}
				break;
			case FUNCTION_W_SFX_VOLUME:
				range = a.valueMask - a.value;
				diff = (*value) - a.value;
				sfx_vol = *value;
				sfx_volume = diff / range;
				Mix_MasterVolume((config::volume >> 1) * sfx_volume);
				workDone = true;
				break;
			default:
				break;
			}
			if(workDone)
				skipWrite = handleReplace(a, value);
		}
	}
	return skipWrite;
}

bool dmg_custom_sound::handleReplace(dmg_custom_sound_address a, u8* value) {
	switch (a.type){
	case REPLACE_NO_CHANGE:
		return false;
		break;
	case REPLACE_CONST_VALUE:
		*value = a.value;
		return false;
		break;
	case REPLACE_SKIP_WRITE:
		return true;
		break;
	default:
		return false;
		break;
	}
}


/********register an read address **********/
void dmg_custom_sound::addRAddress(u16 address, u8 function, u8 returnType, u8 returnValue, u8 valueMask) {
	dmg_custom_sound_address a;
	a.address = address;
	a.function = function;
	a.type = returnType;
	a.value = returnValue;
	a.valueMask = valueMask;

	raddresses.push_back(a);
}

/********read from address **********/
bool dmg_custom_sound::handleRead(u16 address, u8* value) {
	bool skipRead = false;
	u8 result = 0;
	u8 newVal = 0;
	for (const dmg_custom_sound_address& a : raddresses) {
		if (a.address == address) {
			switch (a.function) {
			case FUNCTION_R_CONST:
				newVal = a.value;
				break;
			case FUNCTION_R_BGM_ID:
				newVal = current_bgm;
				break;
			case FUNCTION_R_PLAYING:
				newVal = (Mix_PlayingMusic() ? a.value : a.valueMask);
				break;
			case FUNCTION_R_LOOPING:
				newVal = (looping ? a.value : a.valueMask);
				break;
			case FUNCTION_R_PAUSED:
				newVal = (Mix_PausedMusic() ? a.value : a.valueMask);
				break;
			case FUNCTION_R_BGM_VOL:
				newVal = bgm_vol;
				break;
			case FUNCTION_R_SFX_VOL:
				newVal = sfx_vol;
				break;
			default:
				break;
			}
			handleReturn(a, &result, newVal);
			skipRead = true;
		}
	}
	if (skipRead) *value = result;
	return skipRead;
}

void dmg_custom_sound::handleReturn(dmg_custom_sound_address a, u8* value, u8 newVal) {
	switch (a.type) {
	case RETURN_OVERWRITE:
		*value = newVal;
		break;
	case RETURN_OR_VALUE:
		*value |= newVal;
		break;
	case RETURN_AND_VALUE:
		*value &= newVal;
		break;
	case RETURN_NOT_VALUE:
		*value = ~*value;
		break;
	default:
		break;
	}
}

/********add music **********/
void dmg_custom_sound::addBgm(u8 id, std::string fileName) {
	dmg_custom_sound_file f;
	f.id = id;
	f.fileName = fileName;
	bgmFileName.push_back(f);
}

/********read from sound effect **********/
void dmg_custom_sound::addSfx(u8 id, std::string fileName) {
	dmg_custom_sound_file f;
	f.id = id;
	f.fileName = fileName;
	sfxFileName.push_back(f);
}

void dmg_custom_sound::loadFiles() {
	for(const dmg_custom_sound_file& f : bgmFileName)
		bgm[f.id] = Mix_LoadMUS(f.fileName.c_str());

	for (const dmg_custom_sound_file& f : sfxFileName)
		sfx[f.id] = Mix_LoadWAV(f.fileName.c_str());
}

void dmg_custom_sound::updateVolume(u8 v) {
	Mix_VolumeMusic((v >> 1) * bgm_volume);
	Mix_MasterVolume((v >> 1) * sfx_volume);
}
