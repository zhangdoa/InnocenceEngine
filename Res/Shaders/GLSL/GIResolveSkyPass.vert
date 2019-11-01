#version 450

layout(location = 0) in vec4 input_position;
layout(location = 1) in vec2 input_texcoord;
layout(location = 2) in vec2 input_pada;
layout(location = 3) in vec4 input_normal;
layout(location = 4) in vec4 input_padb;

void main()
{
    gl_Position = input_position;
}

