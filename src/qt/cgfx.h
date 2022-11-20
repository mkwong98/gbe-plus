// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : cgfx.h
// Date : July 25, 2015
// Description : Custom graphics settings
//
// Dialog for various custom graphics options

#ifndef CGFX_GBE_QT
#define CGFX_GBE_QT

#include <vector>

#include <SDL2/SDL.h>

#ifdef GBE_QT_5
#include <QtWidgets>
#endif

#ifdef GBE_QT_4
#include <QtGui>
#endif

#include "common/common.h"
#include "dmg/lcd_data.h"
#include "dmg/custom_graphics_data.h"

class gbe_cgfx : public QDialog
{
	Q_OBJECT
	
	public:
	gbe_cgfx(QWidget *parent = 0);

	void draw_gb_layer(u8 layer);

	void reset_inputs();
	void init();

	QTabWidget* tabs;
	QDialogButtonBox* tabs_button;

	QComboBox* cgfx_scale;

	//Layers tab widgets
	QComboBox* layer_select;
	QSpinBox* render_stop_line;
	QLabel* tile_id;
	QLabel* h_v_flip;
	QLabel* tile_palette;
	QLabel* hash_text;

	QSpinBox* rect_x;
	QSpinBox* rect_w;
	QSpinBox* rect_y;
	QSpinBox* rect_h;

	bool pause;

	u32 rawImageData[160 * 144];
	rendered_screen screenInfo;

	protected:
	void closeEvent(QCloseEvent* event);
	bool eventFilter(QObject* target, QEvent* event);

	QWidget* layers_set;

	QGridLayout* layers_layout;

	QPushButton* a_input;
	QPushButton* b_input;
	QPushButton* select_input;
	QPushButton* start_input;
	QPushButton* left_input;
	QPushButton* right_input;
	QPushButton* up_input;
	QPushButton* down_input;

	QLabel* current_layer;
	QLabel* current_tile;

	void update_preview(u32 x, u32 y);
	virtual std::string get_palette_code(u16 p) = 0;
	virtual void renderTile(u16 tileID, u16 palId, u8 palSel, u8 layer, std::vector<u32>* top, std::vector<u32>* bottom) = 0;
	QImage renderTileToImage(u16 tileID, u16 palId, u8 palSel, u8 layer);

	u32 mouse_start_x, mouse_start_y;
	bool mouse_drag;
	bool meta_highlight;

	dmg_cgfx_data* pack_data;

	protected slots:
	void close_cgfx();
	void dump_selection();
	void copy_selection();
	void layer_change();
	void update_selection();
	void advance_next_frame();
	void update_input_control(int index);
};

class dmg_cgfx : public gbe_cgfx
{
public:

protected:
	std::string get_palette_code(u16 p);
	void renderTile(u16 tileID, u16 palId, u8 palSel, u8 layer, std::vector<u32>* top, std::vector<u32>* bottom);
};

class gbc_cgfx : public gbe_cgfx
{
public:

protected:
	std::string get_palette_code(u16 p);
	void renderTile(u16 tileID, u16 palId, u8 palSel, u8 layer, std::vector<u32>* top, std::vector<u32>* bottom);
};

#endif //CGFX_GBE_QT 
