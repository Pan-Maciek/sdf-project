#version 450 core
uniform sampler2D tex;
layout(location=0) uniform vec2 iResolution;
layout(location=1) uniform float iTime;
layout(location=2) uniform int iFrame;
out vec4 color;

void main()
{
 vec2 uv = gl_FragCoord.xy / iResolution.xy;
    vec3 col = vec3(0.0);
    
if(iFrame>0){
    col = texture( tex, uv ).xyz;
    col /= float(iFrame); 
    col=pow(col,vec3(0.4545));
}

    color= vec4(col,1.0);

}

