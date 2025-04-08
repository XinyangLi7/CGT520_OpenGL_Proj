#version 400
uniform sampler2D diffuse_tex;
uniform float time;
uniform vec3 La;
uniform vec3 Ld;
uniform vec3 Ls;
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float a;
uniform bool useTexture;

in vec2 tex_coord; 
in vec4 vw; 
in vec4 lw; 
in vec4 nw;
in float d;

out vec4 fragcolor; //the output color for this fragment    

void main(void)
{   
	vec4 rw = normalize(reflect(-lw, nw));
	vec3 ka=Ka;
	vec3 kd=Kd;
	if(useTexture){
		ka=texture(diffuse_tex, tex_coord).xyz;
		kd=texture(diffuse_tex, tex_coord).xyz;
	}
	vec4 Ambient=vec4(ka*La,1.0);
	vec4 Diffuse=vec4(kd*Ld*max(dot(nw,lw),0)/(d*d),1.0);
	vec4 Specular = vec4(Ks*Ls*pow(max(0,dot(rw,vw)),a)/(d*d),1.0);
	if(a==0&&dot(rw,vw)<=0)Specular = vec4(0,0,0,1.0);
	fragcolor = Ambient + Diffuse + Specular;
	// //read color from texture and assign to outgoing fragment color
}




















