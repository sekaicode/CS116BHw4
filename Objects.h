/*---------------------------------------------------------------------------*/
/* PROTOTYPES */
class RayObject;

/*---------------------------------------------------------------------------*/
/*  VARIABLES */
//lighting
const GLdouble WHITE[3] =
{ 1.0, 1.0, 1.0 }; //RGB for white
const GLdouble BLACK[3] =
{ 0.0, 0.0, 0.0 }; //RGB for black
const GLdouble RED[3] =
{ 1.0, 0.0, 0.0 }; //RGB for RED
const GLdouble ATTENUATION_FACTOR = 100000;
// used in our lighting equations to model how light attenuates with distance

//camera
const GLdouble CAMERA_POSITION[3] =
{ 0, 100, 200 }; //initial position of camera
const GLdouble LOOK_AT_VECTOR[3] =
{ 0, 0, -160 }; // where camera is looking at
const GLdouble UP_VECTOR[3] =
{ 0, 1, 0 }; // what direction is up for the camera

//board
const GLdouble BOARD_POSITION[3] =
{ 0, 0, -160 }; // where in the scene the board is positioned
const GLdouble BOARD_EDGE_SIZE = 320.0; // how wide the board is
const GLdouble BOARD_HALF_SIZE = BOARD_EDGE_SIZE / 2; // useful to half this size ready for some calculations
const unsigned int NUM_SQUARES = 8; //number of squares wide our chess board is
const GLdouble SQUARE_EDGE_SIZE = BOARD_EDGE_SIZE / NUM_SQUARES; // pixels/per square

//raytracing
const unsigned int MAX_DEPTH = 5; // maximum depth our ray-tracing tree should go to
const GLdouble SMALL_NUMBER = .0001; // used rather than check with zero to avoid round-off problems 
const GLdouble SUPER_SAMPLE_NUMBER = 16; // how many random rays per pixel

//window
GLsizei winWidth = 500, winHeight = 500; // used for size of window
GLsizei initX = 0, initY = 0; // used for initial position of window

/*---------------------------------------------------------------------------*/
/* CLASS DEFINITIONS */
/*
 PURPOSE: encapsulate properties and operations
 for 3 dimension points/vectors used to describe our scene
 REMARK: Many of the operations for Points will be used very often
 during ray-tracing. Since they are short and we want them to be inlined
 we define them in the class defintion itself using a standard one-line
 format (not exactly like the coding guidelines)
 */
class Point
{
private:
   GLdouble _x;
   GLdouble _y;
   GLdouble _z;

public:
   Point()
   {
      _x = 0;
      _y = 0;
      _z = 0;
   }

   Point(GLdouble a, GLdouble b, GLdouble c)
   {
      _x = a;
      _y = b;
      _z = c;
   }

   Point(const GLdouble pt[])
   {
      _x = pt[0];
      _y = pt[1];
      _z = pt[2];
   }

   Point(const Point& p)
   {
      _x = p._x;
      _y = p._y;
      _z = p._z;
   }
   GLdouble x()
   {
      return _x;
   }
   GLdouble y()
   {
      return _y;
   }
   GLdouble z()
   {
      return _z;
   }

   void set(GLdouble a, GLdouble b, GLdouble c)
   {
      _x = a;
      _y = b;
      _z = c;
   }

   bool isZero()
   {
      return (_x == 0 && _y == 0 && _z == 0);
   }
   GLdouble length()
   {
      return sqrt(_x * _x + _y * _y + _z * _z);
   }
   void normalize()
   {
      GLdouble l = length();
      _x /= l;
      _y /= l;
      _z /= l;
   }

   friend Point operator*(GLdouble scalar, const Point &other); //scalar products
   friend Point operator*(const Point &other, GLdouble scalar);

   Point& operator*=(GLdouble scalar)
   {
      _x *= scalar;
      _y *= scalar;
      _z *= scalar;
      return *this;
   }

   Point& operator/=(GLdouble scalar)
   {
      _x /= scalar;
      _y /= scalar;
      _z /= scalar;
      return *this;
   }

   Point operator*(const Point& other) //cross product
   {
      return Point(_y * other._z - other._y * _z, _z * other._x - _x * other._z,
            _x * other._y - _y * other._x);
   }

   GLdouble operator&(const Point& other) //dot Product
   {
      return _x * other._x + _y * other._y + _z * other._z;
   }

   Point operator%(const Point& other) //Hadamard Product
   {
      return Point(_x * other._x, _y * other._y, _z * other._z);
   }
   Point operator+(const Point &other) const
   {
      return Point(_x + other._x, _y + other._y, _z + other._z);
   }

   Point operator-(const Point &other) const
   {
      return Point(_x - other._x, _y - other._y, _z - other._z);
   }

   Point& operator+=(const Point &other)
   {
      _x += other._x;
      _y += other._y;
      _z += other._z;
      return *this;
   }

   Point& operator-=(const Point &other)
   {
      _x -= other._x;
      _y -= other._y;
      _z -= other._z;
      return *this;
   }

   Point& operator=(const Point &other)
   {
      _x = other._x;
      _y = other._y;
      _z = other._z;
      return *this;
   }

   //checks to see if a point equals to another
   bool operator==(const Point &other) const
   {
   		#define EPSILON .00001
      if( abs(_x - other._x) <= EPSILON && abs(_y - other._y) <= EPSILON && abs(_z - other._z) <= EPSILON)
		  	return true;
	  	else
		  	return false;
   }
};

Point whiteColor(WHITE); // some abbreviations for various colors
Point blackColor(BLACK);
Point redColor(RED);
Point lightColor(WHITE); // color of the light

/*
 PURPOSE: encapsulate information about a
 light (basically its color and position) in our scene
 to be ray-traced
 REMARK:
 */
class Light
{
private:
   Point _color;
   Point _position;

public:

   Light(const Point& c, const Point& p)
   {
      _color = c;
      _position = p;
   }
   Point color()
   {
      return _color;
   }
   Point position()
   {
      return _position;
   }
};

/*
 PURPOSE: encapsulate information about lines in our scene
 REMARK:
 A ray that will be ray traced will be such a line
 We will adopt the convention that the _startPoint of such a line is
 where it is coming from and the the _endPoint is mainly used to
 specify the direction of the line
 */
class Line
{
private:
   Point _startPt;
   Point _endPt;

public:
   Line()
   {
      _startPt.set(0.0, 0.0, 0.0);
      _endPt.set(0.0, 0.0, 0.0);
   }
   Line(const Point& p1, const Point& p2)
   {
      _startPt = p1;
      _endPt = p2;
   }

   void set(const Point& p1, const Point& p2)
   {
      _startPt = p1;
      _endPt = p2;
   }

   Point startPoint() const
   {
      return _startPt;
   }
   Point endPoint() const
   {
      return _endPt;
   }

   Point direction() const
   {
      Point p = _endPt - _startPt;
      p.normalize();
      return p;
   }

   GLdouble length() const
   {
      Point p = _endPt - _startPt;
      return p.length();
   }
};

/*
 PURPOSE: storage of info about how a Shape
 will react to various kinds of light in our lighting model
 REMARK:
 We use the Phong lighting model as the basis for calculating
 lighting when we do ray-tracing
 */
class Material
{
private:
   Point _ambient;
   Point _diffuse;
   Point _specular;
   Point _transparency;
   GLdouble _refraction;
public:
   Material()
   {
      _ambient.set(0.0, 0.0, 0.0);
      _diffuse = _ambient;
      _specular = _ambient;
      _transparency = _ambient;
      _refraction = 1;
   }
   Material(const Point& a, const Point& d, const Point& s, const Point& t,
         GLdouble r)
   {
      _ambient = a;
      _diffuse = d;
      _specular = s;
      _transparency = t;
      _refraction = r;
   }
   Material(const Material& m)
   {
      _ambient = m._ambient;
      _diffuse = m._diffuse;
      _specular = m._specular;
      _transparency = m._transparency;
      _refraction = m._refraction;
   }
   Point ambient()
   {
      return _ambient;
   }
   Point diffuse()
   {
      return _diffuse;
   }
   Point specular()
   {
      return _specular;
   }
   Point transparency()
   {
      return _transparency;
   }
   GLdouble refraction()
   {
      return _refraction;
   }
};

Material sphereMaterial(blackColor, .1 * whiteColor, whiteColor, blackColor, 1);
Material tetrahedronMaterial(blackColor, blackColor, .1 * whiteColor,
      whiteColor, 2.0 / 3.0);
Material cubeMaterial(.1 * redColor, .4 * redColor, redColor, blackColor, 1);
Material whiteSquare(.1 * whiteColor, .5 * whiteColor, whiteColor, blackColor,
      1);
// some materials used by objects in  the scene
Material blackSquare(blackColor, .1 * whiteColor, blackColor, blackColor, 1);

/*
 PURPOSE: storage of information about how a ray intersects with a RayObject.
 REMARK:
 A flag is used to say if there was any intersection at all. If there is an intersection
 objects of this class store what was the material of the object that was intersected as well
 as its normal, the transmitted ray and the reflected ray.
 */
class Intersection
{
private:
   bool _intersects;
   Point _point;
   Point _normal;

   Material _material;
   Line _reflectedRay;
   Line _transmittedRay;

public:
   Intersection()
   {
   }
   Intersection(bool intersects, const Point& p, const Point& n,
         const Material& m, const Line& r, const Line& t)
   {
      _intersects = intersects;
      _point = p;
      _normal = n;
      _material = m;
      _reflectedRay = r;
      _transmittedRay = t;
   }
   bool intersects()
   {
      return _intersects;
   }

   Point point()
   {
      return _point;
   }
   Point normal()
   {
      return _normal;
   }

   Material material()
   {
      return _material;
   }

   Line reflectedRay()
   {
      return _reflectedRay;
   }
   Line transmittedRay()
   {
      return _transmittedRay;
   }

   void setIntersect(bool i)
   {
      _intersects = i;
   }
   void setMaterial(const Material& m)
   {
      _material = m;
   }

   void setValues(bool intersects, const Point& p, const Point& n,
         const Material& m, const Line& r, const Line& t)
   {
      _intersects = intersects;
      _point = p;
      _normal = n;
      _material = m;
      _reflectedRay = r;
      _transmittedRay = t;
   }

   void setValues(const Intersection& in)
   {
      _intersects = in._intersects;
      _point = in._point;
      _normal = in._normal;
      _material = in._material;
      _reflectedRay = in._reflectedRay;
      _transmittedRay = in._transmittedRay;
   }
};

/*
 PURPOSE: abstract class serving a base for
 all objects to be drawn in our ray-traced scene
 REMARK:
 */
class RayObject
{
protected:
   Point _position;
   Material _material;
public:
   RayObject(const Point& p, const Material& m)
   {
      _position = p;
      _material = m;
   }

   //returns position of rayobject
   Point position()
   {
      return _position;
   }

   virtual void doIIntersectWith(const Line& l, const Point& positionOffset,
         Intersection& inter) = 0;
   // by overriding intersection in different ways control how rays hit objects in our scene
};

/*
 PURPOSE: Triangle object are used as one of the basic building blocks for Shape's in our
 scen
 REMARK: Triangle's and Shape's are used according to a Composite design pattern to
 define objects in our scene
 */
class Triangle: public RayObject
{
private:
   Point _vertex0;
   Point _vertex1;
   Point _vertex2;

   Point _u;
   Point _v;
   Point _n;

   GLdouble _uv;
   GLdouble _uu;
   GLdouble _vv;
   GLdouble _denominator;

   bool _degenerate;

public:
   Triangle(const Point& p, const Material& m, const Point& p1, const Point& p2,
         const Point& p3) :
         RayObject(p, m)
   {
      _vertex0 = p1;
      _vertex1 = p2;
      _vertex2 = p3;
      //compute intersection with plane of triangle
      _u = _vertex1 - _vertex0;
      _v = _vertex2 - _vertex0;
      _n = _u * _v;

      //handle last degenerates case by saying we don't intersect
      if (_n.length() < SMALL_NUMBER)
         _degenerate = true;
      else
         _degenerate = false;

      _n.normalize();

      _uv = _u & _v;
      _uu = _u & _u;
      _vv = _v & _v;

      _denominator = _uv * _uv - _uu * _vv;

      if (abs(_denominator) < SMALL_NUMBER)
         _degenerate = true;
   }

   /*
    PURPOSE: fill in an Intersection object with information about how the supplied ray
    intersects with the current Triangle based on the given positionOffset Point vector.
    RECEIVES:
    ray -- ray to intersect with this Triangle
    positionOffset -- where in the overall scene this Triangle lives
    inter -- Intersection object to fill in with information about the if and where the ray
    intersects
    RETURNS:
    REMARKS:
    The idea for the following way to test for triangle intersections was
    gotten from
    http://softsurfer.com/Archive/algorithm_0105/algorithm_0105.htm

    Later noticed was in books already had like 3D Computer Graphics by Sam Buss.
    The coding of it is my own.

    Degenerate cases handle by saying there is no intersection
    */
   void doIIntersectWith(const Line& ray, const Point& positionOffset,
         Intersection& inter)
   {
      if (_degenerate)
      {
         inter.setIntersect(false);
         return;
      }

      //get coordinates of triangle given our position
      Point position = _position + positionOffset;
      Point v = position + _vertex0;

      Point p0 = ray.startPoint();
      Point p1 = ray.endPoint();
      Point diffP = p1 - p0;
      GLdouble ndiffP = _n & diffP;

      //handle another degenerate case by saying we don't intersect
      if (abs(ndiffP) < SMALL_NUMBER)
      {
         inter.setIntersect(false);
         return;
      }

      GLdouble m = (_n & (v - p0)) / (_n & diffP);

      if (m < SMALL_NUMBER) //if m is negative then we don't intersect
      {
         inter.setIntersect(false);
         return;
      }

      Point p = p0 + m * diffP; //intersection point with plane

      Point w = p - v;

      //Now check if in triangle
      GLdouble wu = w & _u;
      GLdouble wv = w & _v;

      GLdouble s = (_uv * wv - _vv * wu) / _denominator;
      GLdouble t = (_uv * wu - _uu * wv) / _denominator;

      if (s >= 0 && t >= 0 && s + t <= 1) // intersect
      {
         diffP.normalize(); // now u is as in the book
         Point u = diffP;

         Point r = u - (2 * (u & _n)) * _n;
         Line reflected(p, p + r);

         //Transmitted vector calculated using thin lens equations from book
         GLdouble refractionRatio = _material.refraction();

         Point t(0.0, 0.0, 0.0);

         GLdouble cosThetai = u & _n;
         GLdouble modulus = 1
               - refractionRatio * refractionRatio
                     * (1 - cosThetai * cosThetai);

         if (modulus > 0)
         {
            GLdouble cosThetar = sqrt(modulus);
            t = refractionRatio * u
                  - (cosThetar + refractionRatio * cosThetai) * _n;
         }

         Line transmitted(p, p + t);
         inter.setValues(true, p, _n, _material, reflected, transmitted);
      }
      else // don't intersect
      {
         inter.setIntersect(false);
      }
   }
};

/*
 PURPOSE: Shapes are either composite objects which represent a part of the scene
 to be ray-traced or our primary objects in which case they are used to model sphere's
 in our scene
 REMARK:  Triangles and Shapes are used according to a Composite design pattern to
 define objects in our scene
 */
class Shape: public RayObject
{
protected:
   GLdouble _radius;
   bool _amSphere;
   bool _canIntersectOnlyOneSubObject;

   vector<RayObject *> _subObjects;

public:
   Shape() :
         RayObject(Point(0, 0, 0), Material())
   {
      _radius = 0;
      _amSphere = false;
   }
   Shape(Point p, Material m, GLdouble radius, bool a, bool c = false) :
         RayObject(p, m)
   {
      _radius = radius;
      _amSphere = a;
      _canIntersectOnlyOneSubObject = c;
      _subObjects.clear();
   }

   /*
    PURPOSE: destructs this shape and gets rid of any sub-object on it
    RECEIVES: nothing
    RETURNS: nothing
    REMARKS:
    */
   ~Shape()
   {
      for (size_t i = 0; i < _subObjects.size(); i++)
      {
         delete _subObjects[i];
      }
   }

   void setRadius(GLdouble r)
   {
      _radius = r;
   }

   void addRayObject(RayObject *objects)
   {
      _subObjects.push_back(objects);
   }

   //return the vector
   vector<RayObject*>& subObject()
   {
	   return _subObjects;
   }


   /*
    PURPOSE: used to fill in an Intersection object with information about how the supplied ray
    intersects with the current Shape based on the given positionOffset Point vector.
    RECEIVES:
    ray -- ray to intersect with this Shape
    positionOffset -- where in the overall scene this Shape lives
    inter -- Intersection object to fill in with information about the if and where the ray
    intersects
    RETURNS:
    REMARKS: note this Shape could be a composite object so we recurse through its _subObjects vector
    */
   void doIIntersectWith(const Line& ray, const Point& positionOffset,
         Intersection& inter)
   {
      Point u = ray.direction();
      Point p0 = ray.startPoint();
      Point position = _position + positionOffset;
      Point deltaP = position - p0;
      /* check for sphere intersection if radius is > 0.
       if radius ==0 assume we are dealing with a object which has
       subObjects which do the testing
       */
      if (_radius > 0 || _amSphere)
      {
         GLdouble uDeltaP = u & deltaP;
         GLdouble discriminant = uDeltaP * uDeltaP - (deltaP & deltaP)
               + _radius * _radius;

         GLdouble s = uDeltaP - sqrt(discriminant); //other solution is on far side of sphere

         if (discriminant < 0 || abs(s) < SMALL_NUMBER)
         {
            inter.setIntersect(false);
            return;
         }

         //calculate point of intersection
         Point p = p0 + s * u;
         Point directionP0 = p - position;

         if (_amSphere)
         {
            if (s < SMALL_NUMBER) // if not in front of ray then don't intersect
            {
               inter.setIntersect(false);
               return;
            }

            //reflected vector calculated using equations from book
            Point n(directionP0);
            n.normalize();

            Point r = u - (2 * (u & n)) * n;
            Line reflected(p, p + r);

            //Transmitted vector calculated using thin lens equations from book
            Point t(0.0, 0.0, 0.0);
            GLdouble refractionRatio = _material.refraction();
            GLdouble cosThetai = u & n;
            GLdouble modulus = 1
                  - refractionRatio * refractionRatio
                        * (1 - cosThetai * cosThetai);

            if (modulus > 0)
            {
               GLdouble cosThetar = sqrt(modulus);
               t = refractionRatio * u
                     - (cosThetar + refractionRatio * cosThetai) * n;
            }
            Line transmitted(p, p + t);
            inter.setValues(true, p, n, _material, reflected, transmitted);
         }
      }

      if (!_amSphere)
      {
         inter.setIntersect(false);
         Intersection interTmp;

         GLdouble minDistance = -1.0;
         GLdouble distanceTmp;
         size_t size = _subObjects.size();
         for (size_t i = 0; i < size; i++)
         {
            _subObjects[i]->doIIntersectWith(ray, position, interTmp);

            if (interTmp.intersects())
            {
               Point directionCur = interTmp.point() - p0;
               distanceTmp = directionCur.length();
               if (distanceTmp < minDistance || minDistance < 0.0)
               {
                  minDistance = distanceTmp;
                  inter.setValues(interTmp);
                  if (_canIntersectOnlyOneSubObject)
                     return;
               }
            }
         }
      }
   }
};

Shape scene(BOARD_POSITION, Material(), sqrt((double) 3) * BOARD_HALF_SIZE,
      false); // global shape for whole scene

/*
 PURPOSE: encapsulate information about quadrilaterals
 used in our scene. Quadralaterals are used for
 both quick tests for ray intersection as well as
 subobjects of more complicated objects in our scene
 REMARK:
 */
class Quad: public Shape
{
public:
   /*
    PURPOSE: construct a Quad object (for quadralateral) at the given offset position made of
    the given material and with the supplied points
    RECEIVES:
    p -- position offset into our scene
    m -Material Quad is made out of
    p1, p2, p3, p4 - four local coordinate points (relative to p) that make up the Quad.
    RETURNS:
    REMARKS: can only intersect one of two sub-triangles unless ray is same plane as Quad
    */
   Quad(Point p, Material m, Point p1, Point p2, Point p3, Point p4) :
         Shape(p, m, 0, false, true)
   {
      Point zero(0.0, 0.0, 0.0);

      addRayObject(new Triangle(zero, m, p1, p2, p3));
      addRayObject(new Triangle(zero, m, p1, p3, p4));
   }
};

/*
 PURPOSE: encapsulates information about
 tetrahedrons to be drawn in our scene (in this case just one)
 REMARK:
 */
class Tetrahedron: public Shape
{
public:
   /*
    PURPOSE: construct a Tetrahedron at the given offset position and edgeSize in our Scene
    RECEIVES:
    p -- position offset into our scene
    edgeSize -- size of an edge of our cube
    RETURNS: a Tetrahedron object
    REMARKS:  note tetrahedronMaterial is a global in this file.
    Since the HW description didn't say the tetrahedron was regular, we took the tetrahedron to be the
    one obtained by slicing the cube from a top corner through the diagonal of the bottom face
    */
   Tetrahedron(Point p, GLdouble edgeSize) :
         Shape(p, tetrahedronMaterial, sqrt((double) 3) * edgeSize / 2, false)
   {
      Point zero(0.0, 0.0, 0.0);
      GLdouble halfEdge = edgeSize / 2;

      //bottom
      addRayObject(
            new Triangle(zero, tetrahedronMaterial,
                  Point(-halfEdge, -halfEdge, -halfEdge),
                  Point(halfEdge, -halfEdge, -halfEdge),
                  Point(-halfEdge, -halfEdge, halfEdge)));
      //back
      addRayObject(
            new Triangle(zero, tetrahedronMaterial,
                  Point(-halfEdge, -halfEdge, -halfEdge),
                  Point(-halfEdge, -halfEdge, halfEdge),
                  Point(-halfEdge, halfEdge, -halfEdge)));

      //left
      addRayObject(
            new Triangle(zero, tetrahedronMaterial,
                  Point(-halfEdge, -halfEdge, -halfEdge),
                  Point(-halfEdge, halfEdge, -halfEdge),
                  Point(-halfEdge, -halfEdge, halfEdge)));
      //front
      addRayObject(
            new Triangle(zero, tetrahedronMaterial,
                  Point(-halfEdge, -halfEdge, halfEdge),
                  Point(halfEdge, -halfEdge, -halfEdge),
                  Point(-halfEdge, halfEdge, -halfEdge)));
   }
};

/*
 PURPOSE: encapsulates information about
 spheres to be drawn in our scene (in this case just one)
 REMARK:
 */
class Sphere: public Shape
{
public:
   /*
    PURPOSE: construct a Sphere object at the given offset position and radius in our Scene
    RECEIVES:
    p -- position offset into our scene
    r -- radius of Sphere
    RETURNS: a Sphere object
    REMARKS:  sphereMaterial is a global in this file.
    The constructor mainly just calls the base constructor with the appropriate material
    and with the flag for Shape telling the Shape that it is a non-composite
    sphere (the last paramter true to the Shape constructor)
    */
   Sphere(Point p, GLdouble r) :
         Shape(p, sphereMaterial, r, true)
   {
   }
};

/*
 PURPOSE: encapsulate information about
 cubes to be drawn in our scene (in this case just one)
 REMARK:
 */
class Cube: public Shape
{
public:
   /*
    PURPOSE: constructs a Cube at the given offset position and edgeSize in our Scene
    RECEIVES:
    p -- position offset into our scene
    edgeSize -- size of an edge of our cube
    RETURNS: a cube object
    REMARKS:  note cubeMaterial is a global in this file
    */
   Cube(Point p, GLdouble edgeSize) :
         Shape(p, cubeMaterial, sqrt((double) 3) * edgeSize / 2, false)
   {
      GLdouble halfEdge = edgeSize / 2;

      Point zero(0.0, 0.0, 0.0);
      //top
      addRayObject(
            new Quad(zero, cubeMaterial, Point(-halfEdge, halfEdge, -halfEdge),
                  Point(halfEdge, halfEdge, -halfEdge),
                  Point(halfEdge, halfEdge, halfEdge),
                  Point(-halfEdge, halfEdge, halfEdge)));

      //bottom
      addRayObject(
            new Quad(zero, cubeMaterial, Point(-halfEdge, -halfEdge, -halfEdge),
                  Point(halfEdge, -halfEdge, -halfEdge),
                  Point(halfEdge, -halfEdge, halfEdge),
                  Point(-halfEdge, -halfEdge, halfEdge)));

      //left
      addRayObject(
            new Quad(zero, cubeMaterial, Point(-halfEdge, -halfEdge, -halfEdge),
                  Point(-halfEdge, halfEdge, -halfEdge),
                  Point(-halfEdge, halfEdge, halfEdge),
                  Point(-halfEdge, -halfEdge, halfEdge)));
      //right
      addRayObject(
            new Quad(zero, cubeMaterial, Point(halfEdge, -halfEdge, -halfEdge),
                  Point(halfEdge, halfEdge, -halfEdge),
                  Point(halfEdge, halfEdge, halfEdge),
                  Point(halfEdge, -halfEdge, halfEdge)));
      //back
      addRayObject(
            new Quad(zero, cubeMaterial, Point(-halfEdge, -halfEdge, -halfEdge),
                  Point(halfEdge, -halfEdge, -halfEdge),
                  Point(halfEdge, halfEdge, -halfEdge),
                  Point(-halfEdge, halfEdge, -halfEdge)));

      //front
      addRayObject(
            new Quad(zero, cubeMaterial, Point(-halfEdge, -halfEdge, halfEdge),
                  Point(halfEdge, -halfEdge, halfEdge),
                  Point(halfEdge, halfEdge, halfEdge),
                  Point(-halfEdge, halfEdge, halfEdge)));
   }
};

/*
 PURPOSE: encapsulates information about
 checkerboards to be drawn in our scene (in this case just one)
 REMARK:
 */
class CheckerBoard: public Shape
{
private:
   Quad _boundingSquare; /* this Quad is used for a quick test to see if a
    ray intersects our chessboard */
public:

   /*
    PURPOSE: constructs a Checkerboard object at the supplied offset position
    RECEIVES:
    p - offset position to put chess board at
    RETURNS:
    REMARKS: This constructor makes use of some constant in this file concerning the
    chessboard.
    */
   CheckerBoard(Point p) :
         Shape(), _boundingSquare(p, Material(),
               Point(-BOARD_HALF_SIZE, 0, -BOARD_HALF_SIZE),
               Point(BOARD_HALF_SIZE, 0, -BOARD_HALF_SIZE),
               Point(BOARD_HALF_SIZE, 0, BOARD_HALF_SIZE),
               Point(-BOARD_HALF_SIZE, 0, BOARD_HALF_SIZE)) /*initialize
    bounding square to be used for quick tests if ray intersects
    chess board */
   {
   }
   /*
    PURPOSE: used to calculate how a ray intersects with a Chessboard object
    RECEIVES:
    ray to bounce off of chessboard
    positionoOffset - offset vector for location of chessboard
    inter -- an Intersection object to be filled in with details on if and where ray intersects
    with Chessboard
    RETURNS: nothing
    REMARKS:
    */
   void doIIntersectWith(const Line& ray, const Point& positionOffset,
         Intersection& intersection)
   {
      _boundingSquare.doIIntersectWith(ray, positionOffset, intersection);

      if (intersection.intersects())
      {
         Point p = intersection.point() - positionOffset
               + Point(BOARD_HALF_SIZE, 0, BOARD_HALF_SIZE);
         int squareSum = int(p.x() / SQUARE_EDGE_SIZE)
               + int(p.z() / SQUARE_EDGE_SIZE);

         if ((squareSum & 1) == 0)
         {
            intersection.setMaterial(whiteSquare);
         }
         else
         {
            intersection.setMaterial(blackSquare);
         }
      }
   }
};

