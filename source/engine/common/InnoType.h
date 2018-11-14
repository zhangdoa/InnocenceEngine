#pragma once
#include "../common/stdafx.h"
#include "../common/config.h"

#define INNO_INTERFACE class
#define INNO_IMPLEMENT public
#define INNO_PRIVATE_SCOPE namespace

enum class objectStatus
{
	STANDBY,
	ALIVE,
	SHUTDOWN,
};

using EntityID = unsigned long;

struct timeData
{
	int year;
	unsigned month;
	unsigned day;
	unsigned hour;
	unsigned minute;
	long long second;
	long long millisecond;
};

using Index = unsigned int;

enum class visiblilityType { INVISIBLE, BILLBOARD, STATIC_MESH, SKYBOX, GLASSWARE, EMISSIVE };
// mesh custom types
enum class meshType { NORMAL, BOUNDING_BOX };
enum class meshShapeType { LINE, QUAD, CUBE, SPHERE, CUSTOM };
enum class meshDrawMethod { TRIANGLE, TRIANGLE_STRIP };
// texture custom types
enum class textureType { INVISIBLE, NORMAL, ALBEDO, METALLIC, ROUGHNESS, AMBIENT_OCCLUSION, ENVIRONMENT_CAPTURE, ENVIRONMENT_CONVOLUTION, ENVIRONMENT_PREFILTER, EQUIRETANGULAR, RENDER_BUFFER_SAMPLER, SHADOWMAP };
enum class textureColorComponentsFormat { RED, RG, RGB, RGBA, R8, RG8, RGB8, RGBA8, R16, RG16, RGB16, RGBA16, R16F, RG16F, RGB16F, RGBA16F, R32F, RG32F, RGB32F, RGBA32F, SRGB, SRGBA, SRGB8, SRGBA8, DEPTH_COMPONENT };
enum class texturePixelDataFormat { RED, RG, RGB, RGBA, DEPTH_COMPONENT };
enum class texturePixelDataType { UNSIGNED_BYTE, BYTE, UNSIGNED_SHORT, SHORT, UNSIGNED_INT, INT, FLOAT };
enum class textureWrapMethod { CLAMP_TO_EDGE, REPEAT, CLAMP_TO_BORDER };
enum class textureFilterMethod { NEAREST, LINEAR, LINEAR_MIPMAP_LINEAR };
enum class textureAssignType { ADD, OVERWRITE };
// shader custom types
enum class shaderType { VERTEX, GEOMETRY, FRAGMENT };
using shaderFilePath = std::string;
using shaderCodeContent = std::string;
using shaderData = std::tuple<shaderType, shaderFilePath, shaderCodeContent>;
enum class shaderDrawPolygonType { POINT, LINE, FILL };
enum class shaderDrawTextureType { FULL, POSITION, NORMAL, ALBEDO, MRA };
using shaderDrawPair = std::pair<shaderDrawPolygonType, shaderDrawTextureType>;
// frame and render buffer custom types
enum class frameBufferType { FORWARD, DEFER, SHADOW_PASS, ENVIRONMENT_PASS, PINGPONG };
enum class renderBufferType { NONE, DEPTH, STENCIL, DEPTH_AND_STENCIL };

#ifdef INNO_PLATFORM_MACOS
struct EnumClassHash
{
	template <typename T>
	std::size_t operator()(T t) const
	{
		return static_cast<std::size_t>(t);
	}
};
using texturePair = std::pair<textureType, EntityID>;
using textureMap = std::unordered_map<textureType, EntityID, EnumClassHash>;
using modelPair = std::pair<EntityID, textureMap>;
using modelMap = std::unordered_map<EntityID, textureMap, EnumClassHash>;
using textureFileNamePair = std::pair<textureType, std::string>;
using textureFileNameMap = std::unordered_map<textureType, std::string, EnumClassHash>;
#else
using texturePair = std::pair<textureType, EntityID>;
using textureMap = std::unordered_map<textureType, EntityID>;
using modelPair = std::pair<EntityID, textureMap>;
using modelMap = std::unordered_map<EntityID, textureMap>;
using textureFileNamePair = std::pair<textureType, std::string>;
using textureFileNameMap = std::unordered_map<textureType, std::string>;
#endif

struct meshColor
{
	float albedo_r = 1.0f;
	float albedo_g = 1.0f;
	float albedo_b = 1.0f;
	float metallic = 1.0f;
	float roughness = 1.0f;
	float ao = 1.0f;
	float additionalData1 = 1.0f;
	float additionalData2 = 1.0f;
};

enum class buttonStatus { RELEASED, PRESSED };
using buttonStatusMap = std::unordered_map<int, buttonStatus>;

struct button
{
	int m_code = 0;
	buttonStatus m_status = buttonStatus::RELEASED;

	bool operator==(const button &other) const
	{
		return (m_code == other.m_code && m_status == other.m_status);
	}
};

struct buttonHasher
{
	std::size_t operator()(const button& k) const
	{
		return std::hash<int>()(k.m_code) ^ (std::hash<buttonStatus>()(k.m_status) << 1);
	}
};

using buttonStatusCallbackMap = std::unordered_map<button, std::vector<std::function<void()>*>, buttonHasher>;
using mouseMovementCallbackMap = std::unordered_map<int, std::vector<std::function<void(float)>*>>;

#define INNO_KEY_SPACE              32
#define INNO_KEY_APOSTROPHE         39  /* ' */
#define INNO_KEY_COMMA              44  /* , */
#define INNO_KEY_MINUS              45  /* - */
#define INNO_KEY_PERIOD             46  /* . */
#define INNO_KEY_SLASH              47  /* / */
#define INNO_KEY_0                  48
#define INNO_KEY_1                  49
#define INNO_KEY_2                  50
#define INNO_KEY_3                  51
#define INNO_KEY_4                  52
#define INNO_KEY_5                  53
#define INNO_KEY_6                  54
#define INNO_KEY_7                  55
#define INNO_KEY_8                  56
#define INNO_KEY_9                  57
#define INNO_KEY_SEMICOLON          59  /* ; */
#define INNO_KEY_EQUAL              61  /* = */
#define INNO_KEY_A                  65
#define INNO_KEY_B                  66
#define INNO_KEY_C                  67
#define INNO_KEY_D                  68
#define INNO_KEY_E                  69
#define INNO_KEY_F                  70
#define INNO_KEY_G                  71
#define INNO_KEY_H                  72
#define INNO_KEY_I                  73
#define INNO_KEY_J                  74
#define INNO_KEY_K                  75
#define INNO_KEY_L                  76
#define INNO_KEY_M                  77
#define INNO_KEY_N                  78
#define INNO_KEY_O                  79
#define INNO_KEY_P                  80
#define INNO_KEY_Q                  81
#define INNO_KEY_R                  82
#define INNO_KEY_S                  83
#define INNO_KEY_T                  84
#define INNO_KEY_U                  85
#define INNO_KEY_V                  86
#define INNO_KEY_W                  87
#define INNO_KEY_X                  88
#define INNO_KEY_Y                  89
#define INNO_KEY_Z                  90
#define INNO_KEY_LEFT_BRACKET       91  /* [ */
#define INNO_KEY_BACKSLASH          92  /* \ */
#define INNO_KEY_RIGHT_BRACKET      93  /* ] */
#define INNO_KEY_GRAVE_ACCENT       96  /* ` */
#define INNO_KEY_WORLD_1            161 /* non-US #1 */
#define INNO_KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define INNO_KEY_ESCAPE             256
#define INNO_KEY_ENTER              257
#define INNO_KEY_TAB                258
#define INNO_KEY_BACKSPACE          259
#define INNO_KEY_INSERT             260
#define INNO_KEY_DELETE             261
#define INNO_KEY_RIGHT              262
#define INNO_KEY_LEFT               263
#define INNO_KEY_DOWN               264
#define INNO_KEY_UP                 265
#define INNO_KEY_PAGE_UP            266
#define INNO_KEY_PAGE_DOWN          267
#define INNO_KEY_HOME               268
#define INNO_KEY_END                269
#define INNO_KEY_CAPS_LOCK          280
#define INNO_KEY_SCROLL_LOCK        281
#define INNO_KEY_NUM_LOCK           282
#define INNO_KEY_PRINT_SCREEN       283
#define INNO_KEY_PAUSE              284
#define INNO_KEY_F1                 290
#define INNO_KEY_F2                 291
#define INNO_KEY_F3                 292
#define INNO_KEY_F4                 293
#define INNO_KEY_F5                 294
#define INNO_KEY_F6                 295
#define INNO_KEY_F7                 296
#define INNO_KEY_F8                 297
#define INNO_KEY_F9                 298
#define INNO_KEY_F10                299
#define INNO_KEY_F11                300
#define INNO_KEY_F12                301
#define INNO_KEY_F13                302
#define INNO_KEY_F14                303
#define INNO_KEY_F15                304
#define INNO_KEY_F16                305
#define INNO_KEY_F17                306
#define INNO_KEY_F18                307
#define INNO_KEY_F19                308
#define INNO_KEY_F20                309
#define INNO_KEY_F21                310
#define INNO_KEY_F22                311
#define INNO_KEY_F23                312
#define INNO_KEY_F24                313
#define INNO_KEY_F25                314
#define INNO_KEY_KP_0               320
#define INNO_KEY_KP_1               321
#define INNO_KEY_KP_2               322
#define INNO_KEY_KP_3               323
#define INNO_KEY_KP_4               324
#define INNO_KEY_KP_5               325
#define INNO_KEY_KP_6               326
#define INNO_KEY_KP_7               327
#define INNO_KEY_KP_8               328
#define INNO_KEY_KP_9               329
#define INNO_KEY_KP_DECIMAL         330
#define INNO_KEY_KP_DIVIDE          331
#define INNO_KEY_KP_MULTIPLY        332
#define INNO_KEY_KP_SUBTRACT        333
#define INNO_KEY_KP_ADD             334
#define INNO_KEY_KP_ENTER           335
#define INNO_KEY_KP_EQUAL           336
#define INNO_KEY_LEFT_SHIFT         340
#define INNO_KEY_LEFT_CONTROL       341
#define INNO_KEY_LEFT_ALT           342
#define INNO_KEY_LEFT_SUPER         343
#define INNO_KEY_RIGHT_SHIFT        344
#define INNO_KEY_RIGHT_CONTROL      345
#define INNO_KEY_RIGHT_ALT          346
#define INNO_KEY_RIGHT_SUPER        347
#define INNO_KEY_MENU               348

#define INNO_MOUSE_BUTTON_1         0
#define INNO_MOUSE_BUTTON_2         1
#define INNO_MOUSE_BUTTON_3         2
#define INNO_MOUSE_BUTTON_4         3
#define INNO_MOUSE_BUTTON_5         4
#define INNO_MOUSE_BUTTON_6         5
#define INNO_MOUSE_BUTTON_7         6
#define INNO_MOUSE_BUTTON_8         7
#define INNO_MOUSE_BUTTON_LAST      INNO_MOUSE_BUTTON_8
#define INNO_MOUSE_BUTTON_LEFT      INNO_MOUSE_BUTTON_1
#define INNO_MOUSE_BUTTON_RIGHT     INNO_MOUSE_BUTTON_2
#define INNO_MOUSE_BUTTON_MIDDLE    INNO_MOUSE_BUTTON_3
