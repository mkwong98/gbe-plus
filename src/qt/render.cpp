// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : render.h
// Date : June 22, 2015
// Description : Qt screen rendering
//
// Renders the screen for an emulated system using Qt

#include "render.h"

#include "common/config.h"

namespace qt_gui
{
	QImage* screen = NULL;
	main_menu* draw_surface = NULL;
	SDL_Surface* final_screen = NULL;
}

/****** Renders an LCD's screen buffer to a QImage ******/
void render_screen_sw(u32* image)
{
	int width, height = 0;

	//Determine the dimensions of the source image
	width = config::sys_width;
	height = config::sys_height;

	u32* pt = image;
	//Fill in image with pixels from the emulated LCD
	for (int y = 0; y < height; y++)
	{
		u32* pixel_data = (u32*)qt_gui::screen->scanLine(y);
		std::copy(pt, pt + width, pixel_data);
		pt += width;
	}

	if (qt_gui::draw_surface != NULL) { qt_gui::draw_surface->update(); }

	QApplication::processEvents();
}

/****** Renders an LCD's screen buffer to an SDL Surface ******/
void render_screen_hw(SDL_Surface* image) 
{
	if(config::request_resize) { qt_gui::draw_surface->update(); }

	qt_gui::final_screen = image;

	if(qt_gui::draw_surface != NULL) { qt_gui::draw_surface->hw_screen->updateGL(); }

	QApplication::processEvents();
}
