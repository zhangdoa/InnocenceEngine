#include "GLMesh.h"
#include "GLTexture.h"
#include "GLShader.h"
#include "GLShaderProgram.h"
#include "GLFrameBuffer.h"

#define USE_OPENGL
#ifdef USE_OPENGL
#define MESH_CLASS GLMesh
#define TEXTURE_CLASS GLTexture
#define SHADER_CLASS GLShader
#define SHADER_PROGRAM_CLASS GLShaderProgram
#define FRAMEBUFFER_CLASS GLFrameBuffer
#endif