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
	
	QDialog* layers_tab = new QDialog;

	tabs->addTab(layers_tab, tr("Layers Viewer"));
	tabs_button = new QDialogButtonBox(QDialogButtonBox::Close);

	layers_set = new QWidget(layers_tab);

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
	QPushButton* dump_section_button = new QPushButton("Dump new tiles in current selection");
	QPushButton* copy_section_button = new QPushButton("Copy to clipboard");
	QVBoxLayout* section_final_layout = new QVBoxLayout;
	section_final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	section_final_layout->setSpacing(0);
	section_final_layout->addWidget(section_x_set);
	section_final_layout->addWidget(section_y_set);
	section_final_layout->addWidget(dump_section_button);
	section_final_layout->addWidget(copy_section_button);
	section_set->setLayout(section_final_layout);

	//Layers Tab layout
	QGridLayout* layers_tab_layout = new QGridLayout;
	layers_tab_layout->addWidget(select_set, 0, 0, 1, 1);
	layers_tab_layout->addWidget(current_layer, 1, 0, 1, 1);
	layers_tab_layout->addWidget(frame_control_set, 2, 0, 1, 1);

	layers_tab_layout->addWidget(current_tile, 1, 1, 1, 1);
	layers_tab_layout->addWidget(layer_info, 0, 1, 1, 1);
	layers_tab_layout->addWidget(section_set, 2, 1, 1, 1);
	layers_tab->setLayout(layers_tab_layout);

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
	connect(layer_select, SIGNAL(currentIndexChanged(int)), this, SLOT(layer_change()));
	connect(rect_x, SIGNAL(valueChanged(int)), this, SLOT(update_selection()));
	connect(rect_y, SIGNAL(valueChanged(int)), this, SLOT(update_selection()));
	connect(rect_w, SIGNAL(valueChanged(int)), this, SLOT(update_selection()));
	connect(rect_h, SIGNAL(valueChanged(int)), this, SLOT(update_selection()));
	connect(dump_section_button, SIGNAL(clicked()), this, SLOT(dump_selection()));
	connect(copy_section_button, SIGNAL(clicked()), this, SLOT(copy_selection()));
	connect(next_frame, SIGNAL(clicked()), this, SLOT(advance_next_frame()));

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

	resize(800, 450);
	setWindowTitle(tr("Custom Graphics"));

	render_stop_line->setValue(0x90);

	pause = false;
	hash_text->setScaledContents(true);

	mouse_start_x = mouse_start_y = 0;
	mouse_drag = false;

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

	if (rect_w->value() > 0 && rect_h->value() > 0)
	{
		if (mouse_drag) {
			painter.setPen(QPen(QColor(255, 0, 0)));
			painter.drawRect(rect_x->value() * 2 + 1, rect_y->value() * 2 + 1, rect_w->value() * 2 - 1, rect_h->value() * 2 - 1);
			painter.setPen(QPen(QColor(0, 255, 255)));
			painter.drawRect(rect_x->value() * 2, rect_y->value() * 2, rect_w->value() * 2 - 1, rect_h->value() * 2 - 1);
		}

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

void gbe_cgfx::copy_selection()
{
	if (main_menu::gbe_plus == NULL) { return; }
	if ((rect_w->value() == 0) || (rect_h->value() == 0)) { return; }

	std::vector<pack_tile> addedTile;
	std::string content = "";

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
				t.x = strips[i].x;
				t.y = y - strips[i].line;

				bool added = false;
				//check to see if the tile has been added in current selection
				for (u16 j = 0; j < addedTile.size() && !added; j++)
				{
					if (addedTile[j].tileStr == t.tileStr && addedTile[j].palStr == t.palStr && addedTile[j].x == t.x && addedTile[j].y == t.y)
					{
						added = true;
					}
				}
				if (!added)
				{
					//add to txt
					content += t.tileStr + "," + t.palStr + "," + util::to_str(t.x) + "," + util::to_str(t.y) + "\n";
					addedTile.push_back(t);
				}
			}
		}
	}
	QClipboard* clipboard = QGuiApplication::clipboard();
	QString originalText = clipboard->text();
	clipboard->setText(QString(content.c_str()));
}

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

void gbe_cgfx::init()
{
	cgfx_scale->setCurrentIndex(cgfx::scaling_factor - 1);
}