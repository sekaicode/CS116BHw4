/*---------------------------------------------------------------------------*/
/* INCLUDES */
#include "SdlApp.h"
#include "Objects.h"

/*---------------------------------------------------------------------------*/
/* GLOBALS */


vector<Light> lights;
int redoMenu = 0; //This allows user to redo menu 
vector<Point> currentPosition; //This is the current positions in the scene
static ShaderState *g_shader;// our global shader states

//static const int G_NUM_SHADERS = 1;
//changed array sizes from 3 to 2, revert if things break.

static const char * const G_SHADER_FILES[2] = 
	{"./shaders/basic-gl3.vshader", "./shaders/square-test-gl3.fshader"};


// --------- Geometry
static GLfloat g_eyePosition[3] = { 0.0, 0.0, 1.0};
#define SPHERE 1 // assume sphere in format radius, base point
static GLfloat g_geometryData[5] = { SPHERE, -0.2, 0.0, 0.0, 0.0 };
static Matrix4 g_objectRbt[1] = {Matrix4::makeTranslation(Cvec3(0,0,0))};
static Matrix4 g_skyRbt = Matrix4::makeTranslation(Cvec3(0.0, 0.0, 1.5));
static const float g_frustMinFov = 60.0;  //A minimal of 60 degree field of view
static float g_frustFovY = g_frustMinFov; // FOV in y direction

static const float g_frustNear = -0.1;    // near plane
static const float g_frustFar = -50.0;    // far plane
static int g_windowWidth = 512;
static int g_windowHeight = 512;

static Geometry *g_plane;


/*
 Shader state of a GL program.
 It holds uniform variables and vertex attributes.
 It also holds handle to a program, and attaches shaders to it.
 */

ShaderState::ShaderState(const char *vsfn, const char *fsfn)
{
	readAndCompileShader(program, vsfn, fsfn);
	const GLuint h = program; // short hand

	// Retrieve handles to uniform variables
	h_uProjMatrix = safe_glGetUniformLocation(h, "uProjMatrix");
	h_uModelViewMatrix = safe_glGetUniformLocation(h, "uModelViewMatrix");
	
	// Retrieve handles to vertex attributes
	h_aPosition = safe_glGetAttribLocation(h, "aPosition");
	
	glBindFragDataLocation(h, 0, "fragColor");

	checkGlErrors();
}

// --------- Geometry
// Macro used to obtain relative offset of a field within a struct
#define FIELD_OFFSET(StructType, field) &(((StructType *)0)->field)

/*
 Geometry struct.
 It holds the coordinates of vectices and textures in buffer objects
 and draws them.
 */

Geometry::Geometry(GenericVertex *vtx, unsigned short *idx, int vboLen, int iboLen)
{
	this->vboLen = vboLen;
	this->iboLen = iboLen;

	// create vertex buffer object
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GenericVertex) * vboLen, vtx,
	GL_STATIC_DRAW);

	//create index buffer object
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * iboLen,
			idx, GL_STATIC_DRAW);
}


/*
 PURPOSE: Draws the opengl objects.
 Uses what shader state specifies such as drawing a sphere, giving it
 coordinates, and its texture.
 RECEIVES:	current shader state address
 */
void Geometry::draw(const ShaderState& CUR_SS)
{
	// Enable the attributes used by our shader
	safe_glEnableVertexAttribArray(CUR_SS.h_aPosition);

	// bind vertex buffer object
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	safe_glVertexAttribPointer(CUR_SS.h_aPosition, 3, GL_FLOAT, GL_FALSE,
			sizeof(GenericVertex), FIELD_OFFSET(GenericVertex, pos));
			
	// bind index buffer object
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

	// draw!
	glDrawElements(GL_TRIANGLES, iboLen, GL_UNSIGNED_SHORT, 0);
	

	// Disable the attributes used by our shader
	safe_glDisableVertexAttribArray(CUR_SS.h_aPosition);
}



// takes a projection matrix and send to the the shaders
static void sendProjectionMatrix(const ShaderState& curSS,
                                 const Matrix4& projMatrix)
{
    GLfloat glmatrix[16];
    projMatrix.writeToColumnMajorMatrix(glmatrix); // send projection matrix
    safe_glUniformMatrix4fv(curSS.h_uProjMatrix, glmatrix);
}

// takes MVM and its normal matrix to the shaders
static void sendGeometry(const ShaderState& curSS, const Matrix4& MVM)
{
    GLfloat glmatrix[16];
    MVM.writeToColumnMajorMatrix(glmatrix); // send MVM
    safe_glUniformMatrix4fv(curSS.h_uModelViewMatrix, glmatrix);

	//glUniform1fv(curSS.h_uEyePosition, 4, g_eyePosition);
	//glUniform1fv(curSS.h_uGeometry, 4, g_geometryData);
}

static Matrix4 makeProjectionMatrix()
{
    return Matrix4::makeProjection(g_frustFovY,
        g_windowWidth / static_cast <double> (g_windowHeight),
        g_frustNear, g_frustFar);
}

static void drawStuff()
{
    // build & send proj. matrix to vshader
    const Matrix4 projmat = makeProjectionMatrix();
    sendProjectionMatrix(*g_shader, projmat);

    // use the skyRbt as the eyeRbt
    const Matrix4 eyeRbt = g_skyRbt;
    const Matrix4 invEyeRbt = inv(eyeRbt);

    const Matrix4 groundRbt = Matrix4();  // identity
    Matrix4 MVM = invEyeRbt * groundRbt;
    // draw cubes
    // ==========
    MVM = invEyeRbt * g_objectRbt[0];
    sendGeometry(*g_shader, MVM);
    g_plane->draw(*g_shader);
}

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
	int found = 0; //used to break out of loop
	vector<RayObject *>::iterator it;
	vector<Light>::iterator it2;
	while (tmp != "done")
	{
		if (redoMenu == 0)
		cout << "Enter your object (light, tetrahedron, sphere, cube, cone, cylinder), or \"done\":\n";
	  else if (redoMenu == 1)
		cout << "Re-enter your object (light, tetrahedron, sphere, cube, cone, cylinder), or \"done\":\n"; 
	  cin >> tmp;
		if (tmp == "light")
		{
		 redoMenu = 1;
			cout << "Enter the position of the light:\n";
			cin >> tmp;

		 for (it2 = lights.begin(); it2 != lights.end();)
		 {
				 if (found == 0)
				 {
					 if ((Point(BOARD_POSITION)
									+ Point(0.0, 3.5 * SQUARE_EDGE_SIZE, 0.0)
							+ stringToCoord(tmp)) == (*it2).position())
					 {
							cout<<"Overriding previous object at location with light\n";
							it2 = lights.erase(it2);
							found = 1;
					 }
					 else 
						 ++it2;
				 }
				 else
					 ++it2;
		 }
		 found = 0;

		 //light is 5 squares above board
			lights.push_back(
					Light(lightColor,
							Point(BOARD_POSITION)
									+ Point(0.0, 3.5 * SQUARE_EDGE_SIZE, 0.0)
									+ stringToCoord(tmp)));
		 
		}

		else if (tmp == "tetrahedron")
		{
			redoMenu = 1;
		 cout << "enter the position of the tetrahedron:\n";
			cin >> tmp;
		 
		 for (it = scene.subObject().begin(); it != scene.subObject().end();)
		 {
				 if (found == 0)
				 {
					 if (stringToCoord(tmp) == (*it)->position())
					 {
							cout<<"Overriding previous object at location with tetrahedron\n";
							delete *it;
							it = scene.subObject().erase(it);
							found = 1;
					 }
					 else
						 ++it;
				 }
				 else
					 ++it;
		 }
		 found = 0;

			Tetrahedron *tetrahedron = new Tetrahedron(stringToCoord(tmp),
					SQUARE_EDGE_SIZE);
			scene.addRayObject(tetrahedron);
		 
		}
		else if (tmp == "sphere")
		{
		 redoMenu = 1;
			cout << "enter the position of the sphere:\n";
			cin >> tmp;
			
		 for (it = scene.subObject().begin(); it != scene.subObject().end();)
		 {
				 if (found == 0)
				 {
					 if (stringToCoord(tmp) == (*it)->position())
					 {
							cout<<"Overriding previous object at location with sphere\n";
							delete *it;
							it = scene.subObject().erase(it);
							found = 1;
					 }
					 else
						 ++it;
				 }
				 else
					 ++it;
		 }
		 found = 0;
		 
		 Sphere *sphere = new Sphere(stringToCoord(tmp), SQUARE_EDGE_SIZE / 2);
			scene.addRayObject(sphere);
		 
		}
		else if (tmp == "cube")
		{
		 redoMenu = 1;
			cout << "enter the position of the cube:\n";
			cin >> tmp;
			
		for (it = scene.subObject().begin(); it != scene.subObject().end();)
		 {
				 if (found == 0)
				 {
					 if (stringToCoord(tmp) == (*it)->position())
					 {
							cout<<"Overriding previous object at location with cube\n";
							delete *it;
							it = scene.subObject().erase(it);
							found = 1;
					 }
					 else
						 ++it;
				 }
				 else
					 ++it;
		 }
		 found = 0;
		 
		 
		 Cube *cube = new Cube(stringToCoord(tmp), SQUARE_EDGE_SIZE);
			scene.addRayObject(cube);
		 
		}
		else if (tmp == "cone")
		{
		 redoMenu = 1;
			cout << "enter the position of the cube:\n";
			cin >> tmp;

		 for (it = scene.subObject().begin(); it != scene.subObject().end();)
		 {
				 if (found == 0)
				 {
					 if (stringToCoord(tmp) == (*it)->position())
					 {
							cout<<"Overriding previous object at location with cone\n";
							delete *it;
							it = scene.subObject().erase(it);
							found = 1;
					 }
					 else
						 ++it;
				 }
				 else
					 ++it;
		 }
		 found = 0;

			Cube *cube = new Cube(stringToCoord(tmp), SQUARE_EDGE_SIZE);
			scene.addRayObject(cube);
		 
		}
		else if (tmp == "cylinder")
		{
		 redoMenu = 1;
			cout << "enter the position of the cube:\n";
			cin >> tmp;

		 for (it = scene.subObject().begin(); it != scene.subObject().end();)
		 {
				 if (found == 0)
				 {
					 if (stringToCoord(tmp) == (*it)->position())
					 {
							cout<<"Overriding previous object at location with cylinder\n";
							delete *it;
							it = scene.subObject().erase(it);
							found = 1;
					 }
					 else
						 ++it;
				 }
				 else
					 ++it;
		 }
		 found = 0;

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
	/*
	traceRayScreen(scene, lights, Point(CAMERA_POSITION), Point(LOOK_AT_VECTOR),
			Point(UP_VECTOR), -winWidth / 2, -winHeight / 2, winWidth, winHeight);
	*/
	glUseProgram(g_shader->program);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawStuff();
    
	SDL_GL_SwapWindow(display);
	checkGlErrors();
	
}

void makeObjects()
{
	//make board
	scene.addRayObject(new CheckerBoard(Point(0, 0, 0)));

	//make objects
	//showObjectsMenu();
}

static void initPlane()
{
    int ibLen, vbLen;
    getPlaneVbIbLen(vbLen, ibLen);

    // Temporary storage for cube geometry
    vector<GenericVertex> vtx(vbLen);
    vector<unsigned short> idx(ibLen);

    makePlane(1.1, vtx.begin(), idx.begin());
    g_plane =  new Geometry(&vtx[0], &idx[0], vbLen, ibLen);
}

static void initGLState()
{
    glClearColor(128./255., 200./255., 255./255., 0.);
    glClearDepth(0.);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glReadBuffer(GL_BACK);
    glEnable(GL_FRAMEBUFFER_SRGB);
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
	initGLState();

	makeShaders();
	initPlane();
	makeObjects();
	//makeTextures();
}

void SdlApp::makeShaders() {
	g_shader = new ShaderState(G_SHADER_FILES[0],
		                         G_SHADER_FILES[1]);
	/*
	for (int i = 0; i < G_NUM_SHADERS; ++i) {
		
		g_shaderStates[i].reset(new ShaderState(G_SHADER_FILES[i][0],
		                                        G_SHADER_FILES[i][1]));
	}
	cout << "made shaders." << endl;
	*/
}

void SdlApp::handleEvent(SDL_Event *event) {
	if (event->type ==  SDL_QUIT) {
		running = false;
	}
}

/* PURPOSE: Executes the SDL application. Loops until event to quit.
 RETURN: -1 if fail, 0 success.
 */
int SdlApp::run()
{

	SDL_Event e;
	while (running)
	{
		while (SDL_PollEvent(&e))
			handleEvent(&e);

		//clearCanvas();
		draw();
	}
	SDL_Quit();
	return 0;
}

int main(int argc, char **argv)
{
	return SdlApp().run();
}
