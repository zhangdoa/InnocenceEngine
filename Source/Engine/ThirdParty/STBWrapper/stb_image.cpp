#include "../../Core/InnoMemory.h"
#define STBI_MALLOC(sz)           Inno::InnoMemory::Allocate(sz)
#define STBI_REALLOC(p,newsz)     Inno::InnoMemory::Reallocate(p,newsz)
#define STBI_FREE(p)              Inno::InnoMemory::Deallocate(p)
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"