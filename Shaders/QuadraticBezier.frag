#version 450

// #extension GL_EXT_debug_printf : enable
// #extension GL_EXT_nonuniform_qualifier : enable

// Constants
layout(constant_id = 0) const uint MaxSpriteCount = 1;
const uint NullSpriteIndex = ~0;

// Inputs
layout(location = 0) in vec4 fragColor;
layout(location = 1) in flat vec2 fragLeft;
layout(location = 2) in flat vec2 fragControl;
layout(location = 3) in flat vec2 fragRight;
layout(location = 4) in flat float fragThickness;
layout(location = 5) in flat float fragEdgeSoftness;

// Outputs
layout(location = 0) out vec4 outColor;

// Bindings
layout(set = 0, binding = 0) uniform sampler2D sprites[MaxSpriteCount];


float det(const vec2 a, const vec2 b) { return a.x * b.y - b.x * a.y; }
float dot2(const vec2 v) { return dot(v, v); }
float cro(const vec2 a, const vec2 b) { return a.x * b.y - a.y * b.x; }

// float sdfBezier(const vec2 pos, const vec2 left, const vec2 control, const vec2 right)
// {
//     const vec2 b0 = left - pos;
//     const vec2 b1 = control - pos;
//     const vec2 b2 = right - pos;
//     const float a = det(b0, b2);
//     const float b = 2.0 * det(b1, b0);
//     const float d = 2.0 * det(b2, b1);
//     const float f = b * d - a * a;
//     const vec2 d21 = b2 - b1;
//     const vec2 d10 = b1 - b0;
//     const vec2 d20 = b2 - b0;
//     vec2 gf = 2.0 * (b * d21 + d * d10 + a * d20);
//     gf = vec2(gf.y, -gf.x);
//     const vec2 pp = -f * gf / dot(gf, gf);
//     const vec2 d0p = b0 - pp;
//     const float ap = det(d0p, d20);
//     const float bp = 2.0 * det(d10, d0p);
//     const float t = clamp((ap + bp) / (2.0 * a + b + d), 0.0, 1.0);
//     const vec2 dist = mix(mix(b0, b1, t), mix(b1, b2, t), t);
//    return length(dist);
//}

// float sdfBezier(const vec2 pos, const vec2 left, const vec2 control, const vec2 right)
// {
//     vec2 a = control - left;
//     vec2 b = left - 2.0*control + right;
//     vec2 c = a * 2.0;
//     vec2 d = left - pos;

//     float kk = 1.0/dot(b,b);
//     float kx = kk * dot(a,b);
//     float ky = kk * (2.0*dot(a,a)+dot(d,b))/3.0;
//     float kz = kk * dot(d,a);

//     float res = 0.0;
//     float sgn = 0.0;

//     float p  = ky - kx*kx;
//     float q  = kx*(2.0*kx*kx - 3.0*ky) + kz;
//     float p3 = p*p*p;
//     float q2 = q*q;
//     float h  = q2 + 4.0*p3;

//     if( h>=0.0 )
//     {   // 1 root
//         h = sqrt(h);
//         vec2 x = (vec2(h,-h)-q)/2.0;

//         #if 0
//         // When p≈0 and p<0, h-q has catastrophic cancelation. So, we do
//         // h=√(q²+4p³)=q·√(1+4p³/q²)=q·√(1+w) instead. Now we approximate
//         // √ by a linear Taylor expansion into h≈q(1+½w) so that the q's
//         // cancel each other in h-q. Expanding and simplifying further we
//         // get x=vec2(p³/q,-p³/q-q). And using a second degree Taylor
//         // expansion instead: x=vec2(k,-k-q) with k=(1-p³/q²)·p³/q
//         if( abs(p)<0.001 )
//         {
//             float k = p3/q;              // linear approx
//           //float k = (1.0-p3/q2)*p3/q;  // quadratic approx
//             x = vec2(k,-k-q);
//         }
//         #endif

//         vec2 uv = sign(x)*pow(abs(x), vec2(1.0/3.0));
//         float t = clamp( uv.x+uv.y-kx, 0.0, 1.0 );
//         vec2  q = d+(c+b*t)*t;
//         res = dot2(q);
//     	sgn = cro(c+2.0*b*t,q);
//     }
//     else
//     {   // 3 roots
//         float z = sqrt(-p);
//         float v = acos(q/(p*z*2.0))/3.0;
//         float m = cos(v);
//         float n = sin(v)*1.732050808;
//         vec3  t = clamp( vec3(m+m,-n-m,n-m)*z-kx, 0.0, 1.0 );
//         vec2  qx=d+(c+b*t.x)*t.x; float dx=dot2(qx), sx = cro(c+2.0*b*t.x,qx);
//         vec2  qy=d+(c+b*t.y)*t.y; float dy=dot2(qy), sy = cro(c+2.0*b*t.y,qy);
//         if( dx<dy ) { res=dx; sgn=sx; } else {res=dy; sgn=sy; }
//     }

//     return sqrt( res )*sign(sgn);
// }

vec2 closestPointInSegment( vec2 a, vec2 b )
{
  vec2 ba = b - a;
  return a + ba*clamp( -dot(a,ba)/dot(ba,ba), 0.0, 1.0 );
}

// From: http://research.microsoft.com/en-us/um/people/hoppe/ravg.pdf
vec2 get_distance_vector(vec2 b0, vec2 b1, vec2 b2) {

  float a=det(b0,b2), b=2.0*det(b1,b0), d=2.0*det(b2,b1); // ð›¼,ð›½,ð›¿(ð‘)

  if( abs(2.0*a+b+d) < 1000.0 ) return closestPointInSegment(b0,b2);

  float f=b*d-a*a; // ð‘“(ð‘)
  vec2 d21=b2-b1, d10=b1-b0, d20=b2-b0;
  vec2 gf=2.0*(b*d21+d*d10+a*d20);
  gf=vec2(gf.y,-gf.x); // âˆ‡ð‘“(ð‘)
  vec2 pp=-f*gf/dot(gf,gf); // ð‘â€²
  vec2 d0p=b0-pp; // ð‘â€² to origin
  float ap=det(d0p,d20), bp=2.0*det(d10,d0p); // ð›¼,ð›½(ð‘â€²)
  // (note that 2*ap+bp+dp=2*a+b+d=4*area(b0,b1,b2))
  float t=clamp((ap+bp)/(2.0*a+b+d), 0.0 ,1.0); // ð‘¡Ì…
  return mix(mix(b0,b1,t),mix(b1,b2,t),t); // ð‘£ð‘–= ð‘(ð‘¡Ì…)

}

float sdfBezier(vec2 p, vec2 b0, vec2 b1, vec2 b2)
{
    const vec2 dist = get_distance_vector(b0-p, b1-p, b2-p);
    return sign(dist.y) * length(dist);
}

// // Test if point p crosses line (a, b), returns sign of result
// float testCross(vec2 a, vec2 b, vec2 p) {
//     return sign((b.y-a.y) * (p.x-a.x) - (b.x-a.x) * (p.y-a.y));
// }

// // Determine which side we're on (using barycentric parameterization)
// float signBezier(vec2 A, vec2 B, vec2 C, vec2 p)
// {
//     vec2 a = C - A, b = B - A, c = p - A;
//     vec2 bary = vec2(c.x*b.y-b.x*c.y,a.x*c.y-c.x*a.y) / (a.x*b.y-b.x*a.y);
//     vec2 d = vec2(bary.y * 0.5, 0.0) + 1.0 - bary.x - bary.y;
//     return mix(sign(d.x * d.x - d.y), mix(-1.0, 1.0,
//         step(testCross(A, B, p) * testCross(B, C, p), 0.0)),
//         step((d.x - d.y), 0.0)) * testCross(A, C, B);
// }

// // Solve cubic equation for roots
// vec3 solveCubic(float a, float b, float c)
// {
//     float p = b - a*a / 3.0, p3 = p*p*p;
//     float q = a * (2.0*a*a - 9.0*b) / 27.0 + c;
//     float d = q*q + 4.0*p3 / 27.0;
//     float offset = -a / 3.0;
//     if(d >= 0.0) {
//         float z = sqrt(d);
//         vec2 x = (vec2(z, -z) - q) / 2.0;
//         vec2 uv = sign(x)*pow(abs(x), vec2(1.0/3.0));
//         return vec3(offset + uv.x + uv.y);
//     }
//     float v = acos(-sqrt(-27.0 / p3) * q / 2.0) / 3.0;
//     float m = cos(v), n = sin(v)*1.732050808;
//     return vec3(m + m, -n - m, n - m) * sqrt(-p / 3.0) + offset;
// }

// // Find the signed distance from a point to a bezier curve
// float sdfBezier(vec2 p, vec2 A, vec2 B, vec2 C)
// {
//     B = mix(B + vec2(1e-4), B, abs(sign(B * 2.0 - A - C)));
//     vec2 a = B - A, b = A - B * 2.0 + C, c = a * 2.0, d = A - p;
//     vec3 k = vec3(3.*dot(a,b),2.*dot(a,a)+dot(d,b),dot(d,a)) / dot(b,b);
//     vec3 t = clamp(solveCubic(k.x, k.y, k.z), 0.0, 1.0);
//     vec2 pos = A + (c + b*t.x)*t.x;
//     float dis = length(pos - p);
//     pos = A + (c + b*t.y)*t.y;
//     dis = min(dis, length(pos - p));
//     pos = A + (c + b*t.z)*t.z;
//     dis = min(dis, length(pos - p));
//     return dis * signBezier(A, B, C, p);
// }



// // Test if point p crosses line (a, b), returns sign of result
// float testCross(vec2 a, vec2 b, vec2 p) {
//     return sign((b.y-a.y) * (p.x-a.x) - (b.x-a.x) * (p.y-a.y));
// }

// // Determine which side we're on (using barycentric parameterization)
// float signBezier(vec2 A, vec2 B, vec2 C, vec2 p)
// {
//     vec2 a = C - A, b = B - A, c = p - A;
//     vec2 bary = vec2(c.x*b.y-b.x*c.y,a.x*c.y-c.x*a.y) / (a.x*b.y-b.x*a.y);
//     vec2 d = vec2(bary.y * 0.5, 0.0) + 1.0 - bary.x - bary.y;
//     return mix(sign(d.x * d.x - d.y), mix(-1.0, 1.0,
//         step(testCross(A, B, p) * testCross(B, C, p), 0.0)),
//         step((d.x - d.y), 0.0)) * testCross(A, C, B);
// }

// // Solve cubic equation for roots
// vec3 solveCubic(float a, float b, float c)
// {
//     float p = b - a*a / 3.0, p3 = p*p*p;
//     float q = a * (2.0*a*a - 9.0*b) / 27.0 + c;
//     float d = q*q + 4.0*p3 / 27.0;
//     float offset = -a / 3.0;
//     if(d >= 0.0) {
//         float z = sqrt(d);
//         vec2 x = (vec2(z, -z) - q) / 2.0;
//         vec2 uv = sign(x)*pow(abs(x), vec2(1.0/3.0));
//         return vec3(offset + uv.x + uv.y);
//     }
//     float v = acos(-sqrt(-27.0 / p3) * q / 2.0) / 3.0;
//     float m = cos(v), n = sin(v)*1.732050808;
//     return vec3(m + m, -n - m, n - m) * sqrt(-p / 3.0) + offset;
// }

// // Find the signed distance from a point to a bezier curve
// float sdfBezier(vec2 p, vec2 A, vec2 B, vec2 C)
// {
//     B = mix(B + vec2(1e-4), B, abs(sign(B * 2.0 - A - C)));
//     vec2 a = B - A, b = A - B * 2.0 + C, c = a * 2.0, d = A - p;
//     vec3 k = vec3(3.*dot(a,b),2.*dot(a,a)+dot(d,b),dot(d,a)) / dot(b,b);
//     vec3 t = clamp(solveCubic(k.x, k.y, k.z), 0.0, 1.0);
//     vec2 pos = A + (c + b*t.x)*t.x;
//     float dis = length(pos - p);
//     pos = A + (c + b*t.y)*t.y;
//     dis = min(dis, length(pos - p));
//     pos = A + (c + b*t.z)*t.z;
//     dis = min(dis, length(pos - p));
//     return dis * signBezier(A, B, C, p);  //No need for this sign
// }

void main(void)
{
	const float d = sdfBezier(gl_FragCoord.xy, fragLeft, fragControl, fragRight);
    const float totalThickness = fragThickness + fragEdgeSoftness;
    const float minIntensity = 0.0; //0.1 * float(d < -totalThickness);
    const float strokeIntensity = minIntensity + (1.0 - minIntensity) * smoothstep(totalThickness, fragThickness, abs(d));
    outColor = fragColor * vec4(1.0, 1.0, 1.0, strokeIntensity);
}
