/* INCLUDES */
#ifndef _SdlApp_H_
#define _SdlApp_H_
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <stdexcept>
#if __GNUG__
#   include <tr1/memory>
#endif
#include "cvec.h"
#include "matrix4.h"
#include "geometrymaker.h"
#include "ppm.h"
#include "glsupport.h"
#include <iostream>
#include <SDL2/SDL.h>
#undef main
#include <SDL2/SDL_image.h>
#include <GL/glu.h>
#include <GL/glut.h>

using namespace std;
//for string, vector, iostream, and other standard C++ stuff
using namespace tr1;

/*
 This application uses SDL functions to make window and handle events.
 */
class SdlApp
{
private:
   bool running;
   SDL_Window* display;
   void keydown(const char * key);

public:
   SdlApp();
   int run();
   void handleEvent(SDL_Event* e);
   void clearCanvas();
   void draw();
};



#endif
