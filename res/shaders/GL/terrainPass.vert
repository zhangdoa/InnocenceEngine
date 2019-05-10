// shadertype=glsl
#version 450

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec3 in_Normal;

void main()
{
	//int l_localIndex = gl_VertexID % 4;

	//if (l_localIndex == 0)
	//{
	//	thefrag_TexCoord = vec2(1.0f, 1.0f);
	//}
	//else if (l_localIndex == 1)
	//{
	//	thefrag_TexCoord = vec2(0.0f, 1.0f);
	//}
	//else if (l_localIndex == 2)
	//{
	//	thefrag_TexCoord = vec2(1.0f, 0.0f);
	//}
	//else if (l_localIndex == 3)
	//{
	//	thefrag_TexCoord = vec2(0.0f, 0.0f);
	//}

	gl_Position = vec4(in_Position, 1.0f);
}