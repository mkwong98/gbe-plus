// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : debug_dmg.h
// Date : November 21, 2015
// Description : DMG/GBC debugging UI
//
// Dialog for DMG/GBC debugging
// Shows MMIO registers, CPU state, instructions, memory

#ifndef DMG_DEBUG_GBE_QT
#define DMG_DEBUG_GBE_QT

#ifdef GBE_QT_5
#include <QtWidgets>
#endif

#ifdef GBE_QT_4
#include <QtGui>
#endif

#include "common/common.h"

class dmg_debug : public QDialog
{
	Q_OBJECT
	
	public:
	dmg_debug(QWidget *parent = 0);

	QTabWidget* tabs;
	QDialogButtonBox* tabs_button;
	QPushButton* refresh_button;

	bool pause;
	bool old_pause;
	bool debug_reset;

	void auto_refresh();

	private:
	//MMIO registers
	QLineEdit* mmio_lcdc;
	QLineEdit* mmio_stat;
	QLineEdit* mmio_sx;
	QLineEdit* mmio_sy;
	QLineEdit* mmio_ly;
	QLineEdit* mmio_lyc;
	QLineEdit* mmio_dma;
	QLineEdit* mmio_bgp;
	QLineEdit* mmio_obp0;
	QLineEdit* mmio_obp1;
	QLineEdit* mmio_wx;
	QLineEdit* mmio_wy;

	QLineEdit* mmio_nr10;
	QLineEdit* mmio_nr11;
	QLineEdit* mmio_nr12;
	QLineEdit* mmio_nr13;
	QLineEdit* mmio_nr14;
	QLineEdit* mmio_nr21;
	QLineEdit* mmio_nr22;
	QLineEdit* mmio_nr23;
	QLineEdit* mmio_nr24;
	QLineEdit* mmio_nr30;
	QLineEdit* mmio_nr31;
	QLineEdit* mmio_nr32;
	QLineEdit* mmio_nr33;
	QLineEdit* mmio_nr34;
	QLineEdit* mmio_nr41;
	QLineEdit* mmio_nr42;
	QLineEdit* mmio_nr43;
	QLineEdit* mmio_nr44;
	QLineEdit* mmio_nr50;
	QLineEdit* mmio_nr51;
	QLineEdit* mmio_nr52;

	QLineEdit* mmio_key1;
	QLineEdit* mmio_rp;
	QLineEdit* mmio_vbk;
	QLineEdit* mmio_hdma1;
	QLineEdit* mmio_hdma2;
	QLineEdit* mmio_hdma3;
	QLineEdit* mmio_hdma4;
	QLineEdit* mmio_hdma5;
	QLineEdit* mmio_bcps;
	QLineEdit* mmio_bcpd;
	QLineEdit* mmio_ocps;
	QLineEdit* mmio_ocpd;
	QLineEdit* mmio_svbk;

	QLineEdit* mmio_p1;
	QLineEdit* mmio_div;
	QLineEdit* mmio_tima;
	QLineEdit* mmio_tma;
	QLineEdit* mmio_tac;
	QLineEdit* mmio_ie;
	QLineEdit* mmio_if;
	QLineEdit* mmio_sb;
	QLineEdit* mmio_sc;

	//Memory widgets
	QTextEdit* mem_addr;
	QTextEdit* mem_values;
	QTextEdit* mem_ascii;

	QString addr_text;
	QString values_text;
	QString ascii_text;

	std::string ascii_lookup;

	QScrollBar* mem_scrollbar;

	protected:
	void closeEvent(QCloseEvent* event);

	private slots:
	void scroll_mem(int value);
	void scroll_text(int type);
	void refresh();
	void click_refresh();
	void close_debug();

	void db_reset();
	void db_reset_run();
};


#endif //DMG_DEBUG_GBE_QT
