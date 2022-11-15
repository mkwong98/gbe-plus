// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : general_settings.h
// Date : June 28, 2015
// Description : Main menu
//
// Dialog for various options
// Deals with Graphics, Audio, Input, Paths, etc

#include <iostream>

#include "general_settings.h"
#include "render.h"
#include "qt_common.h"

#include "common/config.h"
#include "common/cgfx_common.h"
#include "common/util.h"

/****** General settings constructor ******/
gen_settings::gen_settings(QWidget *parent) : QDialog(parent)
{
	//Set up tabs
	tabs = new QTabWidget(this);
	
	general = new QDialog;
	display = new QDialog;
	sound = new QDialog;
	controls = new QDialog;
	paths = new QDialog;

	tabs->addTab(general, tr("General"));
	tabs->addTab(display, tr("Display"));
	tabs->addTab(sound, tr("Sound"));
	tabs->addTab(controls, tr("Controls"));
	tabs->addTab(paths, tr("Paths"));

	QWidget* button_set = new QWidget;
	tabs_button = new QDialogButtonBox(QDialogButtonBox::Close);
	controls_combo = new QComboBox;
	controls_combo->addItem("Standard Controls");
	controls_combo->addItem("Advanced Controls");
	controls_combo->addItem("Hotkey Controls");

	QHBoxLayout* button_layout = new QHBoxLayout;
	button_layout->setAlignment(Qt::AlignTop | Qt::AlignRight);
	button_layout->addWidget(controls_combo);
	button_layout->addWidget(tabs_button);
	button_set->setLayout(button_layout);

	//General settings - Use cheats
	QWidget* cheats_set = new QWidget(general);
	QLabel* cheats_label = new QLabel("Use cheats", cheats_set);
	QPushButton* edit_cheats = new QPushButton("Edit Cheats");
	cheats = new QCheckBox(cheats_set);
	cheats->setToolTip("Enables Game Genie and GameShark cheat codes");

	QHBoxLayout* cheats_layout = new QHBoxLayout;
	cheats_layout->addWidget(cheats, 0, Qt::AlignLeft);
	cheats_layout->addWidget(cheats_label, 1, Qt::AlignLeft);
	cheats_layout->addWidget(edit_cheats, 0, Qt::AlignRight);
	cheats_set->setLayout(cheats_layout);

	//General settings - RTC offsets
	QWidget* rtc_set = new QWidget(general);
	QLabel* rtc_label = new QLabel(" ", rtc_set);
	QPushButton* edit_rtc = new QPushButton("Edit RTC Offsets");
	edit_rtc->setToolTip("Adjusts the emulated real-time clock by adding specific offset values.");

	QHBoxLayout* rtc_layout = new QHBoxLayout;
	rtc_layout->addWidget(edit_rtc, 0, Qt::AlignLeft);
	rtc_layout->addWidget(rtc_label, 1, Qt::AlignRight);
	rtc_set->setLayout(rtc_layout);

	//General settings - Emulated CPU Speed
	QWidget* overclock_set = new QWidget(general);
	QLabel* overclock_label = new QLabel("Emulated CPU Speed", overclock_set);
	overclock = new QComboBox(overclock_set);
	overclock->setToolTip("Changes the emulated CPU speed. More demanding, but reduces lag that happens on real hardware. May break some games.");
	overclock->addItem("1x");
	overclock->addItem("2x");
	overclock->addItem("4x");
	overclock->addItem("8x");

	QHBoxLayout* overclock_layout = new QHBoxLayout;
	overclock_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	overclock_layout->addWidget(overclock_label);
	overclock_layout->addWidget(overclock);
	overclock_set->setLayout(overclock_layout);

	//General settings - Enable patches
	QWidget* patch_set = new QWidget(general);
	QLabel* patch_label = new QLabel("Enable ROM patches", patch_set);
	auto_patch = new QCheckBox(patch_set);
	auto_patch->setToolTip("Enables automatically patching a ROM when booting");

	QHBoxLayout* patch_layout = new QHBoxLayout;
	patch_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	patch_layout->addWidget(auto_patch);
	patch_layout->addWidget(patch_label);
	patch_set->setLayout(patch_layout);

	QVBoxLayout* gen_layout = new QVBoxLayout;
	gen_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	gen_layout->addWidget(overclock_set);
	gen_layout->addWidget(cheats_set);
	gen_layout->addWidget(patch_set);
	gen_layout->addWidget(rtc_set);
	general->setLayout(gen_layout);

	//Display settings - Screen scale
	QWidget* screen_scale_set = new QWidget(display);
	QLabel* screen_scale_label = new QLabel("Screen Scale : ");
	screen_scale = new QComboBox(screen_scale_set);
	screen_scale->setToolTip("Scaling factor for the system's original screen size");
	screen_scale->addItem("1x");
	screen_scale->addItem("2x");
	screen_scale->addItem("3x");
	screen_scale->addItem("4x");
	screen_scale->addItem("5x");
	screen_scale->addItem("6x");
	screen_scale->addItem("7x");
	screen_scale->addItem("8x");
	screen_scale->addItem("9x");
	screen_scale->addItem("10x");

	QHBoxLayout* screen_scale_layout = new QHBoxLayout;
	screen_scale_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	screen_scale_layout->addWidget(screen_scale_label);
	screen_scale_layout->addWidget(screen_scale);
	screen_scale_set->setLayout(screen_scale_layout);

	//Display settings - DMG on GBC palette
	QWidget* dmg_gbc_pal_set = new QWidget(display);
	QLabel* dmg_gbc_pal_label = new QLabel("DMG Color Palette : ");
	dmg_gbc_pal = new QComboBox(dmg_gbc_pal_set);
	dmg_gbc_pal->setToolTip("Selects the original Game Boy palette");
	dmg_gbc_pal->addItem("OFF");
	dmg_gbc_pal->addItem("GBC BOOTROM - NO INPUT");
	dmg_gbc_pal->addItem("GBC BOOTROM - UP");
	dmg_gbc_pal->addItem("GBC BOOTROM - DOWN");
	dmg_gbc_pal->addItem("GBC BOOTROM - LEFT");
	dmg_gbc_pal->addItem("GBC BOOTROM - RIGHT");
	dmg_gbc_pal->addItem("GBC BOOTROM - UP+A");
	dmg_gbc_pal->addItem("GBC BOOTROM - DOWN+A");
	dmg_gbc_pal->addItem("GBC BOOTROM - LEFT+A");
	dmg_gbc_pal->addItem("GBC BOOTROM - RIGHT+A");
	dmg_gbc_pal->addItem("GBC BOOTROM - UP+B");
	dmg_gbc_pal->addItem("GBC BOOTROM - DOWN+B");
	dmg_gbc_pal->addItem("GBC BOOTROM - LEFT+B");
	dmg_gbc_pal->addItem("GBC BOOTROM - RIGHT+B");
	dmg_gbc_pal->addItem("DMG - Classic Green");
	dmg_gbc_pal->addItem("DMG - Game Boy Light");

	QHBoxLayout* dmg_gbc_pal_layout = new QHBoxLayout;
	dmg_gbc_pal_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	dmg_gbc_pal_layout->addWidget(dmg_gbc_pal_label);
	dmg_gbc_pal_layout->addWidget(dmg_gbc_pal);
	dmg_gbc_pal_set->setLayout(dmg_gbc_pal_layout);

	//Display settings - OpenGL Fragment Shader
	QWidget* ogl_frag_shader_set = new QWidget(display);
	QLabel* ogl_frag_shader_label = new QLabel("Post-Processing Shader : ");
	ogl_frag_shader = new QComboBox(ogl_frag_shader_set);
	ogl_frag_shader->setToolTip("Applies an OpenGL GLSL post-processing shader for graphical effects.");
	ogl_frag_shader->addItem("OFF");
	ogl_frag_shader->addItem("2xBR");
	ogl_frag_shader->addItem("4xBR");
	ogl_frag_shader->addItem("Bad Bloom");
	ogl_frag_shader->addItem("Badder Bloom");
	ogl_frag_shader->addItem("Chrono");
	ogl_frag_shader->addItem("DMG Mode");
	ogl_frag_shader->addItem("GBA Gamma");
	ogl_frag_shader->addItem("GBC Gamma");
	ogl_frag_shader->addItem("Grayscale");
	ogl_frag_shader->addItem("LCD Mode");
	ogl_frag_shader->addItem("Pastel");
	ogl_frag_shader->addItem("Scale2x");
	ogl_frag_shader->addItem("Scale3x");
	ogl_frag_shader->addItem("Sepia");
	ogl_frag_shader->addItem("Spotlight");
	ogl_frag_shader->addItem("TV Mode");
	ogl_frag_shader->addItem("Washout");

	QHBoxLayout* ogl_frag_shader_layout = new QHBoxLayout;
	ogl_frag_shader_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	ogl_frag_shader_layout->addWidget(ogl_frag_shader_label);
	ogl_frag_shader_layout->addWidget(ogl_frag_shader);
	ogl_frag_shader_set->setLayout(ogl_frag_shader_layout);

	//Display settings - OpenGL Vertex Shader
	QWidget* ogl_vert_shader_set = new QWidget(display);
	QLabel* ogl_vert_shader_label = new QLabel("Vertex Shader : ");
	ogl_vert_shader = new QComboBox(ogl_vert_shader_set);
	ogl_vert_shader->setToolTip("Applies an OpenGL GLSL vertex shader to change screen positions");
	ogl_vert_shader->addItem("Default");
	ogl_vert_shader->addItem("X Inverse");

	QHBoxLayout* ogl_vert_shader_layout = new QHBoxLayout;
	ogl_vert_shader_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	ogl_vert_shader_layout->addWidget(ogl_vert_shader_label);
	ogl_vert_shader_layout->addWidget(ogl_vert_shader);
	ogl_vert_shader_set->setLayout(ogl_vert_shader_layout);

	//Display settings - Use OpenGL
	QWidget* ogl_set = new QWidget(display);
	QLabel* ogl_label = new QLabel("Use OpenGL");
	ogl = new QCheckBox(ogl_set);
	ogl->setToolTip("Draw the screen using OpenGL.\n Allows for faster drawing operations and shader support.");

	QHBoxLayout* ogl_layout = new QHBoxLayout;
	ogl_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	ogl_layout->addWidget(ogl);
	ogl_layout->addWidget(ogl_label);
	ogl_set->setLayout(ogl_layout);

	//Display settings - Use CGFX
	QWidget* load_cgfx_set = new QWidget(display);
	QLabel* load_cgfx_label = new QLabel("Load Custom Graphics (CGFX)");
	load_cgfx = new QCheckBox(load_cgfx_set);
	load_cgfx->setToolTip("Enables custom graphics and loads them when booting a game");

	QHBoxLayout* load_cgfx_layout = new QHBoxLayout;
	load_cgfx_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	load_cgfx_layout->addWidget(load_cgfx);
	load_cgfx_layout->addWidget(load_cgfx_label);
	load_cgfx_set->setLayout(load_cgfx_layout);

	//Display settings - Aspect Ratio
	QWidget* aspect_set = new QWidget(display);
	QLabel* aspect_label = new QLabel("Maintain Aspect Ratio");
	aspect_ratio = new QCheckBox(aspect_set);
	aspect_ratio->setToolTip("Forces GBE+ to maintain the original system's aspect ratio");

	QHBoxLayout* aspect_layout = new QHBoxLayout;
	aspect_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	aspect_layout->addWidget(aspect_ratio);
	aspect_layout->addWidget(aspect_label);
	aspect_set->setLayout(aspect_layout);

	//Display settings - On-Screen Display
	QWidget* osd_set = new QWidget(display);
	QLabel* osd_label = new QLabel("Enable On-Screen Display Messages");
	osd_enable = new QCheckBox(osd_set);
	osd_enable->setToolTip("Displays messages from the emulator on-screen");

	QHBoxLayout* osd_layout = new QHBoxLayout;
	osd_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	osd_layout->addWidget(osd_enable);
	osd_layout->addWidget(osd_label);
	osd_set->setLayout(osd_layout);

	QVBoxLayout* disp_layout = new QVBoxLayout;
	disp_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	disp_layout->addWidget(screen_scale_set);
	disp_layout->addWidget(dmg_gbc_pal_set);
	disp_layout->addWidget(ogl_frag_shader_set);
	disp_layout->addWidget(ogl_vert_shader_set);
	disp_layout->addWidget(ogl_set);
	disp_layout->addWidget(load_cgfx_set);
	disp_layout->addWidget(aspect_set);
	disp_layout->addWidget(osd_set);
	display->setLayout(disp_layout);

	//Sound settings - Output frequency
	QWidget* freq_set = new QWidget(sound);
	QLabel* freq_label = new QLabel("Output Frequency : ");
	freq = new QComboBox(freq_set);
	freq->setToolTip("Selects the final output frequency of all sound.");
	freq->addItem("48000Hz");
	freq->addItem("44100Hz");
	freq->addItem("22050Hz");
	freq->addItem("11025Hz");
	freq->setCurrentIndex(1);

	QHBoxLayout* freq_layout = new QHBoxLayout;
	freq_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	freq_layout->addWidget(freq_label);
	freq_layout->addWidget(freq);
	freq_set->setLayout(freq_layout);

	//Sound settings - Sound samples
	QWidget* sample_set = new QWidget(sound);
	QLabel* sample_label = new QLabel("Sound Samples : ");
	sound_samples = new QSpinBox(sample_set);
	sound_samples->setToolTip("Overrides the number of sound samples SDL will use for all cores. This overrides default values.\nUsers are advised to leave this value at zero.");
	sound_samples->setMaximum(4096);
	sound_samples->setMinimum(0);

	QHBoxLayout* sample_layout = new QHBoxLayout;
	sample_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	sample_layout->addWidget(sample_label);
	sample_layout->addWidget(sound_samples);
	sample_set->setLayout(sample_layout);

	//Sound settings - Sound on/off
	QWidget* sound_on_set = new QWidget(sound);
	QLabel* sound_on_label = new QLabel("Enable Sound");
	sound_on = new QCheckBox(sound_on_set);
	sound_on->setToolTip("Enables all sounds output.");
	sound_on->setChecked(true);

	QHBoxLayout* sound_on_layout = new QHBoxLayout;
	sound_on_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	sound_on_layout->addWidget(sound_on);
	sound_on_layout->addWidget(sound_on_label);
	sound_on_set->setLayout(sound_on_layout);

	//Sound settings - Enable stereo sound
	QWidget* stereo_enable_set = new QWidget(sound);
	QLabel* stereo_enable_label = new QLabel("Enable Stereo Output");
	stereo_enable = new QCheckBox(stereo_enable_set);
	stereo_enable->setToolTip("Enables stereo sound output.");
	stereo_enable->setChecked(true);

	QHBoxLayout* stereo_enable_layout = new QHBoxLayout;
	stereo_enable_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	stereo_enable_layout->addWidget(stereo_enable);
	stereo_enable_layout->addWidget(stereo_enable_label);
	stereo_enable_set->setLayout(stereo_enable_layout);

	//Sound settings - Volume
	QWidget* volume_set = new QWidget(sound);
	QLabel* volume_label = new QLabel("Volume : ");
	volume = new QSlider(sound);
	volume->setToolTip("Master volume for GBE+");
	volume->setMaximum(128);
	volume->setMinimum(0);
	volume->setValue(128);
	volume->setOrientation(Qt::Horizontal);

	QHBoxLayout* volume_layout = new QHBoxLayout;
	volume_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	volume_layout->addWidget(volume_label);
	volume_layout->addWidget(volume);
	volume_set->setLayout(volume_layout);

	QVBoxLayout* audio_layout = new QVBoxLayout;
	audio_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	audio_layout->addWidget(freq_set);
	audio_layout->addWidget(sample_set);
	audio_layout->addWidget(sound_on_set);
	audio_layout->addWidget(stereo_enable_set);
	audio_layout->addWidget(volume_set);
	sound->setLayout(audio_layout);

	//Control settings - Device
	input_device_set = new QWidget(controls);
	QLabel* input_device_label = new QLabel("Input Device : ");
	input_device = new QComboBox(input_device_set);
	input_device->setToolTip("Selects the current input device to configure");
	input_device->addItem("Keyboard");
	input_device->installEventFilter(this);

	QHBoxLayout* input_device_layout = new QHBoxLayout;
	input_device_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	input_device_layout->addWidget(input_device_label);
	input_device_layout->addWidget(input_device);
	input_device_set->setLayout(input_device_layout);

	//Control settings- Dead-zone
	dead_zone_set = new QWidget(controls);
	QLabel* dead_zone_label = new QLabel("Dead Zone : ");
	dead_zone = new QSlider(controls);
	dead_zone->setToolTip("Sets the dead-zone for joystick axes.");
	dead_zone->setMaximum(32767);
	dead_zone->setMinimum(0);
	dead_zone->setValue(16000);
	dead_zone->setOrientation(Qt::Horizontal);

	QHBoxLayout* dead_zone_layout = new QHBoxLayout;
	dead_zone_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	dead_zone_layout->addWidget(dead_zone_label);
	dead_zone_layout->addWidget(dead_zone);
	dead_zone_layout->setContentsMargins(6, 0, 0, 0);
	dead_zone_set->setLayout(dead_zone_layout);

	//Control settings - A button
	input_a_set = new QWidget(controls);
	QLabel* input_a_label = new QLabel("Button A : ");
	input_a = new QLineEdit(controls);
	config_a = new QPushButton("Configure");
	input_a->setMaximumWidth(100);
	config_a->setMaximumWidth(100);

	QHBoxLayout* input_a_layout = new QHBoxLayout;
	input_a_layout->addWidget(input_a_label, 0, Qt::AlignLeft);
	input_a_layout->addWidget(input_a, 0, Qt::AlignLeft);
	input_a_layout->addWidget(config_a, 0, Qt::AlignLeft);
	input_a_layout->setContentsMargins(6, 0, 0, 0);
	input_a_set->setLayout(input_a_layout);

	//Control settings - B button
	input_b_set = new QWidget(controls);
	QLabel* input_b_label = new QLabel("Button B : ");
	input_b = new QLineEdit(controls);
	config_b = new QPushButton("Configure");
	input_b->setMaximumWidth(100);
	config_b->setMaximumWidth(100);

	QHBoxLayout* input_b_layout = new QHBoxLayout;
	input_b_layout->addWidget(input_b_label, 0, Qt::AlignLeft);
	input_b_layout->addWidget(input_b, 0, Qt::AlignLeft);
	input_b_layout->addWidget(config_b, 0, Qt::AlignLeft);
	input_b_layout->setContentsMargins(6, 0, 0, 0);
	input_b_set->setLayout(input_b_layout);

	//Control settings - START button
	input_start_set = new QWidget(controls);
	QLabel* input_start_label = new QLabel("START : ");
	input_start = new QLineEdit(controls);
	config_start = new QPushButton("Configure");
	input_start->setMaximumWidth(100);
	config_start->setMaximumWidth(100);

	QHBoxLayout* input_start_layout = new QHBoxLayout;
	input_start_layout->addWidget(input_start_label, 0, Qt::AlignLeft);
	input_start_layout->addWidget(input_start, 0, Qt::AlignLeft);
	input_start_layout->addWidget(config_start, 0, Qt::AlignLeft);
	input_start_layout->setContentsMargins(6, 0, 0, 0);
	input_start_set->setLayout(input_start_layout);

	//Control settings - SELECT button
	input_select_set = new QWidget(controls);
	QLabel* input_select_label = new QLabel("SELECT : ");
	input_select = new QLineEdit(controls);
	config_select = new QPushButton("Configure");
	input_select->setMaximumWidth(100);
	config_select->setMaximumWidth(100);

	QHBoxLayout* input_select_layout = new QHBoxLayout;
	input_select_layout->addWidget(input_select_label, 0, Qt::AlignLeft);
	input_select_layout->addWidget(input_select, 0, Qt::AlignLeft);
	input_select_layout->addWidget(config_select, 0, Qt::AlignLeft);
	input_select_layout->setContentsMargins(6, 0, 0, 0);
	input_select_set->setLayout(input_select_layout);

	//Control settings - Left
	input_left_set = new QWidget(controls);
	QLabel* input_left_label = new QLabel("LEFT : ");
	input_left = new QLineEdit(controls);
	config_left = new QPushButton("Configure");
	input_left->setMaximumWidth(100);
	config_left->setMaximumWidth(100);

	QHBoxLayout* input_left_layout = new QHBoxLayout;
	input_left_layout->addWidget(input_left_label, 0, Qt::AlignLeft);
	input_left_layout->addWidget(input_left, 0, Qt::AlignLeft);
	input_left_layout->addWidget(config_left, 0, Qt::AlignLeft);
	input_left_layout->setContentsMargins(6, 0, 0, 0);
	input_left_set->setLayout(input_left_layout);

	//Control settings - Right
	input_right_set = new QWidget(controls);
	QLabel* input_right_label = new QLabel("RIGHT : ");
	input_right = new QLineEdit(controls);
	config_right = new QPushButton("Configure");
	input_right->setMaximumWidth(100);
	config_right->setMaximumWidth(100);

	QHBoxLayout* input_right_layout = new QHBoxLayout;
	input_right_layout->addWidget(input_right_label, 0, Qt::AlignLeft);
	input_right_layout->addWidget(input_right, 0, Qt::AlignLeft);
	input_right_layout->addWidget(config_right, 0, Qt::AlignLeft);
	input_right_layout->setContentsMargins(6, 0, 0, 0);
	input_right_set->setLayout(input_right_layout);

	//Control settings - Up
	input_up_set = new QWidget(controls);
	QLabel* input_up_label = new QLabel("UP : ");
	input_up = new QLineEdit(controls);
	config_up = new QPushButton("Configure");
	input_up->setMaximumWidth(100);
	config_up->setMaximumWidth(100);

	QHBoxLayout* input_up_layout = new QHBoxLayout;
	input_up_layout->addWidget(input_up_label, 0, Qt::AlignLeft);
	input_up_layout->addWidget(input_up, 0, Qt::AlignLeft);
	input_up_layout->addWidget(config_up, 0, Qt::AlignLeft);
	input_up_layout->setContentsMargins(6, 0, 0, 0);
	input_up_set->setLayout(input_up_layout);

	//Control settings - Down
	input_down_set = new QWidget(controls);
	QLabel* input_down_label = new QLabel("DOWN : ");
	input_down = new QLineEdit(controls);
	config_down = new QPushButton("Configure");
	input_down->setMaximumWidth(100);
	config_down->setMaximumWidth(100);

	QHBoxLayout* input_down_layout = new QHBoxLayout;
	input_down_layout->addWidget(input_down_label, 0, Qt::AlignLeft);
	input_down_layout->addWidget(input_down, 0, Qt::AlignLeft);
	input_down_layout->addWidget(config_down, 0, Qt::AlignLeft);
	input_down_layout->setContentsMargins(6, 0, 0, 0);
	input_down_set->setLayout(input_down_layout);

	//Advanced control settings - Enable rumble
	rumble_set = new QWidget(controls);
	QLabel* rumble_label = new QLabel("Enable rumble", rumble_set);
	rumble_on = new QCheckBox(rumble_set);

	QHBoxLayout* rumble_layout = new QHBoxLayout;
	rumble_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	rumble_layout->addWidget(rumble_on);
	rumble_layout->addWidget(rumble_label);
	rumble_set->setLayout(rumble_layout);

	//Hotkey settings - Turbo
	hotkey_turbo_set = new QWidget(controls);
	QLabel* hotkey_turbo_label = new QLabel("Toggle Turbo : ");
	input_turbo = new QLineEdit(controls);
	config_turbo = new QPushButton("Configure");
	input_turbo->setMaximumWidth(100);
	config_turbo->setMaximumWidth(100);

	QHBoxLayout* hotkey_turbo_layout = new QHBoxLayout;
	hotkey_turbo_layout->addWidget(hotkey_turbo_label, 1, Qt::AlignLeft);
	hotkey_turbo_layout->addWidget(input_turbo, 1, Qt::AlignLeft);
	hotkey_turbo_layout->addWidget(config_turbo, 1, Qt::AlignLeft);
	hotkey_turbo_layout->setContentsMargins(6, 0, 0, 0);
	hotkey_turbo_set->setLayout(hotkey_turbo_layout);

	//Hotkey settings - Mute
	hotkey_mute_set = new QWidget(controls);
	QLabel* hotkey_mute_label = new QLabel("Toggle Mute : ");
	input_mute = new QLineEdit(controls);
	config_mute = new QPushButton("Configure");
	input_mute->setMaximumWidth(100);
	config_mute->setMaximumWidth(100);

	QHBoxLayout* hotkey_mute_layout = new QHBoxLayout;
	hotkey_mute_layout->addWidget(hotkey_mute_label, 1, Qt::AlignLeft);
	hotkey_mute_layout->addWidget(input_mute, 1, Qt::AlignLeft);
	hotkey_mute_layout->addWidget(config_mute, 1, Qt::AlignLeft);
	hotkey_mute_layout->setContentsMargins(6, 0, 0, 0);
	hotkey_mute_set->setLayout(hotkey_mute_layout);

	controls_layout = new QVBoxLayout;
	controls_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	controls_layout->addWidget(input_device_set);
	controls_layout->addWidget(input_a_set);
	controls_layout->addWidget(input_b_set);
	controls_layout->addWidget(input_start_set);
	controls_layout->addWidget(input_select_set);
	controls_layout->addWidget(input_left_set);
	controls_layout->addWidget(input_right_set);
	controls_layout->addWidget(input_up_set);
	controls_layout->addWidget(input_down_set);
	controls_layout->addWidget(dead_zone_set);
	controls->setLayout(controls_layout);

	advanced_controls_layout = new QVBoxLayout;
	advanced_controls_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	advanced_controls_layout->addWidget(rumble_set);

	hotkey_controls_layout = new QVBoxLayout;
	hotkey_controls_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	hotkey_controls_layout->addWidget(hotkey_turbo_set);
	hotkey_controls_layout->addWidget(hotkey_mute_set);
	
	rumble_set->setVisible(false);

	hotkey_turbo_set->setVisible(false);
	hotkey_mute_set->setVisible(false);

	//Path settings - Screenshot
	QWidget* screenshot_set = new QWidget(paths);
	screenshot_label = new QLabel("Screenshots :  ");
	QPushButton* screenshot_button = new QPushButton("Browse");
	screenshot = new QLineEdit(paths);

	QHBoxLayout* screenshot_layout = new QHBoxLayout;
	screenshot_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	screenshot_layout->addWidget(screenshot_label);
	screenshot_layout->addWidget(screenshot);
	screenshot_layout->addWidget(screenshot_button);
	screenshot_set->setLayout(screenshot_layout);

	//Path settings - Game saves
	QWidget* game_saves_set = new QWidget(paths);
	game_saves_label = new QLabel("Game Saves :  ");
	QPushButton* game_saves_button = new QPushButton("Browse");
	game_saves = new QLineEdit(paths);

	QHBoxLayout* game_saves_layout = new QHBoxLayout;
	game_saves_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	game_saves_layout->addWidget(game_saves_label);
	game_saves_layout->addWidget(game_saves);
	game_saves_layout->addWidget(game_saves_button);
	game_saves_set->setLayout(game_saves_layout);

	//Path settings - Cheats file
	QWidget* cheats_path_set = new QWidget(paths);
	cheats_path_label = new QLabel("Cheats File :  ");
	QPushButton* cheats_path_button = new QPushButton("Browse");
	cheats_path = new QLineEdit(paths);

	QHBoxLayout* cheats_path_layout = new QHBoxLayout;
	cheats_path_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	cheats_path_layout->addWidget(cheats_path_label);
	cheats_path_layout->addWidget(cheats_path);
	cheats_path_layout->addWidget(cheats_path_button);
	cheats_path_set->setLayout(cheats_path_layout);

	QVBoxLayout* paths_layout = new QVBoxLayout;
	paths_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	paths_layout->addWidget(screenshot_set);
	paths_layout->addWidget(game_saves_set);
	paths_layout->addWidget(cheats_path_set);
	paths->setLayout(paths_layout);

	//Setup warning message box
	warning_box = new QMessageBox;
	QPushButton* warning_box_ok = warning_box->addButton("OK", QMessageBox::AcceptRole);
	warning_box->setIcon(QMessageBox::Warning);
	warning_box->hide();

	data_folder = new data_dialog;

	connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(close_input()));
	connect(tabs_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(tabs_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(tabs_button->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(close_settings()));
	connect(overclock, SIGNAL(currentIndexChanged(int)), this, SLOT(overclock_change()));
	connect(auto_patch, SIGNAL(stateChanged(int)), this, SLOT(set_patches()));
	connect(edit_cheats, SIGNAL(clicked()), this, SLOT(show_cheats()));
	connect(edit_rtc, SIGNAL(clicked()), this, SLOT(show_rtc()));
	connect(ogl, SIGNAL(stateChanged(int)), this, SLOT(set_ogl()));
	connect(screen_scale, SIGNAL(currentIndexChanged(int)), this, SLOT(screen_scale_change()));
	connect(aspect_ratio, SIGNAL(stateChanged(int)), this, SLOT(aspect_ratio_change()));
	connect(osd_enable, SIGNAL(stateChanged(int)), this, SLOT(set_osd()));
	connect(dmg_gbc_pal, SIGNAL(currentIndexChanged(int)), this, SLOT(dmg_gbc_pal_change()));
	connect(ogl_frag_shader, SIGNAL(currentIndexChanged(int)), this, SLOT(ogl_frag_change()));
	connect(ogl_vert_shader, SIGNAL(currentIndexChanged(int)), this, SLOT(ogl_vert_change()));
	connect(load_cgfx, SIGNAL(stateChanged(int)), this, SLOT(set_cgfx()));
	connect(volume, SIGNAL(valueChanged(int)), this, SLOT(volume_change()));
	connect(freq, SIGNAL(currentIndexChanged(int)), this, SLOT(sample_rate_change()));
	connect(sound_samples, SIGNAL(valueChanged(int)), this, SLOT(sample_size_change()));
	connect(sound_on, SIGNAL(stateChanged(int)), this, SLOT(mute()));
	connect(dead_zone, SIGNAL(valueChanged(int)), this, SLOT(dead_zone_change()));
	connect(input_device, SIGNAL(currentIndexChanged(int)), this, SLOT(input_device_change()));
	connect(controls_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(switch_control_layout()));
	connect(data_folder, SIGNAL(accepted()), this, SLOT(select_folder()));
	connect(data_folder, SIGNAL(rejected()), this, SLOT(reject_folder()));

	QSignalMapper* paths_mapper = new QSignalMapper(this);
	connect(screenshot_button, SIGNAL(clicked()), paths_mapper, SLOT(map()));
	connect(game_saves_button, SIGNAL(clicked()), paths_mapper, SLOT(map()));
	connect(cheats_path_button, SIGNAL(clicked()), paths_mapper, SLOT(map()));

	paths_mapper->setMapping(screenshot_button, 5);
	paths_mapper->setMapping(game_saves_button, 8);
	paths_mapper->setMapping(cheats_path_button, 9);
	connect(paths_mapper, SIGNAL(mapped(int)), this, SLOT(set_paths(int)));

	QSignalMapper* button_config = new QSignalMapper(this);
	connect(config_a, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_b, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_start, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_select, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_left, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_right, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_up, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_down, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_turbo, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_mute, SIGNAL(clicked()), button_config, SLOT(map()));
	
	button_config->setMapping(config_a, 0);
	button_config->setMapping(config_b, 1);
	button_config->setMapping(config_start, 4);
	button_config->setMapping(config_select, 5);
	button_config->setMapping(config_left, 6);
	button_config->setMapping(config_right, 7);
	button_config->setMapping(config_up, 8);
	button_config->setMapping(config_down, 9);
	button_config->setMapping(config_turbo, 18);
	button_config->setMapping(config_mute, 19);
	connect(button_config, SIGNAL(mapped(int)), this, SLOT(configure_button(int))) ;

	//Final tab layout
	QVBoxLayout* main_layout = new QVBoxLayout;
	main_layout->addWidget(tabs);
	main_layout->addWidget(button_set);
	setLayout(main_layout);

	//Config button formatting
	config_a->setMinimumWidth(150);
	config_b->setMinimumWidth(150);
	config_start->setMinimumWidth(150);
	config_select->setMinimumWidth(150);
	config_left->setMinimumWidth(150);
	config_right->setMinimumWidth(150);
	config_up->setMinimumWidth(150);
	config_down->setMinimumWidth(150);
	config_turbo->setMinimumWidth(150);
	config_mute->setMinimumWidth(150);

	input_a->setReadOnly(true);
	input_b->setReadOnly(true);
	input_start->setReadOnly(true);
	input_select->setReadOnly(true);
	input_left->setReadOnly(true);
	input_right->setReadOnly(true);
	input_up->setReadOnly(true);
	input_down->setReadOnly(true);
	input_turbo->setReadOnly(true);
	input_mute->setReadOnly(true);

	//Install event filters
	config_a->installEventFilter(this);
	config_b->installEventFilter(this);
	config_start->installEventFilter(this);
	config_select->installEventFilter(this);
	config_left->installEventFilter(this);
	config_right->installEventFilter(this);
	config_up->installEventFilter(this);
	config_down->installEventFilter(this);
	config_turbo->installEventFilter(this);
	config_mute->installEventFilter(this);

	input_a->installEventFilter(this);
	input_b->installEventFilter(this);
	input_start->installEventFilter(this);
	input_select->installEventFilter(this);
	input_left->installEventFilter(this);
	input_right->installEventFilter(this);
	input_up->installEventFilter(this);
	input_down->installEventFilter(this);
	input_turbo->installEventFilter(this);
	input_mute->installEventFilter(this);

	//Set focus policies
	config_a->setFocusPolicy(Qt::NoFocus);
	config_b->setFocusPolicy(Qt::NoFocus);
	config_start->setFocusPolicy(Qt::NoFocus);
	config_select->setFocusPolicy(Qt::NoFocus);
	config_left->setFocusPolicy(Qt::NoFocus);
	config_right->setFocusPolicy(Qt::NoFocus);
	config_up->setFocusPolicy(Qt::NoFocus);
	config_down->setFocusPolicy(Qt::NoFocus);
	config_turbo->setFocusPolicy(Qt::NoFocus);
	config_mute->setFocusPolicy(Qt::NoFocus);

	input_a->setFocusPolicy(Qt::NoFocus);
	input_b->setFocusPolicy(Qt::NoFocus);
	input_start->setFocusPolicy(Qt::NoFocus);
	input_select->setFocusPolicy(Qt::NoFocus);
	input_left->setFocusPolicy(Qt::NoFocus);
	input_right->setFocusPolicy(Qt::NoFocus);
	input_up->setFocusPolicy(Qt::NoFocus);
	input_down->setFocusPolicy(Qt::NoFocus);
	input_turbo->setFocusPolicy(Qt::NoFocus);
	input_mute->setFocusPolicy(Qt::NoFocus);

	//Joystick handling
	jstick = SDL_JoystickOpen(0);
	joystick_count = SDL_NumJoysticks();
	
	for(int x = 0; x < joystick_count; x++)
	{
		SDL_Joystick* jstick = SDL_JoystickOpen(x);
		std::string joy_name = SDL_JoystickName(jstick);
		input_device->addItem(QString::fromStdString(joy_name));
	}

	sample_rate = config::sample_rate;
	resize_screen = false;
	grab_input = false;
	input_type = 0;
	last_control_id = 0;

	dmg_cheat_menu = new cheat_menu;
	real_time_clock_menu = new rtc_menu;
	turbo_file_menu = new tbf_menu;

	resize(450, 450);
	setWindowTitle(tr("GBE+ Settings"));
}

/****** Sets various widgets to values based on the current config paramters (from .ini file) ******/
void gen_settings::set_ini_options()
{
	//Emulated CPU speed
	overclock->setCurrentIndex(config::oc_flags);

	//Enable patches
	if(config::use_patches) { auto_patch->setChecked(true); }
	else { auto_patch->setChecked(false); }

	//Use cheats
	if(config::use_cheats) { cheats->setChecked(true); }
	else { cheats->setChecked(false); }

	//RTC offsets
	real_time_clock_menu->secs_offset->setValue(config::rtc_offset[0]);
	real_time_clock_menu->mins_offset->setValue(config::rtc_offset[1]);
	real_time_clock_menu->hours_offset->setValue(config::rtc_offset[2]);
	real_time_clock_menu->days_offset->setValue(config::rtc_offset[3]);

	//Screen scale options
	screen_scale->setCurrentIndex(config::scaling_factor - 1);

	//DMG-on-GBC palette options
	dmg_gbc_pal->setCurrentIndex(config::dmg_gbc_pal);

	//OpenGL Fragment Shader
	if(config::fragment_shader == (config::data_path + "shaders/fragment.fs")) { ogl_frag_shader->setCurrentIndex(0); }
	else if(config::fragment_shader == (config::data_path + "shaders/2xBR.fs")) { ogl_frag_shader->setCurrentIndex(1); }
	else if(config::fragment_shader == (config::data_path + "shaders/4xBR.fs")) { ogl_frag_shader->setCurrentIndex(2); }
	else if(config::fragment_shader == (config::data_path + "shaders/bad_bloom.fs")) { ogl_frag_shader->setCurrentIndex(3); }
	else if(config::fragment_shader == (config::data_path + "shaders/badder_bloom.fs")) { ogl_frag_shader->setCurrentIndex(4); }
	else if(config::fragment_shader == (config::data_path + "shaders/chrono.fs")) { ogl_frag_shader->setCurrentIndex(5); }
	else if(config::fragment_shader == (config::data_path + "shaders/dmg_mode.fs")) { ogl_frag_shader->setCurrentIndex(6); }
	else if(config::fragment_shader == (config::data_path + "shaders/gba_gamma.fs")) { ogl_frag_shader->setCurrentIndex(7); }
	else if(config::fragment_shader == (config::data_path + "shaders/gbc_gamma.fs")) { ogl_frag_shader->setCurrentIndex(8); }
	else if(config::fragment_shader == (config::data_path + "shaders/grayscale.fs")) { ogl_frag_shader->setCurrentIndex(9); }
	else if(config::fragment_shader == (config::data_path + "shaders/lcd_mode.fs")) { ogl_frag_shader->setCurrentIndex(10); }
	else if(config::fragment_shader == (config::data_path + "shaders/pastel.fs")) { ogl_frag_shader->setCurrentIndex(11); }
	else if(config::fragment_shader == (config::data_path + "shaders/scale2x.fs")) { ogl_frag_shader->setCurrentIndex(12); }
	else if(config::fragment_shader == (config::data_path + "shaders/scale3x.fs")) { ogl_frag_shader->setCurrentIndex(13); }
	else if(config::fragment_shader == (config::data_path + "shaders/sepia.fs")) { ogl_frag_shader->setCurrentIndex(14); }
	else if(config::fragment_shader == (config::data_path + "shaders/spotlight.fs")) { ogl_frag_shader->setCurrentIndex(15); }
	else if(config::fragment_shader == (config::data_path + "shaders/tv_mode.fs")) { ogl_frag_shader->setCurrentIndex(16); }
	else if(config::fragment_shader == (config::data_path + "shaders/washout.fs")) { ogl_frag_shader->setCurrentIndex(17); }

	//OpenGL Vertex Shader
	if(config::vertex_shader == (config::data_path + "shaders/vertex.fs")) { ogl_vert_shader->setCurrentIndex(0); }
	else if(config::vertex_shader == (config::data_path + "shaders/invert_x.vs")) { ogl_vert_shader->setCurrentIndex(1); }

	//OpenGL option
	if(config::use_opengl)
	{
		ogl->setChecked(true);
		ogl_frag_shader->setEnabled(true);
		ogl_vert_shader->setEnabled(true);
	}

	else
	{
		ogl->setChecked(false);
		ogl_frag_shader->setEnabled(false);
		ogl_vert_shader->setEnabled(false);
	}

	//CGFX option
	if(cgfx::load_cgfx)
	{
		load_cgfx->setChecked(true);
	}

	else
	{
		load_cgfx->setChecked(false);
	}

	//Maintain aspect ratio option
	if(config::maintain_aspect_ratio) { aspect_ratio->setChecked(true); }
	else { aspect_ratio->setChecked(false); }

	//OSD option
	if(config::use_osd) { osd_enable->setChecked(true); }
	else { osd_enable->setChecked(false); }

	//Sample rate option
	switch((int)config::sample_rate)
	{
		case 11025: freq->setCurrentIndex(3); break;
		case 22050: freq->setCurrentIndex(2); break;
		case 44100: freq->setCurrentIndex(1); break;
		case 48000: freq->setCurrentIndex(0); break;
	}

	//Sample size
	sound_samples->setValue(config::sample_size);

	//Volume option
	volume->setValue(config::volume);

	//Mute option
	if(config::mute == 1)
	{
		config::volume = 0;
		sound_on->setChecked(false);
		volume->setEnabled(false);
	}

	else
	{
		sound_on->setChecked(true);
		volume->setEnabled(true);
	}

	//Stereo sound option
	if(config::use_stereo) { stereo_enable->setChecked(true); }
	else { stereo_enable->setChecked(false); }

	//Dead-zone
	dead_zone->setValue(config::dead_zone);

	//Keyboard controls
	input_a->setText(QString::number(config::gbe_key_a));
	input_b->setText(QString::number(config::gbe_key_b));
	input_start->setText(QString::number(config::gbe_key_start));
	input_select->setText(QString::number(config::gbe_key_select));
	input_left->setText(QString::number(config::gbe_key_left));
	input_right->setText(QString::number(config::gbe_key_right));
	input_up->setText(QString::number(config::gbe_key_up));
	input_down->setText(QString::number(config::gbe_key_down));
	input_turbo->setText(QString::number(config::hotkey_turbo));
	input_mute->setText(QString::number(config::hotkey_mute));

	//BIOS, Boot ROM and Manifest paths
	QString path_6(QString::fromStdString(config::ss_path));
	QString path_9(QString::fromStdString(config::save_path));
	QString path_10(QString::fromStdString(config::cheats_path));

	//Rumble
	if(config::use_haptics) { rumble_on->setChecked(true); }
	else { rumble_on->setChecked(false); }

	screenshot->setText(path_6);
	game_saves->setText(path_9);
	cheats_path->setText(path_10);
}



/****** Changes the emulated CPU speed ******/
void gen_settings::overclock_change()
{
	config::oc_flags = overclock->currentIndex();
}

/****** Toggles whether to enable auto-patching ******/
void gen_settings::set_patches()
{
	if(auto_patch->isChecked()) { config::use_patches = true; }
	else { config::use_patches = false; }
}

/****** Displays the cheats window ******/
void gen_settings::show_cheats()
{
	dmg_cheat_menu->fetch_cheats();
	dmg_cheat_menu->show();
}

/****** Displays the cheats window ******/
void gen_settings::show_rtc()
{
	real_time_clock_menu->show();
}

/****** Toggles enabling or disabling the fragment and vertex shader widgets when setting OpenGL ******/
void gen_settings::set_ogl()
{
	if(ogl->isChecked())
	{
		ogl_frag_shader->setEnabled(true);
		ogl_vert_shader->setEnabled(true);
	}

	else
	{
		ogl_frag_shader->setEnabled(false);
		ogl_frag_shader->setEnabled(false);
	}
}

/****** Changes the display scale ******/
void gen_settings::screen_scale_change()
{
	config::scaling_factor = (screen_scale->currentIndex() + 1);
	resize_screen = true;
}

/****** Sets whether to maintain the aspect ratio or not ******/
void gen_settings::aspect_ratio_change()
{
	if(aspect_ratio->isChecked()) { config::maintain_aspect_ratio = true; }
	else { config::maintain_aspect_ratio = false; }
}

/****** Toggles enabling or disabling on-screen display messages ******/
void gen_settings::set_osd()
{
	if(osd_enable->isChecked()) { config::use_osd = true; }
	else { config::use_osd = false; }
}

/****** Changes the emulated DMG-on-GBC palette ******/
void gen_settings::dmg_gbc_pal_change()
{
	config::dmg_gbc_pal = (dmg_gbc_pal->currentIndex());
	set_dmg_colors(config::dmg_gbc_pal);
}

/****** Changes the current OpenGL fragment shader ******/
void gen_settings::ogl_frag_change()
{
	switch(ogl_frag_shader->currentIndex())
	{
		case 0: config::fragment_shader = config::data_path + "shaders/fragment.fs"; break;
		case 1: config::fragment_shader = config::data_path + "shaders/2xBR.fs"; break;
		case 2: config::fragment_shader = config::data_path + "shaders/4xBR.fs"; break;
		case 3: config::fragment_shader = config::data_path + "shaders/bad_bloom.fs"; break;
		case 4: config::fragment_shader = config::data_path + "shaders/badder_bloom.fs"; break;
		case 5: config::fragment_shader = config::data_path + "shaders/chrono.fs"; break;
		case 6: config::fragment_shader = config::data_path + "shaders/dmg_mode.fs"; break;
		case 7: config::fragment_shader = config::data_path + "shaders/gba_gamma.fs"; break;
		case 8: config::fragment_shader = config::data_path + "shaders/gbc_gamma.fs"; break;
		case 9: config::fragment_shader = config::data_path + "shaders/grayscale.fs"; break;
		case 10: config::fragment_shader = config::data_path + "shaders/lcd_mode.fs"; break;
		case 11: config::fragment_shader = config::data_path + "shaders/pastel.fs"; break;
		case 12: config::fragment_shader = config::data_path + "shaders/scale2x.fs"; break;
		case 13: config::fragment_shader = config::data_path + "shaders/scale3x.fs"; break;
		case 14: config::fragment_shader = config::data_path + "shaders/sepia.fs"; break;
		case 15: config::fragment_shader = config::data_path + "shaders/spotlight.fs"; break;
		case 16: config::fragment_shader = config::data_path + "shaders/tv_mode.fs"; break;
		case 17: config::fragment_shader = config::data_path + "shaders/washout.fs"; break;
	}

	if((main_menu::gbe_plus != NULL) && (config::use_opengl))
	{
		qt_gui::draw_surface->hw_screen->reload_shaders();
		qt_gui::draw_surface->hw_screen->update();
	}
}

/****** Changes the current OpenGL vertex shader ******/
void gen_settings::ogl_vert_change()
{
	switch(ogl_vert_shader->currentIndex())
	{
		case 0: config::vertex_shader = config::data_path + "shaders/vertex.vs"; break;
		case 1: config::vertex_shader = config::data_path + "shaders/invert_x.vs"; break;
	}

	if((main_menu::gbe_plus != NULL) && (config::use_opengl))
	{
		qt_gui::draw_surface->hw_screen->reload_shaders();
		qt_gui::draw_surface->hw_screen->update();
	}
}

/****** Toggles activation of custom graphics ******/
void gen_settings::set_cgfx()
{
	if(load_cgfx->isChecked())
	{
		cgfx::load_cgfx = true;
	}

	else
	{
		cgfx::load_cgfx = false;
	}
}

/****** Dynamically changes the core's volume ******/
void gen_settings::volume_change() 
{
	//Update volume while playing
	if((main_menu::gbe_plus != NULL) && (sound_on->isChecked()))
	{
		main_menu::gbe_plus->update_volume(volume->value());
	}

	//Update the volume while using only the GUI
	else { config::volume = volume->value(); }	
}

/****** Updates the core's volume - Used when loading save states ******/
void gen_settings::update_volume() { mute(); }

/****** Mutes the core's volume ******/
void gen_settings::mute()
{
	//Mute/unmute while playing
	if(main_menu::gbe_plus != NULL)
	{
		//Unmute, use slider volume
		if(sound_on->isChecked())
		{
			main_menu::gbe_plus->update_volume(volume->value());
			volume->setEnabled(true);
		}

		//Mute
		else
		{
			main_menu::gbe_plus->update_volume(0);
			volume->setEnabled(false);
		}
	}

	//Mute/unmute while using only the GUI
	else
	{
		//Unmute, use slider volume
		if(sound_on->isChecked())
		{
			config::volume = volume->value();
			volume->setEnabled(true);
		}

		//Mute
		else
		{
			config::volume = 0;
			volume->setEnabled(false);
		}
	}
}

/****** Changes the core's sample rate ******/
void gen_settings::sample_rate_change()
{
	switch(freq->currentIndex())
	{
		case 0: sample_rate = 48000.0; break;
		case 1: sample_rate = 44100.0; break;
		case 2: sample_rate = 22050.0; break;
		case 3: sample_rate = 11025.0; break;
	}
}

/****** Changes the core's sample size ******/
void gen_settings::sample_size_change()
{
	config::sample_size = sound_samples->value();
}

/****** Sets a path via file browser ******/
void gen_settings::set_paths(int index)
{
	QString path;

	//Open file browser for Boot ROMs, cheats, and manifests
	if((index < 5) || (index >= 9))
	{
		path = QFileDialog::getOpenFileName(this, tr("Open"), "", tr("All files (*)"));
		if(path.isNull()) { return; }
	}

	//Open folder browser for screenshots, CGFX dumps, game saves
	else
	{
		//Open the data folder for CGFX dumps
		//On Linux or Unix, this is supposed to be a hidden folder, so we need a custom dialog
		//This uses relative paths, but for game saves we need full path, so ignore if index is 8
		if((index >= 6) && (index != 8))
		{
			data_folder->open_data_folder();			

			while(!data_folder->finish) { QApplication::processEvents(); }
	
			path = data_folder->directory().path();
			path = data_folder->path.relativeFilePath(path);
		}

		else { path = QFileDialog::getExistingDirectory(this, tr("Open"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks); }
		
		if(path.isNull()) { return; }	

		//Make sure path is complete, e.g. has the correct separator at the end
		//Qt doesn't append this automatically
		std::string temp_str = path.toStdString();
		std::string temp_chr = "";
		temp_chr = temp_str[temp_str.length() - 1];

		if((temp_chr != "/") && (temp_chr != "\\")) { path.append("/"); }
		path = QDir::toNativeSeparators(path);
	}

	switch(index)
	{
		case 5:
			config::ss_path = path.toStdString();
			screenshot->setText(path);
			break;
		case 8:
			config::save_path = path.toStdString();
			game_saves->setText(path);
			break;

		case 9:
			config::cheats_path = path.toStdString();
			cheats_path->setText(path);

			//Make sure to update cheats from new file
			if(!dmg_cheat_menu->empty_cheats) { dmg_cheat_menu->empty_cheats = parse_cheats_file(true); }
			else { parse_cheats_file(false); }

			break;

	}
}

/****** Rebuilds input device index ******/
void gen_settings::rebuild_input_index()
{
	SDL_JoystickUpdate();

	//Rebuild input device index
	if(SDL_NumJoysticks() != joystick_count)
	{
		joystick_count = SDL_NumJoysticks();
		input_device->clear();
		input_device->addItem("Keyboard");

		for(int x = 0; x < joystick_count; x++)
		{
			SDL_Joystick* jstick = SDL_JoystickOpen(x);
			std::string joy_name = SDL_JoystickName(jstick);
			input_device->addItem(QString::fromStdString(joy_name));
			
			if(x == 0) { config::joy_sdl_id = SDL_JoystickInstanceID(jstick); }
		}

		//Set input index to 0
		input_device->setCurrentIndex(0);

		//Set joy id to 0 until new joystick selected
		config::joy_id = 0;
	}
}

/****** Changes the input device to configure ******/
void gen_settings::input_device_change()
{
	input_type = input_device->currentIndex();

	//Switch to keyboard input configuration
	if(input_type == 0)
	{
		input_a->setText(QString::number(config::gbe_key_a));
		input_b->setText(QString::number(config::gbe_key_b));
		input_start->setText(QString::number(config::gbe_key_start));
		input_select->setText(QString::number(config::gbe_key_select));
		input_left->setText(QString::number(config::gbe_key_left));
		input_right->setText(QString::number(config::gbe_key_right));
		input_up->setText(QString::number(config::gbe_key_up));
		input_down->setText(QString::number(config::gbe_key_down));
	}

	else
	{
		input_a->setText(QString::number(config::gbe_joy_a));
		input_b->setText(QString::number(config::gbe_joy_b));
		input_start->setText(QString::number(config::gbe_joy_start));
		input_select->setText(QString::number(config::gbe_joy_select));
		input_left->setText(QString::number(config::gbe_joy_left));
		input_right->setText(QString::number(config::gbe_joy_right));
		input_up->setText(QString::number(config::gbe_joy_up));
		input_down->setText(QString::number(config::gbe_joy_down));

		//Use new joystick id
		config::joy_id = input_device->currentIndex() - 1;
		jstick = SDL_JoystickOpen(config::joy_id);
		config::joy_sdl_id = SDL_JoystickInstanceID(jstick);
	}

	close_input();
}

/****** Dynamically changes the core pad's dead-zone ******/
void gen_settings::dead_zone_change() { config::dead_zone = dead_zone->value(); }

/****** Prepares GUI to receive input for controller configuration ******/
void gen_settings::configure_button(int button)
{
	if(grab_input) { return; }

	grab_input = true;

	switch(button)
	{
		case 0: 
			input_delay(config_a);
			input_a->setFocus();
			input_index = 0;
			break;

		case 1: 
			input_delay(config_b);
			input_b->setFocus();
			input_index = 1;
			break;

		case 4: 
			input_delay(config_start);
			input_start->setFocus();
			input_index = 4;
			break;

		case 5: 
			input_delay(config_select);
			input_select->setFocus();
			input_index = 5;
			break;

		case 6: 
			input_delay(config_left);
			input_left->setFocus();
			input_index = 6;
			break;

		case 7: 
			input_delay(config_right);
			input_right->setFocus();
			input_index = 7;
			break;

		case 8: 
			input_delay(config_up);
			input_up->setFocus();
			input_index = 8;
			break;

		case 9: 
			input_delay(config_down);
			input_down->setFocus();
			input_index = 9;
			break;

		case 18:
			input_delay(config_turbo);
			input_turbo->setFocus();
			input_index = 18;
			break;

		case 19:
			input_delay(config_mute);
			input_turbo->setFocus();
			input_index = 19;
			break;
	}

	if(input_type != 0) { process_joystick_event(); }
}			

/****** Delays input from the GUI ******/
void gen_settings::input_delay(QPushButton* input_button)
{
	//Delay input for joysticks
	if(input_type != 0)
	{
		input_button->setText("Enter Input - 3");
		QApplication::processEvents();
		input_button->setText("Enter Input - 3");
		input_button->update();
		QApplication::processEvents();
		SDL_Delay(1000);

		input_button->setText("Enter Input - 2");
		QApplication::processEvents();
		input_button->setText("Enter Input - 2");
		input_button->update();
		QApplication::processEvents();
		SDL_Delay(1000);

		input_button->setText("Enter Input - 1");
		QApplication::processEvents();
		input_button->setText("Enter Input - 1");
		input_button->update();
		QApplication::processEvents();
		SDL_Delay(1000);
	}

	//Grab input immediately for keyboards
	else { input_button->setText("Enter Input"); }	
}

/****** Handles joystick input ******/
void gen_settings::process_joystick_event()
{
	SDL_Event joy_event;
	int pad = 0;

	while(SDL_PollEvent(&joy_event))
	{
		//Generate pad id
		switch(joy_event.type)
		{
			case SDL_JOYBUTTONDOWN:
				pad = 100 + joy_event.jbutton.button; 
				grab_input = false;
				break;

			case SDL_JOYAXISMOTION:
				if(abs(joy_event.jaxis.value) >= config::dead_zone)
				{
					pad = 200 + (joy_event.jaxis.axis * 2);
					if(joy_event.jaxis.value > 0) { pad++; }
					grab_input = false;
				}

				break;

			case SDL_JOYHATMOTION:
				pad = 300 + (joy_event.jhat.hat * 4);
				grab_input = false;
						
				switch(joy_event.jhat.value)
				{
					case SDL_HAT_RIGHT: pad += 1; break;
					case SDL_HAT_UP: pad += 2; break;
					case SDL_HAT_DOWN: pad += 3; break;
				}

				break;
		}
	}

	switch(input_index)
	{
		case 0:
			if(pad != 0)
			{
				config::gbe_joy_a = pad;
				input_a->setText(QString::number(pad));
			}

			config_a->setText("Configure");
			input_a->clearFocus();
			break;

		case 1:
			if(pad != 0)
			{
				config::gbe_joy_b = pad;
				input_b->setText(QString::number(pad));
			}

			config_b->setText("Configure");
			input_b->clearFocus();
			break;

		case 4:
			if(pad != 0)
			{
				config::gbe_joy_start = pad;
				input_start->setText(QString::number(pad));
			}

			config_start->setText("Configure");
			input_start->clearFocus();
			break;

		case 5:
			if(pad != 0)
			{
				config::gbe_joy_select = pad;
				input_select->setText(QString::number(pad));
			}

			config_select->setText("Configure");
			input_select->clearFocus();
			break;

		case 6:
			if(pad != 0)
			{
				config::gbe_joy_left = pad;
				input_left->setText(QString::number(pad));
			}

			config_left->setText("Configure");
			input_left->clearFocus();
			break;

		case 7:
			if(pad != 0)
			{
				config::gbe_joy_right = pad;
				input_right->setText(QString::number(pad));
			}

			config_right->setText("Configure");
			input_right->clearFocus();
			break;

		case 8:
			if(pad != 0)
			{
				config::gbe_joy_up = pad;
				input_up->setText(QString::number(pad));
			}

			config_up->setText("Configure");
			input_up->clearFocus();
			break;

		case 9:
			if(pad != 0)
			{
				config::gbe_joy_down = pad;
				input_down->setText(QString::number(pad));
			}

			config_down->setText("Configure");
			input_down->clearFocus();
			break;

	}

	input_index = -1;
	QApplication::processEvents();
	grab_input = false;
}

/****** Close all pending input configuration ******/
void gen_settings::close_input()
{
	config_a->setText("Configure");
	config_b->setText("Configure");
	config_start->setText("Configure");
	config_select->setText("Configure");
	config_left->setText("Configure");
	config_right->setText("Configure");
	config_up->setText("Configure");
	config_down->setText("Configure");
	config_turbo->setText("Configure");
	config_mute->setText("Configure");

	input_index = -1;
	grab_input = false;

	//Additionally, controls combo to visible or invisible when switching tabs
	if(tabs->currentIndex() == 3) { controls_combo->setVisible(true); }
	else { controls_combo->setVisible(false); }
}

/****** Changes Qt widget layout - Switches between advanced control configuration mode ******/
void gen_settings::switch_control_layout()
{
	//Switch to Advanced Control layout
	if(controls_combo->currentIndex() == 1)
	{
		//Set all advanced control widgets to visible
		for(int x = 0; x < advanced_controls_layout->count(); x++)
		{
			advanced_controls_layout->itemAt(x)->widget()->setVisible(true);
		}

		//Set all standard control widgets to invisible
		for(int x = 0; x < controls_layout->count(); x++)
		{
			controls_layout->itemAt(x)->widget()->setVisible(false);
		}

		//Set all hotkey control widgets to invisible
		for(int x = 0; x < hotkey_controls_layout->count(); x++)
		{
			hotkey_controls_layout->itemAt(x)->widget()->setVisible(false);
		}

		delete controls->layout();
		advanced_controls_layout->insertWidget(0, input_device_set);
		controls->setLayout(advanced_controls_layout);

		input_device_set->setVisible(true);
		input_device_set->setEnabled(true);
	}

	//Switch to Standard Control layout
	else if(controls_combo->currentIndex() == 0)
	{
		//Set all advanced control widgets to invisible
		for(int x = 0; x < advanced_controls_layout->count(); x++)
		{
			advanced_controls_layout->itemAt(x)->widget()->setVisible(false);
		}

		//Set all standard control widgets to visible
		for(int x = 0; x < controls_layout->count(); x++)
		{
			controls_layout->itemAt(x)->widget()->setVisible(true);
		}

		//Set all hotkey control widgets to invisible
		for(int x = 0; x < hotkey_controls_layout->count(); x++)
		{
			hotkey_controls_layout->itemAt(x)->widget()->setVisible(false);
		}

		delete controls->layout();
		controls_layout->insertWidget(0, input_device_set);
		controls->setLayout(controls_layout);

		input_device_set->setVisible(true);
		input_device_set->setEnabled(true);
	}

	//Switch to Hotkey layout
	else if(controls_combo->currentIndex() == 2)
	{
		//Set all advanced control widgets to invisible
		for(int x = 0; x < advanced_controls_layout->count(); x++)
		{
			advanced_controls_layout->itemAt(x)->widget()->setVisible(false);
		}

		//Set all standard control widgets to invisible
		for(int x = 0; x < controls_layout->count(); x++)
		{
			controls_layout->itemAt(x)->widget()->setVisible(false);
		}

		//Set all hotkey control widgets to visible
		for(int x = 0; x < hotkey_controls_layout->count(); x++)
		{
			hotkey_controls_layout->itemAt(x)->widget()->setVisible(true);
		}

		delete controls->layout();
		hotkey_controls_layout->insertWidget(0, input_device_set);
		controls->setLayout(hotkey_controls_layout);

		input_device_set->setVisible(true);
		input_device_set->setEnabled(false);
		input_device->setCurrentIndex(0);
	}

	//Switch to Virtual Cursor layout
	else if(controls_combo->currentIndex() == 4)
	{
		//Set all advanced control widgets to invisible
		for(int x = 0; x < advanced_controls_layout->count(); x++)
		{
			advanced_controls_layout->itemAt(x)->widget()->setVisible(false);
		}

		//Set all standard control widgets to invisible
		for(int x = 0; x < controls_layout->count(); x++)
		{
			controls_layout->itemAt(x)->widget()->setVisible(false);
		}

		//Set all hotkey control widgets to invisible
		for(int x = 0; x < hotkey_controls_layout->count(); x++)
		{
			hotkey_controls_layout->itemAt(x)->widget()->setVisible(false);
		}

		input_device_set->setVisible(true);
		input_device_set->setEnabled(false);
		input_device->setCurrentIndex(0);
	}

	//Rebuild old layout (was deleted above)
	switch(last_control_id)
	{
		case 0:
			controls_layout = new QVBoxLayout;
			controls_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
			controls_layout->addWidget(input_a_set);
			controls_layout->addWidget(input_b_set);
			controls_layout->addWidget(input_start_set);
			controls_layout->addWidget(input_select_set);
			controls_layout->addWidget(input_left_set);
			controls_layout->addWidget(input_right_set);
			controls_layout->addWidget(input_up_set);
			controls_layout->addWidget(input_down_set);
			controls_layout->addWidget(dead_zone_set);
			break;

		case 1:
			advanced_controls_layout = new QVBoxLayout;
			advanced_controls_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
			advanced_controls_layout->addWidget(rumble_set);
			break;

		case 2:
			hotkey_controls_layout = new QVBoxLayout;
			hotkey_controls_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
			hotkey_controls_layout->addWidget(hotkey_turbo_set);
			hotkey_controls_layout->addWidget(hotkey_mute_set);
			break;

	}

	last_control_id = controls_combo->currentIndex();
}


/****** Closes the settings window ******/
void gen_settings::closeEvent(QCloseEvent* event)
{
	//Close any on-going input configuration
	close_input();
}

/****** Closes the settings window - Used for the Close tab button ******/
void gen_settings::close_settings()
{
	//Close any on-going input configuration
	close_input();
}

/****** Handle keypress input ******/
void gen_settings::keyPressEvent(QKeyEvent* event)
{
	if(grab_input)
	{
		last_key = qtkey_to_sdlkey(event->key());

		switch(input_index)
		{
			case 0:
				if(last_key != -1)
				{
					config::gbe_key_a = last_key;
					input_a->setText(QString::number(last_key));
				}

				config_a->setText("Configure");
				input_a->clearFocus();
				break;

			case 1:
				if(last_key != -1)
				{
					config::gbe_key_b = last_key;
					input_b->setText(QString::number(last_key));
				}

				config_b->setText("Configure");
				input_b->clearFocus();
				break;

			case 4:
				if(last_key != -1)
				{
					config::gbe_key_start = last_key;
					input_start->setText(QString::number(last_key));
				}

				config_start->setText("Configure");
				input_start->clearFocus();
				break;

			case 5:
				if(last_key != -1)
				{
					config::gbe_key_select = last_key;
					input_select->setText(QString::number(last_key));
				}

				config_select->setText("Configure");
				input_select->clearFocus();
				break;

			case 6:
				if(last_key != -1)
				{
					config::gbe_key_left = last_key;
					input_left->setText(QString::number(last_key));
				}

				config_left->setText("Configure");
				input_left->clearFocus();
				break;

			case 7:
				if(last_key != -1)
				{
					config::gbe_key_right = last_key;
					input_right->setText(QString::number(last_key));
				}

				config_right->setText("Configure");
				input_right->clearFocus();
				break;

			case 8:
				if(last_key != -1)
				{
					config::gbe_key_up = last_key;
					input_up->setText(QString::number(last_key));
				}

				config_up->setText("Configure");
				input_up->clearFocus();
				break;

			case 9:
				if(last_key != -1)
				{
					config::gbe_key_down = last_key;
					input_down->setText(QString::number(last_key));
				}

				config_down->setText("Configure");
				input_down->clearFocus();
				break;

			case 18:
				if(last_key != -1)
				{
					config::hotkey_turbo = last_key;
					input_turbo->setText(QString::number(last_key));
				}

				config_turbo->setText("Configure");
				input_turbo->clearFocus();
				break;

			case 19:
				if(last_key != -1)
				{
					config::hotkey_mute = last_key;
					input_mute->setText(QString::number(last_key));
				}

				config_mute->setText("Configure");
				input_mute->clearFocus();
				break;

		}

		grab_input = false;
		input_index = -1;
	}
}

/****** Event filter for settings window ******/
bool gen_settings::eventFilter(QObject* target, QEvent* event)
{
	//Filter all keypresses, pass them along to input configuration if necessary
	if(event->type() == QEvent::KeyPress)
	{
		QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
		keyPressEvent(key_event);
	}

	//Check mouse click for input device
	else if((target == input_device) && (event->type() == QEvent::MouseButtonPress))
	{
		rebuild_input_index();
	}

	return QDialog::eventFilter(target, event);
}

/****** Selects folder ******/
void gen_settings::select_folder() { data_folder->finish = true; }

/****** Rejectss folder ******/
void gen_settings::reject_folder()
{
	data_folder->finish = true;
	data_folder->setDirectory(data_folder->last_path);
}
