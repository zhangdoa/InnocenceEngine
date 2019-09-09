#pragma once
#include "Config.h"
#include "InnoContainer.h"

#if defined INNO_PLATFORM_WIN
#define INNO_FORCEINLINE __forceinline
#else
#define INNO_FORCEINLINE __attribute__((always_inline)) inline
#endif

enum class ObjectStatus
{
	Created,
	Activated,
	Suspended,
	Terminated,
};

enum class ObjectSource
{
	Runtime,
	Asset,
};

enum class ObjectUsage
{
	Gameplay,
	Engine,
};

using EntityID = FixedSizeString<32>;
using ComponentName = FixedSizeString<128>;
using EntityName = FixedSizeString<128>;

enum class ComponentType
{
	TransformComponent,
	VisibleComponent,
	LightComponent,
	CameraComponent,
	PhysicsDataComponent,
	MeshDataComponent,
	MaterialDataComponent,
	TextureDataComponent,
	SkeletonDataComponent,
	AnimationDataComponent,
	RenderPassDataComponent,
	ShaderProgramComponent,
	SamplerDataComponent,
	GPUBufferDataComponent
};

struct TimeData
{
	int32_t Year;
	uint32_t Month;
	uint32_t Day;
	uint32_t Hour;
	uint32_t Minute;
	int64_t Second;
	int64_t Millisecond;
};

enum class FileExplorerIconType { OBJ, PNG, SHADER, UNKNOWN };
enum class WorldEditorIconType { DIRECTIONAL_LIGHT, POINT_LIGHT, SPHERE_LIGHT, UNKNOWN };

struct AssetMetadata
{
	std::string fullPath;
	std::string fileName;
	std::string extension;
	FileExplorerIconType iconType;
};

struct DirectoryMetadata
{
	uint32_t depth = 0;
	std::string directoryName = "root";
	DirectoryMetadata* parentDirectory = 0;
	std::vector<DirectoryMetadata> childrenDirectories;
	std::vector<AssetMetadata> childrenAssets;
};

enum class LogLevel { Verbose, Success, Warning, Error };

enum class IOMode { Text, Binary };

enum class VisiblilityType { Invisible, BillBoard, Opaque, Transparent, Emissive, Debug };

enum class RenderPassType { Shadow, GI, Opaque, Light, Transparent, Terrain, PostProcessing, Development };

// shader custom types
enum class ShaderStage
{
	Vertex,
	Hull,
	Domain,
	Geometry,
	Pixel,
	Compute
};

using ShaderFilePath = FixedSizeString<128>;

struct ShaderFilePaths
{
	ShaderFilePath m_VSPath = "";
	ShaderFilePath m_HSPath = "";
	ShaderFilePath m_DSPath = "";
	ShaderFilePath m_GSPath = "";
	ShaderFilePath m_PSPath = "";
	ShaderFilePath m_CSPath = "";
};

struct MeshCustomMaterial
{
	float AlbedoR = 1.0f;
	float AlbedoG = 1.0f;
	float AlbedoB = 1.0f;
	float Alpha = 1.0f;
	float Metallic = 0.0f;
	float Roughness = 1.0f;
	float AO = 1.0f;
	float Thickness = 1.0f;
};

struct ButtonState
{
	int32_t m_code = 0;
	bool m_isPressed = false;

	bool operator==(const ButtonState &other) const
	{
		return (
			m_code == other.m_code
			&& m_isPressed == other.m_isPressed
			);
	}
};

struct ButtonStateHasher
{
	std::size_t operator()(const ButtonState& k) const
	{
		return std::hash<int32_t>()(k.m_code) ^ (std::hash<bool>()(k.m_isPressed) << 1);
	}
};

enum class EventLifeTime { OneShot, Continuous };

struct ButtonEvent
{
	EventLifeTime m_eventLifeTime = EventLifeTime::OneShot;
	void* m_eventHandle = 0;

	bool operator==(const ButtonEvent &other) const
	{
		return (m_eventLifeTime == other.m_eventLifeTime
			&& m_eventHandle == other.m_eventHandle
			);
	}
};

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
