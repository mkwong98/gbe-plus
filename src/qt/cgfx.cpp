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

	data_folder = new data_dialog;

	connect(tabs_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(tabs_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(tabs_button->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(close_cgfx()));
	connect(blank, SIGNAL(stateChanged(int)), this, SLOT(set_blanks()));
	connect(auto_trans, SIGNAL(stateChanged(int)), this, SLOT(set_auto_trans()));
	connect(layer_select, SIGNAL(currentIndexChanged(int)), this, SLOT(layer_change()));
	connect(data_folder, SIGNAL(accepted()), this, SLOT(select_folder()));
	connect(data_folder, SIGNAL(rejected()), this, SLOT(reject_folder()));
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

	//Advanced menu widgets
	QWidget* dest_set = new QWidget(advanced_box);
	dest_label = new QLabel("Destination Folder :  ");
	dest_browse = new QPushButton("Browse");
	dest_folder = new QLineEdit(dest_set);
	dest_folder->setReadOnly(true);
	dest_label->resize(100, dest_label->height());

	QWidget* name_set = new QWidget(advanced_box);
	name_label = new QLabel("Tile Name :  ");
	name_browse = new QPushButton("Browse");
	dest_name = new QLineEdit(name_set);

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
	QHBoxLayout* dest_layout = new QHBoxLayout;
	dest_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	dest_layout->addWidget(dest_label);
	dest_layout->addWidget(dest_folder);
	dest_layout->addWidget(dest_browse);
	dest_set->setLayout(dest_layout);

	QHBoxLayout* name_layout = new QHBoxLayout;
	name_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	name_layout->addWidget(name_label);
	name_layout->addWidget(dest_name);
	name_layout->addWidget(name_browse);
	name_set->setLayout(name_layout);

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
	advanced_box_layout->addWidget(dest_set);
	advanced_box_layout->addWidget(name_set);
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
	connect(dest_browse, SIGNAL(clicked()), this, SLOT(browse_advanced_dir()));
	connect(name_browse, SIGNAL(clicked()), this, SLOT(browse_advanced_file()));
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

/****** Optionally shows the advanced menu before dumping - OBJ version ******/
void gbe_cgfx::show_advanced_obj(int index)
{
	if(advanced->isChecked()) 
	{
		QString path = QString::fromStdString(cgfx::dump_obj_path);
		if(last_custom_path != "") { path = QString::fromStdString(last_custom_path); }

		//Set the default destination
		if(!path.isNull())
		{
			//Use relative paths
			QDir folder(QString::fromStdString(config::data_path));
			path = folder.relativeFilePath(path);

			//Make sure path is complete, e.g. has the correct separator at the end
			//Qt doesn't append this automatically
			std::string temp_str = path.toStdString();
			std::string temp_chr = "";
			temp_chr = temp_str[temp_str.length() - 1];

			if((temp_chr != "/") && (temp_chr != "\\")) { path.append("/"); }
			path = QDir::toNativeSeparators(path);

			dest_folder->setText(path);
		}

		cgfx::dump_name = "";
		dest_name->setText("");

		dump_type = 1;
		advanced_index = index;
		advanced_box->show();
		advanced_box->raise();
	}

	else { dump_obj(index); }
}

/****** Optionally shows the advanced menu before dumping - BG version ******/
void gbe_cgfx::show_advanced_bg(int index)
{
	if(advanced->isChecked()) 
	{
		QString path = QString::fromStdString(cgfx::dump_bg_path);
		if(last_custom_path != "") { path = QString::fromStdString(last_custom_path); }

		//Set the default destination
		if(!path.isNull())
		{
			//Use relative paths
			QDir folder(QString::fromStdString(config::data_path));
			path = folder.relativeFilePath(path);

			//Make sure path is complete, e.g. has the correct separator at the end
			//Qt doesn't append this automatically
			std::string temp_str = path.toStdString();
			std::string temp_chr = "";
			temp_chr = temp_str[temp_str.length() - 1];

			if((temp_chr != "/") && (temp_chr != "\\")) { path.append("/"); }
			path = QDir::toNativeSeparators(path);

			dest_folder->setText(path);
		}

		cgfx::dump_name = "";
		dest_name->setText("");

		dump_type = 0;
		advanced_index = index;
		advanced_box->show();
		advanced_box->raise();
	}

	else { dump_bg(index); }
}

/****** Grabs an OBJ in VRAM and converts it to a QImage - DMG Version ******/
QImage dmg_cgfx::grab_obj_data(int obj_index)
{
	std::vector<u32> obj_pixels;

	//Determine if in 8x8 or 8x16 mode
	u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

	//Setup palettes
	u8 obp[4][2];

	u8 obp0 = main_menu::gbe_plus->ex_read_u8(REG_OBP0);
	u8 obp1 = main_menu::gbe_plus->ex_read_u8(REG_OBP1);

	for(u32 x = 0; x < 4; x++)
	{
		obp[x][0] = (obp0 >> (x * 2)) & 0x3;
		obp[x][1] = (obp1 >> (x * 2)) & 0x3;
	}

	//Grab palette number from OAM
	u8 pal_num = (main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 3) & 0x10) ? 1 : 0;
	u8 tile_num = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 2);

	if(obj_height == 16) { tile_num &= ~0x1; }

	//Grab OBJ tile addr from index
	u16 obj_tile_addr = 0x8000 + (tile_num << 4);

	//Pull data from VRAM into the ARGB vector
	for(int x = 0; x < obj_height; x++)
	{
		//Grab bytes from VRAM representing 8x1 pixel data
		u16 raw_data = (main_menu::gbe_plus->ex_read_u8(obj_tile_addr + 1) << 8) | main_menu::gbe_plus->ex_read_u8(obj_tile_addr);

		//Grab individual pixels
		for(int y = 7; y >= 0; y--)
		{
			u8 raw_pixel = ((raw_data >> 8) & (1 << y)) ? 2 : 0;
			raw_pixel |= (raw_data & (1 << y)) ? 1 : 0;

			if((raw_pixel == 0) && (cgfx::auto_obj_trans)) { obj_pixels.push_back(cgfx::transparency_color); }

			else
			{
				switch(obp[raw_pixel][pal_num])
				{
					case 0: 
						obj_pixels.push_back(0xFFFFFFFF);
						break;

					case 1: 
						obj_pixels.push_back(0xFFC0C0C0);
						break;

					case 2: 
						obj_pixels.push_back(0xFF606060);
						break;

					case 3: 
						obj_pixels.push_back(0xFF000000);
						break;
				}
			}
		}

		obj_tile_addr += 2;
	}

	QImage raw_image(8, obj_height, QImage::Format_ARGB32);	

	//Copy raw pixels to QImage
	for(int x = 0; x < obj_pixels.size(); x++)
	{
		raw_image.setPixel((x % 8), (x / 8), obj_pixels[x]);
	}

	//Scale final output to 64x64 or 32x64
	QImage final_image = (obj_height == 8) ? raw_image.scaled(64, 64) : raw_image.scaled(32, 64);
	return final_image;
}

/****** Grabs an OBJ in VRAM and converts it to a QImage - GBC Version ******/
QImage gbc_cgfx::grab_obj_data(int obj_index)
{
	std::vector<u32> obj_pixels;

	//Determine if in 8x8 or 8x16 mode
	u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

	//Grab palette number from OAM
	u8 pal_num = (main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 3) & 0x7);
	u8 tile_num = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 2);

	if(obj_height == 16) { tile_num &= ~0x1; }
	
	//Grab VRAM banks
	u8 current_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);
	u8 obj_vram_bank = (main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 3) & 0x8) ? 1 : 0;
	main_menu::gbe_plus->ex_write_u8(REG_VBK, obj_vram_bank);

	//Setup palettes
	u32 obp[4];

	u32* color = main_menu::gbe_plus->get_obj_palette(pal_num);

	obp[0] = *color; color += 8;
	obp[1] = *color; color += 8;
	obp[2] = *color; color += 8;
	obp[3] = *color; color += 8;

	//Grab OBJ tile addr from index
	u16 obj_tile_addr = 0x8000 + (tile_num << 4);

	//Pull data from VRAM into the ARGB vector
	for(int x = 0; x < obj_height; x++)
	{
		//Grab bytes from VRAM representing 8x1 pixel data
		u16 raw_data = (main_menu::gbe_plus->ex_read_u8(obj_tile_addr + 1) << 8) | main_menu::gbe_plus->ex_read_u8(obj_tile_addr);

		//Grab individual pixels
		for(int y = 7; y >= 0; y--)
		{
			u8 raw_pixel = ((raw_data >> 8) & (1 << y)) ? 2 : 0;
			raw_pixel |= (raw_data & (1 << y)) ? 1 : 0;

			if((raw_pixel == 0) && (cgfx::auto_obj_trans)) { obj_pixels.push_back(cgfx::transparency_color); }
			else { obj_pixels.push_back(obp[raw_pixel]); }
		}

		obj_tile_addr += 2;
	}

	//Return VRAM bank to normal
	main_menu::gbe_plus->ex_write_u8(REG_VBK, current_vram_bank);

	QImage raw_image(8, obj_height, QImage::Format_ARGB32);	

	//Copy raw pixels to QImage
	for(int x = 0; x < obj_pixels.size(); x++)
	{
		raw_image.setPixel((x % 8), (x / 8), obj_pixels[x]);
	}

	//Scale final output to 64x64 or 32x64
	QImage final_image = (obj_height == 8) ? raw_image.scaled(64, 64) : raw_image.scaled(32, 64);
	return final_image;
}

/****** Grabs a BG tile in VRAM and converts it to a QImage - DMG version ******/
QImage dmg_cgfx::grab_bg_data(int bg_index)
{
	std::vector<u32> bg_pixels;

	//Setup palette
	u8 bgp[4];

	u8 value = main_menu::gbe_plus->ex_read_u8(REG_BGP);

	for(u32 x = 0; x < 4; x++) { bgp[x] = (value >> (x * 2)) & 0x3; }

	//Grab bg tile addr from index
	u16 tile_num = bg_index;
	u16 bg_tile_addr = 0x8000 + (tile_num << 4);

	//Pull data from VRAM into the ARGB vector
	for(int x = 0; x < 8; x++)
	{
		//Grab bytes from VRAM representing 8x1 pixel data
		u16 raw_data = (main_menu::gbe_plus->ex_read_u8(bg_tile_addr + 1) << 8) | main_menu::gbe_plus->ex_read_u8(bg_tile_addr);

		//Grab individual pixels
		for(int y = 7; y >= 0; y--)
		{
			u8 raw_pixel = ((raw_data >> 8) & (1 << y)) ? 2 : 0;
			raw_pixel |= (raw_data & (1 << y)) ? 1 : 0;

			switch(bgp[raw_pixel])
			{
				case 0: 
					bg_pixels.push_back(0xFFFFFFFF);
					break;

				case 1: 
					bg_pixels.push_back(0xFFC0C0C0);
					break;

				case 2: 
					bg_pixels.push_back(0xFF606060);
					break;

				case 3: 
					bg_pixels.push_back(0xFF000000);
					break;
			}
		}

		bg_tile_addr += 2;
	}

	QImage raw_image(8, 8, QImage::Format_ARGB32);	

	//Copy raw pixels to QImage
	for(int x = 0; x < bg_pixels.size(); x++)
	{
		raw_image.setPixel((x % 8), (x / 8), bg_pixels[x]);
	}

	//Scale final output to 64x64
	QImage final_image = raw_image.scaled(64, 64);
	return final_image;
}

/****** Grabs a BG tile and converts it to a QImage - GBC Version ******/
QImage gbc_cgfx::grab_bg_data(int bg_index)
{
	std::vector<u32> bg_pixels;

	u8 pal_num = 0;
	int original_bg_index = bg_index;

	//Grab BG tile addr from index
	u16 tile_num = bg_index;
	u16 bg_tile_addr = 0x8000 + (tile_num << 4);

	//Estimate tile numbers
	if(bg_index > 255) { bg_index -= 255; }

	//Estimate the tilemap
	u16 tilemap_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x8) ? 0x9C00 : 0x9800; 

	//Grab VRAM banks
	u8 current_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);
	u8 bg_vram_bank = 0;
	main_menu::gbe_plus->ex_write_u8(REG_VBK, 0);

	//Estimate the color palette
	for(u16 x = tilemap_addr; x < (tilemap_addr + 0x400); x++)
	{
		u8 map_entry = main_menu::gbe_plus->ex_read_u8(x);

		if(map_entry == bg_index)
		{
			main_menu::gbe_plus->ex_write_u8(REG_VBK, 1);
			pal_num = (main_menu::gbe_plus->ex_read_u8(x) & 0x7);
			bg_vram_bank = (main_menu::gbe_plus->ex_read_u8(x) & 0x8) ? 1 : 0;

			estimated_vram_bank[original_bg_index] = bg_vram_bank;
			estimated_palette[original_bg_index] = pal_num;
		}
	}

	main_menu::gbe_plus->ex_write_u8(REG_VBK, bg_vram_bank);

	//Setup palettes
	u32 bgp[4];

	u32* color = main_menu::gbe_plus->get_bg_palette(pal_num);

	bgp[0] = *color; color += 8;
	bgp[1] = *color; color += 8;
	bgp[2] = *color; color += 8;
	bgp[3] = *color; color += 8;

	//Pull data from VRAM into the ARGB vector
	for(int x = 0; x < 8; x++)
	{
		//Grab bytes from VRAM representing 8x1 pixel data
		u16 raw_data = (main_menu::gbe_plus->ex_read_u8(bg_tile_addr + 1) << 8) | main_menu::gbe_plus->ex_read_u8(bg_tile_addr);

		//Grab individual pixels
		for(int y = 7; y >= 0; y--)
		{
			u8 raw_pixel = ((raw_data >> 8) & (1 << y)) ? 2 : 0;
			raw_pixel |= (raw_data & (1 << y)) ? 1 : 0;

			bg_pixels.push_back(bgp[raw_pixel]);
		}

		bg_tile_addr += 2;
	}

	//Return VRAM bank to normal
	main_menu::gbe_plus->ex_write_u8(REG_VBK, current_vram_bank);

	QImage raw_image(8, 8, QImage::Format_ARGB32);	

	//Copy raw pixels to QImage
	for(int x = 0; x < bg_pixels.size(); x++)
	{
		raw_image.setPixel((x % 8), (x / 8), bg_pixels[x]);
	}

	//Scale final output to 64x64
	QImage final_image = raw_image.scaled(64, 64);
	return final_image;
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

/****** Closes the Advanced menu ******/
void gbe_cgfx::close_advanced()
{
	advanced_box->hide();
	cgfx::dump_name = "";
}

/****** Dumps the selected OBJ ******/
void gbe_cgfx::dump_obj(int obj_index)
{
	main_menu::gbe_plus->dump_obj(obj_index);

	//Show redump warning
	if(!cgfx::last_added)
	{
		redump_hash->show();
		redump_hash->raise();
	}

	while(redump_hash->isVisible())
	{
		SDL_Delay(16);
		QApplication::processEvents();
	}

	//Redump if necessary
	if(redump)
	{
		cgfx::ignore_existing_hash = true;
		main_menu::gbe_plus->dump_obj(obj_index);
		cgfx::ignore_existing_hash = false;
		redump = false;
	}

	//Show save failure warning
	if((!cgfx::last_saved) && (cgfx::last_added))
	{
		save_fail->show();
		save_fail->raise();
	}

	while(save_fail->isVisible())
	{
		SDL_Delay(16);
		QApplication::processEvents();
	}
}

/****** Dumps the selected BG ******/
void gbe_cgfx::dump_bg(int bg_index)
{
	main_menu::gbe_plus->dump_bg(bg_index);

	//Show redump warning
	if(!cgfx::last_added)
	{
		redump_hash->show();
		redump_hash->raise();
	}

	while(redump_hash->isVisible())
	{
		SDL_Delay(16);
		QApplication::processEvents();
	}

	//Redump if necessary
	if(redump)
	{
		cgfx::ignore_existing_hash = true;
		main_menu::gbe_plus->dump_bg(bg_index);
		cgfx::ignore_existing_hash = false;
		redump = false;
	}

	//Show save failure warning
	if((!cgfx::last_saved) && (cgfx::last_added))
	{
		save_fail->show();
		save_fail->raise();
	}

	while(save_fail->isVisible())
	{
		SDL_Delay(16);
		QApplication::processEvents();
	}
}

/****** Sets flag to redump a tile ******/
void gbe_cgfx::redump_tile() { redump = true; }

/****** Toggles whether to ignore blank dumps ******/
void gbe_cgfx::set_blanks()
{
	if(blank->isChecked()) { cgfx::ignore_blank_dumps = true; }
	else { cgfx::ignore_blank_dumps = false; }
}

/****** Toggles whether to automatically add the transparency color when dumping OBJs ******/
void gbe_cgfx::set_auto_trans()
{
	if(auto_trans->isChecked()) { cgfx::auto_obj_trans = true; }
	else { cgfx::auto_obj_trans = false; }
}

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



/****** Dumps the tile from a given layer ******/
void dmg_cgfx::dump_layer_tile(u32 x, u32 y)
{
	if(main_menu::gbe_plus == NULL) { return; }

	x >>= 1;
	y >>= 1;

	//Dump from DMG BG
	if(layer_select->currentIndex() == 0) 
	{
		//Determine BG Map & Tile address
		u16 bg_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x8) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		//Determine the map entry from on-screen coordinates
		u8 tile_x = main_menu::gbe_plus->ex_read_u8(REG_SX) + x;
		u8 tile_y = main_menu::gbe_plus->ex_read_u8(REG_SY) + y;
		u16 map_entry = (tile_x / 8) + ((tile_y / 8) * 32);

		u8 map_value = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(bg_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		u16 bg_index = (((bg_tile_addr + (map_value << 4)) & ~0x8000) >> 4);

		dump_type = 0;
		show_advanced_bg(bg_index);
	}

	//Dump from DMG Window
	else if(layer_select->currentIndex() == 1) 
	{
		//Determine BG Map & Tile address
		u16 win_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x40) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		u8 wx = main_menu::gbe_plus->ex_read_u8(REG_WX);
		wx = (wx < 7) ? 0 : (wx - 7); 
	
		//Determine the map entry from on-screen coordinates
		u8 tile_x = x - wx;
		u8 tile_y = y - main_menu::gbe_plus->ex_read_u8(REG_WY);
		u16 map_entry = (tile_x / 8) + ((tile_y / 8) * 32);

		u8 map_value = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(bg_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		u16 bg_index = (((bg_tile_addr + (map_value << 4)) & ~0x8000) >> 4);

		dump_type = 0;
		show_advanced_bg(bg_index);
	}

	//Dump from DMG or GBC OBJ 
	else if(layer_select->currentIndex() == 2) 
	{
		dump_obj_layer_tile(x, y);
	}
}

/****** Dumps the tile from a given layer ******/
void gbc_cgfx::dump_layer_tile(u32 x, u32 y)
{
	if (main_menu::gbe_plus == NULL) { return; }

	x >>= 1;
	y >>= 1;

	//Dump from GBC BG
	if (layer_select->currentIndex() == 0)
	{
		//Determine BG Map & Tile address
		u16 bg_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x8) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		//Determine the map entry from on-screen coordinates
		u8 tile_x = main_menu::gbe_plus->ex_read_u8(REG_SX) + x;
		u8 tile_y = main_menu::gbe_plus->ex_read_u8(REG_SY) + y;
		u16 map_entry = (tile_x / 8) + ((tile_y / 8) * 32);

		u8 original_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);

		//Read the BG attributes
		main_menu::gbe_plus->ex_write_u8(REG_VBK, 1);
		u8 bg_attribute = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);
		u8 pal_num = (bg_attribute & 0x7);
		u8 vram_bank = (bg_attribute & 0x8) ? 1 : 0;

		main_menu::gbe_plus->ex_write_u8(REG_VBK, 0);
		u8 map_value = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if (bg_tile_addr == 0x8800)
		{
			if (map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		u16 bg_index = (((bg_tile_addr + (map_value << 4)) & ~0x8000) >> 4);

		cgfx::gbc_bg_vram_bank = vram_bank;
		cgfx::gbc_bg_color_pal = pal_num;
		main_menu::gbe_plus->ex_write_u8(REG_VBK, original_vram_bank);

		dump_type = 0;
		show_advanced_bg(bg_index);
	}

	//Dump from GBC Window
	else if (layer_select->currentIndex() == 1)
	{
		//Determine BG Map & Tile address
		u16 win_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x40) ? 0x9C00 : 0x9800;
		u16 win_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		//Determine the map entry from on-screen coordinates
		u8 wx = main_menu::gbe_plus->ex_read_u8(REG_WX);
		wx = (wx < 7) ? 0 : (wx - 7);

		u8 tile_x = x - wx;
		u8 tile_y = y - main_menu::gbe_plus->ex_read_u8(REG_WY);
		u16 map_entry = (tile_x / 8) + ((tile_y / 8) * 32);

		u8 original_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);

		//Read the BG attributes
		main_menu::gbe_plus->ex_write_u8(REG_VBK, 1);
		u8 bg_attribute = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);
		u8 pal_num = (bg_attribute & 0x7);
		u8 vram_bank = (bg_attribute & 0x8) ? 1 : 0;

		main_menu::gbe_plus->ex_write_u8(REG_VBK, 0);
		u8 map_value = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if (win_tile_addr == 0x8800)
		{
			if (map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		u16 bg_index = (((win_tile_addr + (map_value << 4)) & ~0x8000) >> 4);

		cgfx::gbc_bg_vram_bank = vram_bank;
		cgfx::gbc_bg_color_pal = pal_num;
		main_menu::gbe_plus->ex_write_u8(REG_VBK, original_vram_bank);

		dump_type = 0;
		show_advanced_bg(bg_index);
	}

	//Dump from DMG or GBC OBJ 
	else if (layer_select->currentIndex() == 2)
	{
		dump_obj_layer_tile(x, y);
	}
}

void gbe_cgfx::dump_obj_layer_tile(u32 x, u32 y)
{
	//Determine if in 8x8 or 8x16 mode
	u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

	for (int obj_index = 0; obj_index < 40; obj_index++)
	{
		//Grab X-Y OBJ coordinates
		u8 obj_x = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 1);
		u8 obj_y = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4));

		obj_x -= 8;
		obj_y -= 16;

		u8 test_left = ((obj_x + 8) > 0x100) ? 0 : obj_x;
		u8 test_right = (obj_x + 8);

		u8 test_top = ((obj_y + obj_height) > 0x100) ? 0 : obj_y;
		u8 test_bottom = (obj_y + obj_height);

		dump_type = 1;

		if ((x >= test_left) && (x <= test_right) && (y >= test_top) && (y <= test_bottom)) { show_advanced_obj(obj_index); }
	}
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
		QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
		u32 x = mouse_event->x();
		u32 y = mouse_event->y();		

		//Layers tab - Check to see if mouse is double-clicked over current layer
		if(target == current_layer)
		{
			//Update the preview
			if((mouse_event->x() <= 320) && (mouse_event->y() <= 288)) { dump_layer_tile(x, y); }
		}


		//OBJ Meta - Delete entire meta tile
		else if((target == obj_meta_img) && (mouse_event->buttons() == Qt::RightButton))
		{
			QImage temp_obj(320, 320, QImage::Format_ARGB32);
			temp_obj.fill(qRgb(255, 255, 255));
			obj_meta_pixel_data = temp_obj;
			obj_meta_img->setPixmap(QPixmap::fromImage(obj_meta_pixel_data));

			for(int a = 0; a < obj_meta_str.size(); a++) { obj_meta_str[a] = ""; }
		}
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
			//Add to the meta-tile
			if(mouse_event->buttons() == Qt::LeftButton)
			{
				if((x < 320) && (y < 320))
				{
					u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

					//Copy data from OBJ index to preview
					u16 obj_index = obj_meta_index->value();
					QImage selected_img = (obj_height == 16) ? grab_obj_data(obj_index).scaled(16, 32) : grab_obj_data(obj_index).scaled(16, 16);

					x &= ~0xF;
					y &= (obj_height == 16) ? ~0x1F : ~0xF;

					obj_height *= 2;

					for(int sy = y, ty = 0; sy < (y + obj_height); sy++, ty++)
					{
						u32* out_pixel_data = (u32*)obj_meta_pixel_data.scanLine(sy);
						u32* in_pixel_data = (u32*)selected_img.scanLine(ty);

						//Highlight affected parts of the scanline
						for(int sx = x, tx = 0; sx < (x + 16); sx++, tx++)
						{	
							out_pixel_data[sx] = in_pixel_data[tx];
						}
					}

					int w = obj_meta_width->value() * 16;
					int h = obj_meta_height->value() * 16;

					obj_meta_img->setPixmap(QPixmap::fromImage(obj_meta_pixel_data).copy(0, 0, w, h));

					//Generate meta_tile manifest data
					u16 tile_number = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 2);
					if(obj_height == 32) { tile_number &= ~0x1; }
					u32 obj_addr = 0x8000 + (tile_number * 16);
					obj_index |= (tile_number << 8); 

					u8 obj_type = 1;
					u32 obj_id = (x / 16) + ((y / obj_height) * 20);

					//Generate base metatile data (hash + VRAM address)
					obj_meta_str[obj_id] = main_menu::gbe_plus->get_hash(obj_index, obj_type);
					obj_meta_addr[obj_id] = obj_addr;
				}
			}

			//Delete from the meta tile
			else if(mouse_event->buttons() == Qt::RightButton)
			{
				if((x < 320) && (y < 320))
				{
					u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

					x &= ~0xF;
					y &= (obj_height == 16) ? ~0x1F : ~0xF;

					obj_height *= 2;

					//Erase pixel data from OBJ meta tile
					for(int sy = y, ty = 0; sy < (y + obj_height); sy++, ty++)
					{
						u32* out_pixel_data = (u32*)obj_meta_pixel_data.scanLine(sy);

						//Highlight affected parts of the scanline
						for(int sx = x, tx = 0; sx < (x + 16); sx++, tx++)
						{	
							out_pixel_data[sx] = 0xFFFFFFFF;
						}
					}

					int w = obj_meta_width->value() * 16;
					int h = obj_meta_height->value() * 16;

					obj_meta_img->setPixmap(QPixmap::fromImage(obj_meta_pixel_data).copy(0, 0, w, h));

					//Erase manifest entry
					u32 obj_id = (x / 16) + ((y / obj_height) * obj_meta_width->value());
					obj_meta_str[obj_id] = "";
				}
			}
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

/****** Updates the settings window ******/
void gbe_cgfx::paintEvent(QPaintEvent* event)
{
	//Update GUI elements of the advanced menu
	name_label->setMinimumWidth(dest_label->width());
}

/****** Dumps tiles and writes a manifest entry  ******/
void gbe_cgfx::write_manifest_entry()
{
	//Open manifest file, then write to it
	std::ofstream file(get_manifest_file().c_str(), std::ios::out | std::ios::app);

	//Show warning if manifest file cannot be accessed
	if((!file.is_open()) && (enable_manifest_critical))
	{
		advanced_box->hide();
		manifest_write_fail->show();
		manifest_write_fail->raise();
	}

	while(manifest_write_fail->isVisible())
	{
		SDL_Delay(16);
		QApplication::processEvents();
	}

	//Process File Name
	QString path = dest_name->text();

	if(path.toStdString().empty()) { cgfx::dump_name.clear(); }
	else if(!path.isNull()) { cgfx::dump_name = path.toStdString() + ".png"; }
	else { cgfx::dump_name.clear(); }

	//Dump BG
	if(dump_type == 0) 
	{
		std::string temp_bg_path = cgfx::dump_bg_path;
		cgfx::dump_bg_path = dest_folder->text().toStdString();
		dump_bg(advanced_index);
		cgfx::dump_bg_path = temp_bg_path;
	}

	//Dump OBJ
	else
	{
		std::string temp_obj_path = cgfx::dump_obj_path;
		cgfx::dump_obj_path = dest_folder->text().toStdString();
		dump_obj(advanced_index);
		cgfx::dump_obj_path = temp_obj_path;
	}

	//Prepare a string for the manifest entry
	//Hash + Hash.bmp + Type + EXT_VRAM_ADDR + EXT_AUTO_BRIGHT
	std::string entry = "";

	std::string gfx_name = "";
	std::string dest_file = (dest_name->text().toStdString().empty()) ? cgfx::last_hash : dest_name->text().toStdString();

	if(dest_folder->text().isNull()) { gfx_name = cgfx::last_hash + ".png"; }
	else { gfx_name = dest_folder->text().toStdString() + dest_file + ".png"; }
	
	std::string gfx_type = util::to_str(cgfx::last_type);
	std::string gfx_addr = (ext_vram->isChecked()) ? util::to_hex_str(cgfx::last_vram_addr).substr(2) : "0";
	std::string gfx_bright = (ext_bright->isChecked()) ? "1" : "0";

	entry = "[" + cgfx::last_hash + ":'" + gfx_name + "':" + gfx_type + ":" + gfx_addr + ":" + gfx_bright + "]";

	//Write manifest entry only if file can be accessed
	if(file.is_open())
	{
		file << "\n" << entry;
		file.close();
	}
	
	advanced_box->hide();

	//Update manifest tab if necessary
	parse_manifest_items();
}

/****** Deletes a specific entry from the manifest ******/
bool gbe_cgfx::delete_manifest_entry(int index)
{
	std::vector <std::string> manifest;

	std::ifstream in_file(get_manifest_file().c_str(), std::ios::in);
	std::string input_line = "";
	std::string line_char = "";

	u32 entry_count = 0;

	if(!in_file.is_open())
	{
		std::cout<<"CGFX::Could not open manifest file " << get_manifest_file() << ". Check file path or permissions. \n";
		return false; 
	}

	//Cycle through whole file, line-by-line
	while(getline(in_file, input_line))
	{
		line_char = input_line[0];
		bool ignore = false;	
		u8 item_count = 0;
		bool is_meta = false;
		bool is_entry = false;

		//Check if line starts with [ - if not, skip line
		if(line_char == "[")
		{
			is_entry = true;
			std::string line_item = "";

			//Cycle through line, character-by-character
			for(int x = 0; ++x < input_line.length();)
			{
				line_char = input_line[x];

				//Check for single-quotes, don't parse ":" or "]" within them
				if((line_char == "'") && (!ignore)) { ignore = true; }
				else if((line_char == "'") && (ignore)) { ignore = false; }

				//Check the character for item limiter : or ] - Push to Vector
				else if(((line_char == ":") || (line_char == "]")) && (!ignore)) 
				{
					//Determine if entry is a meta tile
					if(item_count == 1)
					{
						u32 match_number = 0;
						std::string meta_file = line_item;

						std::size_t match = meta_file.find_last_of("_") + 1;
						if(match != std::string::npos) { meta_file = meta_file.substr(match); }
						if(util::from_str(meta_file, match_number)) { is_meta = true; }
					}

					line_item = "";
					item_count++;
				}

				else { line_item += line_char; }
			}

			//If not a meta tile entry, increment entry count
			if(!is_meta) { entry_count++; }
		}

		//Rebuild manifest with all lines except those belonging to the specific entry
		if((entry_count != (index + 1)) || (!is_entry)) { manifest.push_back(input_line); }
	}
	
	in_file.close();

	//If manifest is empty, quit now
	if(manifest.empty()) { return false; }

	std::ofstream out_file(get_manifest_file().c_str(), std::ios::out | std::ios::trunc);

	if(!out_file.is_open())
	{
		std::cout<<"CGFX::Could not open manifest file " << get_manifest_file() << ". Check file path or permissions. \n";
		return false; 
	}

	//Save new manifest file
	for(int x = 0; x < manifest.size(); x++) { out_file << manifest[x] << "\n";; }

	out_file.close();

	parse_manifest_items();

	return true;
}

/****** Browse for a directory to use in the advanced menu ******/
void gbe_cgfx::browse_advanced_dir()
{
	QString path;

	data_folder->open_data_folder();			

	while(!data_folder->finish) { QApplication::processEvents(); }
	
	path = data_folder->directory().path();
	path = data_folder->path.relativeFilePath(path);

	advanced_box->raise();

	if(path.isNull()) { return; }

	//Make sure path is complete, e.g. has the correct separator at the end
	//Qt doesn't append this automatically
	std::string temp_str = path.toStdString();
	std::string temp_chr = "";
	temp_chr = temp_str[temp_str.length() - 1];

	if((temp_chr != "/") && (temp_chr != "\\")) { path.append("/"); }
	path = QDir::toNativeSeparators(path);

	dest_folder->setText(path);
	last_custom_path = dest_folder->text().toStdString();
}

/****** Browse for a directory to use in the advanced menu ******/
void gbe_cgfx::browse_advanced_file()
{
	QString path;

	path = QFileDialog::getOpenFileName(this, tr("Open"), QString::fromStdString(config::data_path), tr("All files (*)"));
	advanced_box->raise();

	if(path.isNull()) { return; }

	//Use relative paths
	QFileInfo file(path);
	path = file.fileName();
	cgfx::dump_name = path.toStdString();

	dest_name->setText(path);
}

/****** Selects folder ******/
void gbe_cgfx::select_folder() { data_folder->finish = true; }

/****** Rejects folder ******/
void gbe_cgfx::reject_folder()
{
	data_folder->finish = true;
	data_folder->setDirectory(data_folder->last_path);
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
	SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, 160, 144, 32, SDL_PIXELFORMAT_ARGB8888);
	SDL_Surface* s2 = SDL_CreateRGBSurfaceWithFormat(0, 160, 144, 32, SDL_PIXELFORMAT_ARGB8888);

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
						u16 pIdx = 0;
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
								fillpt += 160;
							}
							u16 pIdx = 0;
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
	file.close();

	SDL_Surface* ims = SDL_CreateRGBSurfaceWithFormat(0, 160 * (cgfx_scale->currentIndex() + 1), 144 * (cgfx_scale->currentIndex() + 1), 32, SDL_PIXELFORMAT_ARGB8888);
	SDL_BlitScaled(s, NULL, ims, NULL);
	IMG_SavePNG(ims, (get_game_cgfx_folder() + cgfx::meta_dump_name + ".png").c_str());
	SDL_FreeSurface(ims);

	if (layer_select->currentIndex() <= 1)
	{
		SDL_Surface* ims2 = SDL_CreateRGBSurfaceWithFormat(0, 160 * (cgfx_scale->currentIndex() + 1), 144 * (cgfx_scale->currentIndex() + 1), 32, SDL_PIXELFORMAT_ARGB8888);
		SDL_BlitScaled(s2, NULL, ims2, NULL);
		IMG_SavePNG(ims2, (get_game_cgfx_folder() + cgfx::meta_dump_name + "b.png").c_str());
		SDL_FreeSurface(ims2);
	}
	
	SDL_FreeSurface(s);
	SDL_FreeSurface(s2);

}

/****** Dumps OBJ Meta Tile to file ******/
void gbe_cgfx::dump_obj_meta_tile()
{
	//Grab metatile name
	cgfx::meta_dump_name = obj_meta_name->text().toStdString();
	if(cgfx::meta_dump_name.empty()) { cgfx::meta_dump_name = "OBJ_META"; }

	//Save OBJ meta tile to image
	QString file_path(QString::fromStdString(config::data_path + cgfx::dump_obj_path + cgfx::meta_dump_name + ".bmp"));

	u32 w = obj_meta_width->value() * 16;
	u32 h = obj_meta_height->value() * 16;

	u32 s_width = w / 2;
	u32 s_height = h / 2;

	if(!obj_meta_pixel_data.copy(0, 0, w, h).scaled(s_width, s_height).save(file_path))
	{
		save_fail->show();
		save_fail->raise();
	}

	while(save_fail->isVisible())
	{
		SDL_Delay(16);
		QApplication::processEvents();
	}

	//Open manifest file, then write to it
	std::ofstream file(get_manifest_file().c_str(), std::ios::out | std::ios::app);
	std::string entry = "";

	//TODO - Add a Qt warning here
	if(!file.is_open()) { return; }

	u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;
	std::string type = (obj_height == 8) ? ":1]" : ":2]";

	//Write main entry
	entry = "['" + cgfx::dump_obj_path + cgfx::meta_dump_name + ".bmp" + "':" + cgfx::meta_dump_name + type;
	file << "\n" << entry;

	//Generate manifest entries for selected tiles
	for(int y = 0; y < obj_meta_height->value(); y++) 
	{
		for(int x = 0; x < obj_meta_width->value(); x++)
		{
			u16 obj_id = (y * 20) + x;
			u16 meta_id = (y * obj_meta_width->value()) + x;

			if(obj_meta_str[obj_id] != "")
			{
				//Grab metatile name
				cgfx::meta_dump_name = obj_meta_name->text().toStdString();
				if(cgfx::meta_dump_name.empty()) { cgfx::meta_dump_name = "OBJ_META"; }

				std::string entry = "";
				std::string hash = obj_meta_str[obj_id];
				std::string type = "1";
				std::string name = cgfx::meta_dump_name + "_" + util::to_str(meta_id);
				std::string vram = obj_meta_vram_addr->isChecked() ? util::to_hex_str(obj_meta_addr[obj_id]) : "0";
				std::string bright = obj_meta_auto_bright->isChecked() ? "1" : "0";
				
				entry = "[" + hash + ":" + name + ":" + type + ":" + vram + ":" + bright + "]";

				file << "\n" << entry;
			}
		}
	}

	file.close();

	//Update manifest tab if necessary
	parse_manifest_items();
}	

/****** Hashes the tile from a given layer ******/
std::string dmg_cgfx::hash_tile(u8 x, u8 y)
{
	//Hash from DMG BG
	if(layer_select->currentIndex() == 0) 
	{
		//Determine BG Map & Tile address
		u16 bg_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x8) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		//Determine the map entry from on-screen coordinates
		u8 tile_x = (x + main_menu::gbe_plus->ex_read_u8(REG_SX));
		u8 tile_y = (y + main_menu::gbe_plus->ex_read_u8(REG_SY));
		u16 map_entry = (tile_x / 8) + ((tile_y / 8) * 32);

		u8 map_value = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(bg_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		bg_tile_addr += (map_value * 16);
		cgfx::last_vram_addr = bg_tile_addr;

		return main_menu::gbe_plus->get_hash(bg_tile_addr, 2);
	}

	//Hash from DMG Window
	else if(layer_select->currentIndex() == 1) 
	{
		//Determine BG Map & Tile address
		u16 win_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x40) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		//Determine the map entry from on-screen coordinates
		u8 wx = main_menu::gbe_plus->ex_read_u8(REG_WX);
		wx = (wx < 7) ? 0 : (wx - 7); 
	
		u8 tile_x = (x - wx) / 8;
		u8 tile_y = (y - main_menu::gbe_plus->ex_read_u8(REG_WY)) / 8;
		u16 map_entry = tile_x + (tile_y * 32);

		u8 map_value = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(bg_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		bg_tile_addr += (map_value * 16);
		cgfx::last_vram_addr = bg_tile_addr;

		return main_menu::gbe_plus->get_hash(bg_tile_addr, 2);
	}

	//Hash from DMG OBJ
	if(layer_select->currentIndex() == 2) 
	{
		//Determine if in 8x8 or 8x16 mode
		u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

		for(u16 obj_index = 0; obj_index < 40; obj_index++)
		{
			//Grab X-Y OBJ coordinates
			u8 obj_x = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 1);
			u8 obj_y = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4));

			obj_x -= 8;
			obj_y -= 16;

			u8 test_left = ((obj_x + 8) > 0x100) ? 0 : obj_x;
			u8 test_right = (obj_x + 8);

			u8 test_top = ((obj_y + obj_height) > 0x100) ? 0 : obj_y;
			u8 test_bottom = (obj_y + obj_height);

			if((x >= test_left) && (x <= test_right) && (y >= test_top) && (y <= test_bottom))
			{
				//Grab address from OAM
				u16 tile_number = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 2);
				if(obj_height == 16) { tile_number &= ~0x1; }
				u16 obj_tile_addr = 0x8000 + (tile_number * 16);
				cgfx::last_vram_addr = obj_tile_addr;
				obj_index |= (tile_number << 8);

				return main_menu::gbe_plus->get_hash(obj_index, 1);
			}
		}
	}

	return "";
}

std::string gbc_cgfx::hash_tile(u8 x, u8 y)
{
	//Hash from GBC BG
	if (layer_select->currentIndex() == 0)
	{
		//Determine BG Map & Tile address
		u16 bg_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x8) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		//Determine the map entry from on-screen coordinates
		u8 tile_x = (x + main_menu::gbe_plus->ex_read_u8(REG_SX));
		u8 tile_y = (y + main_menu::gbe_plus->ex_read_u8(REG_SY));

		u16 map_entry = (tile_x / 8) + ((tile_y / 8) * 32);

		u8 old_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);

		//Read the BG attributes
		main_menu::gbe_plus->ex_write_u8(REG_VBK, 1);
		u8 bg_attribute = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);
		u8 pal_num = (bg_attribute & 0x7);
		u8 vram_bank = (bg_attribute & 0x8) ? 1 : 0;

		main_menu::gbe_plus->ex_write_u8(REG_VBK, 0);
		u8 map_value = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if (bg_tile_addr == 0x8800)
		{
			if (map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		bg_tile_addr += (map_value * 16);
		cgfx::last_vram_addr = bg_tile_addr;

		main_menu::gbe_plus->ex_write_u8(REG_VBK, old_vram_bank);

		cgfx::gbc_bg_vram_bank = vram_bank;
		cgfx::gbc_bg_color_pal = pal_num;

		return main_menu::gbe_plus->get_hash(bg_tile_addr, 2);
	}

	//Hash from GBC Window
	else if (layer_select->currentIndex() == 1)
	{
		//Determine BG Map & Tile address
		u16 win_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x40) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		//Determine the map entry from on-screen coordinates
		u8 wx = main_menu::gbe_plus->ex_read_u8(REG_WX);
		wx = (wx < 7) ? 0 : (wx - 7);

		u8 tile_x = (x - wx) / 8;
		u8 tile_y = (y - main_menu::gbe_plus->ex_read_u8(REG_WY)) / 8;
		u16 map_entry = tile_x + (tile_y * 32);

		u8 old_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);

		//Read the BG attributes
		main_menu::gbe_plus->ex_write_u8(REG_VBK, 1);
		u8 bg_attribute = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);
		u8 pal_num = (bg_attribute & 0x7);
		u8 vram_bank = (bg_attribute & 0x8) ? 1 : 0;

		main_menu::gbe_plus->ex_write_u8(REG_VBK, 0);
		u8 map_value = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if (bg_tile_addr == 0x8800)
		{
			if (map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		bg_tile_addr += (map_value * 16);
		cgfx::last_vram_addr = bg_tile_addr;

		main_menu::gbe_plus->ex_write_u8(REG_VBK, old_vram_bank);

		cgfx::gbc_bg_vram_bank = vram_bank;
		cgfx::gbc_bg_color_pal = pal_num;

		return main_menu::gbe_plus->get_hash(bg_tile_addr, 2);
	}

	//Hash from GBC OBJ
	else if (layer_select->currentIndex() == 2)
	{
		//Determine if in 8x8 or 8x16 mode
		u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

		for (u16 obj_index = 0; obj_index < 40; obj_index++)
		{
			//Grab X-Y OBJ coordinates
			u8 obj_x = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 1);
			u8 obj_y = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4));

			obj_x -= 8;
			obj_y -= 16;

			u8 test_left = ((obj_x + 8) > 0x100) ? 0 : obj_x;
			u8 test_right = (obj_x + 8);

			u8 test_top = ((obj_y + obj_height) > 0x100) ? 0 : obj_y;
			u8 test_bottom = (obj_y + obj_height);

			if ((x >= test_left) && (x <= test_right) && (y >= test_top) && (y <= test_bottom))
			{
				//Grab address from OAM
				u16 tile_number = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 2);
				if (obj_height == 16) { tile_number &= ~0x1; }
				u16 obj_tile_addr = 0x8000 + (tile_number * 16);
				cgfx::last_vram_addr = obj_tile_addr;
				obj_index |= (tile_number << 8);

				//Grab attributes
				u8 attributes = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 3);

				cgfx::gbc_obj_color_pal = attributes & 0x7;
				cgfx::gbc_obj_vram_bank = (attributes & 0x8) ? 1 : 0;

				return main_menu::gbe_plus->get_hash(obj_index, 1);
			}
		}
	}

	return "";
}

/****** Parses manifest entries to be viewed in the GUI ******/
bool gbe_cgfx::parse_manifest_items()
{
	std::vector <std::string> manifest;
	std::vector <u8> manifest_entry_size;

	std::ifstream file(get_manifest_file().c_str(), std::ios::in);
	std::string input_line = "";
	std::string line_char = "";

	if(!file.is_open())
	{
		std::cout<<"CGFX::Could not open manifest file " << get_manifest_file() << ". Check file path or permissions. \n";
		return false; 
	}

	//Cycle through whole file, line-by-line
	while(getline(file, input_line))
	{
		line_char = input_line[0];
		bool ignore = false;	
		u8 item_count = 0;	

		//Check if line starts with [ - if not, check for scaling factor skip line
		if(line_char == "[")
		{
			std::string line_item = "";

			//Cycle through line, character-by-character
			for(int x = 0; ++x < input_line.length();)
			{
				line_char = input_line[x];

				//Check for single-quotes, don't parse ":" or "]" within them
				if((line_char == "'") && (!ignore)) { ignore = true; }
				else if((line_char == "'") && (ignore)) { ignore = false; }

				//Check the character for item limiter : or ] - Push to Vector
				else if(((line_char == ":") || (line_char == "]")) && (!ignore)) 
				{
					manifest.push_back(line_item);
					line_item = "";
					item_count++;
				}

				else { line_item += line_char; }
			}
		}

		//Determine if manifest is properly formed (roughly)
		//Each manifest normal entry should have 5 parameters
		//Each metatile entry should have 3 parameters
		if((item_count != 5) && (item_count != 3) && (item_count != 0))
		{
			std::cout<<"CGFX::Manifest file " << get_manifest_file() << " has some missing parameters for some entries. \n";
			std::cout<<"CGFX::Entry -> " << input_line << "\n";
			file.close();
			return false;
		}

		else if(item_count != 0) { manifest_entry_size.push_back(item_count); }
	}
	
	file.close();

	//Clear previous layout
	QLabel* clear_label = new QLabel(" ");
	manifest_display->setWidget(clear_label);

	//If manifest is empty, quit now
	if(manifest.empty()) { return false; }

	int spacer = 0;
	int entry_count = 0;

	QGridLayout* temp_layout = new QGridLayout;
	temp_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	QWidget* manifest_regular_set = new QWidget;

	QSignalMapper* del_signal = new QSignalMapper(this);

	//Parse entries
	for(int x = 0, y = 0; y < manifest_entry_size.size(); y++)
	{
		//Parse regular entries
		if(manifest_entry_size[y] == 5)
		{
			std::string temp = "";
			bool is_meta = false;
			
			//Grab hash
			temp = "Hash : " + manifest[x++];
			QLabel* hash_label = new QLabel(QString::fromStdString(temp));

			//Grab file associated with hash
			temp = "File Name : " + manifest[x++];
			QLabel* file_label = new QLabel(QString::fromStdString(temp));
			temp = config::data_path + manifest[x - 1];

			//Determine if this belongs to a meta-tile entry
			u32 match_number = 0;
			std::string meta_file = temp;

			std::size_t match = meta_file.find_last_of("_") + 1;
			if(match != std::string::npos) { meta_file = meta_file.substr(match); }
			if(util::from_str(meta_file, match_number)) { is_meta = true; }

			//Try to load into QImage
			QImage temp_img(64, 64, QImage::Format_ARGB32);
			temp_img.fill(qRgb(0, 0, 0));

			u32 meta_width, meta_height = 0;

			if(temp_img.load(QString::fromStdString(temp)))
			{
				//Grab source dimensions
				meta_width = temp_img.width();
				meta_height = temp_img.height();

				//Scale to 128x128
				temp_img = temp_img.scaled(128, 128, Qt::KeepAspectRatio);

				temp = "Size : " + util::to_str(meta_width) + "x" + util::to_str(meta_height);
			}

			QLabel* size_label = new QLabel(QString::fromStdString(temp));

			QLabel* preview = new QLabel;
			preview->setPixmap(QPixmap::fromImage(temp_img));

			//Grab the type
			u32 type_byte = 0;
			util::from_str(manifest[x++], type_byte);

			switch(type_byte)
			{
				case 1: temp = "Type : OBJ"; break;
				case 2: temp = "Type : BG"; break;
		
				//Undefined type
				default:
					std::cout<<"CGFX::Undefined hash type " << (int)type_byte << "\n";
					return false;
			}

			QLabel* type_label = new QLabel(QString::fromStdString(temp));

			//EXT_VRAM_ADDR
			u32 vram_address = 0;
			util::from_hex_str(manifest[x++], vram_address);
			temp = vram_address ? "EXT_VRAM_ADDR - YES" : "EXT_VRAM_ADDR - NO";
			QLabel* vram_label = new QLabel(QString::fromStdString(temp));

			//EXT_AUTO_BRIGHT
			u32 bright_value = 0;
			util::from_str(manifest[x++], bright_value);
			temp = bright_value ? "EXT_AUTO_BRIGHT - YES" : "EXT_AUTO_BRIGHT - NO";
			QLabel* bright_label = new QLabel(QString::fromStdString(temp));

			//Spacer label
			if(!is_meta)
			{
				//Delete
				QPushButton* del_button = new QPushButton("Delete");
				connect(del_button, SIGNAL(clicked()), del_signal, SLOT(map()));
				del_signal->setMapping(del_button, entry_count);

				QLabel* spacer_label = new QLabel(" ");

				temp_layout->addWidget(hash_label, spacer++, 1, 1, 1);
				temp_layout->addWidget(file_label, spacer++, 1, 1, 1);
				temp_layout->addWidget(type_label, spacer++, 1, 1, 1);
				temp_layout->addWidget(size_label, spacer++, 1, 1, 1);
				temp_layout->addWidget(vram_label, spacer++, 1, 1, 1);
				temp_layout->addWidget(bright_label, spacer++, 1, 1, 1);
				temp_layout->addWidget(del_button, spacer++, 1, 1, 1);
				temp_layout->addWidget(spacer_label, spacer++, 1, 1, 1);

				temp_layout->addWidget(preview, (spacer - 8), 0, 6, 1);

				entry_count++;
			}
		}

		//Parse metatile entries
		else
		{
			std::string temp = "";

			//Grab file associated with hash
			temp = "File Name : " + manifest[x++];
			QLabel* file_label = new QLabel(QString::fromStdString(temp));
			temp = config::data_path + manifest[x - 1];
			x++;

			//Try to load into QImage
			QImage temp_img(64, 64, QImage::Format_ARGB32);
			temp_img.fill(qRgb(0, 0, 0));

			u32 meta_width, meta_height = 0;

			if(temp_img.load(QString::fromStdString(temp)))
			{
				//Grab source dimensions
				meta_width = temp_img.width();
				meta_height = temp_img.height();

				//Scale to 128x128
				temp_img = temp_img.scaled(128, 128, Qt::KeepAspectRatio);

				temp = "Size : " + util::to_str(meta_width) + "x" + util::to_str(meta_height);
			}
			
			QLabel* size_label = new QLabel(QString::fromStdString(temp));

			QLabel* preview = new QLabel;
			preview->setPixmap(QPixmap::fromImage(temp_img));

			//Grab metatile form
			u32 form_value = 0;
			util::from_str(manifest[x++], form_value);

			switch(form_value)
			{
				case 0: temp = "Type : BG Metatile"; break;
				case 1: temp = "Type : 8x8 OBJ Metatile"; break;
				case 2: temp = "Type : 8x16 OBJ Metatile"; break;
				
				default: return false;
			}

			QLabel* type_label = new QLabel(QString::fromStdString(temp));

			//Spacer label
			QLabel* spacer_label = new QLabel(" ");

			//Delete
			QPushButton* del_button = new QPushButton("Delete");
			connect(del_button, SIGNAL(clicked()), del_signal, SLOT(map()));
			del_signal->setMapping(del_button, entry_count);

			temp_layout->addWidget(file_label, spacer++, 1, 1, 1);
			temp_layout->addWidget(type_label, spacer++, 1, 1, 1);
			temp_layout->addWidget(size_label, spacer++, 1, 1, 1);
			temp_layout->addWidget(del_button, spacer++, 1, 1, 1);
			temp_layout->addWidget(spacer_label, spacer++, 1, 1, 1);
			temp_layout->addWidget(spacer_label, spacer++, 1, 1, 1);
			temp_layout->addWidget(spacer_label, spacer++, 1, 1, 1);
			temp_layout->addWidget(spacer_label, spacer++, 1, 1, 1);

			temp_layout->addWidget(preview, (spacer - 8), 0, 6, 1);

			entry_count++;
		}
	}

	connect(del_signal, SIGNAL(mapped(int)), this, SLOT(delete_manifest_entry(int))) ;

	manifest_regular_set->setLayout(temp_layout);	
	manifest_display->setWidget(manifest_regular_set);

	return true;
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

/****** Selects the current OBJ for the meta tile ******/
void gbe_cgfx::select_obj()
{
	u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

	//Grab OBJ data from core as QImage
	int obj_index = obj_meta_index->value();
	QImage selected_img = (obj_height == 16) ? grab_obj_data(obj_index).scaled(128, 256) : grab_obj_data(obj_index).scaled(256, 256);
	
	//Replace pixmap
	obj_select_img->setPixmap(QPixmap::fromImage(selected_img));
}

void gbe_cgfx::init()
{
	cgfx_scale->setCurrentIndex(cgfx::scaling_factor - 1);
}