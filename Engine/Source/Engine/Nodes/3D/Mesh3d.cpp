#include "Nodes/3D/Mesh3d.h"
#include "Nodes/3D/StaticMesh3d.h"
#include "Nodes/3D/SkeletalMesh3d.h"
#include "Assets/MaterialInstance.h"
#include "Engine.h"
#include "World.h"

DEFINE_RTTI(Mesh3D);

bool Mesh3D::HandlePropChange(Datum* datum, uint32_t index, const void* newValue)
{
    Property* prop = static_cast<Property*>(datum);
    OCT_ASSERT(prop != nullptr);
    Mesh3D* meshComp = static_cast<Mesh3D*>(prop->mOwner);
    bool success = false;

    if (prop->mName == "Material Override")
    {
        meshComp->SetMaterialOverride(*(Material**)newValue);
        success = true;
    }

    return success;
}

Mesh3D::Mesh3D() :
    mMaterialOverride(nullptr)
{

}

Mesh3D::~Mesh3D()
{

}

const char* Mesh3D::GetTypeName() const
{
    return "Mesh";
}

void Mesh3D::GatherProperties(std::vector<Property>& outProps)
{
    Primitive3D::GatherProperties(outProps);

    SCOPED_CATEGORY("Mesh");

    outProps.push_back(Property(DatumType::Asset, "Material Override", this, &mMaterialOverride, 1, HandlePropChange, int32_t(Material::GetStaticType())));
    outProps.push_back(Property(DatumType::Bool, "Billboard", this, &mBillboard));
}

void Mesh3D::SaveStream(Stream& stream)
{
    Primitive3D::SaveStream(stream);
    stream.WriteAsset(mMaterialOverride);
    stream.WriteBool(mBillboard);
}

void Mesh3D::LoadStream(Stream& stream)
{
    Primitive3D::LoadStream(stream);
    stream.ReadAsset(mMaterialOverride);
    mBillboard = stream.ReadBool();
}

bool Mesh3D::IsShadowMesh3D()
{
    return false;
}

Material* Mesh3D::GetMaterialOverride()
{
    return mMaterialOverride.Get<Material>();
}

void Mesh3D::SetMaterialOverride(Material* material)
{
    mMaterialOverride = material;
}

MaterialInstance* Mesh3D::InstantiateMaterial()
{
    Material* mat = GetMaterial();
    MaterialInstance* matInst = MaterialInstance::New(mat);
    SetMaterialOverride(matInst);
    return matInst;
}

bool Mesh3D::IsBillboard() const
{
    return mBillboard;
}

void Mesh3D::SetBillboard(bool billboard)
{
    mBillboard = billboard;
}

glm::mat4 Mesh3D::ComputeBillboardTransform()
{
    Camera3D* camera = GetWorld()->GetActiveCamera();
    glm::quat camQuat = camera->GetAbsoluteRotationQuat();

    glm::mat4 transform = glm::mat4(1);

    glm::vec3 pos = GetAbsolutePosition();
    glm::vec3 scale = GetAbsoluteScale();
    glm::quat rot = GetRotationQuat(); // Not absolute

    transform = glm::translate(transform, pos);
    transform *= glm::toMat4(camQuat);
    transform *= glm::toMat4(rot);
    transform = glm::scale(transform, scale);

    return transform;
}

glm::mat4 Mesh3D::GetRenderTransform()
{
    glm::mat4 transform;
    if (IsBillboard())
    {
        transform = ComputeBillboardTransform();
    }
    else
    {
        transform = GetTransform();
    }
    return transform;
}

DrawData Mesh3D::GetDrawData()
{
    DrawData data = {};
    Material* material = GetMaterial();

    data.mNode = this;
    data.mMaterial = material;
    data.mShadingModel = material ? material->GetShadingModel() : ShadingModel::Lit;
    data.mBlendMode = material ? material->GetBlendMode() : BlendMode::Opaque;
    data.mPosition = GetAbsolutePosition();
    data.mBounds = GetBounds();
    data.mSortPriority = material ? material->GetSortPriority() : 0;
    data.mDepthless = material ? material->IsDepthTestDisabled() : false;

    return data;
}
