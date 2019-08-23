#version 450
layout(triangles) in;
layout(max_vertices = 18, triangle_strip) out;

struct GeometryInputType
{
    vec4 posWS;
};

layout(binding = 10, std140) uniform GICameraCBuffer
{
    layout(row_major) mat4 GICamera_p;
    layout(row_major) mat4 GICamera_r[6];
    layout(row_major) mat4 GICamera_t;
} _71;

void main()
{
    GeometryInputType param[3] = GeometryInputType[](GeometryInputType(gl_in[0].gl_Position), GeometryInputType(gl_in[1].gl_Position), GeometryInputType(gl_in[2].gl_Position));
    int _285;
    _285 = 0;
    for (; _285 < 6; EndPrimitive(), _285++)
    {
        uint _205 = uint(_285);
        for (int _286 = 0; _286 < 3; )
        {
            vec4 _239 = _71.GICamera_p * (_71.GICamera_r[_285] * (_71.GICamera_t * vec4(param[_286].posWS.xyz * (-1.0), 1.0)));
            vec4 _283 = _239;
            _283.z = _239.w;
            gl_Position = _283;
            gl_Layer = int(_205);
            EmitVertex();
            _286++;
            continue;
        }
    }
}

