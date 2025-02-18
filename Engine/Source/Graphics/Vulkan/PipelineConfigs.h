#pragma once

#if API_VULKAN

#include "Enums.h"
#include "Constants.h"

#include "Graphics/Vulkan/Pipeline.h"

#include "Assertion.h"

#define ENGINE_SHADER_DIR "Engine/Shaders/GLSL/bin/"

class ShadowPipeline : public Pipeline
{
public:

    ShadowPipeline()
    {
        mName = "Shadow Pipeline";
        mRasterizerDiscard = VK_FALSE;
        mFragmentShaderPath = ENGINE_SHADER_DIR "Shadow.frag";
        SetMeshVertexConfigs(
            ENGINE_SHADER_DIR "Shadow.vert",
            ENGINE_SHADER_DIR "Shadow.vert",
            ENGINE_SHADER_DIR "ShadowSkinned.vert",
            ENGINE_SHADER_DIR "Shadow.vert");

        mViewportWidth = SHADOW_MAP_RESOLUTION;
        mViewportHeight = SHADOW_MAP_RESOLUTION;

        mBlendAttachments.clear();
        mPipelineId = PipelineId::Shadow;
    }

    virtual void PopulateLayoutBindings() override
    {
        Pipeline::PopulateLayoutBindings();

        PushSet();
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

        PushSet();
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        // Add texture sampler descriptors for each texture slot
        for (int32_t i = 0; i < MATERIAL_MAX_TEXTURES; ++i)
        {
            AddLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        }
    }
};

class ForwardPipeline : public Pipeline
{
public:

    ForwardPipeline()
    {
        mName = "Forward Pipeline";
        SetMeshVertexConfigs(
            ENGINE_SHADER_DIR "Forward.vert",
            ENGINE_SHADER_DIR "ForwardColor.vert",
            ENGINE_SHADER_DIR "ForwardSkinned.vert",
            ENGINE_SHADER_DIR "ForwardParticle.vert");

        AddVertexConfig(VertexType::VertexInstanceColor, ENGINE_SHADER_DIR "ForwardColor.vert");
        AddVertexConfig(VertexType::VertexColorInstanceColor, ENGINE_SHADER_DIR "ForwardColor.vert");

        mFragmentShaderPath = ENGINE_SHADER_DIR "Forward.frag";
        mDepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        mDepthWriteEnabled = true;
        mCullMode = VK_CULL_MODE_BACK_BIT;

        mPipelineId = PipelineId::Opaque;
    }

    virtual void PopulateLayoutBindings() override
    {
        Pipeline::PopulateLayoutBindings();

        PushSet();
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

        PushSet();
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        // Add texture sampler descriptors for each texture slot
        for (int32_t i = 0; i < MATERIAL_MAX_TEXTURES; ++i)
        {
            AddLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        }
    }
};

class OpaquePipeline : public ForwardPipeline
{
public:
    OpaquePipeline()
    {
        mName = "Opaque Pipeline";
    }
};

class TranslucentPipeline : public ForwardPipeline
{
public:
    TranslucentPipeline()
    {
        mName = "Translucent Pipeline";
        mDepthWriteEnabled = false;
        mBlendAttachments.clear();
        AddMixBlendAttachmentState();
        // TODO: Use premultiplied alpha instead of mix/additive separately.

        mPipelineId = PipelineId::Translucent;
    }
};

class AdditivePipeline : public ForwardPipeline
{
public:
    AdditivePipeline()
    {
        mName = "Additive Pipeline";
        mDepthWriteEnabled = false;
        mBlendAttachments.clear();
        AddAdditiveBlendAttachmentState();

        mPipelineId = PipelineId::Additive;
    }
};

class ShadowMeshBackPipeline : public OpaquePipeline
{
public:

    ShadowMeshBackPipeline()
    {
        mName = "Shadow Back";
        mCullMode = VK_CULL_MODE_FRONT_BIT;
        mDepthCompareOp = VK_COMPARE_OP_GREATER;
        mDepthWriteEnabled = false;

        mBlendAttachments.clear();
        VkPipelineColorBlendAttachmentState blendAttachment = {};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_A_BIT;
        blendAttachment.blendEnable = true;
        blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        mBlendAttachments.push_back(blendAttachment);

        mFragmentShaderPath = ENGINE_SHADER_DIR "ForwardShadow.frag";

        mPipelineId = PipelineId::ShadowMeshBack;
    }
};

class ShadowMeshFrontPipeline : public OpaquePipeline
{
public:

    ShadowMeshFrontPipeline()
    {
        mName = "Shadow Front";
        mCullMode = VK_CULL_MODE_BACK_BIT;
        mDepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        mDepthWriteEnabled = false;

        mBlendAttachments.clear();
        VkPipelineColorBlendAttachmentState blendAttachment = {};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
        blendAttachment.blendEnable = true;
        blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
        blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        mBlendAttachments.push_back(blendAttachment);

        mFragmentShaderPath = ENGINE_SHADER_DIR "ForwardShadow.frag";

        mPipelineId = PipelineId::ShadowMeshFront;
    }
};

class ShadowMeshClearPipeline : public ShadowMeshFrontPipeline
{
public:

    ShadowMeshClearPipeline()
    {
        mDepthCompareOp = VK_COMPARE_OP_ALWAYS;

        mBlendAttachments.clear();
        VkPipelineColorBlendAttachmentState blendAttachment = {};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_A_BIT;
        blendAttachment.blendEnable = true;
        blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        mBlendAttachments.push_back(blendAttachment);

        mPipelineId = PipelineId::ShadowMeshClear;
    }
};

class PostProcessPipeline : public Pipeline
{
public:

    PostProcessPipeline()
    {
        mName = "PostProcess Pipeline";
        SetVertexConfig(VertexType::Max, ENGINE_SHADER_DIR "ScreenRect.vert");
        mFragmentShaderPath = ENGINE_SHADER_DIR "PostProcess.frag";
        mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        mCullMode = VK_CULL_MODE_NONE;
        mDepthTestEnabled = VK_FALSE;

        mPipelineId = PipelineId::PostProcess;
    }

    virtual void PopulateLayoutBindings() override
    {
        Pipeline::PopulateLayoutBindings();

        PushSet();
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
};

class NullPostProcessPipeline : public PostProcessPipeline
{
public:

    NullPostProcessPipeline()
    {
        mName = "NullPostProcess Pipeline";
        mFragmentShaderPath = ENGINE_SHADER_DIR "NullPostProcess.frag";

        mPipelineId = PipelineId::NullPostProcess;
    }

};

class QuadPipeline : public Pipeline
{
public:

    QuadPipeline()
    {
        mName = "Quad Pipeline";
        SetVertexConfig(VertexType::VertexUI, ENGINE_SHADER_DIR "Quad.vert");
        mFragmentShaderPath = ENGINE_SHADER_DIR "Quad.frag";
        mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        mCullMode = VK_CULL_MODE_NONE;
        mDepthTestEnabled = VK_FALSE;
        
        mBlendAttachments[0].blendEnable = VK_TRUE;
        mBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        mBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        mPipelineId = PipelineId::Quad;
    }

    virtual void PopulateLayoutBindings() override
    {
        Pipeline::PopulateLayoutBindings();

        PushSet();
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
};

class TextPipeline : public Pipeline
{
public:

    TextPipeline()
    {
        mName = "Text Pipeline";
        SetVertexConfig(VertexType::VertexUI, ENGINE_SHADER_DIR "Text.vert");
        mFragmentShaderPath = ENGINE_SHADER_DIR "Text.frag";
        mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        mCullMode = VK_CULL_MODE_NONE;
        mDepthTestEnabled = VK_FALSE;

        mBlendAttachments[0].blendEnable = VK_TRUE;
        mBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        mBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        mPipelineId = PipelineId::Text;
    }

    virtual void PopulateLayoutBindings() override
    {
        Pipeline::PopulateLayoutBindings();

        PushSet();
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
};

class PolyPipeline : public Pipeline
{
public:

    PolyPipeline()
    {
        mName = "Poly Pipeline";
        SetVertexConfig(VertexType::VertexUI, ENGINE_SHADER_DIR "Poly.vert");
        mFragmentShaderPath = ENGINE_SHADER_DIR "Poly.frag";
        mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        mCullMode = VK_CULL_MODE_NONE;
        mDepthTestEnabled = VK_FALSE;
        mDynamicLineWidth = true;

        mBlendAttachments[0].blendEnable = VK_TRUE;
        mBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        mBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        mPipelineId = PipelineId::Poly;
    }

    virtual void PopulateLayoutBindings() override
    {
        Pipeline::PopulateLayoutBindings();

        PushSet();
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
};


class SelectedGeometryPipeline : public ForwardPipeline
{
public:

    SelectedGeometryPipeline()
    {
        mName = "Selected Geometry Pipeline";
        mDepthTestEnabled = false;
        mDepthCompareOp = VK_COMPARE_OP_ALWAYS;
        SetMeshVertexConfigs(
            ENGINE_SHADER_DIR "Depth.vert",
            ENGINE_SHADER_DIR "Depth.vert",
            ENGINE_SHADER_DIR "DepthSkinned.vert",
            ENGINE_SHADER_DIR "Depth.vert");
        mFragmentShaderPath = ENGINE_SHADER_DIR "SelectedGeometry.frag";

        mBlendAttachments.clear(); // Clear all geometry blends
        AddMixBlendAttachmentState();

        mPipelineId = PipelineId::Selected;
    }
};

class HitCheckPipeline : public ForwardPipeline
{
public:

    HitCheckPipeline()
    {
        mName = "Hit Check Pipeline";
        SetMeshVertexConfigs(
            ENGINE_SHADER_DIR "Depth.vert",
            ENGINE_SHADER_DIR "Depth.vert",
            ENGINE_SHADER_DIR "DepthSkinned.vert",
            ENGINE_SHADER_DIR "Depth.vert");
        mFragmentShaderPath = ENGINE_SHADER_DIR "HitCheck.frag";
        mDepthCompareOp = VK_COMPARE_OP_LESS;

        mBlendAttachments.clear(); // Clear all geometry blends
        AddOpaqueBlendAttachmentState();

        mPipelineId = PipelineId::HitCheck;
    }
};

class WireframeGeometryPipeline : public ForwardPipeline
{
public:

    WireframeGeometryPipeline()
    {
        mName = "Wireframe Pipeline";
        mCullMode = VK_CULL_MODE_NONE;
        mDepthTestEnabled = true;
        mDepthCompareOp = VK_COMPARE_OP_LESS;
        mPolygonMode = VK_POLYGON_MODE_LINE;
        mLineWidth = 1.0f;
        mFragmentShaderPath = ENGINE_SHADER_DIR "ColorGeometry.frag";

        mBlendAttachments.clear(); // Clear all geometry blends
        AddOpaqueBlendAttachmentState();

        mPipelineId = PipelineId::Wireframe;
    }
};

class CollisionGeometryPipeline : public WireframeGeometryPipeline
{
public:

    CollisionGeometryPipeline()
    {
        mName = "Collision Geometry Pipeline";
        mLineWidth = 2.0f;
        mDepthWriteEnabled = false;
        mDepthBias = -0.05f;
        mDepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        mPipelineId = PipelineId::Collision;
    }
};

class BakedLightVisPipeline : public ForwardPipeline
{
public:

    BakedLightVisPipeline()
    {
        mName = "BakedLightVis Pipeline";
        mDepthTestEnabled = true;
        mFragmentShaderPath = ENGINE_SHADER_DIR "BakedLightVis.frag";

        mBlendAttachments.clear(); // Clear all geometry blends
        AddOpaqueBlendAttachmentState();

        mPipelineId = PipelineId::BakedLightVis;
    }
};

class LineGeometryPipeline : public Pipeline
{
public:

    LineGeometryPipeline()
    {
        mName = "Line Geometry Pipeline";
        mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        mDepthTestEnabled = true;
        mDepthWriteEnabled = false;
        mDepthCompareOp = VK_COMPARE_OP_LESS;
        mCullMode = VK_CULL_MODE_NONE;
        mLineWidth = 1.0f;

        SetVertexConfig(VertexType::VertexColorSimple, ENGINE_SHADER_DIR "Line.vert");
        mFragmentShaderPath = ENGINE_SHADER_DIR "Line.frag";

        mPipelineId = PipelineId::Line;
    }
};

class PathTracePipeline : public Pipeline
{
public:

    PathTracePipeline()
    {
        mComputePipeline = true;
        mComputeShaderPath = ENGINE_SHADER_DIR "PathTrace.comp";

        mPipelineId = PipelineId::PathTrace;
    }

    virtual void PopulateLayoutBindings() override
    {
        Pipeline::PopulateLayoutBindings();

        PushSet();
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Uniform data
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Triangle data
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Mesh (+ Material) data
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Light data
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, PATH_TRACE_MAX_TEXTURES); // Texture Array
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT); // Output image
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // In/Out baked vertex data
    }
};

class LightBakeDirectPipeline : public PathTracePipeline
{
public:

    LightBakeDirectPipeline()
    {
        mComputeShaderPath = ENGINE_SHADER_DIR "LightBakeDirect.comp";
        mPipelineId = PipelineId::LightBakeDirect;
    }
};

class LightBakeIndirectPipeline : public LightBakeDirectPipeline
{
public:

    LightBakeIndirectPipeline()
    {
        mComputeShaderPath = ENGINE_SHADER_DIR "LightBakeIndirect.comp";
        mPipelineId = PipelineId::LightBakeIndirect;
    }
};

class LightBakeAveragePipeline : public Pipeline
{
public:

    LightBakeAveragePipeline()
    {
        mComputePipeline = true;
        mComputeShaderPath = ENGINE_SHADER_DIR "LightBakeAverage.comp";

        mPipelineId = PipelineId::LightBakeAverage;
    }

    virtual void PopulateLayoutBindings() override
    {
        Pipeline::PopulateLayoutBindings();

        PushSet();
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Uniform data
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Baked vertex data
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Triangle data
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Output average light data
    }
};

class LightBakeDiffusePipeline : public Pipeline
{
public:

    LightBakeDiffusePipeline()
    {
        mComputePipeline = true;
        mComputeShaderPath = ENGINE_SHADER_DIR "LightBakeDiffuse.comp";

        mPipelineId = PipelineId::LightBakeDiffuse;
    }

    virtual void PopulateLayoutBindings() override
    {
        Pipeline::PopulateLayoutBindings();

        PushSet();
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Uniform data
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Baked vertex data
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Triangle data
        AddLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // Output average light data
    }
};

#endif