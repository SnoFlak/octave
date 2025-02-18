#pragma once

#if API_VULKAN

#include "Allocator.h"
#include "Vertex.h"

#include "Graphics/GraphicsTypes.h"
#include "Graphics/Vulkan/VulkanTypes.h"
#include "Graphics/Vulkan/DestroyQueue.h"

#include <vulkan/vulkan.h>

class World;
class DestroyQueue;
class Pipeline;

class Texture;
class Material;
class StaticMesh;
class SkeletalMesh;
class StaticMesh3D;
class SkeletalMesh3D;
class ShadowMesh3D;
class TextMesh3D;
class Particle3D;
class Node3D;
class Quad;
class Text;
class Poly;

struct Bounds;

VkFormat ConvertPixelFormat(PixelFormat pixelFormat);

void CreateBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    Allocation& bufferMemory);

void TransitionImageLayout(
    VkImage image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    int32_t mipLevels = 1,
    int32_t layerCount = 1,
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

void CopyBuffer(
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize size);

void CopyBufferToImage(
    VkBuffer buffer,
    VkImage image,
    uint32_t width,
    uint32_t height);

uint32_t GetFrameIndex();
DestroyQueue* GetDestroyQueue();
VkDevice GetVulkanDevice();
VkCommandBuffer GetCommandBuffer();

void DeviceWaitIdle();

VkCommandBuffer BeginCommandBuffer();
void EndCommandBuffer(VkCommandBuffer commandBuffer);
uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

uint32_t GetFormatPixelSize(VkFormat format);
uint32_t GetFormatBlockSize(VkFormat format);
bool IsFormatBlockCompressed(VkFormat format);
VkImageAspectFlags GetFormatImageAspect(VkFormat format);

std::vector<VkVertexInputBindingDescription> GetVertexBindingDescription(VertexType type);
std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions(VertexType type);

void SetDebugObjectName(VkObjectType objectType, uint64_t object, const char* name);
void BeginDebugLabel(const char* name, glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });
void EndDebugLabel();

void WriteGeometryUniformData(GeometryData& outData, World* world, Node3D* comp, const glm::mat4& transform);
void WriteMaterialUniformData(MaterialData& outData, Material* material);
void GatherGeometryLightUniformData(GeometryData& outData, Material* material, const Bounds& bounds, StaticMesh3D* staticMeshComp = nullptr);

#if _DEBUG
void FullPipelineBarrier();
#endif

// Texture
void CreateTextureResource(Texture* texture, uint8_t* pixels);
void DestroyTextureResource(Texture* texture);

// Material
void CreateMaterialResource(Material* material);
void DestroyMaterialResource(Material* material);
void BindMaterialResource(Material* material, Pipeline* pipeline);
void UpdateMaterialResource(Material* material);
Pipeline* GetMaterialPipeline(Material* material, VertexType vertType);

// StaticMesh
void CreateStaticMeshResource(StaticMesh* staticMesh, bool hasColor, uint32_t numVertices, void* vertices, uint32_t numIndices, IndexType* indices);
void DestroyStaticMeshResource(StaticMesh* staticMesh);
void BindStaticMeshResource(StaticMesh* staticMesh);

// SkeletalMesh
void CreateSkeletalMeshResource(SkeletalMesh* skeletalMesh, uint32_t numVertices, VertexSkinned* vertices, uint32_t numIndices, IndexType* indices);
void DestroySkeletalMeshResource(SkeletalMesh* skeletalMesh);
void BindSkeletalMeshResource(SkeletalMesh* skeletalMesh);
void BindSkeletalMeshResourceIndices(SkeletalMesh* skeletalMesh);

// StaticMeshComp
void CreateStaticMeshCompResource(StaticMesh3D* staticMeshComp);
void DestroyStaticMeshCompResource(StaticMesh3D* staticMeshComp);
void UpdateStaticMeshCompResource(StaticMesh3D* staticMeshComp);
void UpdateStaticMeshCompResourceColors(StaticMesh3D* staticMeshComp);
void DrawStaticMeshComp(StaticMesh3D* staticMeshComp, StaticMesh* meshOverride = nullptr);

// SkeletalMeshComp
void CreateSkeletalMeshCompResource(SkeletalMesh3D* skeletalMeshComp);
void DestroySkeletalMeshCompResource(SkeletalMesh3D* skeletalMeshComp);
void ReallocateSkeletalMeshCompVertexBuffer(SkeletalMesh3D* skeletalMeshComp, uint32_t numVertices);
void UpdateSkeletalMeshCompVertexBuffer(SkeletalMesh3D* skeletalMeshComp, const std::vector<Vertex>& skinnedVertices);
void UpdateSkeletalMeshCompUniformBuffer(SkeletalMesh3D* skeletalMeshComp);
void DrawSkeletalMeshComp(SkeletalMesh3D* skeletalMeshComp);
bool IsCpuSkinningRequired(SkeletalMesh3D* skeletalMeshComp);

// ShadowMeshComp
void DrawShadowMeshComp(ShadowMesh3D* shadowMeshComp);

// TextMeshComp
void CreateTextMeshCompResource(TextMesh3D* textMeshComp);
void DestroyTextMeshCompResource(TextMesh3D* textMeshComp);
void UpdateTextMeshCompVertexBuffer(TextMesh3D* textMeshComp, const std::vector<Vertex>& vertices);
void DrawTextMeshComp(TextMesh3D* textMeshComp);
void UpdateTextMeshCompUniformBuffer(TextMesh3D* textMeshComp);

// ParticleComp
void CreateParticleCompResource(Particle3D* particleComp);
void DestroyParticleCompResource(Particle3D* particleComp);
void UpdateParticleCompResource(Particle3D* particleComp);
void UpdateParticleCompVertexBuffer(Particle3D* particleComp, const std::vector<VertexParticle>& vertices);
void DrawParticleComp(Particle3D* particleComp);

// Quad
void CreateQuadResource(Quad* quad);
void DestroyQuadResource(Quad* quad);
void UpdateQuadResource(Quad* quad);
void DrawQuad(Quad* quad);

// Text
void CreateTextResource(Text* text);
void DestroyTextResource(Text* text);
void CreateTextResourceVertexBuffer(Text* text);
void DestroyTextResourceVertexBuffer(Text* text);
void UpdateTextResourceUniformData(Text* text);
void UpdateTextResourceVertexData(Text* text);
void DrawTextWidget(Text* text);

// Poly
void CreatePolyResource(Poly* poly);
void DestroyPolyResource(Poly* poly);
void UpdatePolyResourceUniformData(Poly* poly);
void UpdatePolyResourceVertexData(Poly* poly);
void DrawPoly(Poly* poly);

// Arbitrary mesh draw
void DrawStaticMesh(StaticMesh* mesh, Material* material, const glm::mat4& transform, glm::vec4 color, uint32_t hitCheckId = 0);

#endif