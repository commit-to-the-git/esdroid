#version 300 es
layout(location=0) in vec2 in_Position;
layout(location=1) in vec2 in_Tex;
layout(location=2) in vec4 in_Color;
out vec4 ex_Pos; out vec2 ex_Tex; out vec4 ex_Col;
layout(std140) uniform ScreenVariables { mat4 CameraView; mat4 Projection; };
layout(std140) uniform ObjectVariables { vec4 MulCol; vec2 TexOffset; vec2 TexScale; };
void main(){
    vec4 ip=vec4(in_Position,-1.0,1.0);
    ip=ip*CameraView; ip=ip*Projection;
    ex_Pos=ip; ex_Tex=in_Tex*TexScale+TexOffset; ex_Col=in_Color;
    gl_Position=ip;
}
