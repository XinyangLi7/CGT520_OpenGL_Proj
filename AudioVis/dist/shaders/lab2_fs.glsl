#version 400
uniform sampler2D diffuse_tex;
uniform float time;

in vec2 tex_coord; 

out vec4 fragcolor; //the output color for this fragment    

void main(void)
{   
	//vec2 disp = sin(5.0*time+5.0*tex_coord.x)*vec2(0.05, 0.05);
	fragcolor = texture(diffuse_tex, tex_coord); //read color from texture and assign to outgoing fragment color
	//fragcolor = vec4(tex_coord, 0.0, 1.0);
	//fragcolor = sin(0.1*gl_FragCoord.x)*texture(diffuse_tex, tex_coord);
	
	//fragcolor = sin(100.0 * sin(time)*tex_coord.x) * texture(diffuse_tex, tex_coord);

	//fragcolor = vec4(sin(1000.0 * sin(time) * tex_coord.x * tex_coord.y));
}




















