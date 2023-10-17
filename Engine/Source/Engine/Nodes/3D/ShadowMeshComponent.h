#pragma once

#include "Nodes/3D/StaticMeshComponent.h"

class ShadowMeshComponent : public StaticMeshComponent
{
public:

    DECLARE_NODE(ShadowMeshComponent, StaticMeshComponent);

    ShadowMeshComponent();
    ~ShadowMeshComponent();

    virtual const char* GetTypeName() const override;
    virtual void GatherProperties(std::vector<Property>& outProps) override;

    virtual void Create() override;
    virtual void Destroy() override;

    virtual void Render() override;

    virtual void SaveStream(Stream& stream) override;
    virtual void LoadStream(Stream& stream) override;

    virtual bool IsShadowMeshComponent() override;

    virtual void GatherProxyDraws(std::vector<DebugDraw>& inoutDraws) override;
};
