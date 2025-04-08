#version 430

layout(local_size_x = 1024) in;

layout(std430, binding = 0) restrict readonly buffer C_IN
{
   vec2 complex_in[];
};

layout(std430, binding = 1) restrict writeonly buffer C_OUT
{
   vec2 complex_out[];
};

layout(std430, binding = 2) restrict readonly buffer RE_IN
{
   float real_in[];
};

layout(std430, binding = 3) restrict writeonly buffer RE_OUT
{
   float real_out[];
};

layout(location = 0) uniform int uMode = 0;
layout(location = 1) uniform float scale = 1.0;

const float pi = 3.141592653589793;

void main()
{
   int gid = int(gl_GlobalInvocationID.x);

	if (uMode == 0) //real to complex
	{
		vec2 c = vec2(real_in[gid], 0.0);
		complex_out[gid] = c;
	}
	else if (uMode == 1) //complex to real (log magnitude)
	{
		vec2 c = complex_in[gid];
		real_out[gid] = scale*log(dot(c,c)+1.0);
	}
	else if (uMode == 2) //shift
	{
		int size = complex_in.length();
		int shift = size/2;
		vec2 c = complex_in[gid];
		complex_out[(gid+shift)%size] = c;
	}
	else if(uMode==3) //real hanning window
	{
		float u = gid/float(real_in.length()-1);
		float w = cos(pi*(u-0.5));
		//real_out[gid] = w;
		real_out[gid] = w*real_in[gid];
	}
	else if(uMode == 4) //real gaussian window
	{
		float u = gid / float(real_in.length() - 1);
		const float sd = 1.0/6.0;
		float z = (u-0.5)/sd;
		float w = exp(-0.5*z*z);
		//w = 1; //no op 
		//real_out[gid] = w;
		real_out[gid] = w*real_in[gid];
	}
	else if (uMode == 5) //to polar
	{
		vec2 c_in = complex_in[gid];
		complex_out[gid] = vec2(log(length(c_in)+1.0), atan(c_in.y, c_in.x));
	}
}
