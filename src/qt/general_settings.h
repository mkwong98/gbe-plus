// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : general_settings.h
// Date : June 28, 2015
// Description : Main menu
//
// Dialog for various options
// Deals with Graphics, Audio, Input, Paths, etc

#ifndef SETTINGS_GBE_QT
#define SETTINGS_GBE_QT

#include <SDL2/SDL.h>

#ifdef GBE_QT_5
#include <QtWidgets>
#endif

#ifdef GBE_QT_4
#include <QtGui>
#endif

#include "common/common.h"
#include "data_dialog.h"
#include "cheat_menu.h"
#include "rtc_menu.h"
#include "tbf_menu.h"

class gen_settings : public QDialog
{
	Q_OBJECT
	
	public:
	gen_settings(QWidget *parent = 0);

	void set_ini_options();

	QTabWidget* tabs;
	QDialogButtonBox* tabs_button;
	QComboBox* controls_combo;
	u8 last_control_id;

	//General tab widgets
	QComboBox* overclock;
	QCheckBox* cheats;
	QPushButton* edit_cheats;
	QCheckBox* auto_patch;

	//Display tab widgets
	QComboBox* screen_scale;
	QComboBox* dmg_gbc_pal;
	QComboBox* ogl_frag_shader;
	QComboBox* ogl_vert_shader;
	QCheckBox* ogl;
	QCheckBox* load_cgfx;
	QCheckBox* aspect_ratio;
	QCheckBox* osd_enable;

	//Sound tab widgets
	QComboBox* freq;
	QSpinBox* sound_samples;
	QSlider* volume;
	QCheckBox* sound_on;
	QCheckBox* stereo_enable;

	data_dialog* data_folder;

	double sample_rate;
	bool resize_screen;

	bool grab_input;
	int last_key;
	int input_index;

	QVBoxLayout* controls_layout;
	QVBoxLayout* advanced_controls_layout;
	QVBoxLayout* hotkey_controls_layout;

	QSlider* dead_zone;

	//Paths tab widgets
	QLineEdit* screenshot;
	QLineEdit* game_saves;
	QLineEdit* cheats_path;

	QLabel* screenshot_label;
	QLabel* game_saves_label;
	QLabel* cheats_path_label;

	//Controls tab widget
	QComboBox* input_device;

	QLineEdit* input_a;
	QLineEdit* input_b;
	QLineEdit* input_start;
	QLineEdit* input_select;
	QLineEdit* input_left;
	QLineEdit* input_right;
	QLineEdit* input_up;
	QLineEdit* input_down;
	QLineEdit* input_turbo;
	QLineEdit* input_mute;

	QPushButton* config_a;
	QPushButton* config_b;
	QPushButton* config_start;
	QPushButton* config_select;
	QPushButton* config_left;
	QPushButton* config_right;
	QPushButton* config_up;
	QPushButton* config_down;
	QPushButton* config_turbo;
	QPushButton* config_mute;

	//Advanced controls tab widget
	QCheckBox* rumble_on;

	//Misc widgets
	cheat_menu* dmg_cheat_menu;
	rtc_menu* real_time_clock_menu;
	tbf_menu* turbo_file_menu;
	QMessageBox* warning_box;

	void update_volume();

	protected:
	void keyPressEvent(QKeyEvent* event);
	void closeEvent(QCloseEvent* event);
	bool eventFilter(QObject* target, QEvent* event);

	private slots:
	void overclock_change();
	void set_patches();
	void show_cheats();
	void show_rtc();
	void set_ogl();
	void screen_scale_change();
	void aspect_ratio_change();
	void set_osd();
	void dmg_gbc_pal_change();
	void ogl_frag_change();
	void ogl_vert_change();
	void set_cgfx();
	void volume_change();
	void sample_rate_change();
	void sample_size_change();
	void mute();
	void set_paths(int index);
	void rebuild_input_index();
	void input_device_change();
	void dead_zone_change();
	void configure_button(int button);
	void close_input();
	void close_settings();
	void switch_control_layout();
	void select_folder();
	void reject_folder();

	private:
	QDialog* general;
	QDialog* display;
	QDialog* sound;
	QDialog* controls;
	QDialog* paths;

	QWidget* input_device_set;
	QWidget* input_a_set;
	QWidget* input_b_set;
	QWidget* input_start_set;
	QWidget* input_select_set;
	QWidget* input_left_set;
	QWidget* input_right_set;
	QWidget* input_up_set;
	QWidget* input_down_set;
	QWidget* dead_zone_set;

	QWidget* rumble_set;

	QWidget* hotkey_turbo_set;
	QWidget* hotkey_mute_set;

	void process_joystick_event();
	void input_delay(QPushButton* input_button); 

	SDL_Joystick* jstick;
	int input_type;
	u32 joystick_count;
};

#endif //SETTINGS_GBE_QT 
