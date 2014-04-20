#version 130

in vec3 vPosition;

out vec4 fragColor;

struct ray {
	vec3 org;    //origin position of the ray
	vec3 dir;    //normalized direction of ray
}

struct sphere {
	vec3 pos;
	float rad;
}

struct triangle {
	vec3 v0;
	vec3 v1;
	vec3 v2;
}

struct collision {
	vec3 pos;
	vec3 norm;
}

vec3 sphereIntersect(vec3 org, vec3 dir, vec3 pos, float rad) {
	vec3 deltaP = pos - org;
	float dirDeltaP = dot(dir, deltaP);
	float discriminant = dirDeltaP * dirDeltaP - dot(deltaP, deltaP) + rad * rad;
	if (discriminant < 0.0) {
		return vec3(0, 0, 0);		//no intersection lol
	}
	return org + (dirDeltaP - sqrt(discriminant)) * dir;
}

vec3 sphereNormal(vec3 center, vec3 pos) {
	return normalize(pos - center);
}


vec3 triIntersect(vec3 org, vec3 dir, vec3 p0, vec3 p1, vec3 p2) {
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

vec3 triNormal(vec3 p0, vec3 p1, vec3 p2) {
	return normalize(cross(p1 - p0, p2 - p0));
}


void main() 
{
	/*
	if (triIntersect(vec3(0,0,0), normalize(vPosition), vec3(0,0,-7), vec3(3,0,-7), vec3(0,0.5,-7)) != vec3(0, 0, 0)){
		fragColor = vec4(1, 0, 0, 0);
	} else {
		fragColor = vec4(0, 0, 0, 0);
	}
	*/
	/*
	vec3 sphereCenter = vec3(-1, -1, -5);
	vec3 intersect = sphereIntersect(vec3(0,0,0), normalize(vPosition), sphereCenter, 1f);
	if (intersect != vec3(0, 0, 0)){
		fragColor = vec4(sphereNormal(sphereCenter, intersect), 0);
	} else {
		fragColor = vec4(0, 0, 0, 0);
	}
	*/
	
	//raytracing code
	vec3 sphere = vec3(0, 0, -7);
	vec3 p0 = vec3(-1, -2, -5);
	vec3 p1 = vec3(1, -2, -5);
	vec3 p2 = vec3(0, -2, -7);
	
	vec3 org = vec3(0, 0, 0);
	vec3 dir = normalize(vPosition);
	vec3 sphInt = sphereIntersect(org, dir, sphere, 1);
	vec3 triInt = triIntersect(org, dir, p0, p1, p2);
	if (sphInt == vec3(0, 0, 0)) {
		if (triInt == vec3(0, 0, 0)) {
			fragColor = vec4(0, 0, 0, 0);
		} else {
			fragColor = vec4(1, 0, 0, 0);
		}
	} else {
		if (triInt == vec3(0, 0, 0) || (length(triInt-org) > length(sphInt - org)) ) {
			vec3 normal = sphereNormal(sphere, sphInt);
			dir = dir - 2 * normal * dot(dir, normal);
			org = sphInt;
			triInt = triIntersect(org, dir, p0, p1, p2);
		}
		if (triInt != vec3(0, 0, 0)) {
			fragColor = vec4(1, 0, 0, 0);
		} else {
			fragColor = vec4(.1, .1, .1, 0);
		}
	}
	
	//fragColor = vec4(triIntersect(vec3(0,0,0), normalize(vPosition), vec3(-1,0,-7), vec3(2,0,-7), vec3(1,1,-7))[0], 0f, 0f, 0f);
}
