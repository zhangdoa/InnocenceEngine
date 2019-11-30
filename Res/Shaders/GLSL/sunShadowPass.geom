// shadertype=glsl
#include "common/common.glsl"
layout(triangles) in;
layout(triangle_strip, max_vertices = NR_CSM_SPLITS * 3) out;

void main()
{
	for (int CSMSplitIndex = 0; CSMSplitIndex < NR_CSM_SPLITS; ++CSMSplitIndex)
	{
		gl_Layer = CSMSplitIndex;
		for (int i = 0; i < 3; ++i)
		{
			gl_Position = CSMCBuffer.data[CSMSplitIndex].p * CSMCBuffer.data[CSMSplitIndex].v * gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}
}