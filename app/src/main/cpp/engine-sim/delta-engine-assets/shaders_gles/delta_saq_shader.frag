#version 300 es
precision highp float;
layout uniform sampler2D layer0;
layout uniform sampler2D layer1;
out vec4 out_Color;
in vec4 ex_Pos; in vec2 ex_Tex; in vec3 ex_Normal;
void main(){vec4 l0=texture(layer0,ex_Tex).rgba; vec4 l1=texture(layer1,ex_Tex).rgba; float a=l1.a; out_Color=a*l1+(1.0-a)*l0;}
