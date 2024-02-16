#include "midi_driver.h"
#include "common\config.h"
#include "common\util.h"

dmg_midi_driver* dmg_midi_driver::midi = 0;

dmg_midi_driver::dmg_midi_driver() {
	try {
		midiout = new RtMidiOut(RtMidi::Api::UNSPECIFIED);
		midiout->openPort(0);

		//fill freq chart
		for (u8 i = 0; i < 128; i++) {
			double n = i;
			//formula is 440 * pow(2, (n - 69.0) / 12.0), we use 0.5 to get max and min
			freqChart[i][0] = 440 * pow(2, (n - 69.5) / 12.0);
			freqChart[i][1] = 440 * pow(2, (n - 68.5) / 12.0);
		}

		for (u8 i = 0; i < 2; i++) {
			for (u8 j = 0; j < 4; j++) {
				instrument[i][j].instID = 0;
				instrument[i][j].hasReplacement = false;
				instrument[i][j].useHarmonic = false;
			}
		}

		for (u16 i = 0; i < 256; i++) {
			noise[i] = 0;
		}

		for (u8 i = 0; i < 6; i++) {
			channel[i].pitch = 0;
			channel[i].playing = false;
			channel[i].volume = 0;
			channel[i].duty = 0;
		}
		currentNoise = 0;
		replaceNoise = false;
		noiseHalf = false;
	}
	catch (RtMidiError& error) {
		midiout = 0;
	}
}

dmg_midi_driver::~dmg_midi_driver()
{
	if (midiout) {
		for (u8 i = 0; i < 4; i++) {
			sendNoteOff(i);
		}
		midiout->closePort();
		delete midiout;
	}
}

/****** initialize ******/
void dmg_midi_driver::init() {
	if (midiout) {
		//init midi
		if (midiout->getPortCount()) {

			for (u8 i = 0; i < 4; i++) {
				if (config::use_stereo) {
					//panoramic is fixed to left, left, right, right
					sendMidiMessage(CONTROLLER_CHANGE | i, CONTROLLER_PANORAMIC, (i & 0x02 ? 0x7F : 0), 2);
				}
				else {
					//panoramic is fixed to centre
					sendMidiMessage(CONTROLLER_CHANGE | i, CONTROLLER_PANORAMIC, 0x40, 2);
				}
			}

			//panoramic for wave channels
			if (config::use_stereo) {
				sendMidiMessage(CONTROLLER_CHANGE | 4, CONTROLLER_PANORAMIC, 0x00, 2);
				sendMidiMessage(CONTROLLER_CHANGE | 5, CONTROLLER_PANORAMIC, 0x7F, 2);
			}
			else {
				sendMidiMessage(CONTROLLER_CHANGE | 4, CONTROLLER_PANORAMIC, 0x40, 2);
				sendMidiMessage(CONTROLLER_CHANGE | 5, CONTROLLER_PANORAMIC, 0x40, 2);
			}

			//config::osd_message = "init midi ";
			//config::osd_count = 180;

		}
		else {
			delete midiout;
			midiout = 0;
		}
	}
}

/****** add mapping to instrument ******/
void dmg_midi_driver::addReplacement(u8 sq, u8 duty, u8 insID, bool useHarmonic) {
	instrument[sq][duty].instID = insID;
	instrument[sq][duty].hasReplacement = true;
	instrument[sq][duty].useHarmonic = useHarmonic;
	//config::osd_message = "add rep " + util::to_str(sq) + " " + util::to_str(duty) + " " + util::to_str(instrument[sq][duty].useHarmonic);
	//config::osd_count = 180;
}


/****** Send message to midi device ******/
void dmg_midi_driver::sendMidiMessage(u8 status, u8 data1, u8 data2, u8 len) {
	std::vector<unsigned char> message;
	message.push_back(status);
	if (len >= 1) message.push_back(data1);
	if (len >= 2) message.push_back(data2);
	midiout->sendMessage(&message);
}

/****** Stop a playing note on specific channel ******/
void dmg_midi_driver::sendNoteOff(u8 c) {
	if (channel[c].playing){
		sendMidiMessage(NOTE_OFF | c, channel[c].pitch, 0, 2);
		if (channelUseHarmonic(c)) {
			if (channel[c].pitch <= 115)
				sendMidiMessage(NOTE_OFF | c, channel[c].pitch + 12, 0, 2);
			if (channel[c].pitch >= 12)
				sendMidiMessage(NOTE_OFF | c, channel[c].pitch - 12, 0, 2);
		}
		channel[c].playing = false;
	}
}

/****** Start playing a note on specific channel ******/
void dmg_midi_driver::sendNoteOn(u8 c, u8 v, u8 p) {
	bool useH = channelUseHarmonic(c);
	if (useH) {
		sendMidiMessage(CONTROLLER_CHANGE | c, CONTROLLER_VOLUME, v / 3, 2);
	}
	else {
		sendMidiMessage(CONTROLLER_CHANGE | c, CONTROLLER_VOLUME, v, 2);
	}
	sendMidiMessage(NOTE_ON | c, p, 0x40, 2);
	if (useH) {
		//config::osd_message = "useH " + util::to_str(c) + " " + util::to_str(channel[c].duty) + " " + util::to_str(instrument[c][channel[c].duty].useHarmonic);
		//config::osd_count = 180;
		if (p <= 115)
			sendMidiMessage(NOTE_ON | c, p + 12, 0x40, 2);
		if (p >= 12)
			sendMidiMessage(NOTE_ON | c, p - 12, 0x40, 2);
	}
	channel[c].pitch = p;
	channel[c].volume = v;
	channel[c].playing = true;
}

/****** set the instrument for a channel ******/
void dmg_midi_driver::setInstrument(u8 sq, u8 duty) {
	//config::osd_message = "setInstrument " + util::to_str(c) + ", " + util::to_str(duty);
	//config::osd_count = 180;
	for (u8 i = 0; i <= 2; i += 2) {
		sendMidiMessage(PROGRAM_CHANGE | (sq + i), instrument[sq][duty].instID, 0, 1);
		channel[sq + i].duty = duty;
		channel[sq + i].hasReplace = true;
	}
}

/****** set instrument on duty change******/
void dmg_midi_driver::dutyChange(u8 sq, u8 duty) {
	//config::osd_message = "duty change " + util::to_str(sq) + ", " + util::to_str(duty);
	//config::osd_count = 180;
	if(channel[sq].duty != duty) stopSound(sq);
	if (instrument[sq][duty].hasReplacement) {
		setInstrument(sq, duty);
	}
	else {
		channel[sq].duty = duty;
		channel[sq].hasReplace = false;
		channel[sq + 2].duty = duty;
		channel[sq + 2].hasReplace = false;
	}
}

/****** play sound ******/
void dmg_midi_driver::playSound(u8 sq, u8 vol, double freq, bool left, bool right){	
	if (!channel[sq].hasReplace) return;

	//find pitch from frequency
	u8 p = frequencyToPitch(freq);

	//stop playing with volume is 0
	if (vol == 0) {
		stopSound(sq);

		//keep the pitch data for later, in case of envlope up
		channel[sq].pitch = p;
		channel[sq + 2].pitch = p;
		return;
	}

	u8 v = volumeConvert(vol);

	//config::osd_message = "playSound " + util::to_str(sq) + ", " + util::to_str(v) + ", " + util::to_str(p);
	//config::osd_count = 180;

	//check is playing already before starting
	for (u8 i = 0; i <= 2; i += 2) {
		if ((left && i == 0) || (right && i == 2)) {
			bool needWork = true;
			if (channel[sq + i].playing) {
				if (channel[sq + i].volume != v || channel[sq + i].pitch != p)
					sendNoteOff(sq + i);
				else
					needWork = false;
			}
			if (needWork) {
				sendNoteOn(sq + i, v, p);
			}
		}
	}
}

/****** stop sound ******/
void dmg_midi_driver::stopSound(u8 sq) {
	sendNoteOff(sq);
	sendNoteOff(sq + 2);
}

/****** pause play sound ******/
void dmg_midi_driver::pause() {
	//send note off without changing the playing flag
	for (u8 i = 0; i < 6; i++) {
		if (channel[i].playing) {
			sendMidiMessage(NOTE_OFF | i, channel[i].pitch, 0, 2);
			if (channelUseHarmonic(i)) {
				if (channel[i].pitch <= 115)
					sendMidiMessage(NOTE_OFF | i, channel[i].pitch + 12, 0, 2);
				if (channel[i].pitch >= 12)
					sendMidiMessage(NOTE_OFF | i, channel[i].pitch - 12, 0, 2);
			}
		}
	}
	if (replaceNoise) {
		sendMidiMessage(NOTE_OFF | 9, currentNoise, 0, 2);
	}
}

/****** unpause play sound ******/
void dmg_midi_driver::unpause() {
	//send note on base on the playing flag
	for (u8 i = 0; i < 6; i++) {
		if (channel[i].playing) {
			sendMidiMessage(NOTE_ON | i, channel[i].pitch, 0x40, 2);
			if (channelUseHarmonic(i)) {
				if (channel[i].pitch <= 115)
					sendMidiMessage(NOTE_ON | i, channel[i].pitch + 12, 0x40, 2);
				if (channel[i].pitch >= 12)
					sendMidiMessage(NOTE_ON | i, channel[i].pitch - 12, 0x40, 2);
			}
		}
	}
	if (replaceNoise) {
		sendMidiMessage(NOTE_ON | 9, currentNoise, (noiseHalf ? 0x20 : 0x40), 2);
	}
}


/****** handle sweep up or down ******/
void dmg_midi_driver::sq1SweepTo(double freq) {
	u8 p = frequencyToPitch(freq);
	u16 dif;
	for (u8 i = 0; i <= 2; i += 2) {
		if (channel[i].playing) {
			dif = 0x2000 + p - channel[i].pitch;
			sendMidiMessage(PITCH_BEND | i, dif & 0x007F, dif >> 7, 2);
			channel[i].pitch = p;
		}
	}
	//config::osd_message = "sweep " + util::to_str(p);
	//config::osd_count = 180;
}

/****** convert frequency to pitch ******/
u8 dmg_midi_driver::frequencyToPitch(double freq) {
	//find pitch from frequency
	if (freq < freqChart[0][0]) return 0;
	else if (freq > freqChart[127][1]) return 127;
	else {
		u8 p;
		bool needWork = true;
		u8 rangeMin = 0;
		u8 rangeMax = 127;
		p = 63;
		while (needWork) {
			if (freq < freqChart[p][0]) rangeMax = p - 1;
			else if (freq > freqChart[p][1]) rangeMin = p + 1;
			else needWork = false;

			if (needWork) p = (rangeMin + rangeMax) / 2;
		}
		return p;
	}
}

/****** set volume ******/
void dmg_midi_driver::changeVolume(u8 sq, u8 vol) {
	u8 v = volumeConvert(vol);
	for (u8 i = 0; i <= 2; i += 2) {
		if (channel[sq + i].playing) {
			if (channelUseHarmonic(sq + i)) {
				sendMidiMessage(CONTROLLER_CHANGE | (sq + i), CONTROLLER_VOLUME, v / 3, 2);
			}
			else {
				sendMidiMessage(CONTROLLER_CHANGE | (sq + i), CONTROLLER_VOLUME, v, 2);
			}
			channel[sq + i].volume = v;
		}
	}
	//config::osd_message = "envelope " + util::to_str(v);
	//config::osd_count = 180;
}


/****** convert 4 bit vol to 7 bit vol ******/
u8 dmg_midi_driver::volumeConvert(u8 vol) {
	//get correct volume from 0x00-0x0F to 0x00-0x7F
	//then multiply by master volume
	u16 v = (vol << 3) | (vol >> 1);
	return (v * config::volume) >> 7;
}

/****** query whether the channel has replacement ******/
bool dmg_midi_driver::checkHasReplace(u8 sq) {
	//config::osd_message = "checkHasReplace " + util::to_str(sq);
	//config::osd_count = 180;

	return channel[sq].hasReplace;
}

/****** add noise replacement for a specific nr43 value ******/
void dmg_midi_driver::addNoiseReplacement(u8 nr43v, u8 insID) {
	noise[nr43v] = insID;
}

/****** play noise replacement ******/
void dmg_midi_driver::playNoise(u8 nr43v, u8 vol, bool left, bool right) {
	if (replaceNoise) stopNoise();
	if (noise[nr43v] && (left || right)) {
		sendMidiMessage(CONTROLLER_CHANGE | 9, CONTROLLER_VOLUME, volumeConvert(vol), 2);
		//full velocity when centre, half otherwise
		u8 velocity;
		if ((left && right) || !config::use_stereo) {
			sendMidiMessage(CONTROLLER_CHANGE | 9, CONTROLLER_PANORAMIC, 0x40, 2);
			velocity = 0x40;
			noiseHalf = false;
		}
		else {
			velocity = 0x20;
			noiseHalf = true;
			if (left) {
				sendMidiMessage(CONTROLLER_CHANGE | 9, CONTROLLER_PANORAMIC, 0, 2);
			}
			else {
				sendMidiMessage(CONTROLLER_CHANGE | 9, CONTROLLER_PANORAMIC, 0x7F, 2);
			}
		}
		sendMidiMessage(NOTE_ON | 9, noise[nr43v], velocity, 2);
		currentNoise = nr43v;
		replaceNoise = true;
	}
}

/****** change the volume of noise channel ******/
void dmg_midi_driver::changeNoiseVolume(u8 vol) {
	sendMidiMessage(CONTROLLER_CHANGE | 9, CONTROLLER_VOLUME, volumeConvert(vol), 2);
}


/****** stop noise replacement ******/
void dmg_midi_driver::stopNoise() {
	if (replaceNoise){
		//config::osd_message = "stop noise " + util::to_str(currentNoise);
		//config::osd_count = 180;
		sendMidiMessage(NOTE_OFF | 9, currentNoise, 0, 2);
		replaceNoise = false;
	}
}

/****** query whether this value in NR43 has replacement ******/
bool dmg_midi_driver::checkNoiseHasReplace() {
	return replaceNoise;
}


/****** play wave replacement ******/
void dmg_midi_driver::playWave(u8 vol, double freq, bool left, bool right) {
	if (!wave.size()) return;

	u32 newWaveID;
	for (u32 i = 0; i < wave.size(); i++) {
		if (memcmp(wave[i].waveRam, waveRam, 16)) {
			newWaveID = i;
			break;
		}
		else if (i == wave.size() - 1) {
			//no match
			return;
		}
	}

	//find pitch from frequency
	u8 p = frequencyToPitch(freq);

	//stop playing with volume is 0
	if (vol == 4) {
		stopWave();
		return;
	}

	u8 v = volumeConvert(0x0F >> vol);

	//check is playing already before starting
	for (u8 i = 4; i <= 5; i++) {
		if ((left && i == 4) || (right && i == 5)) {
			bool needWork = true;
			if (channel[i].playing) {
				if (channel[i].volume != v || channel[i].pitch != p || newWaveID != currentWaveID)
					sendNoteOff(i);
				else
					needWork = false;
			}
			if (needWork) {
				//config::osd_message = "playWave " + util::to_str(i) + " " + util::to_str(v) + " " + util::to_str(p);
				//config::osd_count = 180;
				sendMidiMessage(PROGRAM_CHANGE | i, wave[newWaveID].instID, 0, 1);
				sendNoteOn(i, v, p);
			}
		}
	}
	currentWaveID = newWaveID;
}

/****** stop wave replacement ******/
void dmg_midi_driver::stopWave() {
	sendNoteOff(4);
	sendNoteOff(5);
}


/****** check replacement used uses harmonic ******/
bool dmg_midi_driver::channelUseHarmonic(u8 c) {
	if (c < 4) 
		return instrument[c & 0x01][channel[c].duty].useHarmonic;
	else 
		return wave[currentWaveID].useHarmonic;
}

/****** add a wave replacement ******/
void dmg_midi_driver::addWaveReplacement(u32* waveForm, u8 insID, bool useHarmonic) {
	dmg_midi_wave w;
	w.instID = insID;
	w.useHarmonic = useHarmonic;
	for(u8 i = 0; i < 4; i++)
		w.waveRam[i] = waveForm[i];

	wave.push_back(w);
	//config::osd_message = "add Wave " + util::to_str(waveForm[0]);
	//config::osd_count = 180;
}

bool dmg_midi_driver::checkWaveHasReplace() {
	return channel[4].playing;
}
