#version 430

layout(local_size_x = 1024) in;

layout(std430, binding = 0) restrict readonly buffer SIGNAL_IN
{
   vec2 complex_in[];
};

layout(std430, binding = 1) restrict writeonly buffer SIGNAL_OUT
{
   vec2 complex_out[];
};

layout(location = 0) uniform int N;
layout(location = 1) uniform int Ns = 2;
layout(location = 2) uniform int dir = +1;

const float pi = 3.141592653589793;

vec2 cmul(vec2 a, vec2 b)
{
   return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

void main()
{
   const int i = int(gl_GlobalInvocationID.x);
   if(i>=complex_out.length()) return;

   int k = Ns / 2;
   int a = (i / Ns) * k;
   int b = i % k;
   int i0 = a + b;
   int i1 = i0 + (N / 2);
   float t = 2.0 * pi * i / Ns;
   vec2 f = vec2(cos(t), sin(t));
   f.y *= -dir; //dir = +1 is the forward fft, -1 is ifft. Take complex conjugate for ifft
   vec2 x0 = complex_in[i0];
   vec2 x1 = complex_in[i1];
   vec2 x = x0 + cmul(f, x1);

   //do scaling in final pass of inv
   if (Ns == N && dir == -1)
   {
      x = x/float(N);
   }

   complex_out[i] = x;
}
