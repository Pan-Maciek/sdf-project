#version 450 core
uniform sampler2D tex;
layout(location=0) uniform vec2 iResolution;
layout(location=1) uniform float iTime;
layout(location=2) uniform int iFrame;
layout(location=3) uniform vec3 bbox_center;
layout(location=4) uniform vec3 bbox_dim;

uint id;
float it=0.;
struct kd_node {
    float split;
	int flags;
};

layout(binding=0,std430) buffer vert {
    vec4 vertices[];
};

layout(binding=1,std430) buffer ind {
    int indices[];
};

layout(binding=2,std430) buffer acc {
    kd_node nodes[];
};

layout(binding=3,std430) buffer prim {
    uint primitives[];
};

out vec4 color;


float hash(float seed) {
    return fract(sin(seed)*43758.5453);
}

vec2 opU(vec2 d1, vec2 d2) {
	return (d1.x<d2.x) ? d1 : d2;
}

float dot2(in vec3 v ) { return dot(v,v); }

float udTriangle( in vec3 v1, in vec3 v2, in vec3 v3, in vec3 p ) {
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

#define kd_isleaf(node) ((node.flags & 3) == 3)

#define kd_split(node) node.split
#define kd_split_axis(node) (node.flags & 3)
#define kd_above_child(node) (node.flags >> 2)

#define kd_pcount(node) (node.flags >> 2)
#define kd_poffset(node) floatBitsToInt(node.split)

struct uint_stack { uint data[32]; uint pointer; };
#define stack_init(s) s.pointer = 0;
#define stack_isempty(s) (s.pointer == 0)
#define stack_push(s,x) s.data[s.pointer++] = x
#define stack_pop(s) s.data[--s.pointer]

#define loop for(;;)

float sdBox( vec3 p, vec3 b ) {
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

vec2 sdMesh(vec3 p, float d, float t, float h) {

    if (sdBox(p - bbox_center, bbox_dim) > d)
        return vec2(d, 3.5);

    uint_stack stack;
    stack_init(stack);
    kd_node node;

    uint idx = 0, pcount, i, poffset;
    bool up = false;
    float tmp_d;

    float iterate = 0;
	loop {
        node = nodes[idx];
        // if (d < 0.0001) return vec2(d, min(iterate, 1.));
        if (d < 0.0001) return vec2(d, 1);
        iterate += 1./100.;
        // it += 1./1000.;

		// up - indicates if we move up the stack
		// only inner nodes are on the stack => we skip leaf check
		// if there is possibility of better distance in other node we traverse there
		if (up) {
			tmp_d = abs(p[kd_split_axis(node)] - kd_split(node))+0.015;
			if (p[kd_split_axis(node)] < kd_split(node) && tmp_d < d) { // low child was chosen while traversing down
				idx = kd_above_child(node);
				up = false;
				continue;
			} else if (p[kd_split_axis(node)] >= kd_split(node) && tmp_d < d) { // high child was chosen while traversing down
				idx = idx + 1; // idx + 1 is kd_below_child
				up = false;
				continue;
			} // distance to split is to high, continue traversing up (end of loop)
		} else if (kd_isleaf(node)) {
			pcount = kd_pcount(node);
			poffset = kd_poffset(node);
            it += 1./1000.;
			if (pcount == 1) {
                tmp_d = udTriangle(
					vertices[primitives[3*poffset]].xyz,
					vertices[primitives[3*poffset+1]].xyz,
					vertices[primitives[3*poffset+2]].xyz,
					p
				);
                if (tmp_d <= d) {
                    d = tmp_d;
                    id = 3 * poffset;
                }
                it += 1./1000.;
			} else for (i = 0; i < pcount; ++i) {
                tmp_d = udTriangle(
					vertices[primitives[3*indices[poffset+i]]].xyz,
					vertices[primitives[3*indices[poffset+i] + 1]].xyz,
					vertices[primitives[3*indices[poffset+i] + 2]].xyz,
					p
				);
                if (tmp_d <= d) {
                    d = tmp_d;
                    id = 3*indices[poffset+i];
                }
                it += 1./1000.;
			}
			// traverse up (end of loop)
		} else {
            it += 1./1000.;
			stack_push(stack, idx);
			if (p[kd_split_axis(node)] < kd_split(node)) idx = idx + 1; // choose low child
			else idx = kd_above_child(node); // choose high child 
			continue;
		}

		// traversing up
		if (stack_isempty(stack)) break;
		idx = stack_pop(stack);
		up = true;
    }

	// return vec2(d, min(iterate, 1.));
	return vec2(d, 1);
}

float sdSphere( vec3 p, float s ) {
  return length(p)-s;
}

vec2 mapSDF(vec3 pos){
    vec2 d2=vec2(pos.y+1.,3.5);
    return d2;
    // vec2 d1=sdMesh(pos-vec3(0.2,-0.5,0.5), 1., 100,100);
	// return d1;
}

vec2 map(vec3 pos,float t,float h){
   // vec2 d1=sdMesh(pos-vec3(0.5,-0.5,0.5), pos.y + 1.,t,h);
     vec2 d1=sdMesh(pos-vec3(-0.3,-0.5,0.8), 1.,t,h);
	return d1;
}

vec3 calcNormal(vec3 pos){
	return normalize(cross(
        vertices[primitives[id+1]].xyz-vertices[primitives[id]].xyz,
        vertices[primitives[id+2]].xyz-vertices[primitives[id]].xyz
    ));   
}

vec3 calcNormal2(vec3 pos){
    vec2 e=vec2(1.,0.)*0.001;
	return normalize(vec3(map(pos+e.xyy,0.1,0.1).x-map(pos-e.xyy,0.1,0.1).x,
                          map(pos+e.yxy,0.1,0.1).x-map(pos-e.yxy,0.1,0.1).x,
                         map(pos+e.yyx,0.1,0.1).x-map(pos-e.yyx,0.1,0.1).x));
}

vec3 calcNormalSDF(vec3 pos){
    vec2 e=vec2(1.,0.)*0.001;
	return normalize(vec3(mapSDF(pos+e.xyy).x-mapSDF(pos-e.xyy).x,
                          mapSDF(pos+e.yxy).x-mapSDF(pos-e.yxy).x,
                         mapSDF(pos+e.yyx).x-mapSDF(pos-e.yyx).x));
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
    float t=0.1;
    float mat=-1.;
    vec2 h=vec2(0.1,0.);
    for(int i=0;i<64;i++){
    	vec3 pos=ro+t*rd;
        h=map(pos,t,h.x);
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
    float tmax=30.;
    float t=0.1;
    float h=0.1;
    float ph = 1e10;
    for(int i=0;i<64;i++){
        vec3 pos=ro+t*rd;
        h=map(pos,t,h).x;
        float y = h*h/(2.0*ph);
        float d = sqrt(h*h-y*y);
        result = min( result, 32.*d/max(0.0,t-y) );
        ph = h;
        t+=h;
        if(abs(h.x)<0.001*t||t>tmax)
            break;
    }
    return max(result,0.01);
}

float checkersTexture( in vec2 p )
{
    vec2 q = floor(p);
    return mod( q.x+q.y, 2.0 );            // xor pattern
}

vec3 calculateColor(vec3 ro,vec3 rd,vec3 col,float off1){
    vec3 rds=rd;
    vec3 fade=vec3(1.);
    vec3 total=vec3(0.);
    vec3 sunDir=normalize(vec3(4.,5.,1.2));
    vec3 sunColor=vec3(2.,2.,1.);
    vec3 pos;
    float fDist;
    vec3 oro=ro;
    vec3 ord=rd;
    vec3 nor;
    for(int i=0;i<1;i++){
    	vec2 t=castRay(ro,rd);
        if(t.y>-1.){
            if(i==0)
                fDist=t.x;
        	vec4 mat=vec4(0.1,0.2,0.2,0.01);
           
            // return vec3(it, it, it);
            return vec3(t.y,t.y,t.y);
        	pos=ro+rd*t.x;
            if(t.y<3.)
            nor=calcNormal(pos);
            // return vec3(nor.x, nor.y, nor.z);
            else
            nor=calcNormalSDF(pos);
            //return vec3(nor.x,nor.y,nor.z);
            float sunDiffuse=clamp(dot(nor,sunDir),0.,1.);
        	//float sunShadow=castShadows(pos+nor*0.001,sunDir);
            // return vec3(nor.x, nor.y, nor.z);
            

            vec3 diffuse=sunColor*sunDiffuse; 
            //return vec3(sunShadow,sunShadow,sunShadow);
            
      		fade*=mat.xyz;


            //if monte carlo
           /* vec3 skyPoint = cosineDirection( off1 + 7.1*float(iFrame) + 581.123 + float(i)*92.13, nor);
        	float skySha = castShadows( pos + nor*0.1, skyPoint);//skyPoint
        	diffuse += vec3(0.2,0.4,0.7) * skySha;*/
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
 
    float pattern=smoothstep(-0.00001,0.00001,cos(5.*pos.x)*cos(5.*pos.y)*cos(5.*pos.z));
	return total+pattern*0.02;

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

