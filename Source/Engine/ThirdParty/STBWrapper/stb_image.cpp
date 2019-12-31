#include "../../Core/InnoMemory.h"
#define STBI_MALLOC(sz)           InnoMemory::Allocate(sz)
#define STBI_REALLOC(p,newsz)     InnoMemory::Reallocate(p,newsz)
#define STBI_FREE(p)              InnoMemory::Deallocate(p)
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"