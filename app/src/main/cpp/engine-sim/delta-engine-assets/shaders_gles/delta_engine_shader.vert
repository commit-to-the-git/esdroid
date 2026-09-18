#version 300 es
layout(location=0) in vec4 in_Position;
layout(location=1) in vec2 in_Tex;
layout(location=2) in vec4 in_Normal;
out vec4 ex_Pos; out vec2 ex_Tex; out vec3 ex_Normal;
layout(std140) uniform ScreenVariables {
    mat4 CameraView; mat4 Projection; vec4 CameraEye; vec4 FogColor;
    float FogNear; float FogFar;
};
layout(std140) uniform ObjectVariables {
    mat4 Transform; vec2 TexOffset; vec2 TexScale; vec4 Scale;
    vec4 BaseColor; vec4 Emission; float SpecularMix; float DiffuseMix;
    float Metallic; float DiffuseRoughness; float SpecularPower; float IncidentSpecular;
    int ColorReplace; int Lit;
};
void main(){
    vec4 inputPos=vec4(in_Position.xyz,1.0);
    inputPos.xyz*=Scale.xyz;
    inputPos=inputPos*Transform; ex_Pos=inputPos;
    inputPos=inputPos*CameraView; inputPos=inputPos*Projection;
    vec4 fn=vec4(in_Normal.xyz,0.0); ex_Normal=vec3(fn*Transform);
    gl_Position=inputPos; ex_Tex=in_Tex;
}
