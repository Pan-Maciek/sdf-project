#version 450 core
uniform sampler2D tex;
layout(location=0) uniform vec2 iResolution;
layout(location=1) uniform float iTime;
layout(location=2) uniform int iFrame;
layout(location=3) uniform int pCount;

struct bbox{
    vec3 min;
    vec3 max;
};

struct nodeKd
{
    float split;
	int flags;
};

layout(binding=0,std430) buffer vert{
    vec4 vertices[];
};

layout(binding=1,std430) buffer ind{
    int indices[];
};

layout(binding=2,std430) buffer acc{
    nodeKd nodes[];
};


out vec4 color;


float hash(float seed)
{
    return fract(sin(seed)*43758.5453 );
}

vec2 opU( vec2 d1, vec2 d2 )
{
	return (d1.x<d2.x) ? d1 : d2;
}

float dot2( in vec3 v ) { return dot(v,v); }

float udTriangle( in vec3 v1, in vec3 v2, in vec3 v3, in vec3 p )
{
    // prepare data    
    vec3 v21 = v2 - v1; vec3 p1 = p - v1;
    vec3 v32 = v3 - v2; vec3 p2 = p - v2;
    vec3 v13 = v1 - v3; vec3 p3 = p - v3;
    vec3 nor = cross( v21, v13 );

    return sqrt( // inside/outside test    
                 (sign(dot(cross(v21,nor),p1)) + 
                  sign(dot(cross(v32,nor),p2)) + 
                  sign(dot(cross(v13,nor),p3))<2.0) 
                  ?
                  // 3 edges    
                  min( min( 
                  dot2(v21*clamp(dot(v21,p1)/dot2(v21),0.0,1.0)-p1), 
                  dot2(v32*clamp(dot(v32,p2)/dot2(v32),0.0,1.0)-p2) ), 
                  dot2(v13*clamp(dot(v13,p3)/dot2(v13),0.0,1.0)-p3) )
                  :
                  // 1 face    
                  dot(nor,p1)*dot(nor,p1)/dot2(nor) );
}

float sdMesh(vec3 p) {
    
  // float d = udTriangle(vec3(-1.,1.,1.),vec3(1.,1.,1.),vec3(1.,-1.,1.),p);
   float d = udTriangle(vertices[indices[0].x].xyz,vertices[indices[0].y].xyz,vertices[indices[0].z].xyz,p);

   for (int i = 1; i < pCount; i++) {
        d = min(d, udTriangle(vertices[indices[i].x].xyz, vertices[indices[i].y].xyz, vertices[indices[i].z].xyz,p));
    }
    
    return d;
}

float sdSphere( vec3 p, float s )
{
  return length(p)-s;
}

float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

vec2 map(vec3 pos){
    
    vec2 d1=vec2(sdSphere(pos-vec3(0.,0.6,0.),0.5),1.5);
    vec3 q2=vec3(fract(pos.x*0.2)-0.5,pos.y*0.2,fract(pos.z*0.2)-0.5);
	vec2 d2=vec2(sdBox(q2,vec3(0.2,0.5,0.2)),2.5);
    d1=opU(d1,d2);
    d2=vec2(pos.y+1.,3.5);
    d1=opU(d1,d2);
	return d1;
}


vec3 calcNormal(vec3 pos){
    vec2 e=vec2(1.,0.)*0.001;
	return normalize(vec3(map(pos+e.xyy).x-map(pos-e.xyy).x,
                          map(pos+e.yxy).x-map(pos-e.yxy).x,
                         map(pos+e.yyx).x-map(pos-e.yyx).x));
}

//random cosine dist https://people.cs.kuleuven.be/~philip.dutre/GI/TotalCompendium.pdf
vec3 cosineDirection( in float seed, in vec3 nor)
{

    vec3 tc = vec3( 1.0+nor.z-nor.xy*nor.xy, -nor.x*nor.y)/(1.0+nor.z);
    vec3 uu = vec3( tc.x, tc.z, -nor.x );
    vec3 vv = vec3( tc.z, tc.y, -nor.y );
    
    float u = hash( 78.233 + seed);
    float v = hash( 10.873 + seed);
    float a = 6.283185 * v;

    return  sqrt(u)*(cos(a)*uu + sin(a)*vv) + sqrt(1.0-u)*nor;
}

//random cone dist https://people.cs.kuleuven.be/~philip.dutre/GI/TotalCompendium.pdf
vec3 uniformVector( in float seed)
{
    float a = 3.141593*hash( 78.233 + seed);
    float b = 6.283185*hash( 10.873 + seed);
    return vec3( sin(b)*sin(a), cos(b)*sin(a), cos(a) );
}

vec2 castRay(vec3 ro,vec3 rd){
 	float tmax=20.0;
    float t=0.;
    float mat=-1.;
    for(int i=0;i<256;i++){
    	vec3 pos=ro+t*rd;
        vec2 h=map(pos);
        mat=h.y;
        t+=h.x;
        if(abs(h.x)<(0.001)*t||t>tmax)
            break;
    }
    if(t>tmax) mat=-1.;
    return vec2(t,mat);
}

float castShadows(vec3 ro, vec3 rd){
	float result=1.;
    float tmax=10.;
    float t=0.05;
    for(int i=0;i<64;i++){
        vec3 pos=ro+t*rd;
        float h=map(pos).x;
        result=min(result,8.*h/t);
        t+=h;
        if(abs(h)<0.001*t||t>tmax)
            break;
    }


    return max(result,0.01);
    
}


vec3 calculateColor(vec3 ro,vec3 rd,vec3 col,float off1){
    vec3 rds=rd;
    vec3 fade=vec3(1.);
    vec3 total=vec3(0.);
    vec3 sunDir=normalize(vec3(4.,5.,1.2));
    vec3 sunColor=vec3(3.,3.,1.5);
    vec3 pos;
    float fDist;
    vec3 oro=ro;
    vec3 ord=rd;
    for(int i=0;i<2;i++){
    	vec2 t=castRay(ro,rd);
        if(t.y>-1.){
            if(i==0)
                fDist=t.x;
        	vec4 mat=vec4(0.2,0.2,0.2,0.2);
            if(t.y<2.)
                mat=vec4(0.2,0.2,0.3,0.001);
            else if(t.y<3.)
                mat=vec4(0.1,0.2,0.2,0.01);
            else if(t.y<4.)
            	mat=vec4(0.1,0.15,0.2,0.01);
            else if(t.y<5.) 
                mat=vec4(0.2,0.2,0.2,0.01);
            
        	pos=ro+rd*t.x;
            vec3 nor=calcNormal(pos);
            float sunDiffuse=clamp(dot(nor,sunDir),0.,1.);
        	float sunShadow=castShadows(pos+nor*0.01,sunDir);
            

            vec3 diffuse=sunColor*sunDiffuse*sunShadow; 
      		fade*=mat.xyz;
        
            //if monte carlo
            vec3 skyPoint = cosineDirection( off1 + 7.1*float(iFrame) + 581.123 + float(i)*92.13, nor);
        	float skySha = castShadows( pos + nor*0.01, skyPoint);//skyPoint
        	diffuse += vec3(0.2,0.4,0.7) * skySha;
            ro=pos+nor*0.001;

            total+=fade*diffuse;
		
            float diff=mat.w;
            if( hash(off1 +7.7*float(i)) < diff )
                rd=cosineDirection(76.2 + 73.1*float(i) + off1 + 17.7*float(iFrame), nor);
     		else
                rd = normalize(reflect(rd, nor)) + uniformVector(off1 + 111.123 + 65.2*float(i)) * diff;
                

        }else{
        if(i==0)
            return col;
            break;
        }
        
    	
    }
    //fog
    //exclude beam from fog, otherwise beam would appear on floor
    total = mix( total, vec3(0.02,0.04,0.07)*0.6*rds.y+abs(sin(0.005*(iTime-3.))*1./(abs(rds.x+0.01)+0.01))*vec3(0.1,0.2,0.3), 1.0-exp( -0.0005*fDist*fDist*fDist ) );
	return total;

}
void main()
{
 vec3 tot=vec3(0.);
 for(int i=0;i<1;i++){
   	float off1=hash(dot( gl_FragCoord.xy, vec2(12.9898, 78.233))+iTime*354.+float(i)*1111.45);
    vec2 off2=vec2(hash(off1+11.54),hash(off1+56.97));
    
	vec2 q = (2.*(gl_FragCoord.xy+off2)-iResolution.xy )/iResolution.y;
	vec3 ro=vec3(0.,0.,2.);
    //ro+=vec3(0.,0.,-iTime);
    vec3 rd=normalize(vec3(q.x,q.y,-1.));
    
    vec3 col =vec3(0.02,0.04,0.07)*0.6*rd.y;
    if(iTime>3.)
        col+=abs(sin(0.005*(iTime-3.))*1./(rd.x+0.01))*vec3(0.1,0.2,0.3);
    col=calculateColor(ro,rd,col,off1);
    tot+=col;
   // col=pow(col,vec3(0.4545));
	

    //tot += texture( iChannel0, gl_FragCoord.xy/iResolution.xy ).xyz;
    //fragColor = vec4(col,1.0);
}
 //tot/=2.;
   tot += texture( tex, gl_FragCoord.xy/iResolution.xy ).xyz;
   //tot=mix(texture( tex, gl_FragCoord.xy/iResolution.xy ).xyz,tot,iFrame/10000.);
   //tot=pow(tot,vec3(0.4545));
   color = vec4(tot,1.0);
   //color =mix(tot.xyzz, texture(tex, gl_FragCoord.xy/iResolution.xy), pow(.0001, iTimeDelta));

}
