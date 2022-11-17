// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : cgfx.h
// Date : July 25, 2015
// Description : Custom graphics settings
//
// Dialog for various custom graphics options

#include "common/cgfx_common.h"
#include "common/util.h"
#include "cgfx.h"
#include "main_menu.h"
#include "render.h"

#include <fstream>
#include <direct.h>

/****** General settings constructor ******/
gbe_cgfx::gbe_cgfx(QWidget *parent) : QDialog(parent)
{
	//Set up tabs
	tabs = new QTabWidget(this);
	
	QDialog* config_tab = new QDialog;
	QDialog* layers_tab = new QDialog;
	QDialog* manifest_tab = new QDialog;
	QDialog* obj_meta_tab = new QDialog;

	tabs->addTab(layers_tab, tr("Layers Viewer"));
	tabs->addTab(obj_meta_tab, tr("Meta Tile Editor"));
	tabs->addTab(manifest_tab, tr("Manifest"));
	tabs->addTab(config_tab, tr("Configure"));

	tabs_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Setup Configure widgets
	QWidget* blank_set = new QWidget(config_tab);
	QLabel* blank_label = new QLabel("Ignore blank/empty tiles when dumping", blank_set);
	blank = new QCheckBox(blank_set);

	QWidget* advanced_set = new QWidget(config_tab);
	QLabel* advanced_label = new QLabel("Use advanced menu", advanced_set);
	advanced = new QCheckBox(advanced_set);

	QWidget* auto_trans_set = new QWidget(config_tab);
	QLabel* auto_trans_label = new QLabel("Automatically add transparency color when dumping OBJs", auto_trans_set);
	auto_trans = new QCheckBox(auto_trans_set);

	layers_set = new QWidget(layers_tab);
	layers_layout = new QGridLayout;

	//Setup Layers widgets
	QImage temp_img(320, 288, QImage::Format_ARGB32);
	temp_img.fill(qRgb(0, 0, 0));

	QImage temp_pix(128, 128, QImage::Format_ARGB32);
	temp_pix.fill(qRgb(0, 0, 0));

	current_layer = new QLabel;
	current_layer->setPixmap(QPixmap::fromImage(temp_img));
	current_layer->setMouseTracking(true);
	current_layer->installEventFilter(this);

	current_tile = new QLabel;
	current_tile->setPixmap(QPixmap::fromImage(temp_pix));

	//Layer Label Info
	QWidget* layer_info = new QWidget;

	tile_id = new QLabel("Tile ID : ");
	h_v_flip = new QLabel("H-Flip :    V Flip : ");
	tile_palette = new QLabel("Tile Palette : ");
	hash_text = new QLabel("Tile Data : ");

	QVBoxLayout* layer_info_layout = new QVBoxLayout;
	layer_info_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	layer_info_layout->addWidget(tile_id);
	layer_info_layout->addWidget(h_v_flip);
	layer_info_layout->addWidget(tile_palette);
	layer_info_layout->addWidget(hash_text);
	layer_info->setLayout(layer_info_layout);

	//Layer combo-box
	QWidget* select_set = new QWidget(layers_tab);
	QLabel* select_set_label = new QLabel("Graphics Layer : ");
	layer_select = new QComboBox(select_set);
	layer_select->addItem("Background");
	layer_select->addItem("Window");
	layer_select->addItem("OBJ");
	layer_select->setCurrentIndex(0);

	QHBoxLayout* layer_select_layout = new QHBoxLayout;
	layer_select_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	layer_select_layout->addWidget(select_set_label);
	layer_select_layout->addWidget(layer_select);
	select_set->setLayout(layer_select_layout);

	//Input control 1
	QWidget* input_set_1 = new QWidget(layers_tab);
	a_input = new QPushButton("A");
	b_input = new QPushButton("B");
	select_input = new QPushButton("SELECT");
	start_input = new QPushButton("START");

	QHBoxLayout* input_set_1_layout = new QHBoxLayout;
	input_set_1_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	input_set_1_layout->addWidget(a_input);
	input_set_1_layout->addWidget(b_input);
	input_set_1_layout->addWidget(select_input);
	input_set_1_layout->addWidget(start_input);
	input_set_1->setLayout(input_set_1_layout);

	//Input control 2
	QWidget* input_set_2 = new QWidget(layers_tab);
	left_input = new QPushButton("LEFT");
	right_input = new QPushButton("RIGHT");
	up_input = new QPushButton("UP");
	down_input = new QPushButton("DOWN");

	QHBoxLayout* input_set_2_layout = new QHBoxLayout;
	input_set_2_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	input_set_2_layout->addWidget(left_input);
	input_set_2_layout->addWidget(right_input);
	input_set_2_layout->addWidget(up_input);
	input_set_2_layout->addWidget(down_input);
	input_set_2->setLayout(input_set_2_layout);

	//Frame control
	QGroupBox* frame_control_set = new QGroupBox(tr("Frame Control"));
	QPushButton* next_frame = new QPushButton("Advance Next Frame");

	QWidget* render_stop_set = new QWidget(layers_tab);
	QLabel* render_stop_label = new QLabel("Stop LCD rendering on line: ");
	render_stop_line = new QSpinBox(render_stop_set);
	render_stop_line->setRange(0, 0x90);

	QHBoxLayout* render_stop_layout = new QHBoxLayout;
	render_stop_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	render_stop_layout->addWidget(render_stop_label);
	render_stop_layout->addWidget(render_stop_line);
	render_stop_set->setLayout(render_stop_layout);

	QVBoxLayout* frame_control_layout = new QVBoxLayout;
	frame_control_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	frame_control_layout->addWidget(render_stop_set);
	frame_control_layout->addWidget(next_frame);
	frame_control_layout->addWidget(input_set_1);
	frame_control_layout->addWidget(input_set_2);
	frame_control_set->setLayout(frame_control_layout);

	//Layer section selector - X
	QWidget* section_x_set = new QWidget(layers_tab);
	QLabel* section_x_label = new QLabel("Tile X :\t");
	QLabel* section_w_label = new QLabel("Tile W :\t");
	
	rect_x = new QSpinBox(section_x_set);
	rect_x->setRange(0, 159);

	rect_w = new QSpinBox(section_x_set);
	rect_w->setRange(0, 159);

	QHBoxLayout* section_x_layout = new QHBoxLayout;
	section_x_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	section_x_layout->addWidget(section_x_label);
	section_x_layout->addWidget(rect_x);
	section_x_layout->addWidget(section_w_label);
	section_x_layout->addWidget(rect_w);
	section_x_set->setLayout(section_x_layout);

	//Layer section selector - Y
	QWidget* section_y_set = new QWidget(layers_tab);
	QLabel* section_y_label = new QLabel("Tile Y :\t");
	QLabel* section_h_label = new QLabel("Tile H :\t");
	
	rect_y = new QSpinBox(section_y_set);
	rect_y->setRange(0, 143);

	rect_h = new QSpinBox(section_y_set);
	rect_h->setRange(0, 143);

	QHBoxLayout* section_y_layout = new QHBoxLayout;
	section_y_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	section_y_layout->addWidget(section_y_label);
	section_y_layout->addWidget(rect_y);
	section_y_layout->addWidget(section_h_label);
	section_y_layout->addWidget(rect_h);
	section_y_set->setLayout(section_y_layout);

	//Layer GroupBox for section
	QGroupBox* section_set = new QGroupBox(tr("Selection"));
	QPushButton* copy_section_button = new QPushButton("Copy to meta tile editor");
	QPushButton* dump_section_button = new QPushButton("Dump new tiles in current selection");
	QVBoxLayout* section_final_layout = new QVBoxLayout;
	section_final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	section_final_layout->setSpacing(0);
	section_final_layout->addWidget(section_x_set);
	section_final_layout->addWidget(section_y_set);
	section_final_layout->addWidget(dump_section_button);
	section_set->setLayout(section_final_layout);

	//OBJ Meta Tile widgets
	QWidget* obj_meta_preview_set = new QWidget(obj_meta_tab);

	QWidget* obj_size_set_1 = new QWidget(obj_meta_tab);
	QWidget* obj_size_set_2 = new QWidget(obj_meta_tab);
	QLabel* obj_meta_width_label = new QLabel("Tile Width :\t");
	QLabel* obj_meta_height_label = new QLabel("Tile Height :\t");
	
	obj_meta_width = new QSpinBox;
	obj_meta_width->setRange(1, 20);
	
	obj_meta_height = new QSpinBox;
	obj_meta_height->setRange(1, 20);

	QImage temp_obj(320, 320, QImage::Format_ARGB32);
	temp_obj.fill(qRgb(255, 255, 255));
	obj_meta_pixel_data = temp_obj;

	obj_meta_img = new QLabel;
	obj_meta_img->setPixmap(QPixmap::fromImage(temp_obj));
	obj_meta_img->installEventFilter(this);
	obj_meta_img->setMouseTracking(true);

	QHBoxLayout* obj_size_layout_1 = new QHBoxLayout;
	obj_size_layout_1->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	obj_size_layout_1->addWidget(obj_meta_width_label);
	obj_size_layout_1->addWidget(obj_meta_width);
	obj_size_set_1->setLayout(obj_size_layout_1);

	QHBoxLayout* obj_size_layout_2 = new QHBoxLayout;
	obj_size_layout_2->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	obj_size_layout_2->addWidget(obj_meta_height_label);
	obj_size_layout_2->addWidget(obj_meta_height);
	obj_size_set_2->setLayout(obj_size_layout_2);

	QPushButton* dump_obj_meta_button = new QPushButton("Dump OBJ Meta Tile");

	QWidget* obj_name_set = new QWidget(obj_meta_tab);
	obj_name_set->setMaximumWidth(320);
	QLabel* obj_name_label = new QLabel("Meta Tile Name: ");
	obj_meta_name = new QLineEdit;

	QHBoxLayout* obj_name_layout = new QHBoxLayout;
	obj_name_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	obj_name_layout->addWidget(obj_name_label);
	obj_name_layout->addWidget(obj_meta_name);
	obj_name_set->setLayout(obj_name_layout);

	QWidget* obj_option_set = new QWidget(obj_meta_tab);
	obj_meta_vram_addr = new QCheckBox;
	obj_meta_auto_bright = new QCheckBox;

	QLabel* obj_vram_text = new QLabel("EXT_VRAM_ADDR");
	QLabel* obj_bright_text = new QLabel("EXT_AUTO_BRIGHT");

	QHBoxLayout* obj_option_layout = new QHBoxLayout;
	obj_option_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	obj_option_layout->addWidget(obj_vram_text);
	obj_option_layout->addWidget(obj_meta_vram_addr);
	obj_option_layout->addWidget(obj_bright_text);
	obj_option_layout->addWidget(obj_meta_auto_bright);
	obj_option_set->setLayout(obj_option_layout);

	QVBoxLayout* obj_meta_preview_layout = new QVBoxLayout;
	obj_meta_preview_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	obj_meta_preview_layout->addWidget(obj_size_set_1);
	obj_meta_preview_layout->addWidget(obj_size_set_2);
	obj_meta_preview_layout->addWidget(obj_meta_img);
	obj_meta_preview_layout->addWidget(obj_name_set);
	obj_meta_preview_layout->addWidget(obj_option_set);
	obj_meta_preview_layout->addWidget(dump_obj_meta_button);
	obj_meta_preview_set->setLayout(obj_meta_preview_layout);

	QWidget* obj_resource_set = new QWidget(obj_meta_tab);
	obj_meta_index = new QSpinBox(obj_resource_set);
	obj_meta_index->setRange(0, 39);
	QLabel* obj_meta_index_label = new QLabel("OBJ Index :\t");

	QHBoxLayout* obj_resource_layout = new QHBoxLayout;
	obj_resource_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	obj_resource_layout->addWidget(obj_meta_index_label);
	obj_resource_layout->addWidget(obj_meta_index);
	obj_resource_set->setLayout(obj_resource_layout);

	QWidget* obj_data_set = new QWidget(obj_meta_tab);
	obj_select_img = new QLabel;
	obj_select_img->setPixmap(QPixmap::fromImage(temp_obj.scaled(256, 256)));

	QVBoxLayout* obj_data_layout = new QVBoxLayout;
	obj_data_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	obj_data_layout->addWidget(obj_resource_set);
	obj_data_layout->addWidget(obj_select_img);
	obj_data_set->setLayout(obj_data_layout);

	//OBJ Meta Tile layout
	QGridLayout* obj_meta_layout = new QGridLayout;
	obj_meta_layout->addWidget(obj_meta_preview_set, 0, 0, 1, 1);
	obj_meta_layout->addWidget(obj_data_set, 0, 1, 1, 1);
	obj_meta_tab->setLayout(obj_meta_layout);

	//Manifest widgets
	manifest_display = new QScrollArea(manifest_tab);

	//Configure Tab layout
	QHBoxLayout* advanced_layout = new QHBoxLayout;
	advanced_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	advanced_layout->addWidget(advanced);
	advanced_layout->addWidget(advanced_label);
	advanced_set->setLayout(advanced_layout);

	QHBoxLayout* blank_layout = new QHBoxLayout;
	blank_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	blank_layout->addWidget(blank);
	blank_layout->addWidget(blank_label);
	blank_set->setLayout(blank_layout);

	QHBoxLayout* auto_trans_layout = new QHBoxLayout;
	auto_trans_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	auto_trans_layout->addWidget(auto_trans);
	auto_trans_layout->addWidget(auto_trans_label);
	auto_trans_set->setLayout(auto_trans_layout);

	QVBoxLayout* config_tab_layout = new QVBoxLayout;
	config_tab_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	config_tab_layout->addWidget(advanced_set);
	config_tab_layout->addWidget(blank_set);
	config_tab_layout->addWidget(auto_trans_set);
	config_tab->setLayout(config_tab_layout);

	//Layers Tab layout
	QGridLayout* layers_tab_layout = new QGridLayout;
	layers_tab_layout->addWidget(select_set, 0, 0, 1, 1);
	layers_tab_layout->addWidget(current_layer, 1, 0, 1, 1);
	layers_tab_layout->addWidget(frame_control_set, 2, 0, 1, 1);

	layers_tab_layout->addWidget(current_tile, 1, 1, 1, 1);
	layers_tab_layout->addWidget(layer_info, 0, 1, 1, 1);
	layers_tab_layout->addWidget(section_set, 2, 1, 1, 1);
	layers_tab->setLayout(layers_tab_layout);

	//Manifest tab layout
	QHBoxLayout* manifest_layout = new QHBoxLayout;
	manifest_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	manifest_layout->addWidget(manifest_display);
	manifest_tab->setLayout(manifest_layout);
	
	//Display settings - CGFX scale
	QWidget* cgfx_scale_set = new QWidget(this);
	QLabel* cgfx_scale_label = new QLabel("Custom Graphics (CGFX) Scale : ");
	cgfx_scale = new QComboBox(cgfx_scale_set);
	cgfx_scale->setToolTip("Scaling factor for all custom graphics.\nOnly applies when CGFX are loaded.");
	cgfx_scale->addItem("1x");
	cgfx_scale->addItem("2x");
	cgfx_scale->addItem("3x");
	cgfx_scale->addItem("4x");
	cgfx_scale->addItem("5x");
	cgfx_scale->addItem("6x");
	cgfx_scale->addItem("7x");
	cgfx_scale->addItem("8x");
	cgfx_scale->addItem("9x");
	cgfx_scale->addItem("10x");

	QHBoxLayout* cgfx_scale_layout = new QHBoxLayout;
	cgfx_scale_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	cgfx_scale_layout->addWidget(cgfx_scale_label);
	cgfx_scale_layout->addWidget(cgfx_scale);
	cgfx_scale_set->setLayout(cgfx_scale_layout);

	//Final tab layout
	QVBoxLayout* main_layout = new QVBoxLayout;
	main_layout->addWidget(cgfx_scale_set);
	main_layout->addWidget(tabs);
	main_layout->addWidget(tabs_button);
	setLayout(main_layout);

	connect(tabs_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(tabs_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(tabs_button->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(close_cgfx()));
	connect(blank, SIGNAL(stateChanged(int)), this, SLOT(set_blanks()));
	connect(auto_trans, SIGNAL(stateChanged(int)), this, SLOT(set_auto_trans()));
	connect(layer_select, SIGNAL(currentIndexChanged(int)), this, SLOT(layer_change()));
	connect(rect_x, SIGNAL(valueChanged(int)), this, SLOT(update_selection()));
	connect(rect_y, SIGNAL(valueChanged(int)), this, SLOT(update_selection()));
	connect(rect_w, SIGNAL(valueChanged(int)), this, SLOT(update_selection()));
	connect(rect_h, SIGNAL(valueChanged(int)), this, SLOT(update_selection()));
	connect(dump_section_button, SIGNAL(clicked()), this, SLOT(dump_selection()));
	connect(next_frame, SIGNAL(clicked()), this, SLOT(advance_next_frame()));
	connect(obj_meta_width, SIGNAL(valueChanged(int)), this, SLOT(update_obj_meta_size()));
	connect(obj_meta_height, SIGNAL(valueChanged(int)), this, SLOT(update_obj_meta_size()));
	connect(obj_meta_index, SIGNAL(valueChanged(int)), this, SLOT(select_obj()));
	connect(dump_obj_meta_button, SIGNAL(clicked()), this, SLOT(dump_obj_meta_tile()));

	QSignalMapper* input_signal = new QSignalMapper(this);
	connect(a_input, SIGNAL(clicked()), input_signal, SLOT(map()));
	connect(b_input, SIGNAL(clicked()), input_signal, SLOT(map()));
	connect(select_input, SIGNAL(clicked()), input_signal, SLOT(map()));
	connect(start_input, SIGNAL(clicked()), input_signal, SLOT(map()));
	connect(left_input, SIGNAL(clicked()), input_signal, SLOT(map()));
	connect(right_input, SIGNAL(clicked()), input_signal, SLOT(map()));
	connect(up_input, SIGNAL(clicked()), input_signal, SLOT(map()));
	connect(down_input, SIGNAL(clicked()), input_signal, SLOT(map()));

	input_signal->setMapping(a_input, 0);
	input_signal->setMapping(b_input, 1);
	input_signal->setMapping(select_input, 2);
	input_signal->setMapping(start_input, 3);
	input_signal->setMapping(left_input, 4);
	input_signal->setMapping(right_input, 5);
	input_signal->setMapping(up_input, 6);
	input_signal->setMapping(down_input, 7);
	connect(input_signal, SIGNAL(mapped(int)), this, SLOT(update_input_control(int))) ;

	//CGFX advanced dumping pop-up box
	advanced_box = new QDialog();
	advanced_box->resize(500, 250);
	advanced_box->setWindowTitle("Advanced Tile Dumping");
	advanced_box->hide();

	QWidget* ext_vram_set = new QWidget(advanced_box);
	QLabel* ext_vram_label = new QLabel("EXT_VRAM_ADDR", ext_vram_set);
	ext_vram = new QCheckBox(ext_vram_set);

	QWidget* ext_bright_set = new QWidget(advanced_box);
	QLabel* ext_bright_label = new QLabel("EXT_AUTO_BRIGHT", ext_bright_set);
	ext_bright = new QCheckBox(ext_bright_set);

	dump_button = new QPushButton("Dump Tile", advanced_box);
	cancel_button = new QPushButton("Cancel", advanced_box);

	advanced_buttons = new QDialogButtonBox(advanced_box);
	advanced_buttons->setOrientation(Qt::Horizontal);
	advanced_buttons->addButton(dump_button, QDialogButtonBox::ActionRole);
	advanced_buttons->addButton(cancel_button, QDialogButtonBox::ActionRole);

	//Advanced menu layouts

	QHBoxLayout* ext_vram_layout = new QHBoxLayout;
	ext_vram_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	ext_vram_layout->addWidget(ext_vram);
	ext_vram_layout->addWidget(ext_vram_label);
	ext_vram_set->setLayout(ext_vram_layout);

	QHBoxLayout* ext_bright_layout = new QHBoxLayout;
	ext_bright_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	ext_bright_layout->addWidget(ext_bright);
	ext_bright_layout->addWidget(ext_bright_label);
	ext_bright_set->setLayout(ext_bright_layout);

	QVBoxLayout* advanced_box_layout = new QVBoxLayout;
	advanced_box_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	advanced_box_layout->addWidget(ext_vram_set);
	advanced_box_layout->addWidget(ext_bright_set);
	advanced_box_layout->addWidget(advanced_buttons);
	advanced_box->setLayout(advanced_box_layout);

	//Manifest write entry failure pop-up
	manifest_write_fail = new QMessageBox;
	QPushButton* manifest_write_fail_ok = manifest_write_fail->addButton("OK", QMessageBox::AcceptRole);
	QPushButton* manifest_write_fail_ignore = manifest_write_fail->addButton("Do not show this message again", QMessageBox::AcceptRole);
	manifest_write_fail->setText("Could not access the manifest file! Manifest entry was not written, please check file path and permissions");
	manifest_write_fail->setIcon(QMessageBox::Critical);
	manifest_write_fail->hide();

	//Graphics dump failure pop-up
	save_fail = new QMessageBox;
	QPushButton* save_fail_ok = save_fail->addButton("OK", QMessageBox::AcceptRole);
	save_fail->setText("Error - Could not write PNG to destination file. Check file path and permissions");
	save_fail->setIcon(QMessageBox::Warning);
	save_fail->hide();

	//Redump existing hash
	redump_hash = new QMessageBox;
	QPushButton* redump_hash_ok = redump_hash->addButton("Redump Tile", QMessageBox::AcceptRole);
	QPushButton* redump_hash_cancel = redump_hash->addButton("Cancel", QMessageBox::RejectRole);
	redump_hash->setText("You are attempting to dump a tile that you have already dumped recently.\nWould you like to dump this tile again?");
	redump_hash->setIcon(QMessageBox::Warning);
	redump_hash->hide();
	
	connect(dump_button, SIGNAL(clicked()), this, SLOT(write_manifest_entry()));
	connect(cancel_button, SIGNAL(clicked()), this, SLOT(close_advanced()));
	connect(manifest_write_fail_ignore, SIGNAL(clicked()), this, SLOT(ignore_manifest_criticals()));
	connect(redump_hash_ok, SIGNAL(clicked()), this, SLOT(redump_tile()));

	estimated_palette.resize(384, 0);
	estimated_vram_bank.resize(384, 0);

	resize(800, 450);
	setWindowTitle(tr("Custom Graphics"));

	dump_type = 0;
	advanced_index = 0;
	last_custom_path = "";

	min_x_rect = min_y_rect = max_x_rect = max_y_rect = 255;
	render_stop_line->setValue(0x90);

	pause = false;
	hash_text->setScaledContents(true);

	enable_manifest_critical = true;
	redump = false;

	mouse_start_x = mouse_start_y = 0;
	mouse_drag = false;

	obj_meta_width->setValue(20);
	obj_meta_height->setValue(20);

	obj_meta_str.resize(400, "");
	obj_meta_addr.resize(400, 0);

	pack_data = (dmg_cgfx_data*)(main_menu::gbe_plus->get_core_data(3));
}


/****** Closes the CGFX window ******/
void gbe_cgfx::closeEvent(QCloseEvent* event) { close_cgfx(); }

/****** Closes the CGFX window ******/
void gbe_cgfx::close_cgfx()
{
	reset_inputs();

	qt_gui::draw_surface->findChild<QAction*>("pause_action")->setEnabled(true);

	pause = false;
	config::pause_emu = false;
	last_custom_path = "";
	advanced_box->hide();
}

/****** Sets flag to redump a tile ******/
void gbe_cgfx::redump_tile() { redump = true; }

/****** Changes the current viewable layer for dumping ******/
void gbe_cgfx::layer_change()
{
	switch (layer_select->currentIndex())
	{
	//BG, Window, OBJ
	case 0: draw_gb_layer(0); break;
	case 1: draw_gb_layer(1); break;
	case 2: draw_gb_layer(2); break;
	}
}

/****** Draw DMG and GBC layers ******/
void gbe_cgfx::draw_gb_layer(u8 layer)
{
	if(main_menu::gbe_plus == NULL) { return; }
	SDL_Surface* s = (SDL_Surface*)(main_menu::gbe_plus->get_core_data(layer));

	rendered_screen* sc = &(pack_data->screen_data);
	screenInfo.rendered_palette = sc->rendered_palette;
	screenInfo.rendered_tile = sc->rendered_tile;
	for (u8 y = 0; y < 144; y++) {
		screenInfo.scanline[y] = sc->scanline[y];
	}

	SDL_LockSurface(s);
	u32* pt = (u32*)(s->pixels);
	//keep a copy
	std::copy(pt, pt + (160 * 144), rawImageData);
	SDL_UnlockSurface(s);

	update_selection();
}

void gbe_cgfx::update_preview(u32 x, u32 y)
{
	if (main_menu::gbe_plus == NULL) { return; }

	x >>= 1;
	y >>= 1;

	std::vector<tile_strip> strips;
	switch (layer_select->currentIndex())
	{
	case 0: strips = screenInfo.scanline[y].rendered_bg; break;
	case 1: strips = screenInfo.scanline[y].rendered_win; break;
	case 2: strips = screenInfo.scanline[y].rendered_obj; break;
	}

	for (int i = 0; i < strips.size(); i++)
	{
		//test strip belongs to a tile within the selected area
		if ((strips[i].x + 7) >= x && (strips[i].x <= x))
		{
			//Tile info - ID
			QString id("Tile ID : ");
			id += QString::number(strips[i].entity_id);
			if (strips[i].line >= 8) id += "a";
			tile_id->setText(id);

			//Tile info - H/V Flip
			QString flip("H-Flip : ");
			flip += (strips[i].hflip ? "Y" : "N");
			flip += "    V-Flip : ";
			flip += (strips[i].vflip ? "Y" : "N");
			h_v_flip->setText(flip);

			//Tile info - Palette
			QString pal("Tile Palette : ");
			pal += get_palette_code(strips[i].palette_id).c_str();
			tile_palette->setText(pal);

			//Tile info - Hash
			QString hashed = QString("Tile Data : ");
			for (u8 l = 0; l < 8; l++) {
				hashed += util::to_hex_strXXXX(screenInfo.rendered_tile[strips[i].pattern_id].tile.line[l]).c_str();
			}
			hash_text->setText(hashed);

			QImage final_image = renderTileToImage(strips[i].pattern_id, strips[i].palette_id, strips[i].palette_sel, layer_select->currentIndex()).scaled(256, 256);
			current_tile->setPixmap(QPixmap::fromImage(final_image));
			return;
		}
	}
}

std::string dmg_cgfx::get_palette_code(u16 p)
{
	return util::to_hex_strXX(screenInfo.rendered_palette[p].code).c_str();
}

std::string gbc_cgfx::get_palette_code(u16 p)
{
	//Tile info - Palette
	std::string pal = "";
	for (u8 i = 0; i < 4; i++) {
		pal += util::to_hex_strXXXX(screenInfo.rendered_palette[p].colour[i]).c_str();
	}
	return pal;
}

void dmg_cgfx::renderTile(u16 tileID, u16 palId, u8 palSel, u8 layer, std::vector<u32>* top, std::vector<u32>* bottom)
{
	for (u8 l = 0; l < 8; l++)
	{
		u16 raw_data = screenInfo.rendered_tile[tileID].tile.line[l];
		//Grab individual pixels
		for (int y = 7; y >= 0; y--)
		{
			u8 raw_pixel = ((raw_data >> 8) & (1 << y)) ? 2 : 0;
			raw_pixel |= (raw_data & (1 << y)) ? 1 : 0;
			if (layer <= 1)
			{
				if (raw_pixel == 0)
				{
					bottom->push_back(config::DMG_BG_PAL[screenInfo.rendered_palette[palId].colour[raw_pixel]]);
					top->push_back(0);
				}
				else
				{
					top->push_back(config::DMG_BG_PAL[screenInfo.rendered_palette[palId].colour[raw_pixel]]);
					bottom->push_back(0);
				}
			}
			else
			{
				if (raw_pixel == 0)
				{
					top->push_back(0);
					bottom->push_back(0);
				}
				else
				{
					top->push_back(config::DMG_OBJ_PAL[screenInfo.rendered_palette[palId].colour[raw_pixel]][palSel]);
					bottom->push_back(0);
				}
			}
		}
	}

}

void gbc_cgfx::renderTile(u16 tileID, u16 palId, u8 palSel, u8 layer, std::vector<u32>* top, std::vector<u32>* bottom)
{
	for (u8 l = 0; l < 8; l++)
	{
		u16 raw_data = screenInfo.rendered_tile[tileID].tile.line[l];
		//Grab individual pixels
		for (int y = 7; y >= 0; y--)
		{
			u8 raw_pixel = ((raw_data >> 8) & (1 << y)) ? 2 : 0;
			raw_pixel |= (raw_data & (1 << y)) ? 1 : 0;
			if (layer <= 1)
			{
				if (raw_pixel == 0)
				{
					bottom->push_back(screenInfo.rendered_palette[palId].renderColour[raw_pixel]);
					top->push_back(0);
				}
				else
				{
					top->push_back(screenInfo.rendered_palette[palId].renderColour[raw_pixel]);
					bottom->push_back(0);
				}
			}
			else
			{
				if (raw_pixel == 0)
				{
					top->push_back(0);
					bottom->push_back(0);
				}
				else
				{
					top->push_back(screenInfo.rendered_palette[palId].renderColour[raw_pixel]);
					bottom->push_back(0);
				}
			}
		}
	}

}

QImage gbe_cgfx::renderTileToImage(u16 tileID, u16 palId, u8 palSel, u8 layer) {
	std::vector<u32> top_pixels;
	std::vector<u32> bottom_pixels;

	renderTile(tileID, palId, palSel, layer, &top_pixels, &bottom_pixels);

	QImage raw_image(8, 8, QImage::Format_ARGB32);

	//Copy raw pixels to QImage
	for (int x = 0; x < top_pixels.size(); x++)
	{
		raw_image.setPixel((x % 8), (x / 8), top_pixels[x] | bottom_pixels[x]);
	}
	return raw_image;
}


/****** Event filter for meta tile tabs ******/
bool gbe_cgfx::eventFilter(QObject* target, QEvent* event)
{
	//Mouse motion
	if(event->type() == QEvent::MouseMove)
	{
		QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
		u32 x = mouse_event->x();
		u32 y = mouse_event->y();	

		//Layers tab - Check to see if mouse is hovered over current layer
		if(target == current_layer)
		{	
			//Update the preview
			if((mouse_event->x() <= 320) && (mouse_event->y() <= 288)) { update_preview(x, y); }

			//Update highlighting when dragging the mouse
			if(mouse_drag)
			{
				x >>= 1;
				y >>= 1;

				if (x <= mouse_start_x)
				{
					rect_x->setValue(x);
					rect_w->setValue(mouse_start_x - x);
				}
				else
				{
					rect_w->setValue(x - mouse_start_x);
				}

				if (y <= mouse_start_y)
				{
					rect_y->setValue(y);
					rect_h->setValue(mouse_start_y - y);
				}
				else
				{
					rect_h->setValue(y - mouse_start_y);
				}
				update_selection();
			}
		}

		//OBJ Meta Tile tab
		else if(target == obj_meta_img)
		{
			//Highlight selected OBJ tile
			if((x < 320) && (y < 320))
			{
				//Make sure X and Y coordinates are within proper range
				if(x >= (obj_meta_width->value() * 16)) { return QDialog::eventFilter(target, event); }
				if(y >= (obj_meta_height->value() * 16)) { return QDialog::eventFilter(target, event); }

				u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

				//Grab affected X and Y coordinates
				x &= ~0xF;
				y &= (obj_height == 16) ? ~0x1F : ~0xF;

				obj_height *= 2;

				QImage highlight = obj_meta_pixel_data;

				//Cycle scanline by scanline
				for(int sy = y; sy < (y + obj_height); sy++)
				{
					u32* pixel_data = (u32*)highlight.scanLine(sy);

					//Highlight affected parts of the scanline
					for(int sx = x; sx < (x + 16); sx++)
					{	
						pixel_data[sx] += 0x00808080;
					}
				}

				int w = obj_meta_width->value() * 16;
				int h = obj_meta_height->value() * 16;

				obj_meta_img->setPixmap(QPixmap::fromImage(highlight).copy(0, 0, w, h));

				meta_highlight = true;
			}

			//Return image to original state
			else if(meta_highlight)
			{
				int w = obj_meta_width->value() * 16;
				int h = obj_meta_height->value() * 16;

				obj_meta_img->setPixmap(QPixmap::fromImage(obj_meta_pixel_data).copy(0, 0, w, h));
			}
		}
	}
	
	//Double-Click
	else if(event->type() == QEvent::MouseButtonDblClick)
	{

	}

	//Single click
	else if(event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
		u32 x = mouse_event->x();
		u32 y = mouse_event->y();

		//Layers tab
		if(target == current_layer && !mouse_drag)
		{
			mouse_drag = true;
			mouse_start_x = x / 2;
			mouse_start_y = y / 2;
			rect_x->setValue(mouse_start_x);
			rect_y->setValue(mouse_start_y);
			rect_w->setValue(0);
			rect_h->setValue(0);
		}

		//OBJ Meta Tile tab
		else if(target == obj_meta_img)
		{

		}
	}

	//Check to see if mouse is released from single-click over current layer
	else if((event->type() == QEvent::MouseButtonRelease) && (mouse_drag))
	{
		mouse_drag = false;
		update_selection();
	}


	return QDialog::eventFilter(target, event);
}


/****** Updates the dumping selection for multiple tiles in the layer tab ******/
void gbe_cgfx::update_selection()
{
	u32* pt = rawImageData;
	QImage raw_image(160, 144, QImage::Format_ARGB32);
	//Fill in image with pixels from the emulated LCD
	for (int y = 0; y < 144; y++)
	{
		u32* pixel_data = (u32*)(raw_image.scanLine(y));
		std::copy(pt, pt + 160, pixel_data);
		pt += 160;
	}
	raw_image = raw_image.scaled(320, 288);
	
	//draw a box when dragging mouse
	QPainter painter(&raw_image);

	if (mouse_drag) {
		painter.setPen(QPen(QColor(255, 0, 0)));
		painter.drawRect(rect_x->value() * 2 + 1, rect_y->value() * 2 + 1, rect_w->value() * 2 - 1, rect_h->value() * 2 - 1);
		painter.setPen(QPen(QColor(0, 255, 255)));
		painter.drawRect(rect_x->value() * 2, rect_y->value() * 2, rect_w->value() * 2 - 1, rect_h->value() * 2 - 1);
	}
	if (rect_w->value() > 0 && rect_h->value() > 0)
	{

		//hightlight selected tiles
		for (int y = 0; y < 144; y++) 
		{
			std::vector<tile_strip> strips;
			switch (layer_select->currentIndex())
			{
			case 0: strips = screenInfo.scanline[y].rendered_bg; break;
			case 1: strips = screenInfo.scanline[y].rendered_win; break;
			case 2: strips = screenInfo.scanline[y].rendered_obj; break;
			}
			for (int i = 0; i < strips.size(); i++)
			{
				//test strip belongs to a tile within the selected area
				if ((strips[i].x + 7) >= rect_x->value() && strips[i].x <= (rect_x->value() + rect_w->value())
					&& (y - (strips[i].line >= 8 ? strips[i].line - 8 : strips[i].line) + 7) >= rect_y->value() 
					&& (y - (strips[i].line >= 8 ? strips[i].line - 8 : strips[i].line)) <= (rect_y->value() + rect_h->value()))
				{
					if (strips[i].line == 0 || strips[i].line == 7 || strips[i].line == 8 || strips[i].line == 15)
					{
						//draw top or bottom line
						painter.setPen(QPen(QColor(0, 255, 0)));
						painter.drawLine(strips[i].x * 2 + 1, y * 2 + 1, strips[i].x * 2 + 15, y * 2 + 1);
						painter.setPen(QPen(QColor(255, 0, 255)));
						painter.drawLine(strips[i].x * 2, y * 2, strips[i].x * 2 + 14, y * 2);
					}
					else
					{
						//draw side
						painter.setPen(QPen(QColor(0, 255, 0)));
						painter.drawLine(strips[i].x * 2 + 1, y * 2, strips[i].x * 2 + 1, y * 2 + 1);
						painter.drawLine(strips[i].x * 2 + 15, y * 2, strips[i].x * 2 + 15, y * 2 + 1);
						painter.setPen(QPen(QColor(255, 0, 255)));
						painter.drawLine(strips[i].x * 2, y * 2, strips[i].x * 2, y * 2 + 1);
						painter.drawLine(strips[i].x * 2 + 14, y * 2, strips[i].x * 2 + 14, y * 2 + 1);
					}
				}
			}
		}
	}

	//Set label Pixmap
	current_layer->setPixmap(QPixmap::fromImage(raw_image));
}

/****** Dumps the selection of multiple tiles to a file ******/
void gbe_cgfx::dump_selection()
{
	if(main_menu::gbe_plus == NULL) { return; }
	if((rect_w->value() == 0) || (rect_h->value() == 0)) { return; }

	_mkdir(get_game_cgfx_folder().c_str());

	//Grab metatile name
	cgfx::meta_dump_name = util::timeStr();

	//open hires.txt
	std::ofstream file(get_manifest_file().c_str(), std::ios::out | std::ios::app);

	if (!cgfx::loaded)
	{
		file << "<scale>" << cgfx_scale->currentIndex() + 1 << "\n";
	}

	//create a blank image
	SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, 160, 320, 32, SDL_PIXELFORMAT_ARGB8888);
	SDL_Surface* s2 = SDL_CreateRGBSurfaceWithFormat(0, 160, 320, 32, SDL_PIXELFORMAT_ARGB8888);

	file << "\n<img>" << cgfx::meta_dump_name << ".png";
	if (layer_select->currentIndex() <= 1)
	{
		file << "," << cgfx::meta_dump_name << "b.png";
	}
	file << "\n";

	std::vector<pack_tile> addedTile;
	u16 drawX = 0;
	u16 drawY = 0;

	for (u16 y = 0; y < 144; y++)
	{
		std::vector<tile_strip> strips;
		switch (layer_select->currentIndex())
		{
		case 0: strips = screenInfo.scanline[y].rendered_bg; break;
		case 1: strips = screenInfo.scanline[y].rendered_win; break;
		case 2: strips = screenInfo.scanline[y].rendered_obj; break;
		}
		for (u16 i = 0; i < strips.size(); i++)
		{
			//test strip belongs to a tile within the selected area
			if ((strips[i].x + 7) >= rect_x->value() && strips[i].x <= (rect_x->value() + rect_w->value())
				&& (y - (strips[i].line >= 8 ? strips[i].line - 8 : strips[i].line) + 7) >= rect_y->value()
				&& (y - (strips[i].line >= 8 ? strips[i].line - 8 : strips[i].line)) <= (rect_y->value() + rect_h->value()))
			{
				pack_tile t;
				t.tileStr = "";
				for (u8 l = 0; l < 8; l++) {
					t.tileStr += util::to_hex_strXXXX(screenInfo.rendered_tile[strips[i].pattern_id].tile.line[l]);
				}
				t.palStr = get_palette_code(strips[i].palette_id);

				bool added = false;
				//check to see if the tile has been added in current selection
				for (u16 j = 0; j < addedTile.size() && !added; j++)
				{
					if (addedTile[j].tileStr == t.tileStr && addedTile[j].palStr == t.palStr)
					{
						added = true;
					}
				}				
				if (!added)
				{
					//look for the tile in the pack
					for (u16 tileIdx = 0; tileIdx < pack_data->tiles.size() && !added; tileIdx++)
					{
						if (pack_data->tiles[tileIdx].condApps.size() == 0 && pack_data->tiles[tileIdx].tileStr == t.tileStr && pack_data->tiles[tileIdx].palStr == t.palStr)
						{
							added = true;
						}
					}
					if (!added)
					{
						//add to img
						std::vector<u32> top;
						std::vector<u32> bottom;
						renderTile(strips[i].pattern_id, strips[i].palette_id, strips[i].palette_sel, layer_select->currentIndex(), &top, &bottom);
						SDL_LockSurface(s);
						u32* fillpt = (u32*)(s->pixels) + (drawY * 160) + drawX;
						u32* srcpt = top.data();
						for (u16 pY = 0; pY < 8; pY++)
						{
							for (u16 pX = 0; pX < 8; pX++)
							{
								*fillpt = *srcpt;
								fillpt++;
								srcpt++;
							}
							fillpt += 152;
						}
						SDL_UnlockSurface(s);

						//copy to bottom layer
						if (layer_select->currentIndex() <= 1) 
						{
							SDL_LockSurface(s2);
							u32* fillpt = (u32*)(s2->pixels) + (drawY * 160) + drawX;
							u32* srcpt = bottom.data();
							for (u16 pY = 0; pY < 8; pY++)
							{
								for (u16 pX = 0; pX < 8; pX++)
								{
									*fillpt = *srcpt;
									fillpt++;
									srcpt++;
								}
								fillpt += 152;
							}
							SDL_UnlockSurface(s2);
						}

						//add to txt
						file << "<tile>" << pack_data->imgs.size() << "," << t.tileStr << "," << t.palStr << "," << drawX * (cgfx_scale->currentIndex() + 1) << "," << drawY * (cgfx_scale->currentIndex() + 1) << ",1.1,N\n";
						drawX += 8;
						if (drawX == 160)
						{
							drawX = 0;
							drawY += 8;
						}

						addedTile.push_back(t);
					}
				}
			}
		}
	}
	//add screen as reference
	drawY += 16;

	SDL_LockSurface(s);
	u32* fillpt = (u32*)(s->pixels) + (drawY * 160);
	std::copy(rawImageData, rawImageData + (160 * 144), fillpt);
	SDL_UnlockSurface(s);

	drawY += 144;
	file.close();

	SDL_Rect r;
	r.x = 0;
	r.y = 0;
	r.w = 160;
	r.h = drawY;

	SDL_Surface* ims = SDL_CreateRGBSurfaceWithFormat(0, 160 * (cgfx_scale->currentIndex() + 1), drawY * (cgfx_scale->currentIndex() + 1), 32, SDL_PIXELFORMAT_ARGB8888);
	SDL_BlitScaled(s, &r, ims, NULL);
	IMG_SavePNG(ims, (get_game_cgfx_folder() + cgfx::meta_dump_name + ".png").c_str());
	SDL_FreeSurface(ims);

	if (layer_select->currentIndex() <= 1)
	{
		SDL_Surface* ims2 = SDL_CreateRGBSurfaceWithFormat(0, 160 * (cgfx_scale->currentIndex() + 1), drawY * (cgfx_scale->currentIndex() + 1), 32, SDL_PIXELFORMAT_ARGB8888);
		SDL_BlitScaled(s2, &r, ims2, NULL);
		IMG_SavePNG(ims2, (get_game_cgfx_folder() + cgfx::meta_dump_name + "b.png").c_str());
		SDL_FreeSurface(ims2);
	}
	
	SDL_FreeSurface(s);
	SDL_FreeSurface(s2);

	//signal the core to reload the pack
	main_menu::gbe_plus->get_core_data(4);
}


/****** Ignores manifest criticals until program quits ******/
void gbe_cgfx::ignore_manifest_criticals() { enable_manifest_critical = false; }

/****** Advances to the next frame from within the CGFX screen ******/
void gbe_cgfx::advance_next_frame()
{
	if(main_menu::gbe_plus == NULL) { return; }

	u8 on_status = 0;
	u8 next_ly = main_menu::gbe_plus->ex_read_u8(REG_LY);
	next_ly += 1;
	if(next_ly >= 0x90) { next_ly = 0; }

	//Run until next LY or LCD disabled
	while(main_menu::gbe_plus->ex_read_u8(REG_LY) != next_ly)
	{
		on_status = main_menu::gbe_plus->ex_read_u8(REG_LCDC);
		
		if((on_status & 0x80) == 0)
		{
			layer_change();
			return;
		}

		main_menu::gbe_plus->step();
	}

	//Run until emulator hits old LY value or LCD disabled
	while(main_menu::gbe_plus->ex_read_u8(REG_LY) != render_stop_line->value())
	{
		on_status = main_menu::gbe_plus->ex_read_u8(REG_LCDC);

		if((on_status & 0x80) == 0)
		{
			layer_change();
			return;
		}

		main_menu::gbe_plus->step();
	}

	layer_change();
}

/****** Updates input control when advancing frames ******/
void gbe_cgfx::update_input_control(int index)
{
	//Set QPushButtons to flat or raise them
	switch(index)
	{
		case 0x0:
			a_input->isFlat() ? a_input->setFlat(false) : a_input->setFlat(true);
			break;
	
		case 0x1:
			b_input->isFlat() ? b_input->setFlat(false) : b_input->setFlat(true);
			break;

		case 0x2:
			select_input->isFlat() ? select_input->setFlat(false) : select_input->setFlat(true);
			break;

		case 0x3:
			start_input->isFlat() ? start_input->setFlat(false) : start_input->setFlat(true);
			break;

		case 0x4:
			left_input->isFlat() ? left_input->setFlat(false) : left_input->setFlat(true);
			if(!left_input->isFlat()) { right_input->setFlat(true); }
			break;

		case 0x5:
			right_input->isFlat() ? right_input->setFlat(false) : right_input->setFlat(true);
			if(!right_input->isFlat()) { left_input->setFlat(true); }
			break;

		case 0x6:
			up_input->isFlat() ? up_input->setFlat(false) : up_input->setFlat(true);
			if(!up_input->isFlat()) { down_input->setFlat(true); }
			break;

		case 0x7:
			down_input->isFlat() ? down_input->setFlat(false) : down_input->setFlat(true);
			if(!down_input->isFlat()) { up_input->setFlat(true); }
			break;
	}

	if(main_menu::gbe_plus == NULL) { return; }

	//Send input state to core
	if(a_input->isFlat()) { main_menu::gbe_plus->feed_key_input(config::gbe_key_a, false); }
	else { main_menu::gbe_plus->feed_key_input(config::gbe_key_a, true); }

	if(b_input->isFlat()) { main_menu::gbe_plus->feed_key_input(config::gbe_key_b, false); }
	else { main_menu::gbe_plus->feed_key_input(config::gbe_key_b, true); }

	if(select_input->isFlat()) { main_menu::gbe_plus->feed_key_input(config::gbe_key_select, false); }
	else { main_menu::gbe_plus->feed_key_input(config::gbe_key_select, true); }

	if(start_input->isFlat()) { main_menu::gbe_plus->feed_key_input(config::gbe_key_start, false); }
	else { main_menu::gbe_plus->feed_key_input(config::gbe_key_start, true); }

	if(left_input->isFlat()) { main_menu::gbe_plus->feed_key_input(config::gbe_key_left, false); }

	else
	{ 
		main_menu::gbe_plus->feed_key_input(config::gbe_key_left, true);
		right_input->setFlat(true);
	}

	if(right_input->isFlat()) { main_menu::gbe_plus->feed_key_input(config::gbe_key_right, false); }

	else
	{
		main_menu::gbe_plus->feed_key_input(config::gbe_key_right, true);
		left_input->setFlat(true);
	}

	if(up_input->isFlat()) { main_menu::gbe_plus->feed_key_input(config::gbe_key_up, false); }

	else
	{ 
		main_menu::gbe_plus->feed_key_input(config::gbe_key_up, true);
		down_input->setFlat(true);
	}

	if(down_input->isFlat()) { main_menu::gbe_plus->feed_key_input(config::gbe_key_down, false); }

	else
	{
		main_menu::gbe_plus->feed_key_input(config::gbe_key_down, true);
		up_input->setFlat(true);
	}
}

/****** Resets input control when opening or closing the CGFX menu ******/
void gbe_cgfx::reset_inputs()
{
	a_input->setFlat(true);
	b_input->setFlat(true);
	select_input->setFlat(true);
	start_input->setFlat(true);
	left_input->setFlat(true);
	right_input->setFlat(true);
	up_input->setFlat(true);
	down_input->setFlat(true);

	if(main_menu::gbe_plus == NULL) { return; }

	main_menu::gbe_plus->feed_key_input(config::gbe_key_a, false);
	main_menu::gbe_plus->feed_key_input(config::gbe_key_b, false);
	main_menu::gbe_plus->feed_key_input(config::gbe_key_select, false);
	main_menu::gbe_plus->feed_key_input(config::gbe_key_start, false);
	main_menu::gbe_plus->feed_key_input(config::gbe_key_up, false);
	main_menu::gbe_plus->feed_key_input(config::gbe_key_down, false);
	main_menu::gbe_plus->feed_key_input(config::gbe_key_left, false);
	main_menu::gbe_plus->feed_key_input(config::gbe_key_right, false);
}

/****** Updates the OBJ Meta Tile preview size ******/
void gbe_cgfx::update_obj_meta_size()
{
	//Limit height to even numbers only when in 8x16 mode
	if(main_menu::gbe_plus != NULL)
	{
		u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

		if(obj_height == 16)
		{
			obj_meta_height->setSingleStep(2);
			obj_meta_height->setRange(2, 20);
		}

		else
		{
			obj_meta_height->setSingleStep(1);
			obj_meta_height->setRange(1, 20);
		}
	}
	
	int w = obj_meta_width->value() * 16;
	int h = obj_meta_height->value() * 16;

	obj_meta_img->setPixmap(QPixmap::fromImage(obj_meta_pixel_data).copy(0, 0, w, h));
}


void gbe_cgfx::init()
{
	cgfx_scale->setCurrentIndex(cgfx::scaling_factor - 1);
}