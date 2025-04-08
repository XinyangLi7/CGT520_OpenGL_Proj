#version 400            
uniform mat4 PVM;
uniform float time;
uniform vec4 debug = vec4(0.0);


in vec3 pos_attrib; //this variable holds the position of mesh vertices
in vec2 tex_coord_attrib;
in vec3 normal_attrib;  

out vec2 tex_coord; 

void main(void)
{
	//vec3 disp = sin(5.0*debug.x*time+10.0*pos_attrib.x)*vec3(0.0, 0.05, 0.0);
	gl_Position = PVM*vec4(pos_attrib, 1.0); //clip coordinates transform vertices and send result into pipeline
	tex_coord = tex_coord_attrib; //send tex_coord to fragment shader
}