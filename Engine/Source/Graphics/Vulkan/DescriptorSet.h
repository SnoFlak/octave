#pragma once

#if API_VULKAN

#include "Graphics/GraphicsConstants.h"
#include "Graphics/Vulkan/VulkanConstants.h"

#include "Graphics/Vulkan/Image.h"

#include <stdint.h>
#include <vulkan/vulkan.h>

class Buffer;
class UniformBuffer;

enum class DescriptorType
{
    Uniform,
    Image,
    ImageArray,
    StorageBuffer,
    StorageImage,

    Count
};

struct DescriptorBinding
{
    DescriptorType mType = DescriptorType::Count;
    void* mObject = nullptr;
    std::vector<Image*> mImageArray;
};

class DescriptorSet
{
public:

    DescriptorSet(VkDescriptorSetLayout layout);

    // Updates the current frame's descriptor.
    void UpdateImageDescriptor(int32_t binding, Image* image);
    void UpdateImageArrayDescriptor(int32_t binding, const std::vector<Image*>& imageArray);
    void UpdateUniformDescriptor(int32_t binding, UniformBuffer* uniformBuffer);
    void UpdateStorageBufferDescriptor(int32_t binding, Buffer* storageBuffer);
    void UpdateStorageImageDescriptor(int32_t binding, Image* storageImage);

    void Bind(VkCommandBuffer cb, uint32_t index, VkPipelineLayout pipelineLayout, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

    VkDescriptorSet Get();
    VkDescriptorSet Get(uint32_t frameIndex);

private:

    friend class DestroyQueue;
    ~DescriptorSet();

    void MarkDirty();
    void RefreshBindings(uint32_t frameIndex);

    DescriptorBinding mBindings[MAX_DESCRIPTORS_PER_SET] = { };

    VkDescriptorSet mDescriptorSets[MAX_FRAMES] = { };
    bool mDirty[MAX_FRAMES] = { };
};

#endif // API_VULKAN