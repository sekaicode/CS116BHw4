#version 130

uniform sampler2D uTexUnit0;

in vec3 vNormal;
in vec3 vPosition;
in vec2 vTexCoord0;

out vec4 fragColor;

void main() 
{
    fragColor = texture2D(uTexUnit0, vTexCoord0);
}
