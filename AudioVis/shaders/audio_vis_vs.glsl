#version 450   
uniform mat4 PV;
uniform mat4 M;
uniform int pass = 0;
uniform float time;
uniform int buildCount;

layout(std430, binding = 0) restrict readonly buffer FFT_IN
{
	vec2 fft[];
};

in vec3 pos_attrib; //this variable holds the position of mesh vertices
in vec2 tex_coord_attrib;
in vec3 normal_attrib;  

out vec2 tex_coord;
out vec3 p;
out vec3 n;

void main(void)
{
	if(pass==0)
	{
		gl_Position = PV*vec4(pos_attrib, 1.0);
		p = pos_attrib.xyz;
	}
	if(pass==1||pass==2||pass==3||pass==4||pass==5)
	{
		int bin = int(fft.length() * abs(tex_coord_attrib.x));
		vec2 f = fft[bin];
		float s = 1.0+0.25*log( length(f)+ 1.0);
		//gl_Position = PVM*vec4(2.0*s*pos_attrib, 1.0); //clip coordinates transform vertices and send result into pipeline

		gl_Position = PV*M*vec4(pos_attrib, 1.0); 
		tex_coord = tex_coord_attrib; 

		p = vec3(M*vec4(pos_attrib, 1.0));		//world-space vertex position
		n = vec3(M*vec4(normal_attrib, 0.0));	//world-space normal vector
	}
	if(pass==6){
		int bin = int(fft.length() * abs(buildCount*0.0004));
		vec2 f = fft[bin];
		float s = 1+1.5*log(length(f)+ 1.0);
		gl_Position = PV*M*vec4(pos_attrib.x,pos_attrib.y*s,pos_attrib.z, 1.0); 
		tex_coord = tex_coord_attrib; 

		p = vec3(M*vec4(pos_attrib.x,pos_attrib.y*s,pos_attrib.z, 1.0));		//world-space vertex position
		n = normalize(vec3(M*vec4(normal_attrib, 0.0)));	//world-space normal vector
	}
	if(pass==7){
		gl_Position = vec4(pos_attrib, 1.0);
	}
}