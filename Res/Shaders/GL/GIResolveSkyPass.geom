#version 450
layout(triangles) in;
layout(max_vertices = 18, triangle_strip) out;

struct GeometryInputType
{
    vec4 posWS;
};

layout(binding = 10, std140) uniform GICameraCBuffer
{
    layout(row_major) mat4 GI_cam_p;
    layout(row_major) mat4 GI_cam_r[6];
    layout(row_major) mat4 GI_cam_t;
} _71;

void main()
{
    GeometryInputType param[3] = GeometryInputType[](GeometryInputType(gl_in[0].gl_Position), GeometryInputType(gl_in[1].gl_Position), GeometryInputType(gl_in[2].gl_Position));
    int _286;
    _286 = 0;
    for (; _286 < 6; EndPrimitive(), _286++)
    {
        uint _206 = uint(_286);
        for (int _287 = 0; _287 < 3; )
        {
            vec4 _240 = _71.GI_cam_p * (_71.GI_cam_r[_286] * (_71.GI_cam_t * vec4(param[_287].posWS.xyz * (-1.0), 1.0)));
            vec4 _284 = _240;
            _284.z = _240.w;
            gl_Position = _284;
            gl_Layer = int(_206);
            EmitVertex();
            _287++;
            continue;
        }
    }
}

