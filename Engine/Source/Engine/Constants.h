#pragma once

#define ENGINE_NAME "Octave"
#define MAX_PATH_SIZE 260

#define DEFAULT_GAME_NAME "Game"
#define DEFAULT_WINDOW_WIDTH 1280
#define DEFAULT_WINDOW_HEIGHT 720

#define DEFAULT_TEXTURE_DIRECTORY_NAME "Engine/Assets/Textures/"
#define DEFAULT_DIFFUSE_TEXTURE_NAME "T_White"
#define DEFAULT_SPECULAR_TEXTURE_NAME "T_Black"
#define DEFAULT_NORMAL_TEXTURE_NAME "T_DefaultNormal"
#define DEFAULT_REFLECTIVE_TEXTURE_NAME "T_Black"
#define DEFAULT_EMISSIVE_TEXTURE_NAME "T_Black"
#define DEFAULT_ORM_TEXTURE_NAME "T_DefaultORM"

#define DEFAULT_TEXTURE_SIZE 4
#define MATERIAL_MAX_TEXTURES 4
#define MAX_LIGHTS_PER_FRAME 32
#define MAX_LIGHTS_PER_DRAW 8
#define MAX_BONE_INFLUENCES 4
#define MAX_BONES 128
#define MAX_COLLISION_SHAPES 16
#define MAX_UV_MAPS 2
#define LIGHT_BAKE_SCALE 4.0f

#define DEFAULT_AMBIENT_LIGHT_COLOR glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)
#define DEFAULT_SHADOW_COLOR glm::vec4(0.0f, 0.0f, 0.0f, 0.8f)

#define SHADOW_MAP_RESOLUTION 2048
#define SHADOW_RANGE 50.0f
#define SHADOW_RANGE_Z 400.0f

#define LOGGING_ENABLED 1
#define CONSOLE_ENABLED 1
#define DEBUG_DRAW_ENABLED 1

#define INVALID_TYPE_ID 0
#define INVALID_NET_ID 0

#define INVALID_HOST_ID 0
#define SERVER_HOST_ID 1

#define MAX_NET_FUNC_PARAMS 8

#define OCT_SESSION_NAME_LEN 31
#define OCT_MAX_SESSION_LIST_SIZE 32

#define EMBED_ALL_ASSETS 1

#define LARGE_BOUNDS 10000.0f

#if EDITOR
#define ASSET_LIVE_REF_TRACKING 1
#else
#define ASSET_LIVE_REF_TRACKING 0
#endif

#define LUA_ENABLED 1
#define LUA_TYPE_CHECK 1
#define LUA_SAFE_NODE 0

// Flag to enable conversion of old Level + Blueprint assets into new Scene format.
// Delete this after converting Yami to nodes.
#define OCT_SCENE_CONVERSION 1
