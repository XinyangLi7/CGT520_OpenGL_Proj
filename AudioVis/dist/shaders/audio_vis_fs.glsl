#version 450
uniform sampler2D diffuse_tex;
uniform samplerCube skybox_tex;
uniform int pass = 0;
uniform float time;

uniform vec3 tree;
uniform vec3 yellow;
uniform vec3 red;
uniform vec3 blue;
uniform int buildCount;
uniform vec3 randColor;

uniform vec3 eye_w; //world-space eye position (the first argument to lookAt)

layout(std430, binding = 0) restrict readonly buffer FFT_IN
{
	vec2 fft[];
};

in vec2 tex_coord; 
in vec3 p;
in vec3 n;

out vec4 fragcolor; //the output color for this fragment    

void main(void)
{   
	if(pass==0)
	{
		fragcolor = texture(skybox_tex, normalize(p))*0.6;
	}
	if(pass==1)
	{
		fragcolor = vec4(tree, 1.0);
	}
	if(pass==2)
	{
		fragcolor = vec4(yellow, 1.0);
	}
	if(pass==3)
	{
		fragcolor = vec4(red, 1.0);
	}
	if(pass==4)
	{
		fragcolor = vec4(blue, 1.0);
	}
	if(pass==5)
	{
		fragcolor = vec4(0.5f,0.5f,0.55f, 1.0);
	}
	if(pass==6)
	{
		int bin = int(fft.length() * abs(buildCount*0.0004));
		vec2 f = fft[bin];
		float s = 0.4+1.2*log(length(f)+ 1.0);
		float R = min(1.0, 0.5 + 0.6 * (s-0.6));
		float G = min(1.0, 0.5 + 0.3 * (s-0.6));
		float B = max(0.0, 0.5 - 0.5 * (s-0.6));
		vec3 timeColor = vec3(sin(time+randColor.x),sin(time+3.14*(0.67+randColor.y)),sin(time+3.14*(1.33+randColor)));
		vec3 winColor = randColor*s + 0.3*vec3(R, G, B)+ (s+1)*0.04*timeColor;
		if(1-n.y<=0.01){
			fragcolor = 0.7*vec4(0.8f,0.8f,0.88f, 1.0)+0.3*vec4(winColor, 1.0);
		}
		else{
			vec2 plane = vec2(-n.z,n.x);
			vec2 pos = vec2(p.x,p.z);
			float proj = dot(pos,plane)/length(plane)+0.0015;
			if(mod(p.y, 0.01) > 0.006&& mod(proj, 0.01) >= 0.005){
				fragcolor = vec4(winColor, 1.0);
			}
			else{
				fragcolor = vec4(0.1f,0.15f,0.2f, 1.0);
			}
		}
	}
	if(pass==7){
		//float alpha = 1.0 - length(gl_PointCoord - vec2(0.5)); // Circular fade
		fragcolor = vec4(1.0f, 1.0f, 1.0f, 0.6f); // White snowflake with fade
	}
}
