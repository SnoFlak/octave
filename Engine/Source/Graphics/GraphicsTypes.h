#pragma once

#include <stdint.h>
#include "Graphics/GraphicsConstants.h"

#if API_VULKAN
#include "Graphics/Vulkan/Buffer.h"
#include "Graphics/Vulkan/Image.h"
#include "Graphics/Vulkan/MultiBuffer.h"
#include "Graphics/Vulkan/DescriptorSet.h"
#endif

#if API_VULKAN
extern class VulkanContext* gVulkanContext;
#endif

struct GraphicsState
{
#if API_VULKAN
    class VulkanContext* mVulkanContext = gVulkanContext;
#endif

    float mResolutionScale = 1.0f;
};

enum class PixelFormat
{
    LA4,
    RGB565,
    RGBA8,
    CMPR,
    RGBA5551,

    R8,
    R32U,
    R32F,
    RGBA16F,

    Depth24Stencil8,
	Depth32FStencil8,
    Depth16,
    Depth32F,

    Count
};

enum class PipelineId
{
    // Graphics
    Shadow,
    Opaque,
    Translucent,
    Additive,
    DepthlessOpaque,
    DepthlessTranslucent,
    DepthlessAdditive,
    CullFrontOpaque,
    CullFrontTranslucent,
    CullFrontAdditive,
    CullNoneOpaque,
    CullNoneTranslucent,
    CullNoneAdditive,
    ShadowMeshBack,
    ShadowMeshFront,
    ShadowMeshClear,
    Selected,
    Wireframe,
    Collision,
    BakedLightVis,
    Line,
    PostProcess,
    NullPostProcess,
    Quad,
    Text,
    Poly,

    // Compute
    PathTrace,
    LightBakeDirect,
    LightBakeIndirect,
    LightBakeAverage,
    LightBakeDiffuse,

    // Editor
    HitCheck,

    Count
};

enum class RenderPassId
{
    Shadows,
    Forward,
    PostProcess,
    Ui,
    Clear,

    HitCheck,

    Count
};

enum class FilterType
{
    Nearest,
    Linear,

    Count
};

enum class WrapMode
{
    Clamp,
    Repeat,
    Mirror,

    Count
};

#if API_VULKAN
typedef uint32_t IndexType;
#else
typedef uint16_t IndexType;
#endif

struct TextureResource
{
#if API_VULKAN
    Image* mImage = nullptr;
#endif
};

struct MaterialResource
{
#if API_VULKAN
    DescriptorSet* mDescriptorSet = nullptr;
    UniformBuffer* mUniformBuffer = nullptr;
#endif
};

struct StaticMeshResource
{
#if API_VULKAN
    Buffer* mVertexBuffer = nullptr;
    Buffer* mIndexBuffer = nullptr;
#endif
};

struct SkeletalMeshResource
{
#if API_VULKAN
    Buffer* mVertexBuffer = nullptr;
    Buffer* mIndexBuffer = nullptr;
#endif
};

struct StaticMeshCompResource
{
#if API_VULKAN
    DescriptorSet* mDescriptorSet = nullptr;
    UniformBuffer* mUniformBuffer = nullptr;
    Buffer* mColorVertexBuffer = nullptr;
#endif
};

struct SkeletalMeshCompResource
{
#if API_VULKAN
    DescriptorSet* mDescriptorSet = nullptr;
    UniformBuffer* mUniformBuffer = nullptr;
    MultiBuffer* mVertexBuffer = nullptr;
#endif
};

struct TextMeshCompResource
{
#if API_VULKAN
    DescriptorSet* mDescriptorSet = nullptr;
    UniformBuffer* mUniformBuffer = nullptr;
    Buffer* mVertexBuffer = nullptr;
#endif
};

struct QuadResource
{
#if API_VULKAN
    Buffer* mVertexBuffer = nullptr;
    UniformBuffer* mUniformBuffer = nullptr;
    DescriptorSet* mDescriptorSet = nullptr;
#endif
};

struct PolyResource
{
#if API_VULKAN
    Buffer* mVertexBuffer = nullptr;
    UniformBuffer* mUniformBuffer = nullptr;
    DescriptorSet* mDescriptorSet = nullptr;
    uint32_t mNumVerts = 0;
#endif
};

struct TextResource
{
#if API_VULKAN
    Buffer* mVertexBuffer = nullptr;
    UniformBuffer* mUniformBuffer = nullptr;
    DescriptorSet* mDescriptorSet = nullptr;
    uint32_t mNumBufferCharsAllocated = 0;
#endif
};

struct ParticleCompResource
{
#if API_VULKAN
    DescriptorSet* mDescriptorSet = nullptr;
    UniformBuffer* mUniformBuffer = nullptr;
    MultiBuffer* mVertexBuffer = nullptr;
    MultiBuffer* mIndexBuffer = nullptr;
    uint32_t mNumVerticesAllocated = 0;
#endif
};
