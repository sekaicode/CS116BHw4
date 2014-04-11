
/*---------------------------------------------------------------------------*/
/* INCLUDES */
#include "SdlApp.h"

/*---------------------------------------------------------------------------*/
/*   CONSTANTS */
//lighting
const GLdouble WHITE[3] = {1.0, 1.0, 1.0}; //RGB for white
const GLdouble BLACK[3] = {0.0, 0.0, 0.0}; //RGB for black
const GLdouble RED[3] = {1.0, 0.0, 0.0}; //RGB for RED
const GLdouble ATTENUATION_FACTOR = 100000; 
   // used in our lighting equations to model how light attenuates with distance

//camera
const GLdouble CAMERA_POSITION[3] = {0, 100, 200}; //initial position of camera
const GLdouble LOOK_AT_VECTOR[3] = {0, 0, -160}; // where camera is looking at
const GLdouble UP_VECTOR[3] = {0, 1, 0}; // what direction is up for the camera

//board
const GLdouble BOARD_POSITION[3] = {0, 0, -160}; // where in the scene the board is positioned   
const GLdouble BOARD_EDGE_SIZE = 320.0; // how wide the board is
const GLdouble BOARD_HALF_SIZE = BOARD_EDGE_SIZE/2; // useful to half this size ready for some calculations
const unsigned int NUM_SQUARES = 8; //number of squares wide our chess board is
const GLdouble SQUARE_EDGE_SIZE = BOARD_EDGE_SIZE/NUM_SQUARES; // pixels/per square

//raytracing
const unsigned int MAX_DEPTH = 5; // maximum depth our ray-tracing tree should go to
const GLdouble SMALL_NUMBER = .0001; // used rather than check with zero to avoid round-off problems 
const GLdouble SUPER_SAMPLE_NUMBER = 16; // how many random rays per pixel

/*---------------------------------------------------------------------------*/
/* PROTOTYPES */
class RayObject;

/*---------------------------------------------------------------------------*/
/* CLASS DEFINITIONS */
/*
PURPOSE: Used to encapsulate the properties and operations
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
         {_x = 0; _y = 0; _z =0;}
   
      Point(GLdouble a, GLdouble b, GLdouble c)
         {_x = a; _y = b; _z =c;}
     
      Point(const GLdouble pt[])
         {_x = pt[0]; _y = pt[1]; _z = pt[2];}
                
      Point(const Point& p)
         {_x = p._x; _y = p._y; _z = p._z;}
      GLdouble x(){return _x;}
      GLdouble y(){return _y;}
      GLdouble z(){return _z;}
                  
      void set(GLdouble a, GLdouble b, GLdouble c)
         {_x = a; _y = b; _z =c;}
      
      bool isZero(){ return (_x == 0 && _y ==0 && _z==0);}
      GLdouble length(){return sqrt(_x*_x + _y*_y + _z*_z); }
      void normalize(){ GLdouble l = length(); _x /=l; _y/= l; _z /= l;}
      
      friend Point operator*(GLdouble scalar, const Point &other); //scalar products
      friend Point operator*(const Point &other, GLdouble scalar);         

      Point& operator*=(GLdouble scalar)
         {_x *= scalar; _y *= scalar; _z *= scalar; return *this;}         

      Point& operator/=(GLdouble scalar)
         {_x /= scalar; _y /= scalar; _z /= scalar; return *this;}         
      
      Point operator*(const Point& other) //cross product
         {return Point(_y*other._z - other._y*_z, _z*other._x - _x*other._z, _x*other._y - _y*other._x); }

      GLdouble operator&(const Point& other)//dot Product
         {return _x * other._x + _y * other._y + _z * other._z; }

      Point operator%(const Point& other)//Hadamard Product
         {return Point(_x * other._x, _y * other._y, _z * other._z); }
      Point operator+(const Point &other) const
         {return Point(_x + other._x, _y + other._y, _z + other._z); }
            
      Point operator-(const Point &other) const
         {return Point(_x - other._x, _y - other._y, _z - other._z); }
          
      Point& operator+=(const Point &other)
         {_x += other._x; _y += other._y; _z += other._z; return *this;}
            
      Point& operator-=(const Point &other)
         {_x -= other._x; _y -= other._y; _z -= other._z; return *this;}

      Point& operator=(const Point &other)
         {_x = other._x; _y = other._y; _z = other._z; return *this;}
};

/*
PURPOSE: Use to encapsulate information about a 
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
     
      Light(const Point& c, const Point& p){ _color = c; _position = p;}
      Point color(){return _color;}
      Point position(){return _position;}  
};

/*
PURPOSE: Used to encapsulate information about lines in our scene
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
      Line(){_startPt.set(0.0, 0.0, 0.0); _endPt.set(0.0, 0.0, 0.0);}
      Line(const Point& p1, const Point& p2) {_startPt = p1; _endPt = p2;}
      
      void set(const Point& p1, const Point& p2){_startPt = p1; _endPt = p2;}

      Point startPoint() const {return _startPt;}
      Point endPoint() const {return _endPt;}

      Point direction() const
      {
         Point p =  _endPt - _startPt;
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
PURPOSE: Used to hold information about how a Shape
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
         { _ambient.set(0.0, 0.0, 0.0); _diffuse =_ambient; _specular = _ambient; _transparency = _ambient;
_refraction = 1;}
      Material(const Point& a, const Point& d, const Point& s, const Point& t, GLdouble r)
         {_ambient = a; _diffuse = d; _specular = s; _transparency = t; _refraction = r;}
      Material(const Material& m)
         {_ambient = m._ambient; _diffuse = m._diffuse; _specular = m._specular; 
          _transparency = m._transparency; _refraction = m._refraction;}
      Point ambient(){ return _ambient;}
      Point diffuse(){ return _diffuse;}
      Point specular(){ return _specular;}
      Point transparency(){ return _transparency;}
      GLdouble refraction(){ return _refraction;}
};

/*
PURPOSE: used to store information about how a ray intersects with a RayObject.
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
      Intersection(){}
      Intersection(bool intersects, const Point& p, const Point& n, const Material& m, const Line& 
r, const Line& t)
         {_intersects = intersects; _point = p; _normal = n; _material = m; _reflectedRay = r; _transmittedRay =
t;}
      bool intersects(){return _intersects;}

      Point point(){return _point;}
      Point normal(){return _normal;}
      
      Material material(){return _material;}
      
      Line reflectedRay(){return _reflectedRay;}
      Line transmittedRay(){return _transmittedRay;}
      
      void setIntersect(bool i){_intersects = i;}
      void setMaterial(const Material& m){_material = m;}
      
      void setValues(bool intersects, const Point& p, const Point& n, const Material& m, const
Line& r, const Line& t)
         {_intersects = intersects; _point = p; _normal = n; _material = m; _reflectedRay = r; _transmittedRay =
t;}

      void setValues(const Intersection& in)
         {_intersects = in._intersects; _point = in._point; _normal = in._normal; 
           _material = in._material; _reflectedRay = in._reflectedRay; _transmittedRay = in._transmittedRay;}
};

/*
PURPOSE: abstract class which serves as a base for
   all objects to be drawn in our ray-traced scene
REMARK:
*/
class RayObject
{
   protected:
      Point _position;
      Material _material;
   public:
      RayObject(const Point& p, const Material& m )
         {_position = p; _material = m;}
      virtual void intersection(const Line& l, const Point& positionOffset, Intersection& inter) = 0; 
         // by overriding intersection in different ways control how rays hit objects in our scene
};

/*
PURPOSE: Triangle object are used as one of the basic building blocks for Shape's in our
   scen
REMARK: Triangle's and Shape's are used according to a Composite design pattern to 
   define objects in our scene         
*/
class Triangle : public RayObject
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
      Triangle(const Point& p, const Material& m, const Point& p1, const Point& p2, const
Point& p3) : RayObject(p,m)
      {
         _vertex0 = p1;
         _vertex1 = p2; 
         _vertex2 = p3;
         //compute intersection with plane of triangle
         _u = _vertex1 - _vertex0;
         _v = _vertex2 - _vertex0;
         _n = _u*_v;

         //handle last degenerates case by saying we don't intersect
         if(_n.length() < SMALL_NUMBER) _degenerate = true;
         else _degenerate = false;

         _n.normalize(); 
                  
         _uv = _u & _v;
         _uu = _u & _u;
         _vv = _v & _v;
   
         _denominator = _uv*_uv - _uu*_vv;
   
         if( abs(_denominator) < SMALL_NUMBER) _degenerate = true;
      }
      
      void intersection(const Line& l, const Point& positionOffset, Intersection& inter);
};

/*
PURPOSE: Shape's are either composite objects which represent a part of the scene
   to be ray-traced or our primary objects in which case they are used to model sphere's
   in our scene
REMARK:  Triangle's and Shape's are used according to a Composite design pattern to 
   define objects in our scene                 
*/
class Shape : public RayObject
{
   protected:
      GLdouble _radius;
      bool _amSphere;
      bool _canIntersectOnlyOneSubObject;
      
      vector<RayObject *> _subObjects;

   public:
      Shape() : RayObject(Point(0,0,0), Material())
         {_radius = 0; _amSphere = false;}
      Shape(Point p, Material m, GLdouble radius, bool a, bool c = false) : RayObject(p,m)
         {_radius = radius; _amSphere = a; _canIntersectOnlyOneSubObject = c; _subObjects.clear();}
 
      ~Shape();
      
      void setRadius(GLdouble r){_radius = r;}
      
      void addRayObject(RayObject *objects)
         {_subObjects.push_back(objects);}
      
      void intersection(const Line& l, const Point& positionOffset, Intersection& inter);
};

/*
PURPOSE: encapsulate information about quadrilaterals
   used in our scene. Quadralaterals are used for
   both quick tests for ray intersection as well as
   subobjects of more complicated objects in our scene
REMARK:         
*/
class Quad : public Shape
{
   public:
      Quad(Point p, Material m, Point p1, Point p2, Point p3, Point p4);
};

/*
PURPOSE: encapsulates information about
   tetrahedrons to be drawn in our scene (in this case just one)
REMARK:         
*/
class Tetrahedron :  public Shape

{
   public:
      Tetrahedron(Point p, GLdouble edgeSize);
};

/*
PURPOSE: encapsulates information about
   spheres to be drawn in our scene (in this case just one)
REMARK:         
*/
class Sphere :  public Shape

{
   public:
      Sphere(Point p, GLdouble radius);
};

/*
PURPOSE: encapsulates information about
   cubes to be drawn in our scene (in this case just one)
REMARK:         
*/
class Cube :  public Shape
{
   public:
      Cube(Point p, GLdouble edgeSize);
};

/*
PURPOSE: encapsulates information about
   checkerboards to be drawn in our scene (in this case just one)
REMARK:         
*/
class CheckerBoard :  public Shape
{
   private:
      Quad _boundingSquare; /* this Quad is used for a quick test to see if a
         ray intersects our chessboard */
   public:
      CheckerBoard(Point p);
      void intersection(const Line& l, const Point& positionOffset, Intersection& inter);
};

/*---------------------------------------------------------------------------*/
/* GLOBALS */
GLsizei winWidth = 500, winHeight = 500; // used for size of window
GLsizei initX = 50, initY = 50; // used for initial position of window
Point lightPosition(0.0, 0.0, 0.0); /* although the ray tracer actually supports
   giving it a vector of lights, this program only makes use of one
   light which is placed at lightPosition The value is later changed from this
   default value to a value on the chess board.*/
Point lightColor(WHITE); // color of the light

Point whiteColor(WHITE); // some abbreviations for various colors
Point blackColor(BLACK);
Point redColor(RED);
Material whiteSquare(.1*whiteColor, .5*whiteColor, whiteColor, blackColor, 1); 
   // some materials used by objects in  the scene
Material blackSquare(blackColor, .1*whiteColor, blackColor, blackColor, 1);
Material sphereMaterial(blackColor, .1*whiteColor, whiteColor, blackColor, 1);
Material tetrahedronMaterial(blackColor, blackColor, .1*whiteColor, whiteColor, 2.0/3.0);
Material cubeMaterial(.1*redColor, .4*redColor, redColor, blackColor, 1);
Shape scene(BOARD_POSITION, Material(), sqrt((double)3)*BOARD_HALF_SIZE, false); // global shape for whole scene

/*---------------------------------------------------------------------------*/
/*   IMPLEMENTATIONS */
//Triangle Class Implementations
/*
PURPOSE: used to fill in an Intersection object with information about how the supplied ray
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
void Triangle::intersection(const Line& ray, const Point& positionOffset, Intersection& inter)
{ 
   if(_degenerate)
   {
      inter.setIntersect(false);
      return;
   } 
   
   //get coordinates of triangle given our position
   Point position = _position + positionOffset;
   Point v0 = position + _vertex0;
   Point v1 = position + _vertex1;
   Point v2 = position + _vertex2;
   
   Point p0 = ray.startPoint();
   Point p1 = ray.endPoint();
   Point diffP = p1 - p0;
   GLdouble ndiffP = _n&diffP;
   
   //handle another degenerate case by saying we don't intersect
   if( abs(ndiffP) < SMALL_NUMBER)
   {
      inter.setIntersect(false);
      return;
   }   
   
   GLdouble m = (_n & (v0 - p0))/ (_n & diffP);

   if( m < SMALL_NUMBER) //if m is negative then we don't intersect
   {
      inter.setIntersect(false);
      return;
   }
      
   Point p = p0 + m*diffP; //intersection point with plane
   
   Point w = p - v0;
   
   //Now check if in triangle      
   GLdouble wu = w & _u;
   GLdouble wv = w & _v;
   
   GLdouble s = (_uv*wv - _vv*wu)/_denominator;
   GLdouble t = (_uv*wu - _uu*wv)/_denominator;
   
   if( s >= 0 && t >=0 && s + t <= 1) // intersect
   {
      diffP.normalize(); // now u is as in the book
      Point u=diffP;
      
      Point r =  u - (2*(u & _n))*_n; 
      Line reflected(p, p + r);
      
      //Transmitted vector calculated using thin lens equations from book
      GLdouble refractionRatio = _material.refraction();

      Point t(0.0, 0.0, 0.0);
      
      GLdouble cosThetai = u & _n;
      GLdouble modulus = 1 - refractionRatio*refractionRatio*( 1- cosThetai*cosThetai);
      
      if( modulus > 0)
      {
         GLdouble cosThetar = sqrt(modulus);
         t = refractionRatio*u - ( cosThetar + refractionRatio*cosThetai)*_n;
      }      
      
      Line transmitted(p, p + t);
      inter.setValues(true, p, _n, _material, reflected, transmitted);
   }
   else // don't intersect
   {
      inter.setIntersect(false);
   }
}

//Shape Class Implementations
/*
PURPOSE: destructs this shape and gets rid of any sub-object on it
RECEIVES: nothing
RETURNS: nothing
REMARKS:    
*/
Shape::~Shape()
{
      for(size_t i = 0; i < _subObjects.size() ; i++)
      {
         delete _subObjects[i];
      }
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
void Shape::intersection(const Line& ray, const Point& positionOffset, Intersection& inter)
{
   Point u = ray.direction();
   Point p0 = ray.startPoint();
   Point position = _position + positionOffset;
   Point deltaP = position - p0;
   /* check for sphere intersection if radius is > 0.
      if radius ==0 assume we are dealing with a object which has
      subObjects which do the testing
   */
   if(_radius > 0 || _amSphere)
   {
      GLdouble uDeltaP = u & deltaP;
      GLdouble discriminant = uDeltaP*uDeltaP - (deltaP & deltaP) + _radius*_radius;

      GLdouble s = uDeltaP - sqrt(discriminant); //other solution is on far side of sphere

      if( discriminant < 0 || abs(s) < SMALL_NUMBER )
      {
         inter.setIntersect(false);
         return;
      }

      //calculate point of intersection
      Point p = p0 + s*u;
      Point directionP0 = p - position ;
   
      if(_amSphere)
      {
         if(s < SMALL_NUMBER) // if not in front of ray then don't intersect
         {
            inter.setIntersect(false);
            return;
         }
         
         //reflected vector calculated using equations from book
         Point n(directionP0);
         n.normalize();
         
         Point r =  u - (2*(u & n))*n; 
         Line reflected(p, p + r);
         
         //Transmitted vector calculated using thin lens equations from book
         Point t(0.0, 0.0, 0.0);
         GLdouble refractionRatio = _material.refraction();
         GLdouble cosThetai = u & n;
         GLdouble modulus = 1 - refractionRatio*refractionRatio*( 1- cosThetai*cosThetai);
         
         if( modulus > 0)
         {
            GLdouble cosThetar = sqrt(modulus);
            t = refractionRatio*u - ( cosThetar + refractionRatio*cosThetai)*n;
         }
         Line transmitted(p, p + t);
         inter.setValues(true, p, n, _material, reflected, transmitted);
      }
   }
   
   if(!_amSphere)
   {
      inter.setIntersect(false);
      Intersection interTmp;
   
      GLdouble minDistance = -1.0;
      GLdouble distanceTmp;
      size_t size = _subObjects.size();
      for(size_t i = 0; i < size; i++)
      {
         _subObjects[i]->intersection(ray, position, interTmp);
                  
         if(interTmp.intersects())
         {
            Point directionCur = interTmp.point() - p0;
            distanceTmp = directionCur.length();
            if(distanceTmp < minDistance|| minDistance < 0.0)
            {
               minDistance = distanceTmp;
               inter.setValues(interTmp);
               if(_canIntersectOnlyOneSubObject) return;
            }
         }
      }
   }
}

//Quad Class Implementations
/*
PURPOSE: constructs a Quad object (for quadralateral) at the given offset position made of
   the given material and with the supplied points
RECEIVES:
   p -- position offset into our scene
   m -Material Quad is made out of
   p1, p2, p3, p4 - four local coordinate points (relative to p) that make up the Quad.
RETURNS: 
REMARKS: can only intersect one of two sub-triangles unless ray is same plane as Quad 
*/
Quad::Quad(Point p, Material m, Point p1, Point p2, Point p3, Point p4) : Shape(p, m, 0, false, true)
{
   Point zero(0.0, 0.0, 0.0);
   
   addRayObject(new Triangle(zero, m, p1, p2, p3));
   addRayObject(new Triangle(zero, m, p1, p3, p4));
}

//Sphere Class Implementations
/*
PURPOSE: constructs a Sphere object at the given offset position and radius in our Scene
RECEIVES:
 p -- position offset into our scene
 r -- radius of Sphere
RETURNS: a Sphere object
REMARKS:  sphereMaterial is a global in this file.
      The constructor mainly just calls the base constructor with the appropriate material
      and with the flag for Shape telling the Shape that it is a non-composite
      sphere (the last paramter true to the Shape constructor)
*/
Sphere::Sphere(Point p, GLdouble r) : Shape(p, sphereMaterial, r, true)
{
}

//Tetrahedron Class Implementations
/*
PURPOSE: constructs a Tetrahedron at the given offset position and edgeSize in our Scene
RECEIVES:
 p -- position offset into our scene
 edgeSize -- size of an edge of our cube
RETURNS: a Tetrahedron object
REMARKS:  note tetrahedronMaterial is a global in this file.
   Since the HW description didn't say the tetrahedron was regular, we took the tetrahedron to be the
   one obtained by slicing the cube from a top corner through the diagonal of the bottom face
*/
Tetrahedron::Tetrahedron(Point p, GLdouble edgeSize) : Shape(p, tetrahedronMaterial, sqrt((double)3)*edgeSize/2,
false)
{
   Point zero(0.0, 0.0, 0.0);
   GLdouble halfEdge = edgeSize/2;

   //bottom
   addRayObject(new Triangle(zero, tetrahedronMaterial, Point(-halfEdge, -halfEdge, -halfEdge), 
                     Point(halfEdge, -halfEdge, -halfEdge), 
                     Point(-halfEdge, -halfEdge, halfEdge)));
    //back
    addRayObject(new Triangle(zero, tetrahedronMaterial, Point(-halfEdge, -halfEdge, -halfEdge), 
                     Point(-halfEdge, -halfEdge, halfEdge), 
                     Point(-halfEdge, halfEdge, -halfEdge)));

   //left
   addRayObject(new Triangle(zero, tetrahedronMaterial, Point(-halfEdge, -halfEdge, -halfEdge), 
                     Point(-halfEdge, halfEdge, -halfEdge), 
                     Point(-halfEdge, -halfEdge, halfEdge)));
   //front
   addRayObject(new Triangle(zero, tetrahedronMaterial,
                     Point(-halfEdge, -halfEdge, halfEdge),
                     Point(halfEdge, -halfEdge, -halfEdge),
                     Point(-halfEdge, halfEdge, -halfEdge)));
}

//Cube Class Implementations
/*
PURPOSE: constructs a Cube at the given offset position and edgeSize in our Scene
RECEIVES:
 p -- position offset into our scene
 edgeSize -- size of an edge of our cube
RETURNS: a cube object
REMARKS:  note cubeMaterial is a global in this file
*/
Cube::Cube(Point p, GLdouble edgeSize) : Shape(p, cubeMaterial, sqrt((double)3)*edgeSize/2, false)
{
   GLdouble halfEdge = edgeSize/2;

   Point zero(0.0, 0.0, 0.0);
   //top
   addRayObject(new Quad(zero, cubeMaterial, Point(-halfEdge, halfEdge, -halfEdge), 
                     Point(halfEdge, halfEdge, -halfEdge), 
                     Point(halfEdge, halfEdge, halfEdge), 
                     Point(-halfEdge, halfEdge, halfEdge)));

   //bottom
   addRayObject(new Quad(zero, cubeMaterial, Point(-halfEdge, -halfEdge, -halfEdge), 
                     Point(halfEdge, -halfEdge, -halfEdge), 
                     Point(halfEdge, -halfEdge, halfEdge), 
                     Point(-halfEdge, -halfEdge, halfEdge)));

   //left
   addRayObject(new Quad(zero, cubeMaterial, Point(-halfEdge, -halfEdge, -halfEdge), 
                     Point(-halfEdge, halfEdge, -halfEdge), 
                     Point(-halfEdge, halfEdge, halfEdge), 
                     Point(-halfEdge, -halfEdge, halfEdge)));
   //right
   addRayObject(new Quad(zero, cubeMaterial, Point(halfEdge, -halfEdge, -halfEdge), 
                     Point(halfEdge, halfEdge, -halfEdge), 
                     Point(halfEdge, halfEdge, halfEdge), 
                     Point(halfEdge, -halfEdge, halfEdge)));
    //back
    addRayObject(new Quad(zero, cubeMaterial, Point(-halfEdge, -halfEdge, -halfEdge), 
                     Point(halfEdge, -halfEdge, -halfEdge), 
                     Point(halfEdge, halfEdge, -halfEdge), 
                     Point(-halfEdge, halfEdge, -halfEdge)));
                     
   //front
    addRayObject(new Quad(zero, cubeMaterial, Point(-halfEdge, -halfEdge, halfEdge), 
                     Point(halfEdge, -halfEdge, halfEdge), 
                     Point(halfEdge, halfEdge, halfEdge), 
                     Point(-halfEdge, halfEdge, halfEdge)));
}

//CheckerBoard Class Implementations
/*
PURPOSE: constructs a Checkerboard object at the supplied offset position
RECEIVES:
   p - offset position to put chess board at
RETURNS: 
REMARKS: This constructor makes use of some constant in this file concerning the
   chessboard.
*/
CheckerBoard::CheckerBoard(Point p) : Shape(),
   _boundingSquare(p, Material(),  Point(- BOARD_HALF_SIZE, 0, - BOARD_HALF_SIZE), 
                     Point( BOARD_HALF_SIZE, 0, - BOARD_HALF_SIZE), 
                     Point( BOARD_HALF_SIZE, 0, BOARD_HALF_SIZE), 
                     Point(- BOARD_HALF_SIZE, 0, BOARD_HALF_SIZE)) /*initialize
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
void CheckerBoard::intersection(const Line& ray, const Point& positionOffset, Intersection& inter) 
{
   _boundingSquare.intersection(ray, positionOffset, inter);
   
   if(inter.intersects())
   {
      Point p = inter.point() - positionOffset + Point(BOARD_HALF_SIZE, 0, BOARD_HALF_SIZE);
      int squareSum = int(p.x()/SQUARE_EDGE_SIZE) + int(p.z()/SQUARE_EDGE_SIZE);
      
      if((squareSum & 1) == 0) 
      {
         inter.setMaterial(whiteSquare);
      }
      else
      {
         inter.setMaterial(blackSquare);
      }
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
      //while (SDL_PollEvent(&e))
      //   handleEvent(&e);

      clearCanvas();
      draw();
   }
   SDL_Quit();
   return 0;
}

//CLASSLESS FUNCTIONS
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
   return Point(scalar*p._x, scalar*p._y, scalar*p._z);
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
   return Point(scalar*p._x, scalar*p._y, scalar*p._z);
}

/*
PURPOSE: generate a random vector of length 1
RECEIVES: Nothing
RETURNS: Nothing
REMARKS:
*/
Point randomUnit()
{
   //generate random point within unit sphere
   Point vec(0.0, 0.0, 0.0);

   while(vec.isZero())
   {
      vec = Point(double(rand())/(RAND_MAX+1.0) - .5, 
               double(rand())/(RAND_MAX+1.0) - .5,
               double(rand())/(RAND_MAX+1.0) - .5);
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
inline GLdouble attenuation(GLdouble distance)
{
   return ATTENUATION_FACTOR/(ATTENUATION_FACTOR + distance*distance);
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
void rayTraceRay(Shape& scene, vector<Light> lights, const Line& ray, Point& color, unsigned int 
depth)
{
   Intersection intersection;
   scene.intersection(ray, Point(0.0, 0.0, 0.0), intersection);
   
   if(!intersection.intersects()) return;
   
   Point pt = intersection.point();
   Material material = intersection.material();
   Line reflectedRay = intersection.reflectedRay();
   Line transmittedRay = intersection.transmittedRay();
   Line shadowRay;
   Point lColor;
   
   size_t size = lights.size();
   for(size_t i = 0 ; i < size; i++)
   {
      shadowRay.set(pt, lights[i].position());
      Intersection shadowIntersection;

      scene.intersection(shadowRay, Point(0.0, 0.0, 0.0), shadowIntersection );
      
      if(!shadowIntersection.intersects() || !shadowIntersection.material().transparency().isZero())
      {
         lColor = attenuation(shadowRay.length())*lights[i].color();
         color += (material.ambient()% lColor) +
            abs(intersection.normal() & shadowRay.direction())*(material.diffuse()% lColor) +
            abs(ray.direction() & reflectedRay.direction())*(material.specular()% lColor);
      }
   }
   
   if(depth > 0)
   {
      Point transmittedColor(0.0, 0.0, 0.0);
      Point reflectedColor(0.0, 0.0, 0.0);

      Point transparency = material.transparency();
      Point opacity = Point(1.0, 1.0, 1.0) - transparency;
            
      if(!transparency.isZero() && transparency.length() > SMALL_NUMBER) //if not transparent then don't send ray
      {
         rayTraceRay(scene, lights, transmittedRay, transmittedColor, depth - 1);
         color += (transparency % transmittedColor);
      }
      if(!opacity.isZero()) // if completely transparent don't send reflect ray
      {
         rayTraceRay(scene, lights, reflectedRay, reflectedColor, depth - 1);
         color += (opacity % reflectedColor);
      }
   }
}

/*
PURPOSE: Does the ray-tracing of a shape according to the supplied lights, camera dimension and screen dimensions
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
void rayTraceScreen(Shape& scene, vector<Light>& lights, Point camera, Point lookAt, Point up, int
bottomX, int bottomY, int width, int height)
{
   Point lookDirection = lookAt - camera;
   Point right = lookDirection * up;
  
   right.normalize();
   Point rightOffset = width*right;

   up = right*lookDirection;
   up.normalize();
   
   Point screenPt = lookAt + bottomX*right + bottomY*up;
   
   Line ray;
   Point color(0.0, 0.0, 0.0);
   Point avgColor(0.0, 0.0, 0.0);
   
   Point weightedColor(0.0, 0.0, 0.0);
   Point oldWeightedColor(0.0, 0.0, 0.0);
   GLdouble k;
               
   glBegin(GL_POINTS);
      for(int j = 0; j < height; j++)
      {
         for(int i = 0; i < width; i++)
         {
            for(k = 0.0; k < SUPER_SAMPLE_NUMBER; k++)
            {
               ray.set(camera, screenPt + .5*randomUnit() );
               
               color.set(0.0, 0.0, 0.0);
               
               rayTraceRay(scene, lights, ray, color, MAX_DEPTH);
                           
               oldWeightedColor = (k + 1.0)*avgColor;

               avgColor += color;
                              
               weightedColor = k*avgColor;
               if((weightedColor - oldWeightedColor).length() < SMALL_NUMBER*k*(k+1)) break;
            } 
            //if ( k <  SUPER_SAMPLE_NUMBER && k >1) cout <<"hello"<<k<<endl;      
    
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
Point  convertStringCoordinate(string coordString)
{
      Point firstSquare(-BOARD_EDGE_SIZE/2, 0.0, BOARD_EDGE_SIZE/2);
            
      Point rowOffset(0.0, 0.0, -(double(coordString[0] - 'a') + .5)*SQUARE_EDGE_SIZE);
         //negative because farther back == higher row number
      Point colOffset((double(coordString[1] - '0' - 1) + .5)*SQUARE_EDGE_SIZE, 0.0, 0.0);
      Point heightOffset(0.0, 1.5*SQUARE_EDGE_SIZE, 0.0);
      
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
void init()
{
   Point boardPosition(0.0, 0.0, 0.0);
   CheckerBoard *checkerBoard = new CheckerBoard(boardPosition);
   scene.addRayObject(checkerBoard);

   string tmp;
   
   while (tmp != "done")
   {
			cout << "Please enter your object (light, tetrahedron, sphere, cube, cone, cylinder):\n";
				 cin >> tmp;
		 if (tmp == "light")
		 {
			 cout << "Please enter the position of the light:\n";
			 cin >> tmp;
			 //tmp ="b6"; //commented values are nice values to demonstrate the ray tracer
			 lightPosition = Point(BOARD_POSITION) + Point(0.0, 3.5*SQUARE_EDGE_SIZE, 0.0) + convertStringCoordinate(tmp);
				//with what convertStringCoordinate gives makes 5 squares above board
		  }
		    
		 if (tmp == "tetrahedron")
		 {
			 cout << "Please enter the position of the tetrahedron:\n";
			 cin >>tmp;
			 //tmp = "b4";
			 Tetrahedron *tetrahedron = new Tetrahedron(convertStringCoordinate(tmp), SQUARE_EDGE_SIZE);
			 scene.addRayObject(tetrahedron);
		}
		 if (tmp == "sphere")
		 {
			 cout << "Please enter the position of the sphere:\n";
			 cin >> tmp;
			 //tmp = "d7";
			 Sphere *sphere = new Sphere(convertStringCoordinate(tmp), SQUARE_EDGE_SIZE/2);
			 scene.addRayObject(sphere);
		}
		 if (tmp == "cube")
		 {
			 cout << "Please enter the position of the cube:\n";
			 cin >> tmp;
			 //tmp = "a7";
			 Cube *cube = new Cube(convertStringCoordinate(tmp), SQUARE_EDGE_SIZE);   
			 scene.addRayObject(cube);
		 }
		}
   
   glClearColor(0.0, 0.0, 0.0, 0.0);
        
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
   vector<Light> lights;
   
   lights.push_back(Light(lightColor, lightPosition));
   
   Point camera(CAMERA_POSITION);
   Point lookAt(LOOK_AT_VECTOR);   
   Point up(UP_VECTOR); 
   
  rayTraceScreen(scene, lights, camera, lookAt, up, -winWidth/2, -winHeight/2, winWidth, winHeight);
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
   glEnable(GL_MULTISAMPLE); //enable oversampling
   init();

   //makeShaders();
   //makeObjects();
   //makeTextures();
}

/*---------------------------------------------------------------------------*/
int main (int argc, char **argv) 
{
   return SdlApp().run();
}
