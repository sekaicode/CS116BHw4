/*---------------------------------------------------------------------------*/
/* INCLUDES */
#include "SdlApp.h"
#include "Objects.h"

/*---------------------------------------------------------------------------*/
/* GLOBALS */
vector<Light> lights;

/*---------------------------------------------------------------------------*/
/* FUNCTIONS */
/*
 PURPOSE: multiplies the supplied point vector by the scalar amount
 RECEIVES:
 p - Point vector (x,y,z)
 scalar -- the scalar `a' to multiply by
 RETURNS:
 the point vector
 (a*x, a*y, a*z)
 REMARKS:
 */
Point operator*(GLdouble scalar, const Point& p)
{
   return Point(scalar * p._x, scalar * p._y, scalar * p._z);
}

/*
 PURPOSE: multiplies the supplied point vector by the scalar amount
 RECEIVES:
 p - Point vector (x,y,z)
 scalar -- the scalar `a' to multiply by
 RETURNS:
 the point vector
 (a*x, a*y, a*z)
 REMARKS:
 */
Point operator*(const Point& p, GLdouble scalar)
{
   return Point(scalar * p._x, scalar * p._y, scalar * p._z);
}

/*
 PURPOSE: generate a random vector of length 1
 RECEIVES: Nothing
 RETURNS: Nothing
 REMARKS:
 */
Point randomlyPoint()
{
   //generate random point within unit sphere
   Point vec(0.0, 0.0, 0.0);

   while (vec.isZero())
   {
      vec = Point(double(rand()) / (RAND_MAX + 1.0) - .5,
            double(rand()) / (RAND_MAX + 1.0) - .5,
            double(rand()) / (RAND_MAX + 1.0) - .5);
   }
   vec.normalize(); //push out to unit sphere

   return vec;
}

/*
 PURPOSE: calculates how much light intensities will decay with distance
 RECEIVES: distance -- to use
 RETURNS: decimal value between 0 and 1 by which an intensity at the given distance
 would be reduced
 REMARKS:
 */
inline GLdouble attenuate(GLdouble distance)
{
   return ATTENUATION_FACTOR / (ATTENUATION_FACTOR + distance * distance);
}

/*
 PURPOSE: Does ray tracing of a single ray in a scene according to the supplied lights to the perscribed
 depth
 RECEIVES:
 scene -- Shape to do ray-tracing one
 lights -- Light's which are lighting the scene
 ray -- to be used for ray-tracing consists of two points (starting point to do ray-tracing from plus
 another point which together give the direction of the initial ray.)
 color -- used to store the color returned by doing the ray tracing
 depth -- in terms of tree of sub-rays we calculate
 RETURNS:  Nothing
 REMARKS:
 */
void traceRay(Shape& scene, vector<Light> lights, const Line& ray, Point& color,
      unsigned int depth)
{
   Intersection intersection;
   scene.doIIntersectWith(ray, Point(0.0, 0.0, 0.0), intersection);

   if (!intersection.intersects())
      return;

   Point pt = intersection.point();
   Material material = intersection.material();
   Line reflectedRay = intersection.reflectedRay();
   Line transmittedRay = intersection.transmittedRay();
   Line shadowRay;
   Point lColor;

   size_t size = lights.size();
   for (size_t i = 0; i < size; i++)
   {
      shadowRay.set(pt, lights[i].position());
      Intersection shadowIntersection;

      scene.doIIntersectWith(shadowRay, Point(0.0, 0.0, 0.0), shadowIntersection);

      if (!shadowIntersection.intersects()
            || !shadowIntersection.material().transparency().isZero())
      {
         lColor = attenuate(shadowRay.length()) * lights[i].color();
         color += (material.ambient() % lColor)
               + abs(intersection.normal() & shadowRay.direction())
                     * (material.diffuse() % lColor)
               + abs(ray.direction() & reflectedRay.direction())
                     * (material.specular() % lColor);
      }
   }

   if (depth > 0)
   {
      Point transmittedColor(0.0, 0.0, 0.0);
      Point reflectedColor(0.0, 0.0, 0.0);

      Point transparency = material.transparency();
      Point opacity = Point(1.0, 1.0, 1.0) - transparency;

      if (!transparency.isZero() && transparency.length() > SMALL_NUMBER) //if not transparent then don't send ray
      {
         traceRay(scene, lights, transmittedRay, transmittedColor, depth - 1);
         color += (transparency % transmittedColor);
      }
      if (!opacity.isZero()) // if completely transparent don't send reflect ray
      {
         traceRay(scene, lights, reflectedRay, reflectedColor, depth - 1);
         color += (opacity % reflectedColor);
      }
   }
}

/*
 PURPOSE: Does the ray-tracing scene objects according to the supplied lights, camera dimension and screen dimensions
 RECEIVES:
 scene --  Shape to be ray-traced (Shapes use the Composite pattern so are made of sub-shapes
 light -- a vector of Light's used to light the scene
 camera -- location of the viewing position
 lookat -- where one is looking at from this position
 up -- what direction is up from this position
 bottomX -- how far to the left from the lookat point is the start of the screen
 bottomy -- how far down from the lookat point is the start of the screen
 width -- width of the screen
 height -- height of the screen
 RETURNS:  Nothing
 REMARKS:
 */
void traceRayScreen(Shape& scene, vector<Light>& lights, Point camera,
      Point lookAt, Point up, int bottomX, int bottomY, int width, int height)
{
   Point lookDirection = lookAt - camera;
   Point right = lookDirection * up;

   right.normalize();
   Point rightOffset = width * right;

   up = right * lookDirection;
   up.normalize();

   Point screenPt = lookAt + bottomX * right + bottomY * up;

   Line ray;
   Point color(0.0, 0.0, 0.0);
   Point avgColor(0.0, 0.0, 0.0);

   Point weightedColor(0.0, 0.0, 0.0);
   Point oldWeightedColor(0.0, 0.0, 0.0);
   GLdouble k;

   glBegin(GL_POINTS);
   for (int j = 0; j < height; j++)
   {
      for (int i = 0; i < width; i++)
      {
         for (k = 0.0; k < SUPER_SAMPLE_NUMBER; k++)
         {
            ray.set(camera, screenPt + .5 * randomlyPoint());

            color.set(0.0, 0.0, 0.0);

            traceRay(scene, lights, ray, color, MAX_DEPTH);

            oldWeightedColor = (k + 1.0) * avgColor;

            avgColor += color;

            weightedColor = k * avgColor;
            if ((weightedColor - oldWeightedColor).length()
                  < SMALL_NUMBER * k * (k + 1))
               break;
         }

         avgColor /= k;
         glColor3d(avgColor.x(), avgColor.y(), avgColor.z());

         glVertex2i(i, j);
         screenPt += right;
      }
      screenPt -= rightOffset;
      screenPt += up;
   }
   glEnd();
}

/*
 PURPOSE: Converts a string of two characters of the form:
 letter row + number color (for example, b4) into coordinates for a piece of
 our ray tracing work.
 RECEIVES: coordString -- string to convert
 RETURNS: Nothing
 REMARKS:
 */
Point stringToCoord(string str)
{
   Point firstSquare(-BOARD_EDGE_SIZE / 2, 0.0, BOARD_EDGE_SIZE / 2);

   Point rowOffset(0.0, 0.0, -(double(str[0] - 'a') + .5) * SQUARE_EDGE_SIZE);
   //negative because farther back == higher row number
   Point colOffset((double(str[1] - '0' - 1) + .5) * SQUARE_EDGE_SIZE, 0.0,
         0.0);
   Point heightOffset(0.0, 1.5 * SQUARE_EDGE_SIZE, 0.0);

   Point square = firstSquare + rowOffset + colOffset + heightOffset;

   return square;
}

/*
 PURPOSE: gets locations of objects from users
 sets the background color to black,
 sets up display list for coordinate axes,
 initializes extrusions vector
 RECEIVES: Nothing
 RETURNS: Nothing
 REMARKS:
 */
void showObjectsMenu()
{
   //make objects
   string tmp;
   while (tmp != "done")
   {
      cout
            << "Enter your object (light, tetrahedron, sphere, cube, cone, cylinder), or \"done\":\n";
      cin >> tmp;
      if (tmp == "light")
      {
         cout << "Enter the position of the light:\n";
         cin >> tmp;
         //light is 5 squares above board
         lights.push_back(
               Light(lightColor,
                     Point(BOARD_POSITION)
                           + Point(0.0, 3.5 * SQUARE_EDGE_SIZE, 0.0)
                           + stringToCoord(tmp)));
      }

      else if (tmp == "tetrahedron")
      {
         cout << "enter the position of the tetrahedron:\n";
         cin >> tmp;
         Tetrahedron *tetrahedron = new Tetrahedron(stringToCoord(tmp),
               SQUARE_EDGE_SIZE);
         scene.addRayObject(tetrahedron);
      }
      else if (tmp == "sphere")
      {
         cout << "enter the position of the sphere:\n";
         cin >> tmp;
         Sphere *sphere = new Sphere(stringToCoord(tmp), SQUARE_EDGE_SIZE / 2);
         scene.addRayObject(sphere);
      }
      else if (tmp == "cube")
      {
         cout << "enter the position of the cube:\n";
         cin >> tmp;
         Cube *cube = new Cube(stringToCoord(tmp), SQUARE_EDGE_SIZE);
         scene.addRayObject(cube);
      }
      else if (tmp == "cone")
      {
         cout << "enter the position of the cube:\n";
         cin >> tmp;
         Cube *cube = new Cube(stringToCoord(tmp), SQUARE_EDGE_SIZE);
         scene.addRayObject(cube);
      }
      else if (tmp == "cylinder")
      {
         cout << "enter the position of the cube:\n";
         cin >> tmp;
         Cube *cube = new Cube(stringToCoord(tmp), SQUARE_EDGE_SIZE);
         scene.addRayObject(cube);
      }
   }

   glViewport(0, 0, winWidth, winHeight);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0, winWidth, 0, winHeight);
}

/* PURPOSE: clear framebuffer color & depth.
 */
void SdlApp::clearCanvas()
{
   glFlush();
   glClearColor(0.0, 0.0, 0.0, 0.0);
   SDL_GL_SwapWindow(display);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/*
 PURPOSE: Used to craw the complete ray-traced chessboard
 RECEIVES: Nothing
 RETURNS:  Nothing
 REMARKS:
 */
void SdlApp::draw()
{
   traceRayScreen(scene, lights, Point(CAMERA_POSITION), Point(LOOK_AT_VECTOR),
         Point(UP_VECTOR), -winWidth / 2, -winHeight / 2, winWidth, winHeight);
}

void makeObjects()
{
   //make board
   scene.addRayObject(new CheckerBoard(Point(0, 0, 0)));

   //make objects
   showObjectsMenu();
}

/* PURPOSE: Creates an sdl application.
 */
SdlApp::SdlApp()
{
   running = true;

   SDL_Init(SDL_INIT_EVERYTHING);

   // Turn on double buffering with a 24bit Z buffer.
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

   display = SDL_CreateWindow("hw4", SDL_WINDOWPOS_UNDEFINED,
   SDL_WINDOWPOS_UNDEFINED, winWidth, winHeight,
         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

   // Create our opengl context and attach it to our window
   SDL_GLContext maincontext = SDL_GL_CreateContext(display);

   // buffer swap syncronized with monitor's vertical refresh 
   SDL_GL_SetSwapInterval(1);

   //init more stuff
   glewInit();
   glClearDepth(0);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_GREATER);

   //makeShaders();
   makeObjects();
   //makeTextures();
}

/* PURPOSE: Executes the SDL application. Loops until event to quit.
 RETURN: -1 if fail, 0 success.
 */
int SdlApp::run()
{
   SDL_Event e;
   while (running)
   {
      //while (SDL_PollEvent(&e))
      //   handleEvent(&e);

      clearCanvas();
      draw();
   }
   SDL_Quit();
   return 0;
}

int main(int argc, char **argv)
{
   return SdlApp().run();
}
