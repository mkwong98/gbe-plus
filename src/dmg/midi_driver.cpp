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
			noise[i].instID = 0;
		}

		for (u8 i = 0; i < 6; i++) {
			channel[i].pitch = 0;
			channel[i].sweepPitch = 0;
			channel[i].playing = false;
			channel[i].volume = 0;
			channel[i].duty = 0;
		}
		currentNoise = 0;
		currentWaveID = 0xFFFFFFFF;
		replaceNoise = false;
		noiseHalf = false;

		//logfile.open("midilog.csv", std::ios::out);

	}
	catch (RtMidiError& error) {
		midiout = 0;
	}
}

dmg_midi_driver::~dmg_midi_driver()
{
	if (midiout) {
		for (u8 i = 0; i < 6; i++) {
			sendNoteOff(i);
		}
		midiout->closePort();
		delete midiout;
	}
	//logfile.close();
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
		}
		else {
			delete midiout;
			midiout = 0;
		}
	}
}

/****** add mapping to instrument ******/
void dmg_midi_driver::addReplacement(u8 sq, u8 duty, u8 insID, bool useHarmonic, u8 vol) {
	instrument[sq][duty].instID = insID;
	instrument[sq][duty].hasReplacement = true;
	instrument[sq][duty].useHarmonic = useHarmonic;
	instrument[sq][duty].volume = vol;
	//SDL_ShowSimpleMessageBox(0,
	//	"",
	//	util::to_str(vol).c_str(),
	//	0);
}

/****** Send message to midi device ******/
void dmg_midi_driver::sendMidiMessage(u8 status, u8 data1, u8 data2, u8 len) {
	std::vector<unsigned char> message;
	message.push_back(status);
	if (len >= 1) message.push_back(data1);
	if (len >= 2) message.push_back(data2);
	midiout->sendMessage(&message);

	//if ((status & 0x0f) == 4)
	//	logfile << "Message," << util::to_hex_str(status) << "," << util::to_str(data1) << "," << util::to_str(data2) << "," << util::to_str(len) << "\n";

}

/****** Stop a playing note on specific channel ******/
void dmg_midi_driver::sendNoteOff(u8 c) {
	if (channel[c].playing){
		//if(c == 4)
		//	logfile << "NoteOff," << util::to_str(c) << "," << util::to_str(channel[c].pitch) << "\n";
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
	//if (c == 4)
	//	logfile << "NoteON," << util::to_str(c) << "," << util::to_str(p) << "\n";

	bool useH = channelUseHarmonic(c);
	if (useH) {
		sendMidiMessage(CONTROLLER_CHANGE | c, CONTROLLER_VOLUME, v / 3, 2);
	}
	else {
		sendMidiMessage(CONTROLLER_CHANGE | c, CONTROLLER_VOLUME, v, 2);
	}
	sendMidiMessage(NOTE_ON | c, p, 0x40, 2);
	if (useH) {
		if (p <= 115)
			sendMidiMessage(NOTE_ON | c, p + 12, 0x40, 2);
		if (p >= 12)
			sendMidiMessage(NOTE_ON | c, p - 12, 0x40, 2);
	}
	channel[c].pitch = p;
	channel[c].sweepPitch = p;
	channel[c].volume = v;
	channel[c].playing = true;
}

/****** set the instrument for a channel ******/
void dmg_midi_driver::setInstrument(u8 sq, u8 duty) {
	for (u8 i = 0; i <= 2; i += 2) {
		sendMidiMessage(PROGRAM_CHANGE | (sq + i), instrument[sq][duty].instID, 0, 1);
		channel[sq + i].duty = duty;
		channel[sq + i].hasReplace = true;
	}
}

/****** set instrument on duty change******/
void dmg_midi_driver::dutyChange(u8 sq, u8 duty) {
	if (channel[sq].duty != duty) stopSound(sq);

	//if(sq == 0 && channel[sq].duty != duty)
	//	logfile << "duty" << " " << util::to_str(sq) << " " << util::to_str(duty) << "\n";

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
	if (blocking || !channel[sq].hasReplace) return;

	//find pitch from frequency
	u8 p = frequencyToPitch(freq);

	//stop playing with volume is 0
	if (vol == 0) {
		stopSound(sq);

		//keep the pitch data for later, in case of envlope up
		channel[sq].pitch = p;
		channel[sq].sweepPitch = p;
		channel[sq + 2].pitch = p;
		channel[sq + 2].sweepPitch = p;
		return;
	}

	u16 v = volumeConvert(vol) * instrument[sq][channel[sq].duty].volume / 100;

	////check is playing already before starting
	//if (sq == 0)
	//	logfile << "playsound," << util::to_str(sq) << "," << util::to_str(p) << "\n";

	if (sq == 0 && channel[0].sweepPitch != channel[0].pitch) {
		clearSweep();
	}

	blocking = true;
	bool needWork;
	for (u8 i = 0; i <= 2; i += 2) {
		needWork = true;
		if ((i == 0 && !left) || i == 2 && !right) {
			needWork = false;
			if (channel[sq + i].playing) sendNoteOff(sq + i);
		}
		else if(channel[sq + i].playing) {
			if (channel[sq + i].pitch == p && channel[sq + i].volume == v) {
				needWork = false;
			}
			else {
				sendNoteOff(sq + i);
			}
		}
		if (needWork) {
			sendNoteOn(sq + i, v, p);
		}
	}
	blocking = false;
}

/****** stop sound ******/
void dmg_midi_driver::stopSound(u8 sq) {
	//if (sq == 0)
	//	logfile << "stopsound," << util::to_str(sq) << "\n";
	if (sq == 0 && channel[0].sweepPitch != channel[0].pitch) {
		clearSweep();
	}
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
void dmg_midi_driver::sq1SweepTo(u8 vol, double freq, bool left, bool right) {
	if (blocking) return;
	u8 p = frequencyToPitch(freq);
	if ((channel[0].playing && channel[0].sweepPitch != p) || (channel[2].playing && channel[2].sweepPitch != p)) {
		u8 dif;
		//logfile << "sweep," << "," << util::to_str(channel[0].sweepPitch) << "," << util::to_str(p) << "\n";
		if (channel[0].playing) {
			if (p > channel[0].sweepPitch) 
				dif = p - channel[0].sweepPitch;
			else
				dif = channel[0].sweepPitch - p;
		}
		else if (channel[2].playing) {
			if (p > channel[2].sweepPitch)
				dif = p - channel[2].sweepPitch;
			else
				dif = channel[2].sweepPitch - p;
		}
		//play sound if the difference is over 2
		if (dif <= 2) {
			sweep(p);
		}
		else {
			playSound(0, vol, freq, left, right);
		}
	}
}

/******** reverse pitch bend ********/
void dmg_midi_driver::clearSweep() {
	//logfile << "clear sweep," << "," << util::to_str(channel[0].sweepPitch) << "," << util::to_str(channel[0].pitch) << "\n";
	sweep(channel[0].pitch);
}

/******* send pitch bend until p ******/
void dmg_midi_driver::sweep(u8 p) {
	blocking = true;
	u8 dif;
	for (u8 i = 0; i <= 2; i += 2) {
		if (channel[i].playing) {
			if (p > channel[i].sweepPitch) {
				dif = p - channel[i].sweepPitch;
				if (dif > 1) {
					sendMidiMessage(PITCH_BEND | i, 0, 0x40, 2);
					channel[i].sweepPitch += 2;
				}
				else if (dif == 1) {
					sendMidiMessage(PITCH_BEND | i, 0, 0x30, 2);
					channel[i].sweepPitch++;
				}
			}
			else if (p < channel[i].sweepPitch) {
				dif = channel[i].sweepPitch - p;
				if (dif > 1) {
					sendMidiMessage(PITCH_BEND | i, 0, 0x00, 2);
					channel[i].sweepPitch -= 2;
				}
				else {
					sendMidiMessage(PITCH_BEND | i, 0, 0x10, 2);
					channel[i].sweepPitch--;
				}
			}
		}
	}
	blocking = false;
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
	if (vol == 0) {
		stopSound(sq);
		return;
	}
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
	return channel[sq].hasReplace;
}

/****** add noise replacement for a specific nr43 value ******/
void dmg_midi_driver::addNoiseReplacement(u8 nr43v, u8 insID, u8 vol) {
	noise[nr43v].instID = insID;
	noise[nr43v].volume = vol;
}

/****** play noise replacement ******/
void dmg_midi_driver::playNoise(u8 nr43v, u8 vol, bool left, bool right) {
	if (replaceNoise) stopNoise();
	if (noise[nr43v].instID && (left || right)) {
		sendMidiMessage(CONTROLLER_CHANGE | 9, CONTROLLER_VOLUME, volumeConvert(vol), 2);
		//full velocity when centre, half otherwise
		u16 velocity;
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
		velocity = velocity * noise[nr43v].volume / 100;
		sendMidiMessage(NOTE_ON | 9, noise[nr43v].instID, velocity, 2);
		currentNoise = nr43v;
		replaceNoise = true;
	}
	//if(!noise[nr43v])
	//	logfile << "unmatched noise:" << util::to_hex_strXX(nr43v) << "\n";

}

/****** change the volume of noise channel ******/
void dmg_midi_driver::changeNoiseVolume(u8 vol) {
	sendMidiMessage(CONTROLLER_CHANGE | 9, CONTROLLER_VOLUME, volumeConvert(vol), 2);
}


/****** stop noise replacement ******/
void dmg_midi_driver::stopNoise() {
	if (replaceNoise){
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
	if (!wave.size()) {
		//logfile << "unmatched wave:" << util::to_hex_strXX(waveRam[0]) << util::to_hex_strXX(waveRam[1]) << util::to_hex_strXX(waveRam[2]) << util::to_hex_strXX(waveRam[3]);
		//logfile << util::to_hex_strXX(waveRam[4]) << util::to_hex_strXX(waveRam[5]) << util::to_hex_strXX(waveRam[6]) << util::to_hex_strXX(waveRam[7]);
		//logfile << util::to_hex_strXX(waveRam[8]) << util::to_hex_strXX(waveRam[9]) << util::to_hex_strXX(waveRam[10]) << util::to_hex_strXX(waveRam[11]);
		//logfile << util::to_hex_strXX(waveRam[12]) << util::to_hex_strXX(waveRam[13]) << util::to_hex_strXX(waveRam[14]) << util::to_hex_strXX(waveRam[15]) << "\n";

		return;
	}

	u32 newWaveID;
	for (u32 i = 0; i < wave.size(); i++) {
		if (memcmp(wave[i].waveRam, waveRam, 16) == 0) {
			newWaveID = i;
			break;
		}
		else if (i == wave.size() - 1) {
			//logfile << "unmatched wave:" << util::to_hex_strXX(waveRam[0]) << util::to_hex_strXX(waveRam[1]) << util::to_hex_strXX(waveRam[2]) << util::to_hex_strXX(waveRam[3]);
			//logfile << util::to_hex_strXX(waveRam[4]) << util::to_hex_strXX(waveRam[5]) << util::to_hex_strXX(waveRam[6]) << util::to_hex_strXX(waveRam[7]);
			//logfile << util::to_hex_strXX(waveRam[8]) << util::to_hex_strXX(waveRam[9]) << util::to_hex_strXX(waveRam[10]) << util::to_hex_strXX(waveRam[11]) ;
			//logfile << util::to_hex_strXX(waveRam[12]) << util::to_hex_strXX(waveRam[13]) << util::to_hex_strXX(waveRam[14]) << util::to_hex_strXX(waveRam[15]) << "\n";

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

	bool waveChanged = newWaveID != currentWaveID;
	currentWaveID = newWaveID;

	u16 v = volumeConvert(0x0F >> vol) * wave[currentWaveID].volume / 100;

	//check is playing already before starting
	bool needWork;
	for (u8 i = 4; i <= 5; i++) {
		needWork = true;
		if ((i == 4 && !left) || i == 5 && !right) {
			needWork = false;
			sendNoteOff(i);
		}
		else if (channel[i].playing) {
			if (channel[i].volume != v || channel[i].pitch != p || waveChanged) {
				sendNoteOff(i);
			}
			else {
				needWork = false;
			}
		}
		if (needWork) {
			if (waveChanged)
				sendMidiMessage(PROGRAM_CHANGE | i, wave[currentWaveID].instID, 0, 1);
			sendNoteOn(i, v, p);
		}
	}
}

/****** stop wave replacement ******/
void dmg_midi_driver::stopWave() {
	//logfile << "Stopwave" << "\n";
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
void dmg_midi_driver::addWaveReplacement(u8* waveForm, u8 insID, bool useHarmonic, u8 vol) {
	dmg_midi_wave w;
	w.instID = insID;
	w.useHarmonic = useHarmonic;
	w.volume = vol;
	for(u8 i = 0; i < 16; i++)
		w.waveRam[i] = waveForm[i];

	wave.push_back(w);
}

bool dmg_midi_driver::checkWaveHasReplace() {
	return channel[4].playing || channel[5].playing;
}

//void dmg_midi_driver::log(std::string a) {
//	logfile << a.c_str() << "\n";
//}
