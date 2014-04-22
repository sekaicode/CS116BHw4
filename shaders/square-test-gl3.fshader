#version 130
//#define NUM_SPHERES 2
#define NUM_TRIANGLES 1
#define NUM_LIGHTS 1

#define NONE 0
#define CHECKERBOARD 1
#define MIRROR 2
#define REDMIRROR 3

#define LIGHT_RAY 0
#define SHADOW_RAY 1

#define EPSILON 0.001

#define MAXDEPTH 20	//20 reflections max

uniform vec3[NUM_SPHERES] u_spherePos;
uniform float[NUM_SPHERES] u_sphereRad;

uniform vec3[3*NUM_TRIANGLES] u_triPoints;
uniform vec3[NUM_LIGHTS] u_lights;

in vec3 vPosition;

out vec4 fragColor;

struct Ray {
	vec3 org;    //origin position of the ray
	vec3 dir;    //normalized direction of ray
};

struct Sphere {
	vec3 pos;
	float rad;
	int surface;
};

struct Triangle {
	vec3 v0;
	vec3 v1;
	vec3 v2;
	int surface;
};

struct Cone {
	vec3 base;
	float height;
	float rad;
};

struct Collision {
	vec3 pos;
	vec3 norm;
	int surface;  //surface type, i.e. checkerboard or mirror
};

struct StackFrame {
	int rayType;    //light or shadow ray.
};

Sphere sphereArray[NUM_SPHERES] = Sphere[NUM_SPHERES](Sphere(vec3(-1.5, -1.5, -8.0), 1.0, MIRROR), Sphere(vec3(1.5, -1.0, -9.0), 1.0, MIRROR));

Triangle triangleArray[NUM_TRIANGLES] = Triangle[NUM_TRIANGLES](Triangle(vec3(-10, -2, -5), vec3(10, -2, -5), vec3(0, -2, -70), CHECKERBOARD));

vec3 lightArray[NUM_LIGHTS] = vec3[NUM_LIGHTS](vec3(0, 1, -8) /*,vec3(0, 2, -10)*/);

vec3 sphereIntersect(Ray ray, Sphere sphere) {
	vec3 org = ray.org;
	vec3 dir = ray.dir;
	vec3 pos = sphere.pos;
	float rad = sphere.rad;
	vec3 deltaP = pos - org;
	float dirDeltaP = dot(dir, deltaP);
	float discriminant = dirDeltaP * dirDeltaP - dot(deltaP, deltaP) + rad * rad;
	if (discriminant < 0.0 || dirDeltaP < sqrt(discriminant)) {
		return vec3(0, 0, 0);		//no intersection lol
	}
	return org + (dirDeltaP - sqrt(discriminant)) * dir;
}


vec3 sphereNormal(Sphere sphere, vec3 pos) {
	return normalize(pos - sphere.pos);
}


//The Moller-Trumbore Algorithm:
vec3 triIntersect(Ray ray, Triangle tri) {
	vec3 org = ray.org;
	vec3 dir = ray.dir;
	vec3 p0 = tri.v0;
	vec3 p1 = tri.v1;
	vec3 p2 = tri.v2;
	vec3 s0 = p1 - p0;
	vec3 s1 = p2 - p0;
	vec3 p = cross(dir, s1);
	float det = dot(s0, p);
	if (det <= 0.0) {
		return vec3(0, 0, 0); //ray parallel to triangle
	}
	vec3 top0 = org - p0;
	float u = dot(top0, p) / det;
	if (u < 0.0 || u > 1.0) {		//outside the triangle
		return vec3(0, 0, 0);
	}
	vec3 q = cross(top0, s0);
	float v = dot(dir, q) / det;
	if (v < 0.0 || u + v > 1.0) {		//outside the triangle
		return vec3(0, 0, 0);
	}
	return org + dir * dot(s1, q) / det;
}

vec3 triNormal(Triangle tri) {
	
	return normalize(cross(tri.v1 - tri.v0, tri.v2 - tri.v0));
}

//doesn't work lol
vec3 coneIntersect(Ray ray, Cone cone) {
	vec3 d = ray.dir;
	vec3 o = ray.org;
	vec3 f = cone.base;
	float height = cone.height;
	float rad = cone.rad;
	float base = cone.base;
	float k = rad / height;
	float a = d.x*d.x - k*(k*d.y*d.y) + d.z*d.z;
	float b = o.x*d.x-f.x*d.x + o.z*d.z-f.z*d.z - k * (o.y+d.y - f.y*d.y);
	float c = (o.x-f.x)*(o.x-f.x) + (o.z-f.z)*(o.z-f.z) - k*(o.y-f.y)*(o.y-f.y);
	if (b*b < 4*a*c) {
		//return vec3(0, 0, 0);
	}
	float t = max(-b + (b*b - sqrt(4*a*c))/(2*a), -b - (b*b - sqrt(4*a*c))/(2*a));
	return vec3(a/10, b/10, c/10);
}

vec3 coneNormal(Sphere sphere, vec3 pos) {
	vec3 delta = pos - sphere.pos;
	return normalize(delta - vec3(0, 1, 0) * dot(vec3(0, 1, 0), delta));
	//return normalize(pos - sphere.pos);
}


Collision getCollision(Ray ray) {
	Collision col;
	float dist = 1/0.0;
	col.surface = NONE;  //default to no collision
	//spheres
	for (int n = 0; n < NUM_SPHERES; n++) {
		vec3 point = sphereIntersect(ray, sphereArray[n]);
		if (point != vec3(0, 0, 0)) {
			float d = length(ray.org - point);
			if (d < dist) {
				dist = d;
				col.pos = point;
				col.norm = sphereNormal(sphereArray[n], point);
				col.surface = sphereArray[n].surface;
			}
		}
	}
	//triangles
	for (int n = 0; n < NUM_TRIANGLES; n++) {
		vec3 point = triIntersect(ray, triangleArray[n]);
		if (point != vec3(0, 0, 0)) {
			float d = length(ray.org - point);
			if (d < dist) {
				dist = d;
				col.pos = point;
				col.norm = triNormal(triangleArray[n]);
				col.surface = triangleArray[n].surface;
			}
		}
	}
	return col;
}
/*
void main() 
{
	
	//raytracing code
	Ray ray = Ray(vec3(0, 0, 0), normalize(vPosition));
	Sphere sphere = Sphere(vec3(0.0, 0.0, -7.0), 1.0, CHECKERBOARD);
	vec3 p0 = vec3(-1, -2, -5);
	vec3 p1 = vec3(1, -2, -5);
	vec3 p2 = vec3(0, -2, -7);
	Triangle tri = Triangle(p0, p1, p2, CHECKERBOARD);
	
	triangleArray[0] = tri;
	sphereArray[0] = sphere;
	
	vec3 org = vec3(0, 0, 0);
	vec3 dir = normalize(vPosition);
	vec3 sphInt = sphereIntersect(ray, sphere);
	vec3 triInt = triIntersect(ray, tri);
	Collision col = getCollision(ray);
	if (sphInt == vec3(0, 0, 0)) {
		if (triInt == vec3(0, 0, 0)) {
			fragColor = vec4(0, 0, 0, 0);
		} else {
			fragColor = vec4(1, 0, 0, 0);
		}
	} else {
		if (triInt == vec3(0, 0, 0) || (length(triInt-org) > length(sphInt - org)) ) {
			vec3 normal = col.norm;
			ray.dir = ray.dir - 2 * normal * dot(ray.dir, normal);
			ray.org = sphInt;
			triInt = triIntersect(ray, tri);
		}
		if (triInt != vec3(0, 0, 0)) {
			fragColor = vec4(1, 0, 0, 0);
		} else {
			fragColor = vec4(.1, .1, .1, 0);
		}
	}
	
	//fragColor = vec4(triIntersect(vec3(0,0,0), normalize(vPosition), vec3(-1,0,-7), vec3(2,0,-7), vec3(1,1,-7))[0], 0f, 0f, 0f);
}
*/

float getLightAmount(Collision col) {
	float light = 0.00;  //ambient lighting
	for (int n = 0; n < NUM_LIGHTS; n++) {
		vec3 toLight = lightArray[n] - col.pos;
		Ray ray = Ray(col.pos, normalize(toLight));
		if (getCollision(ray).surface != NONE) {	//if there is something obstructing the light
			continue;
		}
		light += max(0.0, 5*dot(ray.dir, col.norm)) / dot(toLight, toLight);
	}
	return light;
}

vec4 getCheckerboardColor(vec3 position) {
	return vec4(1, 1, 1, 0) * mod(step(.25, mod(position.x, .5)) + step(.25, mod(position.z, .5)), 2);
}


void main() {
	//fragColor = vec4(0, 0, 1, 0);
	Ray ray = Ray(vec3(0, 0, 0), normalize(vPosition));
	int depth = 1;
	int reddening = 0; //number of red surfaces encountered
	Collision collision = getCollision(ray);
	while (collision.surface >= MIRROR && depth < MAXDEPTH) {
		reddening += (collision.surface == REDMIRROR) ? 1 : 0;
		ray.org = collision.pos;
		ray.dir = ray.dir - 2 * collision.norm * dot(ray.dir, collision.norm);	//reflect the ray
		collision = getCollision(ray);
		depth++;
	}
	if (collision.surface == CHECKERBOARD) {
			float red = float(reddening)/depth;
			fragColor = getLightAmount(collision) * (red*vec4(1, 0, 0, 0) + (1-red)*getCheckerboardColor(collision.pos));
	} else {
			fragColor = vec4(0, 0, 0, 0);
	}
}

