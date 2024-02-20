#ifndef GB_MIDI_DRIVER
#define GB_MIDI_DRIVER

#include <vector>
#include "rtmidi/RtMidi.h"
#include "common.h"

#include <fstream>
#include <sstream>


/*
0 = Acoustic Grand Piano
8 = Celesta
16 = Drawbar Organ
24 = Acoustic Guitar (nylon)
32 = Acoustic Bass
40 = Violin
56 = Trumpet
64 = Soprano Sax
72 = Piccolo
*/

struct dmg_midi_message {
	u8 status;
	u8 data[2];
	u8 dataLen;
};

struct dmg_midi_replacement {
	u8 instID;
	bool useHarmonic;
	bool hasReplacement;
};

struct dmg_midi_wave {
	u8 instID;
	bool useHarmonic;
	u8 waveRam[16];
	u8 volume;
};

struct dmg_midi_channel {
	u8 pitch;
	u8 sweepPitch;
	u8 volume;
	u8 duty;
	bool playing;
	bool hasReplace;
};

class dmg_midi_driver
{
public:

	static dmg_midi_driver* midi;
	u8* waveRam;

	dmg_midi_driver();
	~dmg_midi_driver();
	void init();

	void dutyChange(u8 sq, u8 duty);
	void playSound(u8 sq, u8 vol, double freq, bool left, bool right);
	void stopSound(u8 sq);
	void playNoise(u8 nr43v, u8 vol, bool left, bool right);
	void stopNoise();
	void playWave(u8 vol, double freq, bool left, bool right);
	void stopWave();
	void pause();
	void unpause();
	void sq1SweepTo(u8 vol, double freq, bool left, bool right);
	void changeVolume(u8 sq, u8 vol);
	void changeNoiseVolume(u8 vol);
	void addReplacement(u8 sq, u8 duty, u8 insID, bool useHarmonic);
	void addNoiseReplacement(u8 nr43v, u8 insID);
	void addWaveReplacement(u8* waveForm, u8 insID, bool useHarmonic, u8 vol);
	bool checkHasReplace(u8 sq);
	bool checkNoiseHasReplace();
	bool checkWaveHasReplace();

	//void log(std::string a);

private:
	const u8 NOTE_ON = 0x90;
	const u8 NOTE_OFF = 0x80;
	const u8 CONTROLLER_CHANGE = 0xB0;
	const u8 PROGRAM_CHANGE = 0xC0;
	const u8 PITCH_BEND = 0xE0;

	const u8 CONTROLLER_VOLUME = 7;
	const u8 CONTROLLER_PANORAMIC = 10;


	dmg_midi_replacement instrument[2][4];
	u8 noise[256];
	std::vector<dmg_midi_wave> wave;

	double freqChart[128][2];

	//midi Channels, sq: 2 left and 2 right, wave: 1 left and 1 right
	dmg_midi_channel channel[6];
	u32 currentWaveID;
	u8 currentNoise;
	bool replaceNoise;
	bool noiseHalf;
	bool blocking;

	RtMidiOut* midiout;
	void sendMidiMessage(u8 status, u8 data1, u8 data2, u8 len);
	void sendNoteOff(u8 c);
	void sendNoteOn(u8 c, u8 v, u8 p);
	void setInstrument(u8 c, u8 duty);
	u8 frequencyToPitch(double freq);
	u8 volumeConvert(u8 vol);
	bool channelUseHarmonic(u8 c);
	void clearSweep();
	void sweep(u8 p);
	//std::ofstream logfile;
};

#endif // GB_MIDI_DATA

