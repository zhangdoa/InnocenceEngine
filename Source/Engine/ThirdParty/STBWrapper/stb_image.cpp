#include "../../Common/Memory.h"
#define STBI_MALLOC(sz)           Inno::Memory::Allocate(sz)
#define STBI_REALLOC(p,newsz)     Inno::Memory::Reallocate(p,newsz)
#define STBI_FREE(p)              Inno::Memory::Deallocate(p)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"