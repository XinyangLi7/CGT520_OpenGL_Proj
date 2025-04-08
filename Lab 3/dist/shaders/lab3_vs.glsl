#version 400            
uniform mat4 PVM;
uniform mat4 M;
uniform vec3 lightPos;
uniform vec3 eyePos;

uniform float time;

in vec3 pos_attrib; //this variable holds the position of mesh vertices
in vec2 tex_coord_attrib;
in vec3 normal_attrib;  

out vec2 tex_coord; 
out vec4 vw; 
out vec4 lw; 
out vec4 nw; 
out float d;

void main(void)
{
	vec4 Ew = vec4(eyePos, 1.0);
	vec4 Lw = vec4(lightPos, 1.0);
	vec4 Pw = M*vec4(pos_attrib, 1.0);
	vw = normalize(Ew-Pw);
	lw = normalize(Lw-Pw);
	nw = normalize(M*vec4(normal_attrib,1.0));
	d = distance(Lw,Pw);
	gl_Position = PVM*vec4(pos_attrib, 1.0); //transform vertices and send result into pipeline
	tex_coord = tex_coord_attrib; //send tex_coord to fragment shader
}