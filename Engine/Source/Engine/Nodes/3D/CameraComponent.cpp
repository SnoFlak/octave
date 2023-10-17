#include "Nodes/3D/CameraComponent.h"
#include "Log.h"
#include "Engine.h"
#include "World.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "Maths.h"

#include "Nodes/3D/PrimitiveComponent.h"

#include "Graphics/Graphics.h"

FORCE_LINK_DEF(CameraComponent);
DEFINE_NODE(CameraComponent);

CameraComponent::CameraComponent() :
    mProjectionMode(ProjectionMode::PERSPECTIVE),
    mViewProjectionMatrix(1)
{
    mName = "Camera";
}

CameraComponent::~CameraComponent()
{

}

const char* CameraComponent::GetTypeName() const
{
    return "Camera";
}

void CameraComponent::BeginPlay()
{
    TransformComponent::BeginPlay();

    CameraComponent* activeCam = GetWorld()->GetActiveCamera();

    if (activeCam == nullptr ||
        activeCam == GetWorld()->GetDefaultCamera())
    {
        GetWorld()->SetActiveCamera(this);
    }
}

void CameraComponent::Destroy()
{
    if (GetWorld() &&
        GetWorld()->GetActiveCamera() == this)
    {
        GetWorld()->SetActiveCamera(nullptr);
    }

    Component::Destroy();
}

void CameraComponent::GatherProperties(std::vector<Property>& outProps)
{
    TransformComponent::GatherProperties(outProps);

    outProps.push_back(Property(DatumType::Bool, "Perspective", this, &mProjectionMode));
    outProps.push_back(Property(DatumType::Float, "Field Of View", this, &mPerspectiveSettings.mFovY));
}

void CameraComponent::GatherProxyDraws(std::vector<DebugDraw>& inoutDraws)
{
#if DEBUG_DRAW_ENABLED
    if (GetType() == CameraComponent::GetStaticType() &&
        this != GetWorld()->GetActiveCamera())
    {
        glm::mat4 transform = glm::rotate(DEGREES_TO_RADIANS * -90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
        transform = mTransform * transform;

        DebugDraw debugDraw;
        debugDraw.mMesh = LoadAsset<StaticMesh>("SM_Cone");
        debugDraw.mActor = GetOwner();
        debugDraw.mComponent = this;
        debugDraw.mColor = glm::vec4(0.0f, 0.85f, 0.25f, 1.0f);
        debugDraw.mTransform = glm::scale(transform, { 0.5f, 0.5f, 0.5f });
        inoutDraws.push_back(debugDraw);
    }
#endif
}

void CameraComponent::SaveStream(Stream& stream)
{
    TransformComponent::SaveStream(stream);
    stream.WriteUint8(uint8_t(mProjectionMode));
    stream.WriteFloat(mPerspectiveSettings.mFovY);
}

void CameraComponent::LoadStream(Stream& stream)
{
    TransformComponent::LoadStream(stream);
    mProjectionMode = ProjectionMode(stream.ReadUint8());
    mPerspectiveSettings.mFovY = stream.ReadFloat();
}

void CameraComponent::SetOrthoSettings(float width, float height, float zNear, float zFar)
{
    mOrthoSettings.mWidth = width;
    mOrthoSettings.mHeight = height;
    mOrthoSettings.mNear = zNear;
    mOrthoSettings.mFar = zFar;
}

void CameraComponent::SetPerspectiveSettings(float fovY, float aspectRatio, float zNear, float zFar)
{
    mPerspectiveSettings.mFovY = fovY;
    mPerspectiveSettings.mAspectRatio = aspectRatio;
    mPerspectiveSettings.mNear = zNear;
    mPerspectiveSettings.mFar = zFar;
}

ProjectionMode CameraComponent::GetProjectionMode() const
{
    return mProjectionMode;
}

void CameraComponent::SetProjectionMode(ProjectionMode mode)
{
    mProjectionMode = mode;
}

PerspectiveSettings CameraComponent::GetPerspectiveSettings() const
{
    return mPerspectiveSettings;
}

OrthoSettings CameraComponent::GetOrthoSettings() const
{
    return mOrthoSettings;
}

const glm::mat4& CameraComponent::GetViewProjectionMatrix()
{
    return mViewProjectionMatrix;
}

const glm::mat4& CameraComponent::GetViewMatrix()
{
    return mViewMatrix;
}

const glm::mat4& CameraComponent::GetProjectionMatrix()
{
    return mProjectionMatrix;
}

void CameraComponent::ComputeMatrices()
{
    // Make sure transform is up to date.
    UpdateTransform(false);

    EngineState* engineState = GetEngineState();
    mPerspectiveSettings.mAspectRatio = static_cast<float>(engineState->mWindowWidth) / engineState->mWindowHeight;

    // Use the scaling factor to address Wii widescreen stretching
    mPerspectiveSettings.mAspectRatio *= engineState->mAspectRatioScale;

    mViewMatrix = CalculateViewMatrix();
    mViewMatrix = glm::toMat4(glm::conjugate(GetAbsoluteRotationQuat()));

    mViewMatrix = translate(mViewMatrix, -GetAbsolutePosition());

    if (mProjectionMode == ProjectionMode::ORTHOGRAPHIC)
    {
        mProjectionMatrix = GFX_MakeOrthographicMatrix(
            -mOrthoSettings.mWidth,
            mOrthoSettings.mWidth,
            -mOrthoSettings.mHeight,
            mOrthoSettings.mHeight,
            mOrthoSettings.mNear,
            mOrthoSettings.mFar);

        mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;

        glm::mat4 stdProjMtx = glm::ortho(
            -mOrthoSettings.mWidth,
            mOrthoSettings.mWidth,
            -mOrthoSettings.mHeight,
            mOrthoSettings.mHeight,
            mOrthoSettings.mNear,
            mOrthoSettings.mFar);

        stdProjMtx[1][1] *= -1.0f;

        mStandardViewProjectionMatrix = stdProjMtx * mViewMatrix;
    }
    else
    {
        mProjectionMatrix = GFX_MakePerspectiveMatrix(mPerspectiveSettings.mFovY,
            mPerspectiveSettings.mAspectRatio,
            mPerspectiveSettings.mNear,
            mPerspectiveSettings.mFar);

        mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;

        // Because the 3DS projection matrix is wonky and I don't know how to 
        // derive the clipspace position for World-To-Screen conversions,
        // just create a standard perspective matrix and we can use it for the conversions.
        glm::mat4 stdProjMtx = glm::perspectiveFov(
            glm::radians(mPerspectiveSettings.mFovY),
            mPerspectiveSettings.mAspectRatio,
            1.0f,
            mPerspectiveSettings.mNear,
            mPerspectiveSettings.mFar);

        stdProjMtx[1][1] *= -1.0f;

        mStandardViewProjectionMatrix = stdProjMtx * mViewMatrix;
    }
}

glm::mat4 CameraComponent::CalculateViewMatrix()
{
    glm::mat4 view = glm::toMat4(glm::conjugate(GetAbsoluteRotationQuat()));
    view = translate(mViewMatrix, -GetAbsolutePosition());
    return view;
}

glm::mat4 CameraComponent::CalculateInvViewMatrix()
{
    glm::mat4 invView;

    invView = CalculateViewMatrix();
    invView = glm::inverse(invView);

    return invView;
}

float CameraComponent::GetNearZ() const
{
    return (mProjectionMode == ProjectionMode::ORTHOGRAPHIC) ?
        mOrthoSettings.mNear :
        mPerspectiveSettings.mNear;
}

float CameraComponent::GetFarZ() const
{
    return (mProjectionMode == ProjectionMode::ORTHOGRAPHIC) ?
        mOrthoSettings.mFar :
        mPerspectiveSettings.mFar;
}

float CameraComponent::GetFieldOfView() const
{
    return mPerspectiveSettings.mFovY;
}

float CameraComponent::GetFieldOfViewY() const
{
    return mPerspectiveSettings.mFovY;
}

float CameraComponent::GetFieldOfViewX() const
{
    float aspectRatio = mPerspectiveSettings.mAspectRatio;
    float fovRadiansY = mPerspectiveSettings.mFovY * DEGREES_TO_RADIANS;

    float fovRadiansX = 2 * atanf(tanf(fovRadiansY * 0.5f) * aspectRatio);
    float fovDegreesX = fovRadiansX * RADIANS_TO_DEGREES;

    return fovDegreesX;
}

float CameraComponent::GetAspectRatio() const
{
    return mPerspectiveSettings.mAspectRatio;
}

float CameraComponent::GetWidth() const
{
    return mOrthoSettings.mWidth;
}

float CameraComponent::GetHeight() const
{
    return mOrthoSettings.mHeight;
}

float CameraComponent::GetNearWidth() const
{
    float width = 0.0f;

    if (mProjectionMode == ProjectionMode::PERSPECTIVE)
    {
        width = GetNearHeight() * mPerspectiveSettings.mAspectRatio;
    }
    else
    {
        width = mOrthoSettings.mWidth / 2.0f;
    }

    return width;
}

float CameraComponent::GetNearHeight() const
{
    float height = 0.0f;

    if (mProjectionMode == ProjectionMode::PERSPECTIVE)
    {
        height = 2.0f * mPerspectiveSettings.mNear * tanf(mPerspectiveSettings.mFovY * 0.5f * DEGREES_TO_RADIANS);
    }
    else
    {
        height = mOrthoSettings.mHeight / 2.0f;
    }

    return height;
}

void CameraComponent::SetNearZ(float nearZ)
{
    if (mProjectionMode == ProjectionMode::PERSPECTIVE)
    {
        mPerspectiveSettings.mNear = nearZ;
    }
    else
    {
        mOrthoSettings.mNear = nearZ;
    }
}

void CameraComponent::SetFarZ(float farZ)
{
    if (mProjectionMode == ProjectionMode::PERSPECTIVE)
    {
        mPerspectiveSettings.mNear = farZ;
    }
    else
    {
        mOrthoSettings.mNear = farZ;
    }
}

void CameraComponent::SetFieldOfView(float fovY)
{
    mPerspectiveSettings.mFovY = fovY;
}

void CameraComponent::SetAspectRatio(float aspectRatio)
{
    mPerspectiveSettings.mAspectRatio = aspectRatio;
}

void CameraComponent::SetWidth(float width)
{
    mOrthoSettings.mWidth = width;
}

void CameraComponent::SetHeight(float height)
{
    mOrthoSettings.mHeight = height;
}

glm::vec3 CameraComponent::WorldToScreenPosition(glm::vec3 worldPos)
{
    glm::vec3 screenPos = {};

    if (GetWorld())
    {
        glm::vec4 clipPos = mStandardViewProjectionMatrix * glm::vec4(worldPos, 1.0f);

        float w = clipPos.w;
        clipPos /= w;

        glm::vec2 screen2d = glm::vec2(clipPos);
        screen2d += glm::vec2(1.0f, 1.0f);
        screen2d *= glm::vec2(0.5f, 0.5f);
        screen2d *= glm::vec2(GetEngineState()->mWindowWidth, GetEngineState()->mWindowHeight);

        screenPos.x = screen2d.x;
        screenPos.y = screen2d.y;
        screenPos.z = w;
    }

    return screenPos;
}

glm::vec3 CameraComponent::ScreenToWorldPosition(int32_t x, int32_t y)
{
    float screenX = float(x);
    float screenY = float(y);
    float screenWidth = (float)GetEngineState()->mWindowWidth;
    float screenHeight = (float)GetEngineState()->mWindowHeight;

    float cX = (screenX / screenWidth) * 2.0f - 1.0f;
    float cY = (screenY / screenHeight) * 2.0f - 1.0f;
    float cZ = 0.0f; // Near Plane

    // Use standard VP here because android might rotate the proj matrix.
    glm::mat4 invViewProj = glm::inverse(mStandardViewProjectionMatrix);
    glm::vec4 worldPos4 = invViewProj * glm::vec4(cX, cY, cZ, 1.0f);
    worldPos4 = worldPos4 / worldPos4.w;

    glm::vec3 worldPos = glm::vec3(worldPos4.x, worldPos4.y, worldPos4.z);

    return worldPos;
}

glm::vec3 CameraComponent::TraceScreenToWorld(int32_t x, int32_t y, uint8_t colMask, PrimitiveComponent** outComp)
{
    glm::vec3 worldPos = ScreenToWorldPosition(x, y);

    glm::vec3 startPos = GetAbsolutePosition();
    glm::vec3 rayDir = Maths::SafeNormalize(worldPos - startPos);
    glm::vec3 endPos = startPos + rayDir * GetFarZ();

    RayTestResult result;
    GetWorld()->RayTest(startPos, endPos, colMask, result);

    if (outComp != nullptr)
    {
        *outComp = result.mHitComponent;
    }

    return result.mHitPosition;
}
