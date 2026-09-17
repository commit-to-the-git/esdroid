#version 300 es
precision highp float; precision highp int;
uniform sampler2D diffuseTex;
out vec4 out_Color;
in vec4 ex_Pos; in vec2 ex_Tex; in vec3 ex_Normal;
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
struct Light { vec4 Position; vec4 Color; vec4 Direction; float Attenuation0; float Attenuation1; int FalloffEnabled; int Active; };
layout(std140) uniform Lighting { Light Lights[32]; vec4 AmbientLighting; };
float pow5(float v){return(v*v)*(v*v)*v;}
float f_d(vec3 i,vec3 o,vec3 h,vec3 n,float p,float r){float hi=dot(h,i),hi2=hi*hi,f90=0.5+2.0*hi2*r;float ci=dot(i,n),co=dot(o,n);float fd=(1.0+(f90-1.0)*pow5(1.0-ci))*(1.0+(f90-1.0)*pow5(1.0-co));return clamp(fd*p*ci,0.0,1.0);}
float f_s(vec3 i,vec3 o,vec3 h,vec3 n,float F0,float p,float sp){vec3 r=-reflect(i,n);float it=dot(r,o);if(it<0.0)return 0.0;float f0s=0.08*F0,oh=dot(o,h),s=pow5(1.0-oh),F=f0s+s*(1.0-f0s);return clamp(pow(it,sp)*F*p,0.0,1.0);}
float f_sa(vec3 o,vec3 n,float F0,float p){float f0s=0.08*F0,on=dot(o,n),s=pow5(1.0-on),F=f0s+s*(1.0-f0s);return clamp(F*p,0.0,1.0);}
float l2s(float u){if(u<0.0031308)return 12.92*u;return 1.055*pow(u,1.0/2.4)-0.055;}
float s2l(float u){if(u<0.04045)return u/12.92;return pow((u+0.055)/1.055,2.4);}
vec3 l2s3(vec3 v){return vec3(l2s(v.r),l2s(v.g),l2s(v.b));}
vec3 s2l3(vec3 v){return vec3(s2l(v.r),s2l(v.g),s2l(v.b));}
void main(){
    const float FS=1.0/0.08;
    vec3 tl=vec3(1.0); vec3 n=normalize(ex_Normal.xyz);
    vec4 bc; if(ColorReplace==0){vec4 d=texture(diffuseTex,ex_Tex).rgba;bc=vec4(s2l3(d.rgb),d.a)*BaseColor;}else{bc=BaseColor;}
    tl=bc.rgb;
    if(Lit==1){
        vec3 o=normalize(CameraEye.xyz-ex_Pos.xyz); float co=dot(o,n);
        vec3 as=f_sa(o,n,IncidentSpecular,SpecularMix)*AmbientLighting.rgb;
        vec3 ad=f_d(o,o,o,n,DiffuseMix,DiffuseRoughness)*AmbientLighting.rgb*bc.rgb;
        vec3 am=f_sa(o,n,FS,1.0)*AmbientLighting.rgb*bc.rgb;
        tl=mix(as+ad,am,Metallic); tl+=Emission.rgb;
        for(int li=0;li<32;++li){
            if(Lights[li].Active==0)continue;
            vec3 i=Lights[li].Position.xyz-ex_Pos.xyz; float inv=1.0/length(i); i*=inv;
            float ci=dot(i,n); if(ci<0.0||co<0.0)continue;
            vec3 h=normalize(i+o);
            vec3 df=f_d(i,o,h,n,DiffuseMix,DiffuseRoughness)*bc.rgb*Lights[li].Color.rgb;
            vec3 sp=f_s(i,o,h,n,IncidentSpecular,SpecularMix,SpecularPower)*Lights[li].Color.rgb;
            vec3 mt=vec3(0.0); if(Metallic>0.0)mt=f_s(i,o,h,n,FS,1.0,SpecularPower)*Lights[li].Color.rgb*bc.rgb;
            float sc=-dot(i,Lights[li].Direction.xyz),sa=1.0;
            if(sc>Lights[li].Attenuation0)sa=1.0; else if(sc<Lights[li].Attenuation1)sa=0.0;
            else{float t=Lights[li].Attenuation0-Lights[li].Attenuation1;sa=(t==0.0)?1.0:(sc-Lights[li].Attenuation1)/t;}
            float fo=1.0; if(Lights[li].FalloffEnabled==1)fo=inv*inv;
            vec3 bsdf=mix(df+sp,mt,Metallic);
            tl+=fo*bsdf*sa*sa*sa;
        }
    }
    float dc=length(CameraEye.xyz-ex_Pos.xyz);
    float fa=(clamp(dc,FogNear,FogFar)-FogNear)/(FogFar-FogNear);
    out_Color=vec4(l2s3(mix(tl,FogColor.rgb,fa)),bc.a);
}
