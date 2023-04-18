#version 450

layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 40) out; // 4*n <= 60

// Inputs
layout(location = 0) in vec2 vpos[];
layout(location = 1) in vec3 vcol[];

// Outputs
layout(location = 0) out vec2 fragA0; // Begin
layout(location = 1) out vec2 fragA1; // Coefficient 1
layout(location = 2) out vec2 fragA2; // Coefficient 2
layout(location = 3) out vec2 fragA3; // End
layout(location = 4) out vec3 fragColor; // Color
layout(location = 5) out vec2 fragPos; // Position
layout(location = 6) out vec2 fragRange; // T range of chunk

// Compute cubic bezier
float cubicBezier(float t)
{
    float t2 = t * t;
    float t3 = t2 * t;
    return fragA0 + (fragA1 * t) + (fragA2 * t2) + (fragA3 * t3);
}

void main(void)
{
    int i, j, n = 10, m = 10;             // n,m
    float t, dd, d0, d1, dt = 1.0 / 10.0; // 1/n
    float tt, dtt = 1.0 / 100.0;          // 1/(n*m)
    vec2 p0, p1, p2, p3, u, v;
    vec2 q0, q1, q2, q3;
    p0 = gl_in[0].gl_Position.xy;
    p1 = gl_in[1].gl_Position.xy;
    p2 = gl_in[2].gl_Position.xy;
    p3 = gl_in[3].gl_Position.xy;
    // compute BEZIER coefficients
    fragA0.x = (p0.x);
    fragA1.x = (3.0 * p1.x) - (3.0 * p0.x);
    fragA2.x = (3.0 * p2.x) - (6.0 * p1.x) + (3.0 * p0.x);
    fragA3.x = (p3.x) - (3.0 * p2.x) + (3.0 * p1.x) - (p0.x);
    fragA0.y = (p0.y);
    fragA1.y = (3.0 * p1.y) - (3.0 * p0.y);
    fragA2.y = (3.0 * p2.y) - (6.0 * p1.y) + (3.0 * p0.y);
    fragA3.y = (p3.y) - (3.0 * p2.y) + (3.0 * p1.y) - (p0.y);
    q2 = vec2(0.0, 0.0);
    q3 = vec2(0.0, 0.0);
    // sample curve by chunks
    for (p1 = cubicBezier(0.0), i = 0, t = dt; i < n; i++, t += dt) {
        // sample point
        p0 = p1;
        p1 = cubicBezier(t);
        q0 = q2;
        q1 = q3;
        // compute ~OBB enlarged by D
        u = normalize(p1 - p0);
        v = vec2(u.y, -u.x);
        // resample chunk to compute enlargement
        for (d0 = 0.0, d1 = 0.0, tt = t - dtt, j = 2; j < m; j++, tt -= dtt) {
            dd = dot(cubicBezier(tt) - p0, v);
            d0 = max(-dd, d0);
            d1 = max(+dd, d1);
        }
        d0 += d;
        d1 += d;
        u *= d;
        d0 *= 1.25;
        d1 *= 1.25; // just to be sure
        // enlarge radial
        q2 = p1 + (v * d1);
        q3 = p1 - (v * d0);
        // enlarge axial
        if (i == 0) {
            q0 = p0 + (v * d1) - u;
            q1 = p0 - (v * d0) - u;
        }
        if (i == n - 1) {
            q2 += u;
            q3 += u;
        }
        // pass it as QUAD
        fcol = vcol[0];
        trange = vec2(t - dt, t);
        fpos = q0;
        gl_Position = vec4(q0, 0.0, 1.0);
        EmitVertex();
        fpos = q1;
        gl_Position = vec4(q1, 0.0, 1.0);
        EmitVertex();
        fpos = q2;
        gl_Position = vec4(q2, 0.0, 1.0);
        EmitVertex();
        fpos = q3;
        gl_Position = vec4(q3, 0.0, 1.0);
        EmitVertex();
        EndPrimitive();
    }
}