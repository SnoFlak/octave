#if EDITOR

#include "ActionManager.h"

#if PLATFORM_WINDOWS
#include <Windows.h>
#include <ShlObj.h>
#endif

#include <stdint.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <algorithm>

#include "Log.h"
#include "EditorConstants.h"
#include "Constants.h"
#include "World.h"
#include "Engine.h"
#include "AssetManager.h"
#include "EditorState.h"
#include "EditorUtils.h"
#include "PanelManager.h"
#include "Nodes/Widgets/Button.h"
#include "Assets/Scene.h"
#include "Assets/Texture.h"
#include "Assets/StaticMesh.h"
#include "Assets/SkeletalMesh.h"
#include "Assets/SoundWave.h"
#include "Assets/Font.h"
#include "AssetDir.h"
#include "EmbeddedFile.h"
#include "Utilities.h"
#include "EditorUtils.h"
#include "Log.h"

#include "Nodes/3D/StaticMesh3d.h"
#include "Nodes/3D/PointLight3d.h"
#include "Nodes/3D/DirectionalLight3d.h"
#include "Nodes/3D/Node3d.h"
#include "Nodes/3D/SkeletalMesh3d.h"
#include "Nodes/3D/Box3d.h"
#include "Nodes/3D/Sphere3d.h"
#include "Nodes/3D/Capsule3d.h"
#include "Nodes/3D/Particle3d.h"
#include "Nodes/3D/Audio3d.h"
#include "Nodes/3D/ShadowMesh3d.h"
#include "Nodes/3D/TextMesh3d.h"

#include "System/System.h"

#if EDITOR
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

#define STANDALONE_RELEASE 0

ActionManager* ActionManager::sInstance = nullptr;

TypeId CheckDaeAssetType(const char* path)
{
    TypeId retType = 0;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_FlipUVs);

    if (scene != nullptr &&
        scene->mNumMeshes >= 1)
    {
        retType = scene->mMeshes[0]->HasBones() ? SkeletalMesh::GetStaticType() : StaticMesh::GetStaticType();
    }

    return retType;
}

void ActionManager::Create()
{
    Destroy();
    sInstance = new ActionManager();
}

void ActionManager::Destroy()
{
    if (sInstance != nullptr)
    {
        delete sInstance;
        sInstance = nullptr;
    }
}

ActionManager* ActionManager::Get()
{
    return sInstance;
}

ActionManager::ActionManager()
{

}

ActionManager::~ActionManager()
{

}

void ActionManager::Update()
{

}

void HandleBuildButtonPressed(Button* button)
{
#if NODE_CONV_EDITOR
    std::string buttonText = button->GetTextString();
    ActionManager* am = ActionManager::Get();

    if (buttonText == "Windows")
    {
        am->BuildData(Platform::Windows, false);
    }
    else if (buttonText == "Linux")
    {
        am->BuildData(Platform::Linux, false);
    }
    else if (buttonText == "Android")
    {
        am->BuildData(Platform::Android, false);
    }
    else if (buttonText == "GameCube")
    {
        am->BuildData(Platform::GameCube, false);
    }
    else if (buttonText == "Wii")
    {
        am->BuildData(Platform::Wii, false);
    }
    else if (buttonText == "3DS")
    {
        am->BuildData(Platform::N3DS, false);
    }
    else if (buttonText == "GameCube Embedded")
    {
        am->BuildData(Platform::GameCube, true);
    }
    else if (buttonText == "Wii Embedded")
    {
        am->BuildData(Platform::Wii, true);
    }
    else if (buttonText == "3DS Embedded")
    {
        am->BuildData(Platform::N3DS, true);
    }

    GetActionList()->Hide();
#endif
}

void ActionManager::BuildData(Platform platform, bool embedded)
{
    const EngineState* engineState = GetEngineState();
    bool standalone = engineState->mStandalone;
    const std::string& projectDir = engineState->mProjectDirectory;
    const std::string& projectName = engineState->mProjectName;

    std::vector<std::pair<AssetStub*, std::string> > embeddedAssets;

    if (projectDir == "")
    {
        LogError("Project directory not set?");
        return;
    }

    // Build Data is responsible for 3 things
    // (1) Create a Packaged directory in ProjectDir/Packaged. Erase previous packaged first.
    std::string packagedDir = projectDir + "Packaged/";

    // Create top level Packaged dir first.
    if (!DoesDirExist(packagedDir.c_str()))
    {
        CreateDir(packagedDir.c_str());
    }

    // Create platform-specific packaged dir. Delete old platform dir if it exists.
    packagedDir += GetPlatformString(platform);
    packagedDir += "/";
    if (DoesDirExist(packagedDir.c_str()))
    {
        RemoveDir(packagedDir.c_str());
    }

    CreateDir(packagedDir.c_str());

    // (2) Iterate over AssetDirs and save each file (platform-specific save) to the Packaged folder.
    std::function<void(AssetDir*, bool)> saveDir = [&](AssetDir* dir, bool engine)
    {
        std::string packDir;
        if (engine)
        {
            packDir = packagedDir + dir->mPath + "/";
        }
        else
        {
            packDir = dir->mPath;
            packDir = packDir.substr(projectDir.length());
            packDir = packagedDir + projectName + "/" + packDir;
        }

        if (!DoesDirExist(packDir.c_str()))
        {
            CreateDir(packDir.c_str());
        }

        // Cook the assets into our packaged folder
        for (uint32_t i = 0; i < dir->mAssetStubs.size(); ++i)
        {
            AssetStub* stub = dir->mAssetStubs[i];
            bool alreadyLoaded = (stub->mAsset != nullptr);

            if (!alreadyLoaded)
            {
                AssetManager::Get()->LoadAsset(*stub);
            }

            std::string packFile = packDir + stub->mAsset->GetName() + ".oct";
            stub->mAsset->SaveFile(packFile.c_str(), platform);

            if (true)
            {
                // Save the asset in the src location. There is probably a better time and place for this.
                AssetManager::Get()->SaveAsset(*stub);
            }

            // Currently either embed everything or embed nothing...
            // Embed flag on Asset does nothing, but if we want to keep that feature, then 
            // we need to load the asset if it's not loaded, add to embedded list if it's flagged and then probably unload it after.
            if (embedded)
            {
                embeddedAssets.push_back({ stub, packFile });
            }

            if (!alreadyLoaded)
            {
                AssetManager::Get()->UnloadAsset(*stub);
            }
        }

        // Cook child dirs
        for (uint32_t i = 0; i < dir->mChildDirs.size(); ++i)
        {
            saveDir(dir->mChildDirs[i], engine);
        }
    };

    AssetDir* engineAssetDir = AssetManager::Get()->FindEngineDirectory();
    AssetDir* projectAssetDir = AssetManager::Get()->FindProjectDirectory();
    std::string packEngineDir = packagedDir + engineAssetDir->mName + "/";
    std::string packProjectDir = packagedDir + projectAssetDir->mName + "/";
    CreateDir(packEngineDir.c_str());
    CreateDir(packProjectDir.c_str());

    saveDir(engineAssetDir, true);
    saveDir(projectAssetDir, false);

    // (3) Generate .cpp / .h files (empty if not embedded) using the .oct files in the Packaged folder.
    // (4) Create and save an asset registry file with simple list of asset paths into Packaged folder.
    std::unordered_map<std::string, AssetStub*>& assetMap = AssetManager::Get()->GetAssetMap();
    FILE* registryFile = nullptr;

    std::string registryFileName = packagedDir + projectName + "/AssetRegistry.txt";
    registryFile = fopen(registryFileName.c_str(), "w");

    for (auto pair : assetMap)
    {
        if (pair.second->mAsset != nullptr &&
            pair.second->mEngineAsset)
        {
            AssetManager::Get()->SaveAsset(*pair.second);
        }

        if (registryFile != nullptr)
        {
            const char* regType = Asset::GetNameFromTypeId(pair.second->mType);

            std::string regPath = pair.second->mPath.c_str();
            if (!pair.second->mEngineAsset &&
                regPath.find(projectDir) != std::string::npos)
            {
                regPath = regPath.substr(projectDir.length());
                regPath = projectName + "/" + regPath;
            }

            fprintf(registryFile, "%s,%s\n", regType, regPath.c_str());
        }
    }

    if (registryFile != nullptr)
    {
        fclose(registryFile);
        registryFile = nullptr;
    }

    // Create a Generated folder inside the project folder if it doesn't exist
    if (!DoesDirExist((projectDir + "Generated").c_str()))
    {
        CreateDir((projectDir + "Generated").c_str());
    }

    std::string embeddedHeaderPath = projectDir + "Generated/EmbeddedAssets.h";
    std::string embeddedSourcePath = projectDir + "Generated/EmbeddedAssets.cpp";
    GenerateEmbeddedAssetFiles(embeddedAssets, embeddedHeaderPath.c_str(), embeddedSourcePath.c_str());

    // Generate embedded script source files. If not doing an embedded build, copy over the script folders.
    std::vector<std::string> scriptFiles;

    if (embedded)
    {
        GatherScriptFiles("Engine/Scripts/", scriptFiles);
        GatherScriptFiles(projectDir + "/Scripts/", scriptFiles);
    }
    else
    {
        SYS_Exec(std::string("cp -R Engine/Scripts " + packagedDir + "Engine/Scripts").c_str());
        SYS_Exec(std::string("cp -R " + projectDir + "Scripts " + packagedDir + projectName + "/Scripts").c_str());
    }

    // Generate embedded script source files, even if not doing an embedded build. 
    // So we don't need to worry about whether we include code that links to the embedded script array / script count.
    std::string scriptHeaderPath = projectDir + "Generated/EmbeddedScripts.h";
    std::string scriptSourcePath = projectDir + "Generated/EmbeddedScripts.cpp";
    GenerateEmbeddedScriptFiles(scriptFiles, scriptHeaderPath.c_str(), scriptSourcePath.c_str());

    if (standalone)
    {
        std::string copyGeneratedFolder = "cp -R " + projectDir + "Generated " + "Standalone";
        SYS_Exec(copyGeneratedFolder.c_str());
    }

    // ( ) Maybe copy Project .octp file into the Packaged folder? Are we actually using it?
    {
        std::string copyOctpCmd = "cp " + projectDir + projectName + ".octp " + packagedDir + projectName;
        SYS_Exec(copyOctpCmd.c_str());
    }

    // Write out an Engine.ini file which is used by Standalone game exe.
    FILE* engineIni = fopen(std::string(packagedDir + "Engine.ini").c_str(), "w");
    if (engineIni != nullptr)
    {
        fprintf(engineIni, "project=%s", projectName.c_str());

        fclose(engineIni);
        engineIni = nullptr;
    }

    // Handle SpirV shaders on Vulkan platforms
    // Make sure to do this before copying everything to "assets/" directory in the Android build.
    if (platform == Platform::Windows ||
        platform == Platform::Linux ||
        platform == Platform::Android)
    {
        // Compile shaders
#if PLATFORM_WINDOWS
        SYS_Exec("cd Engine/Shaders/GLSL && \"./compile.bat\"");
#else
        SYS_Exec("cd Engine/Shaders/GLSL && \"./compile.sh\"");
#endif

        // Then copy over the binaries.
        CreateDir((packagedDir + "Engine/Shaders/").c_str());
        CreateDir((packagedDir + "Engine/Shaders/GLSL/").c_str());

        SYS_Exec(std::string("cp -R Engine/Shaders/GLSL/bin " + packagedDir + "Engine/Shaders/GLSL/bin").c_str());
    }

    // ( ) Run the makefile to compile the game.
#if STANDALONE_RELEASE
    bool needCompile = !standalone || embedded || platform == Platform::Android;
#else
    bool needCompile = true;
#endif
    std::string buildProjName = standalone ? "Standalone" : projectName;
    std::string buildProjDir = standalone ? "Standalone/" : projectDir;
    std::string buildDstExeName = standalone ? "Octave" : projectName;

    if (needCompile)
    {
        if (platform == Platform::Windows)
        {
            // If devenv can't be found, add it to your PATH
            // My devenv for VS2017 community was found here:
            // C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE

            std::string solutionPath = "Octave.sln";
            if (engineState->mSolutionPath != "")
            {
                solutionPath = engineState->mSolutionPath;
            }

            std::string devenvCmd = std::string("devenv ") + solutionPath + " /Build \"Release|x64\" /Project " + buildProjName;

            SYS_Exec(devenvCmd.c_str());
        }
        else if (platform == Platform::Android)
        {
            // Copy contents of Packaged/Android folder into Android/app/src/main/assets folder so they get put into the APK.
            std::string androidAssetsDir = buildProjDir + "Android/app/src/main/assets/";
            if (!DoesDirExist(androidAssetsDir.c_str()))
            {
                CreateDir(androidAssetsDir.c_str());
            }

            SYS_Exec(std::string("cp -R " + packagedDir + "/* " + androidAssetsDir).c_str());

            // Invoke the gradle build
            std::string gradleDir = buildProjDir + "Android/";
#if PLATFORM_WINDOWS
            std::string gradleCmd = "cd " + gradleDir + " && gradlew.bat assembleRelease";
#else
            std::string gradleCmd = "cd " + gradleDir + " && \"./gradlew assembleRelease\"";
#endif
            SYS_Exec(gradleCmd.c_str());

            // Rename the executable
            std::string srcExeName = StringToLower(buildProjName);
            srcExeName += "-release.apk";

            std::string dstExeName = buildDstExeName + ".apk";

            std::string renameCmd = std::string("mv ") + buildProjDir + "/Android/app/build/outputs/apk/release/" + srcExeName + " " + buildProjDir + "/Android/app/build/outputs/apk/release/" + dstExeName;
            SYS_Exec(renameCmd.c_str());
        }
        else
        {
            std::string makefilePath = "Makefile_";

            switch (platform)
            {
            case Platform::Linux: makefilePath += "Linux_Game"; break;
            case Platform::GameCube: makefilePath += "GCN"; break;
            case Platform::Wii: makefilePath += "Wii"; break;
            case Platform::N3DS: makefilePath += "3DS"; break;
            default: OCT_ASSERT(0); break;
            }

            std::string makeCmd = std::string("make -C ") + (buildProjDir) + " -f " + makefilePath + " -j 6";
            SYS_Exec(makeCmd.c_str());
        }
    }
    else
    {
        // When running the standalone editor, we don't need to compile 
        // (unless we are embeddeding assets into the executable).
        // But we do need to copy over the Octave.exe and ideally rename it to the project name to make it more official :)
    }

    // ( ) Copy the executable into the Packaged folder.
    std::string exeSrc = buildProjDir + "/Build/";

    if (platform == Platform::Android)
    {
        exeSrc = buildProjDir;
    }

    if (!needCompile)
    {
        // Override exe path for uncompiled standalone builds
#if STANDALONE_RELEASE
        exeSrc = "Standalone/Binaries/";
#else
        exeSrc = "Standalone/Build/";
#endif
    }

    switch (platform)
    {
    case Platform::Windows: exeSrc += "Windows/x64/Release/"; break;
    case Platform::Linux: exeSrc += "Linux/"; break;
    case Platform::Android: exeSrc += "Android/app/build/outputs/apk/release/"; break;
    case Platform::GameCube: exeSrc += "GCN/"; break;
    case Platform::Wii: exeSrc += "Wii/"; break;
    case Platform::N3DS: exeSrc += "3DS/"; break;
    default: OCT_ASSERT(0); break;
    }

    exeSrc += standalone ? "Octave" : projectName;

    std::string extension = ".exe";

    switch (platform)
    {
    case Platform::Windows: extension = ".exe"; break;
    case Platform::Linux: extension = ".out"; break;
    case Platform::Android: extension = ".apk"; break;
    case Platform::GameCube: extension = ".dol"; break;
    case Platform::Wii: extension = ".dol"; break;
    case Platform::N3DS: extension = ".3dsx"; break;
    default: OCT_ASSERT(0); break;
    }

    exeSrc += extension;

    std::string exeCopyCmd = std::string("cp ") + exeSrc + " " + packagedDir;
    SYS_Exec(exeCopyCmd.c_str());

    if (standalone)
    {
        // Rename the executable to the project name
        std::string renameCmd = std::string("mv ") + packagedDir + "Octave" + extension + " " + packagedDir + projectName + extension;
        SYS_Exec(renameCmd.c_str());
    }

    LogDebug("Build Finished");
}

void ActionManager::OnSelectedNodeChanged()
{

}

Node* ActionManager::SpawnNode(TypeId nodeType, Node* parent)
{
    Node* spawnedNode = EXE_SpawnNode(nodeType);

    OCT_ASSERT(spawnedNode != nullptr);
    if (spawnedNode != nullptr)
    {
        parent = parent ? parent : GetWorld()->GetRootNode();
        if (parent != nullptr)
        {
            parent->AddChild(spawnedNode);
        }
        else
        {
            GetWorld()->SetRootNode(spawnedNode);
        }

        GetEditorState()->SetSelectedNode(spawnedNode);
    }

    return spawnedNode;
}

Node* ActionManager::SpawnNode(TypeId nodeType, glm::vec3 position)
{
    Node* node = SpawnNode(nodeType, nullptr);
    Node3D* node3d = node ? node->As<Node3D>() : nullptr;

    if (node3d)
    {
        node3d->SetAbsolutePosition(position);
    }

    return node;
}

Node* ActionManager::SpawnBasicNode(const std::string& name, Node* parent, Asset* srcAsset, bool setWorldPos, glm::vec3 worldPos)
{
    Node* spawnedNode = nullptr;

    if (srcAsset == nullptr)
    {
        srcAsset = GetEditorState()->GetSelectedAsset();
    }

    if (name == BASIC_STATIC_MESH)
    {
        StaticMesh3D* meshNode = EXE_SpawnNode(StaticMesh3D::GetStaticType())->As<StaticMesh3D>();

        StaticMesh* mesh = (StaticMesh*) LoadAsset("SM_Cube");

        if (srcAsset != nullptr &&
            srcAsset->GetType() == StaticMesh::GetStaticType())
        {
            mesh = static_cast<StaticMesh*>(srcAsset);
            meshNode->SetName(mesh->GetName());
        }

        // When spawned by the editor, static meshes have collision enabled on colgroup1
        meshNode->SetStaticMesh(mesh);
        meshNode->EnableOverlaps(false);
        meshNode->EnableCollision(true);
        meshNode->EnablePhysics(false);
        meshNode->SetCollisionGroup(ColGroup1);
        meshNode->SetCollisionMask(~ColGroup1);
        meshNode->SetBakeLighting(true);

        spawnedNode = meshNode;
    }
    else if (name == BASIC_POINT_LIGHT)
    {
        // Spawn point light actor
        PointLight3D* pointLight = EXE_SpawnNode(PointLight3D::GetStaticType())->As<PointLight3D>();

        pointLight->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        pointLight->SetRadius(10.0f);
        pointLight->SetLightingDomain(LightingDomain::All);
        
        spawnedNode = pointLight;
    }
    else if (name == BASIC_NODE_3D)
    {
        spawnedNode = EXE_SpawnNode(Node3D::GetStaticType())->As<Node3D>();
    }
    else if (name == BASIC_DIRECTIONAL_LIGHT)
    {
        DirectionalLight3D* dirLight = EXE_SpawnNode(DirectionalLight3D::GetStaticType())->As<DirectionalLight3D>();
        dirLight->SetLightingDomain(LightingDomain::All);

        spawnedNode = dirLight;
    }
    else if (name == BASIC_SKELETAL_MESH)
    {
        SkeletalMesh3D* skNode = EXE_SpawnNode(SkeletalMesh3D::GetStaticType())->As<SkeletalMesh3D>();

        // TODO: Add a default SkeletalMesh to Engine assets
        SkeletalMesh* mesh = nullptr;

        if (srcAsset != nullptr &&
            srcAsset->GetType() == SkeletalMesh::GetStaticType())
        {
            mesh = static_cast<SkeletalMesh*>(srcAsset);

            // Spawn skeletal mesh actor.
            skNode->SetSkeletalMesh(mesh);
        }

        spawnedNode = skNode;
    }
    else if (name == BASIC_BOX)
    {
        spawnedNode = EXE_SpawnNode(Box3D::GetStaticType())->As<Box3D>();
    }
    else if (name == BASIC_SPHERE)
    {
        spawnedNode = EXE_SpawnNode(Sphere3D::GetStaticType())->As<Sphere3D>();
    }
    else if (name == BASIC_CAPSULE)
    {
        spawnedNode = EXE_SpawnNode(Capsule3D::GetStaticType())->As<Capsule3D>();
    }
    else if (name == BASIC_PARTICLE)
    {
        ParticleSystem* particleSystem = (ParticleSystem*) LoadAsset("P_DefaultParticle");

        if (srcAsset != nullptr &&
            srcAsset->GetType() == ParticleSystem::GetStaticType())
        {
            particleSystem = static_cast<ParticleSystem*>(srcAsset);
        }

        // Spawn a Particle actor
        Particle3D* particleNode = EXE_SpawnNode(Particle3D::GetStaticType())->As<Particle3D>();
        particleNode->SetParticleSystem(particleSystem);

        spawnedNode = particleNode;
    }
    else if (name == BASIC_AUDIO)
    {
        // TODO: Add a default sound in Engine assets
        SoundWave* soundWave = nullptr;

        if (srcAsset != nullptr &&
            srcAsset->GetType() == SoundWave::GetStaticType())
        {
            soundWave = static_cast<SoundWave*>(srcAsset);
        }

        // Spawn an Audio actor
        Audio3D* audioNode = EXE_SpawnNode(Audio3D::GetStaticType())->As<Audio3D>();
        audioNode->SetSoundWave(soundWave);
        audioNode->SetLoop(true);
        audioNode->SetAutoPlay(true);

        spawnedNode = audioNode;
    }
    else if (name == BASIC_SCENE)
    {
        Scene* scene = nullptr;

        if (srcAsset != nullptr &&
            srcAsset->GetType() == Scene::GetStaticType())
        {
            scene = static_cast<Scene*>(srcAsset);
        }

        if (scene != nullptr)
        {
            spawnedNode = EXE_SpawnNode(scene);
        }
    }
    else if (name == BASIC_CAMERA)
    {
        spawnedNode = EXE_SpawnNode(Camera3D::GetStaticType())->As<Camera3D>();
    }
    else if (name == BASIC_TEXT_MESH)
    {
    spawnedNode = EXE_SpawnNode(TextMesh3D::GetStaticType())->As<TextMesh3D>();
    }

    if (spawnedNode != nullptr)
    {
        parent = parent ? parent : GetWorld()->GetRootNode();
        if (parent != nullptr)
        {
            parent->AddChild(spawnedNode);
        }
        else
        {
            GetWorld()->SetRootNode(spawnedNode);
        }

        GetEditorState()->SetSelectedNode(spawnedNode);

        Node3D* node3d = spawnedNode ? spawnedNode->As<Node3D>() : nullptr;

        if (setWorldPos && node3d)
        {
            node3d->SetAbsolutePosition(worldPos);
        }
    }
    else
    {
        LogError("Failed to spawn basic actor: %s", name.c_str());
    }

    return spawnedNode;
}

void ActionManager::ExecuteAction(Action* action)
{
    OCT_ASSERT(std::find(mActionHistory.begin(), mActionHistory.end(), action) == mActionHistory.end());
    OCT_ASSERT(std::find(mActionFuture.begin(), mActionFuture.end(), action) == mActionFuture.end());

    action->Execute();

    // Don't record action history while playing in editor. Too chaotic for undo/redo
    if (IsPlayingInEditor())
    {
        delete action;
        action = nullptr;
    }
    else
    {
        // Limit max number of history?
        const uint32_t MaxActionHistoryCount = 100;
        if (mActionHistory.size() >= MaxActionHistoryCount)
        {
            mActionHistory.erase(mActionHistory.begin());
        }

        mActionHistory.push_back(action);
        ClearActionFuture();
    }
}

void ActionManager::Undo()
{
    if (mActionHistory.size() > 0 &&
        !ImGui::GetIO().WantTextInput)
    {
        Action* action = mActionHistory.back();
        mActionHistory.pop_back();

        LogDebug("Undo %s", action->GetName());
        action->Reverse();

        mActionFuture.push_back(action);
    }
}

void ActionManager::Redo()
{
    if (mActionFuture.size() > 0 &&
        !ImGui::GetIO().WantTextInput)
    {
        Action* action = mActionFuture.back();
        mActionFuture.pop_back();

        LogDebug("Redo %s", action->GetName());
        action->Execute();

        mActionHistory.push_back(action);
    }
}

void ActionManager::EXE_EditProperty(void* owner, PropertyOwnerType ownerType, const std::string& name, uint32_t index, Datum newValue)
{
    ActionEditProperty* action = new ActionEditProperty(owner, ownerType, name, index, newValue);
    ActionManager::Get()->ExecuteAction(action);
}

void ActionManager::EXE_EditTransform(Node3D* transComp, const glm::mat4& transform)
{
    std::vector<Node3D*> transComps;
    transComps.push_back(transComp);

    std::vector<glm::mat4> transforms;
    transforms.push_back(transform);

    ActionEditTransforms* action = new ActionEditTransforms(transComps, transforms);
    ActionManager::Get()->ExecuteAction(action);
}

void ActionManager::EXE_EditTransforms(const std::vector<Node3D*>& transComps, const std::vector<glm::mat4>& newTransforms)
{
    ActionEditTransforms* action = new ActionEditTransforms(transComps, newTransforms);
    ActionManager::Get()->ExecuteAction(action);
}

Node* ActionManager::EXE_SpawnNode(TypeId srcType)
{
    std::vector<TypeId> srcTypes;
    srcTypes.push_back(srcType);

    GetEditorState()->EnsureActiveScene();

    ActionSpawnNodes* action = new ActionSpawnNodes(srcTypes);
    ActionManager::Get()->ExecuteAction(action);

    OCT_ASSERT(action->GetNodes().size() == 1);
    Node* retNode = action->GetNodes()[0];
    return retNode;
}

Node* ActionManager::EXE_SpawnNode(const char* srcTypeName)
{
    std::vector<const char*> srcTypeNames;
    srcTypeNames.push_back(srcTypeName);

    GetEditorState()->EnsureActiveScene();

    ActionSpawnNodes* action = new ActionSpawnNodes(srcTypeNames);
    ActionManager::Get()->ExecuteAction(action);

    OCT_ASSERT(action->GetNodes().size() == 1);
    Node* retNode = action->GetNodes()[0];
    return retNode;
}

Node* ActionManager::EXE_SpawnNode(Scene* srcScene)
{
    std::vector<SceneRef> srcScenes;
    srcScenes.push_back(srcScene);

    GetEditorState()->EnsureActiveScene();

    ActionSpawnNodes* action = new ActionSpawnNodes(srcScenes);
    ActionManager::Get()->ExecuteAction(action);

    OCT_ASSERT(action->GetNodes().size() == 1);
    Node* retNode = action->GetNodes()[0];
    return retNode;
}

Node* ActionManager::EXE_SpawnNode(Node* srcNode)
{
    std::vector<Node*> srcNodes;
    srcNodes.push_back(srcNode);

    GetEditorState()->EnsureActiveScene();

    ActionSpawnNodes* action = new ActionSpawnNodes(srcNodes);
    ActionManager::Get()->ExecuteAction(action);

    OCT_ASSERT(action->GetNodes().size() == 1);
    Node* retNode = action->GetNodes()[0];
    return retNode;
}

void ActionManager::EXE_DeleteNode(Node* node)
{
    std::vector<Node*> nodes;
    nodes.push_back(node);

    ActionDeleteNodes* action = new ActionDeleteNodes(nodes);
    ActionManager::Get()->ExecuteAction(action);
}

std::vector<Node*> ActionManager::EXE_SpawnNodes(const std::vector<Node*>& srcNodes)
{
    OCT_ASSERT(srcNodes.size() > 0);

    GetEditorState()->EnsureActiveScene();

    ActionSpawnNodes* action = new ActionSpawnNodes(srcNodes);
    ActionManager::Get()->ExecuteAction(action);

    OCT_ASSERT(action->GetNodes().size() > 0);
    return action->GetNodes();
}

void ActionManager::EXE_DeleteNodes(const std::vector<Node*>& nodes)
{
    ActionDeleteNodes* action = new ActionDeleteNodes(nodes);
    ActionManager::Get()->ExecuteAction(action);
}

void ActionManager::EXE_AttachNode(Node* node, Node* newParent, int32_t childIndex, int32_t boneIndex)
{
    ActionAttachNode* action = new ActionAttachNode(node, newParent, childIndex, boneIndex);
    ActionManager::Get()->ExecuteAction(action);
}

void ActionManager::EXE_SetRootNode(Node* newRoot)
{
    ActionSetRootNode* action = new ActionSetRootNode(newRoot);
    ActionManager::Get()->ExecuteAction(action);
}

void ActionManager::EXE_SetAbsoluteRotation(Node3D* comp, glm::quat rot)
{
    ActionSetAbsoluteRotation* action = new ActionSetAbsoluteRotation(comp, rot);
    ActionManager::Get()->ExecuteAction(action);
}

void ActionManager::EXE_SetAbsolutePosition(Node3D* comp, glm::vec3 pos)
{
    ActionSetAbsolutePosition* action = new ActionSetAbsolutePosition(comp, pos);
    ActionManager::Get()->ExecuteAction(action);
}

void ActionManager::EXE_SetAbsoluteScale(Node3D* comp, glm::vec3 scale)
{
    ActionSetAbsoluteScale* action = new ActionSetAbsoluteScale(comp, scale);
    ActionManager::Get()->ExecuteAction(action);
}

void ActionManager::ClearActionHistory()
{
    for (uint32_t i = 0; i < mActionHistory.size(); ++i)
    {
        delete mActionHistory[i];
        mActionHistory[i] = nullptr;
    }

    mActionHistory.clear();
}

void ActionManager::ClearActionFuture()
{
    for (uint32_t i = 0; i < mActionFuture.size(); ++i)
    {
        delete mActionFuture[i];
        mActionFuture[i] = nullptr;
    }

    mActionFuture.clear();
}

void ActionManager::ResetUndoRedo()
{
    ClearActionHistory();
    ClearActionFuture();

    for (int32_t i = (int32_t)mExiledNodes.size() - 1; i >= 0; --i)
    {
        Node::Destruct(mExiledNodes[i]);
        mExiledNodes.erase(mExiledNodes.begin() + i);
    }

    // Clear property inspection history also.
    GetEditorState()->ClearInspectHistory();
}

void ActionManager::ExileNode(Node* node)
{
    OCT_ASSERT(std::find(mExiledNodes.begin(), mExiledNodes.end(), node) == mExiledNodes.end());
    OCT_ASSERT(node->GetParent() == nullptr);

    mExiledNodes.push_back(node);

    if (GetEditorState()->IsNodeSelected(node))
    {
        GetEditorState()->SetSelectedNode(nullptr);
    }

    if (GetEditorState()->GetInspectedObject() == node)
    {
        GetEditorState()->InspectObject(nullptr, true);
    }
}

void ActionManager::RestoreExiledNode(Node* node)
{
    bool restored = false;

    for (uint32_t i = 0; i < mExiledNodes.size(); ++i)
    {
        if (mExiledNodes[i] == node)
        {
            mExiledNodes.erase(mExiledNodes.begin() + i);
            restored = true;
            break;
        }
    }

    OCT_ASSERT(restored);
}

void ActionManager::CreateNewProject()
{
    std::string newProjDir = SYS_SelectFolderDialog();
    std::replace(newProjDir.begin(), newProjDir.end(), '\\', '/');

    std::string newProjName = newProjDir;
    size_t slashLoc = newProjName.find_last_of('/');
    if (slashLoc != std::string::npos)
    {
        newProjName = newProjName.substr(slashLoc + 1);
    }

    LogDebug("CreateNewProject: %s @ %s", newProjName.c_str(), newProjDir.c_str());

    // Now that we have the folder, we need to populate it with an Assets and Scripts folder
    std::string assetsFolder = newProjDir + "/Assets";
    std::string scriptsFolder = newProjDir + "/Scripts";
    SYS_CreateDirectory(assetsFolder.c_str());
    SYS_CreateDirectory(scriptsFolder.c_str());

    // Also we need to create an octp so that user can open the project with Ctrl+P
    std::string projectFile = newProjDir + "/" + newProjName.c_str() + ".octp";
    FILE* octpFile = fopen(projectFile.c_str(), "w");
    if (octpFile != nullptr)
    {
        fprintf(octpFile, "name=%s", newProjName.c_str());

        fclose(octpFile);
        octpFile = nullptr;
    }

    // Finally, open the project
    OpenProject(projectFile.c_str());
}

void ActionManager::OpenProject(const char* path)
{
    const char* projectPath = path;
    std::string openPath;

    if (projectPath == nullptr)
    {
        openPath = SYS_OpenFileDialog();

        // Display the Open dialog box. 
        if (openPath != "")
        {
            projectPath = openPath.c_str();
            LogDebug("ProjectDirectory = %s", projectPath);
        }
    }

    if (projectPath != nullptr)
    {
        LoadProject(projectPath);
    }

    // Handle new project directory
    GetEditorState()->ClearAssetDirHistory();
    GetEditorState()->SetAssetDirectory(AssetManager::Get()->FindProjectDirectory(), true);
    GetEditorState()->SetSelectedAssetStub(nullptr);
}

void ActionManager::OpenScene()
{
    // TODO-NODE: This uses SYS_OpenFileDialog() and I think we should just get rid of this function.
    if (GetEngineState()->mProjectPath == "")
        return;

    std::string openPath = SYS_OpenFileDialog();

    // Display the Open dialog box. 
    if (openPath != "")
    {
        std::string filename = strrchr(openPath.c_str(), '/') + 1;
        filename = filename.substr(0, filename.find_last_of('.'));
        AssetStub* stub = FetchAssetStub(filename);

        if (stub != nullptr &&
            stub->mType == Scene::GetStaticType())
        {
            AssetManager::Get()->LoadAsset(*stub);
            Scene* loadedScene = (Scene*)stub->mAsset;

            OpenScene(loadedScene);
        }
        else
        {
            LogError("Failed to fetch Level from AssetManager");
        }
    }
}

void ActionManager::OpenScene(Scene* scene)
{
    GetEditorState()->OpenEditScene(scene);
}

void ActionManager::SaveScene(bool saveAs)
{
    if (GetEngineState()->mProjectPath == "")
        return;

    EditScene* editScene = GetEditorState()->GetEditScene();

    if (saveAs || editScene->mSceneAsset == nullptr)
    {
        GetEditorState()->mRequestSaveSceneAs = true;

#if 0
        // Old method of bringing up OS file browser
        std::string savePath = SYS_SaveFileDialog();

        // Display the Open dialog box. 
        if (savePath != "")
        {
            std::replace(savePath.begin(), savePath.end(), '\\', '/');
            std::string newSceneName = Asset::GetNameFromPath(savePath);
            AssetStub* stub = AssetManager::Get()->CreateAndRegisterAsset(Scene::GetStaticType(), nullptr, newSceneName, false);

            if (stub != nullptr)
            {
                Scene* newScene = (Scene*)stub->mAsset;
                newScene->SetName(newSceneName);
                editScene->mSceneAsset = newScene;
            }
        }
#endif
    }
    else if (editScene->mSceneAsset != nullptr)
    {
        Scene* scene = editScene->mSceneAsset.Get<Scene>();
        scene->Capture(GetWorld()->GetRootNode());
        AssetManager::Get()->SaveAsset(scene->GetName());
    }
}

void ActionManager::SaveSelectedAsset()
{
    AssetStub* selectedStub = GetEditorState()->GetSelectedAssetStub();
    if (selectedStub != nullptr &&
        selectedStub->mAsset != nullptr)
    {
        AssetManager::Get()->SaveAsset(*selectedStub);
    }
}

void ActionManager::DeleteSelectedNodes()
{
    std::vector<Node*> nodes = GetEditorState()->GetSelectedNodes();

    for (uint32_t i = 0; i < nodes.size(); ++i)
    {
        // TODO-NODE: Previously we checked here if the node was the World's active camera.
        //   Now that we use a totally separate editor camera in EditorState, I removed the check.
        //   Verify editor cam works as expected.
        if (nodes[i] == nullptr)
        {
            nodes.erase(nodes.begin() + i);
            --i;
        }
    }

    EXE_DeleteNodes(nodes);
    GetEditorState()->SetSelectedNode(nullptr);
}

void ActionManager::DeleteNode(Node* node)
{
    if (node != nullptr &&
        node != GetEditorState()->GetEditorCamera())
    {
        EXE_DeleteNode(node);
    }
}

Asset* ActionManager::ImportAsset()
{
    Asset* retAsset = nullptr;

    if (GetEngineState()->mProjectPath != "")
    {
        std::string openPath = SYS_OpenFileDialog();

        // Display the Open dialog box. 
        if (openPath != "")
        {
            retAsset = ImportAsset(openPath);
        }
    }
    else
    {
        LogWarning("Cannot import asset. No project loaded.");
    }

    return retAsset;
}

Asset* ActionManager::ImportAsset(const std::string& path)
{
    Asset* retAsset = nullptr;

    std::string filename = strrchr(path.c_str(), '/') + 1;
    int32_t dotIndex = int32_t(filename.find_last_of('.'));
    std::string extension = filename.substr(dotIndex, filename.size() - dotIndex);

    TypeId newType = INVALID_TYPE_ID;

    if (extension == ".png")
    {
        newType = Texture::GetStaticType();
    }
    else if (extension == ".dae" ||
        extension == ".fbx" ||
        extension == ".glb")
    {
        newType = CheckDaeAssetType(path.c_str());
    }
    else if (extension == ".wav")
    {
        newType = SoundWave::GetStaticType();
    }
    else if (extension == ".xml")
    {
        newType = Font::GetStaticType();
    }

    if (newType != INVALID_TYPE_ID)
    {
        Asset* newAsset = Asset::CreateInstance(newType);
        newAsset->Import(path);

        AssetDir* assetDir = GetEditorState()->GetAssetDirectory();
        std::string assetName = filename.substr(0, dotIndex);
        filename = assetName + ".oct";

        // Clear inspected asset if we are reimporting that same asset.
        Asset* oldAsset = FetchAsset(assetName.c_str());

        if (oldAsset != nullptr)
        {
            if (GetEditorState()->GetInspectedObject() == oldAsset)
            {
                GetEditorState()->InspectObject(nullptr, true);
            }
        }

#if ASSET_LIVE_REF_TRACKING
        // If this asset already exists, then we are about to delete it and replace it.
        // So let's fix up any references now or else they will be lost (replaced with nullptr).
        if (oldAsset != nullptr)
        {
            AssetRef::ReplaceReferencesToAsset(oldAsset, newAsset);
        }
#endif

        // If asset already exists, overwrite it. So delete existing asset.
        bool purged = AssetManager::Get()->PurgeAsset(assetName.c_str());
        if (purged)
        {
            LogWarning("Reimporting asset");
        }

        AssetStub* stub = AssetManager::Get()->RegisterAsset(filename, newAsset->GetType(), assetDir, nullptr, false);
        stub->mAsset = newAsset;
        newAsset->SetName(stub->mName);

        // If a StaticMesh/SkeletalMesh is being imported, and there is a selected material, then assign
        // the material to that static mesh.
        if (newAsset != nullptr &&
            (newAsset->Is(StaticMesh::ClassRuntimeId()) || newAsset->Is(SkeletalMesh::ClassRuntimeId())) &&
            GetEditorState()->GetSelectedAsset() != nullptr &&
            GetEditorState()->GetSelectedAsset()->Is(Material::ClassRuntimeId()))
        {
            Material* material = GetEditorState()->GetSelectedAsset()->As<Material>();

            if (newAsset->Is(StaticMesh::ClassRuntimeId()))
            {
                StaticMesh* mesh = newAsset->As<StaticMesh>();
                mesh->SetMaterial(material);
            }
            else if (newAsset->Is(SkeletalMesh::ClassRuntimeId()))
            {
                SkeletalMesh* mesh = newAsset->As<SkeletalMesh>();
                mesh->SetMaterial(material);
            }
        }

        AssetManager::Get()->SaveAsset(*stub);

        retAsset = newAsset;
    }
    else
    {
        LogError("Failed to import Asset. Unrecognized source asset extension.");
    }

    return retAsset;
}

static std::string GetFixedFilename(const char* name, const char* prefix)
{
    std::string nameStr = name;
    size_t extPeriod = nameStr.find_last_of('.');
    if (extPeriod != std::string::npos)
    {
        nameStr = nameStr.substr(0, extPeriod);
    }

    if (nameStr.substr(0, 2) != prefix)
    {
        nameStr = prefix + nameStr;
    }

    return nameStr;
}

static void SpawnAiNode(aiNode* node, const glm::mat4& parentTransform, const std::vector<StaticMesh*>& meshList, const SceneImportOptions& options)
{
    World* world = GetWorld();

    if (node != nullptr)
    {
        glm::mat4 transform = parentTransform * glm::transpose(glm::make_mat4(&node->mTransformation.a1));

        for (uint32_t i = 0; i < node->mNumMeshes; ++i)
        {
            uint32_t meshIndex = node->mMeshes[i];
            StaticMesh3D* newMesh = GetWorld()->SpawnNode<StaticMesh3D>();
            newMesh->SetStaticMesh(meshList[meshIndex]);
            newMesh->SetUseTriangleCollision(meshList[meshIndex]->IsTriangleCollisionMeshEnabled());
            newMesh->SetTransform(transform);
            newMesh->SetName(/*options.mPrefix + */node->mName.C_Str());
            newMesh->EnableCastShadows(true);
            newMesh->SetBakeLighting(true);
            newMesh->SetUseTriangleCollision(true);
            newMesh->AddTag("Scene");
        }
    }

    for (uint32_t i = 0; i < node->mNumChildren; ++i)
    {
        SpawnAiNode(node->mChildren[i], parentTransform, meshList, options);
    }
}

static glm::mat4 GetNodeTransform(aiNode* node)
{
    aiMatrix4x4 transform;

    while (node)
    {
        transform = transform * node->mTransformation;
        node = node->mParent;
    }

    glm::mat4 gMat = glm::transpose(glm::make_mat4(&transform.a1));
    return gMat;
}

void ActionManager::ImportScene(const SceneImportOptions& options)
{
    if (GetEngineState()->mProjectPath == "")
        return;

    std::string openPath = options.mFilePath;

    // Display the Open dialog box. 
    if (openPath != "")
    {
        std::string filename = openPath;
        int32_t dotIndex = int32_t(filename.find_last_of('.'));
        std::string extension = filename.substr(dotIndex, filename.size() - dotIndex);

        std::string importDir;
        size_t slashPos = openPath.find_last_of("/\\");
        if (slashPos != std::string::npos)
        {
            importDir = openPath.substr(0, slashPos + 1);
        }
        else
        {
            importDir = "./";
        }

        if (extension == ".glb" ||
            extension == ".gltf" ||
            extension == ".dae")
        {
            LogDebug("Begin scene import...");
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(filename, aiProcess_FlipUVs);

            if (scene == nullptr)
            {
                LogError("Failed to load scene file");
                return;
            }

            // Get the current directory in the asset panel (all assets will be saved there)
            AssetDir* dir = GetEditorState()->GetAssetDirectory();

            if (dir == nullptr ||
                dir->mParentDir == nullptr)
            {
                LogError("Invalid directory. Use the asset panel to navigate to a valid directory");
                return;
            }

            if (options.mCleanDirectory)
            {
                dir->Purge();
            }

            if (options.mClearWorld)
            {
                DeleteAllNodes();
            }
            else
            {
                // Destroy all actors with a Scene tag.
                // Kinda hacky, but for now, to mark anything spawned as part of a scene,
                // I'm adding a Scene tag. This is to make reimporting scenes easier.
                const std::vector<Node*>& nodes = GetWorld()->GatherNodes();
                std::vector<Node*> delNodes;
                for (int32_t i = int32_t(nodes.size()) - 1; i >= 0; --i)
                {
                    if (nodes[i]->HasTag("Scene"))
                    {
                        delNodes.push_back(nodes[i]);
                    }
                }

                EXE_DeleteNodes(delNodes);
            }

            if (GetWorld()->GetRootNode() == nullptr)
            {
                Node3D* defaultRoot = GetWorld()->SpawnNode<Node3D>();
                defaultRoot->SetName("Root");
            }

            std::vector<Texture*> textureList;
            std::vector<Material*> materialList;
            std::vector<StaticMesh*> meshList;
            std::unordered_map<std::string, Texture*> textureMap;

            uint32_t numMaterials = scene->mNumMaterials;
            for (uint32_t i = 0; i < numMaterials; ++i)
            {
                aiMaterial* aMaterial = scene->mMaterials[i];
                std::string materialName = options.mPrefix + aMaterial->GetName().C_Str();

                if (materialName.size() < 2 || (materialName.substr(0, 2) != "M_"))
                {
                    materialName = std::string("M_") + materialName;
                }

                AssetStub* materialStub = nullptr;
                Material* newMaterial = nullptr;
                if (options.mImportMaterials)
                {
                    materialStub = EditorAddUniqueAsset(materialName.c_str(), dir, Material::GetStaticType(), true);
                    newMaterial = static_cast<Material*>(materialStub->mAsset);
                    newMaterial->SetShadingModel(options.mDefaultShadingModel);
                }

                uint32_t numBaseTextures = aMaterial->GetTextureCount(aiTextureType_DIFFUSE);
                numBaseTextures = glm::clamp(numBaseTextures, 0u, 4u);

                for (uint32_t t = 0; t < numBaseTextures; ++t)
                {
                    aiString path;
                    aiReturn ret = aMaterial->GetTexture(aiTextureType::aiTextureType_DIFFUSE, t, &path);

                    if (ret == aiReturn_SUCCESS)
                    {
                        std::string texturePath = path.C_Str();
                        Texture* textureToAssign = nullptr;
                        LogDebug("Scene Texture: %s", texturePath.c_str());
                        
                        if (textureMap.find(texturePath) != textureMap.end())
                        {
                            // Case 1 - Texture has already been loaded by a previous material
                            textureToAssign = textureMap[texturePath];
                        }
                        else if (options.mImportTextures)
                        {
                            // Case 2 - Texture needs to be loaded.
                            // To make texturing sharing simpler, we only import the texture if
                            //  - There is no texture registered
                            //  - There is a texture registered, and it resides in the current AssetDir

                            bool importTexture = false;

                            std::string assetName = EditorGetAssetNameFromPath(texturePath);
                            if (assetName.size() >= 2 && (strncmp(assetName.c_str(), "T_", 2) == 0))
                            {
                                // Remove the T_ prefix, reapply later.
                                assetName = assetName.substr(2);
                            }

                            assetName = options.mPrefix + assetName;
                            assetName = GetFixedFilename(assetName.c_str(), "T_");

                            AssetStub* existingStub = AssetManager::Get()->GetAssetStub(assetName);
                            if (existingStub && existingStub->mDirectory != dir)
                            {
                                textureToAssign = LoadAsset<Texture>(assetName);
                            }

                            if (textureToAssign == nullptr)
                            {
                                Asset* importedAsset = ImportAsset(importDir + texturePath);
                                OCT_ASSERT(importedAsset == nullptr || importedAsset->GetType() == Texture::GetStaticType());

                                if (importedAsset == nullptr || importedAsset->GetType() == Texture::GetStaticType())
                                {
                                    textureToAssign = (Texture*)importedAsset;
                                }

                                if (importedAsset != nullptr)
                                {
                                    AssetManager::Get()->RenameAsset(importedAsset, assetName);
                                    AssetManager::Get()->SaveAsset(assetName);
                                }
                            }

                            textureMap.insert({ texturePath, textureToAssign });
                        }

                        if (newMaterial != nullptr && textureToAssign != nullptr)
                        {
                            newMaterial->SetTexture(TextureSlot(TEXTURE_0 + t), textureToAssign);
                        }
                    }
                }

                if (materialStub != nullptr)
                {
                    AssetManager::Get()->SaveAsset(*materialStub);
                    materialList.push_back(newMaterial);
                }
            }

            // Create static mesh assets (assign corresponding material)
            uint32_t numMeshes = scene->mNumMeshes;

            for (uint32_t i = 0; i < numMeshes; ++i)
            {
                aiMesh* aMesh = scene->mMeshes[i];
                std::string meshName = options.mPrefix + aMesh->mName.C_Str();
                
                if (meshName.size() < 3 || (meshName.substr(0, 3) != "SM_"))
                {
                    meshName = std::string("SM_") + meshName;
                }

                // Ensure unique name (this normally happens when model has multiple materials).
                std::string uniqueName = meshName;
                int32_t uniqueNum = 1;
                for (int32_t u = 0; u < (int32_t)meshList.size(); ++u)
                {
                    if (meshList[u]->GetName() == uniqueName)
                    {
                        uniqueName = meshName + "_" + std::to_string(uniqueNum);
                        uniqueNum++;
                        u = -1;
                    }
                }
                meshName = uniqueName;

                StaticMesh* existingMesh = LoadAsset<StaticMesh>(meshName);
                StaticMesh* meshToAddToList = existingMesh;

                if (options.mImportMeshes)
                {
                    StaticMesh* newMesh = (StaticMesh*)Asset::CreateInstance(StaticMesh::GetStaticType());
                    newMesh->Create(scene, *aMesh, 0, nullptr); // Collision meshes currently not supported for scene import?
                    meshToAddToList = newMesh;

                    if (existingMesh)
                    {
                        newMesh->SetGenerateTriangleCollisionMesh(existingMesh->IsTriangleCollisionMeshEnabled());
                        newMesh->SetMaterial(existingMesh->GetMaterial());
                        AssetManager::Get()->PurgeAsset(existingMesh->GetName().c_str());
                        existingMesh = nullptr;
                    }

                    if (options.mImportMaterials)
                    {
                        // Find material to use...
                        uint32_t materialIndex = aMesh->mMaterialIndex;
                        OCT_ASSERT(materialIndex < materialList.size());
                        newMesh->SetMaterial(materialList[materialIndex]);
                    }

                    AssetStub* meshStub = EditorAddUniqueAsset(meshName.c_str(), dir, StaticMesh::GetStaticType(), false);
                    meshStub->mAsset = newMesh;
                    newMesh->SetName(meshName);
                    newMesh->SetGenerateTriangleCollisionMesh(true);
                    AssetManager::Get()->SaveAsset(*meshStub);
                }

                meshList.push_back(meshToAddToList);
            }

            // Create Lights
            if (options.mImportLights)
            {
                uint32_t numLights = scene->mNumLights;
                for (uint32_t i = 0; i < numLights; ++i)
                {
                    aiLight* aLight = scene->mLights[i];

                    if (aLight->mType == aiLightSource_POINT)
                    {
                        PointLight3D* pointLight = GetWorld()->SpawnNode<PointLight3D>();

                        glm::vec3 lightColor;
                        lightColor.r = aLight->mColorDiffuse.r;
                        lightColor.g = aLight->mColorDiffuse.g;
                        lightColor.b = aLight->mColorDiffuse.b;
                        lightColor = Maths::SafeNormalize(lightColor);
                        pointLight->SetColor(glm::vec4(lightColor, 1.0f));

                        // For now, set lights to a default radius to 50.0f.
                        // Not sure how to convert attenutation data into a radius.
                        // Maybe I need to rethink how light attenuation is specified.
                        // Using the constant/linear/quadratic coefficients is how GCN and 3DS do it IIRC.
                        pointLight->SetRadius(50.0f);

                        glm::mat4 lightTransform(1);
                        aiNode* lightNode = scene->mRootNode->FindNode(aLight->mName.C_Str());

                        if (lightNode)
                        {
                            lightTransform = GetNodeTransform(lightNode);
                        }

                        pointLight->SetTransform(lightTransform);
                        pointLight->UpdateTransform(true);

                        pointLight->SetName(aLight->mName.C_Str());
                        pointLight->AddTag("Scene");
                    }
                }
            }

            if (options.mImportObjects)
            {
                aiNode* node = scene->mRootNode;
                SpawnAiNode(node, glm::mat4(1), meshList, options);
            }
        }
        else
        {
            LogError("Failed to import scene. File format must be .glb or .gltf");
        }
    }
}

void ActionManager::GenerateEmbeddedAssetFiles(std::vector<std::pair<AssetStub*, std::string> >& assets,
    const char* headerPath,
    const char* sourcePath)
{
    FILE* headerFile = fopen(headerPath, "w");
    FILE* sourceFile = fopen(sourcePath, "w");

    OCT_ASSERT(headerFile != nullptr && sourceFile != nullptr);

    if (headerFile != nullptr && sourceFile != nullptr)
    {
        fprintf(headerFile, "#include <stdint.h>\n");
        fprintf(headerFile, "#include \"EmbeddedFile.h\"\n\n");

        fprintf(headerFile, "extern uint32_t gNumEmbeddedAssets;\n");
        fprintf(headerFile, "extern EmbeddedFile gEmbeddedAssets[];\n\n");

        fprintf(sourceFile, "#include <stdint.h>\n");
        fprintf(sourceFile, "#include \"EmbeddedFile.h\"\n\n");

        std::string initializer;

        for (int32_t i = 0; i < int32_t(assets.size()); ++i)
        {
            AssetStub* stub = assets[i].first;
            const std::string& packPath = assets[i].second;

            Stream stream;
            
            // Handle special case for level
            stream.ReadFile(packPath.c_str(), false);
            uint32_t size = uint32_t(stream.GetSize());
            char* data = stream.GetData();

            std::string sourceString;
            sourceString.reserve(2048);

            std::string assetDataVar = stub->mName + "_Data";
            sourceString += "const char ";
            sourceString += assetDataVar;
            sourceString += "[] = \n{\n";

            for (uint32_t byte = 0; byte < size; ++byte)
            {
                char byteString[8] = {};
                sprintf(byteString, "'\\x%02X',", uint8_t(data[byte]));

                sourceString += byteString;

                if (byte % 8 == 7)
                {
                    sourceString += "\n";
                }
            }

            sourceString += "\n};\n\n";

            fprintf(sourceFile, "%s", sourceString.c_str());

            initializer += "{" + ("\"" + stub->mName + "\",") +
                                 (assetDataVar + ",") +
                                 (std::to_string(size) + ",") +
                                 (stub->mEngineAsset ? "true" : "false") +
                                 "}, \n";
        }

        fprintf(sourceFile, "\n\nuint32_t gNumEmbeddedAssets = %d;\n", uint32_t(assets.size()));

        if (assets.size() > 0)
        {
            fprintf(sourceFile, "\n\nEmbeddedFile gEmbeddedAssets[] = \n{\n");
            fprintf(sourceFile, "%s", initializer.c_str());
            fprintf(sourceFile, "\n};\n");
        }
        else
        {
            fprintf(sourceFile, "\n\nEmbeddedFile gEmbeddedAssets[] = { {} };\n");
        }

        fclose(headerFile);
        headerFile = nullptr;
        fclose(sourceFile);
        sourceFile = nullptr;
    }
}

void ActionManager::GenerateEmbeddedScriptFiles(
    std::vector<std::string> files,
    const char* headerPath,
    const char* sourcePath)
{
    FILE* headerFile = fopen(headerPath, "w");
    FILE* sourceFile = fopen(sourcePath, "w");

    OCT_ASSERT(headerFile != nullptr && sourceFile != nullptr);

    if (headerFile != nullptr && sourceFile != nullptr)
    {
        fprintf(headerFile, "#include <stdint.h>\n");
        fprintf(headerFile, "#include \"EmbeddedFile.h\"\n\n");

        fprintf(headerFile, "extern uint32_t gNumEmbeddedScripts;\n");
        fprintf(headerFile, "extern EmbeddedFile gEmbeddedScripts[];\n\n");

        fprintf(sourceFile, "#include <stdint.h>\n");
        fprintf(sourceFile, "#include \"EmbeddedFile.h\"\n\n");

        std::string initializer;

        for (int32_t i = 0; i < int32_t(files.size()); ++i)
        {
            std::string luaFile = files[i];
            std::string luaClass = ScriptUtils::GetClassNameFromFileName(luaFile);

            Stream stream;

            // Handle special case for level
            stream.ReadFile(luaFile.c_str(), false);
            uint32_t size = uint32_t(stream.GetSize());
            char* data = stream.GetData();

            std::string sourceString;
            sourceString.reserve(2048);

            std::string fileDataVar = luaClass + "_Data";
            sourceString += "const char ";
            sourceString += fileDataVar;
            sourceString += "[] = \n{\n";

            for (uint32_t byte = 0; byte < size; ++byte)
            {
                char byteString[8] = {};
                sprintf(byteString, "'\\x%02X',", uint8_t(data[byte]));

                sourceString += byteString;

                if (byte % 8 == 7)
                {
                    sourceString += "\n";
                }
            }

            sourceString += "\n};\n\n";

            fprintf(sourceFile, "%s", sourceString.c_str());

            initializer += "{" + ("\"" + luaClass + "\",") +
                (fileDataVar + ",") +
                (std::to_string(size) + ",") +
                (/*engineFile ? "true" : */"false") +
                "}, \n";
        }

        fprintf(sourceFile, "\n\nuint32_t gNumEmbeddedScripts = %d;\n", uint32_t(files.size()));

        if (files.size() > 0)
        {
            fprintf(sourceFile, "\n\nEmbeddedFile gEmbeddedScripts[] = \n{\n");
            fprintf(sourceFile, "%s", initializer.c_str());
            fprintf(sourceFile, "\n};\n");
        }
        else
        {
            fprintf(sourceFile, "\n\nEmbeddedFile gEmbeddedScripts[] = { {} };\n");
        }

        fclose(headerFile);
        headerFile = nullptr;
        fclose(sourceFile);
        sourceFile = nullptr;
    }
}

void ActionManager::GatherScriptFiles(const std::string& dir, std::vector<std::string>& outFiles)
{
    // Recursively iterate through the Script directory and find .lua files.
    std::function<void(std::string)> searchDirectory = [&](std::string dirPath)
    {
        std::vector<std::string> subDirectories;
        DirEntry dirEntry = { };

        SYS_OpenDirectory(dirPath, dirEntry);

        while (dirEntry.mValid)
        {
            if (dirEntry.mDirectory)
            {
                // Ignore this directory and parent directory.
                if (dirEntry.mFilename[0] != '.')
                {
                    subDirectories.push_back(dirEntry.mFilename);
                }
            }
            else
            {
                const char* extension = strrchr(dirEntry.mFilename, '.');

                if (extension != nullptr &&
                    strcmp(extension, ".lua") == 0)
                {
                    std::string path = dirPath + dirEntry.mFilename;
                    outFiles.push_back(path);
                }
            }

            SYS_IterateDirectory(dirEntry);
        }

        SYS_CloseDirectory(dirEntry);

        // Discover files of subdirectories.
        for (uint32_t i = 0; i < subDirectories.size(); ++i)
        {
            std::string subDirPath = dirPath + subDirectories[i] + "/";
            searchDirectory(subDirPath);
        }
    };

    searchDirectory(dir);
}

void ActionManager::ClearWorld()
{
    GetEditorState()->SetSelectedNode(nullptr);
    //SetActiveLevel(nullptr);
    GetWorld()->Clear();

    ResetUndoRedo();
}

void ActionManager::DeleteAllNodes()
{
    if (!IsPlayingInEditor())
    {
        GetEditorState()->SetSelectedNode(nullptr);

        if (GetWorld()->GetRootNode() != nullptr)
        {
            EXE_DeleteNode(GetWorld()->GetRootNode());
        }
    }

}

void ActionManager::RecaptureAndSaveAllScenes()
{
    std::unordered_map<std::string, AssetStub*>& assetMap = AssetManager::Get()->GetAssetMap();

    // This will load all assets! 
    for (auto& pair : assetMap)
    {
        if (pair.second->mType == Scene::GetStaticType())
        {
            Asset* asset = AssetManager::Get()->LoadAsset(*pair.second);
            Scene* scene = static_cast<Scene*>(asset);
            OCT_ASSERT(scene != nullptr);

            Node* temp = scene->Instantiate();
            scene->Capture(temp);

            Node::Destruct(temp);
            temp = nullptr;

            AssetManager::Get()->SaveAsset(*pair.second);
        }
    }

    ClearWorld();
}

void ActionManager::ResaveAllAssets()
{
    std::unordered_map<std::string, AssetStub*>& assetMap = AssetManager::Get()->GetAssetMap();

    // This action is really meant for doing project-wide data updates when adding new serialized data to an asset.
    // It is important that assets are only loaded once if the LoadStream() / WriteStream() funcs are different
    for (auto& pair : assetMap)
    {
        Asset* asset = AssetManager::Get()->LoadAsset(*pair.second);
        AssetManager::Get()->SaveAsset(*pair.second);
    }

    // Refsweep afterwards to 
    AssetManager::Get()->RefSweep();
}

void ActionManager::DeleteAsset(AssetStub* stub)
{
    if (stub != nullptr)
    {
        if (GetEditorState()->GetSelectedAssetStub() == stub)
        {
            GetEditorState()->SetSelectedAssetStub(nullptr);
        }

        if (GetEditorState()->GetInspectedObject() == stub->mAsset)
        {
            GetEditorState()->InspectObject(nullptr);
        }

        std::string path = stub->mPath;
        AssetManager::Get()->PurgeAsset(stub->mName.c_str());
        SYS_RemoveFile(path.c_str());
    }
    else
    {
        LogWarning("Can't delete null asset stub.");
    }
}

void ActionManager::DeleteAssetDir(AssetDir* dir)
{
    if (dir != nullptr &&
        dir->mParentDir != nullptr &&
        dir->mParentDir != AssetManager::Get()->GetRootDirectory())
    {
        std::string path = dir->mPath;
        dir->mParentDir->DeleteSubdirectory(dir->mName.c_str());
        SYS_RemoveDirectory(path.c_str());
    }
    else
    {
        LogWarning("Can't delete null asset dir.");
    }
}

void ActionManager::DuplicateNodes(std::vector<Node*> srcNodes)
{
    // Don't use a vector reference for nodes param because we are going to modify the vector anyway.
    std::vector<Node*> dupedNodes;

    RemoveRedundantDescendants(srcNodes);

    OCT_ASSERT(srcNodes.size() > 0);
    dupedNodes = EXE_SpawnNodes(srcNodes);
    OCT_ASSERT(dupedNodes.size() == srcNodes.size());

    if (dupedNodes.size() > 0 &&
        dupedNodes.size() == srcNodes.size())
    {
        for (uint32_t i = 0; i < srcNodes.size(); ++i)
        {
            Node* srcNode = srcNodes[i];
            Node* newNode = dupedNodes[i];
            Node* parent = srcNode->GetParent();
            if (parent == nullptr)
            {
                parent = srcNode;
            }

            parent->AddChild(newNode);
        }

        GetEditorState()->SetSelectedNode(nullptr);

        for (uint32_t i = 0; i < dupedNodes.size(); ++i)
        {
            GetEditorState()->AddSelectedNode(dupedNodes[i], false);
        }
    }
}

void ActionManager::AttachSelectedNodes(Node* newParent, int32_t boneIdx)
{
    if (newParent == nullptr)
        return;

    std::vector<Node*> selNodes = GetEditorState()->GetSelectedNodes();

    if (selNodes.size() == 0)
        return;

    for (uint32_t i = 0; i < selNodes.size(); ++i)
    {
        Node* child = selNodes[i];
        Node* parent = newParent;

        if (child == parent)
            continue;

        int32_t boneIndex = -1;

        if (child->As<Node3D>())
        {
            boneIndex = child->As<Node3D>()->GetParentBoneIndex();
        }

        if (child->GetParent() != parent ||
            boneIdx != boneIndex)
        {
            // TODO: Do one EXE for all nodes so user doesn't have to CTRL+Z N times.
            ActionManager::Get()->EXE_AttachNode(child, parent, -1, boneIdx);

            // Reparenting components should break the scene link.
            // For now, you cannot override scene instance children
            if (newParent->GetParent() != GetWorld()->GetRootNode())
            {
                newParent->SetScene(nullptr);
            }
        }
    }

}

// ---------------------------
// --------- ACTIONS ---------
// ---------------------------

ActionEditProperty::ActionEditProperty(
    void* owner,
    PropertyOwnerType ownerType,
    const std::string& propName,
    uint32_t index,
    Datum value)
{
    mOwner = owner;
    mOwnerType = ownerType;
    mPropertyName = propName;
    mIndex = index;
    mValue = value;

    // Keep a reference
    if (mOwnerType == PropertyOwnerType::Asset)
    {
        mReferencedAsset = (Asset*)mOwner;
    }
}

void ActionEditProperty::GatherProps(std::vector<Property>& props)
{
    if (mOwnerType == PropertyOwnerType::Node)
    {
        Node* node = (Node*)mOwner;
        node->GatherProperties(props);
    }
    else if (mOwnerType == PropertyOwnerType::Asset)
    {
        Asset* asset = (Asset*)mReferencedAsset.Get<Asset>();
        if (asset)
        {
            asset->GatherProperties(props);
        }
    }
    else if (mOwnerType == PropertyOwnerType::Global)
    {
        GatherGlobalProperties(props);
    }
}

Property* ActionEditProperty::FindProp(std::vector<Property>& props, const std::string& name)
{
    Property* prop = nullptr;

    for (uint32_t i = 0; i < props.size(); ++i)
    {
        if (props[i].mName == name)
        {
            prop = &props[i];
            break;
        }
    }

    return prop;
}

void ActionEditProperty::Execute()
{
    std::vector<Property> sProps;
    GatherProps(sProps);

    Property* prop = FindProp(sProps, mPropertyName);

    if (prop != nullptr)
    {
        if (prop->IsVector() && !prop->IsExternal() && mIndex >= prop->GetCount())
        {
            // In the case of Asset properties, they will be nil if not set, thus
            // when we regather script properties at the start of this function, the count will not include
            // those nil values.
            mIndex = prop->GetCount();
            prop->SetCount(prop->GetCount() + 1);
        }

        // TODO-NODE: Support undo/redo for AddPropertyArray. Until then, don't set value on OOB elements.
        if (mIndex < prop->GetCount())
        {
            mPreviousValue.Destroy();
            mPreviousValue.SetType(prop->GetType());
            mPreviousValue.SetCount(1);
            mPreviousValue.SetValue(prop->GetValue(mIndex));

            prop->SetValue(mValue.mData.vp, mIndex, 1);

            // TODO-NODE: Delete this? Shouldn't be necessary anymore now that we are using Imgui
            //   and gathering properties every frame.
#if 0
            // Script properties are stored internally and propagated to scripts
            // so after setting the value we need to refresh property widgets.
                if (!prop->IsExternal())
                {
                    PanelManager::Get()->GetPropertiesPanel()->RefreshProperties();
            }
#endif
        }
    }
}

void ActionEditProperty::Reverse()
{
    std::vector<Property> sProps;
    GatherProps(sProps);

    Property* prop = FindProp(sProps, mPropertyName);

    if (prop != nullptr)
    {
        // TODO-NODE: Support undo/redo for AddPropertyArray. Until then, don't set value on OOB elements.
        if (prop->GetCount() > mIndex)
        {
            prop->SetValue(mPreviousValue.GetValue(0), mIndex, 1);
        }

        // TODO-NODE: Delete this? Shouldn't be necessary anymore now that we are using Imgui
        //   and gathering properties every frame.
#if 0
        // Script properties are stored internally and propagated to scripts
        // so after setting the value we need to refresh property widgets.
        if (!prop->IsExternal())
        {
            PanelManager::Get()->GetPropertiesPanel()->RefreshProperties();
        }
#endif
    }
}

ActionEditTransforms::ActionEditTransforms(
    const std::vector<Node3D*>& nodes,
    const std::vector<glm::mat4>& newTransforms)
{
    mNodes = nodes;
    mNewTransforms = newTransforms;

    OCT_ASSERT(mNodes.size() == mNewTransforms.size());
}

void ActionEditTransforms::Execute()
{
    mPrevTransforms.clear();

    for (uint32_t i = 0; i < mNodes.size(); ++i)
    {
        mPrevTransforms.push_back(mNodes[i]->GetTransform());
        mNodes[i]->SetTransform(mNewTransforms[i]);
    }
}

void ActionEditTransforms::Reverse()
{
    OCT_ASSERT(mPrevTransforms.size() == mNodes.size());

    for (uint32_t i = 0; i < mNodes.size(); ++i)
    {
        mNodes[i]->SetTransform(mPrevTransforms[i]);
    }
}

ActionSpawnNodes::ActionSpawnNodes(const std::vector<TypeId>& types)
{
    mSrcTypes = types;
}

ActionSpawnNodes::ActionSpawnNodes(const std::vector<const char*>& typeNames)
{
    mSrcTypeNames = typeNames;
}

ActionSpawnNodes::ActionSpawnNodes(const std::vector<SceneRef>& scenes)
{
    mSrcScenes = scenes;
}

ActionSpawnNodes::ActionSpawnNodes(const std::vector<Node*>& srcNodes)
{
    mSrcNodes = srcNodes;
    RemoveRedundantDescendants(mSrcNodes);
}

void ActionSpawnNodes::Execute()
{
    if (mNodes.size() == 0)
    {
        // First time executing this action. We need to create the nodes from the src data.
        if (mSrcTypes.size() > 0)
        {
            for (uint32_t i = 0; i < mSrcTypes.size(); ++i)
            {
                Node* newNode = Node::Construct(mSrcTypes[i]);
                OCT_ASSERT(newNode);
                mNodes.push_back(newNode);
            }
        }
        else if (mSrcTypeNames.size() > 0)
        {
            for (uint32_t i = 0; i < mSrcTypeNames.size(); ++i)
            {
                Node* newNode = Node::Construct(mSrcTypeNames[i]);
                OCT_ASSERT(newNode);
                mNodes.push_back(newNode);
            }
        }
        else if (mSrcScenes.size() > 0)
        {
            for (uint32_t i = 0; i < mSrcScenes.size(); ++i)
            {
                Scene* scene = mSrcScenes[i].Get<Scene>();
                if (scene != nullptr)
                {
                    Node* newNode = scene->Instantiate();
                    OCT_ASSERT(newNode);
                    mNodes.push_back(newNode);
                }
                else
                {
                    LogError("Null scene in ActionSpawnNodes::Execute()");
                }
            }
        }
        else if (mSrcNodes.size() > 0)
        {
            for (uint32_t i = 0; i < mSrcNodes.size(); ++i)
            {
                OCT_ASSERT(mSrcNodes[i] != nullptr);

                Node* newNode = mSrcNodes[i]->Clone(true);
                OCT_ASSERT(newNode);
                mNodes.push_back(newNode);
            }
        }
        else
        {
            LogError("Invalid src data for ActionSpawnNodes!");
        }
    }
    else 
    {
        // Second time and beyond. Restore exiled nodes and attach to correct parents.
        for (uint32_t i = 0; i < mNodes.size(); ++i)
        {
            ActionManager::Get()->RestoreExiledNode(mNodes[i]);

            if (mParents[i] != nullptr)
            {
                mNodes[i]->Attach(mParents[i]);

                if (mParentLinkedScenes[i] != nullptr)
                {
                    // Restore the scene link.
                    mParents[i]->SetScene(mParentLinkedScenes[i].Get<Scene>());
                }
            }
            else
            {
                // This must have been the root node?
                OCT_ASSERT(mNodes.size() == 1);
                OCT_ASSERT(GetWorld()->GetRootNode() == nullptr);
                GetWorld()->SetRootNode(mNodes[i]);
            }
        }
    }
}

void ActionSpawnNodes::Reverse()
{
    if (mParents.size() == 0)
    {
        OCT_ASSERT(mParentLinkedScenes.size() == 0);

        // First time reversing, track the parents and parent scene-links.
        for (uint32_t i = 0; i < mNodes.size(); ++i)
        {

            Node* parent = mNodes[i]->GetParent();
            Scene* parentLinkedScene = nullptr;

            if (parent != nullptr && parent->IsSceneLinked())
            {
                parentLinkedScene = parent->GetScene();
            }

            mParents.push_back(parent);
            mParentLinkedScenes.push_back(parentLinkedScene);
        }
    }

    for (uint32_t i = 0; i < mNodes.size(); ++i)
    {
        if (mParents[i] != nullptr)
        {
            mNodes[i]->Detach();
        }
        else
        {
            OCT_ASSERT(mNodes.size() == 1);
            OCT_ASSERT(GetWorld()->GetRootNode() == mNodes[i]);
            GetWorld()->SetRootNode(nullptr);
        }

        ActionManager::Get()->ExileNode(mNodes[i]);
    }
}

ActionDeleteNodes::ActionDeleteNodes(const std::vector<Node*>& nodes)
{
    mNodes = nodes;

    RemoveRedundantDescendants(mNodes);

    for (uint32_t i = 0; i < mNodes.size(); ++i)
    {
        mParents.push_back(mNodes[i]->GetParent());
        if (mParents[i] != nullptr)
        {
            int32_t childIdx = mParents[i]->FindChildIndex(mNodes[i]);
            OCT_ASSERT(childIdx != -1);
            mChildIndices.push_back(childIdx);

            Node3D* node3d = mNodes[i]->As<Node3D>();
            if (node3d != nullptr)
            {
                mBoneIndices.push_back(node3d->GetParentBoneIndex());
            }
            else
            {
                mBoneIndices.push_back(-1);
            }
        }
        else
        {
            // This must be the root node being deleted.
            OCT_ASSERT(mNodes.size() == 1);

            mChildIndices.push_back(-1);
            mBoneIndices.push_back(-1);
        }
    }
}

void ActionDeleteNodes::Execute()
{
    for (uint32_t i = 0; i < mNodes.size(); ++i)
    {
        // Actor is already spawned at this point.
        OCT_ASSERT(mNodes[i]->GetWorld() != nullptr);

        if (IsPlayingInEditor())
        {
            mNodes[i]->SetPendingDestroy(true);
        }
        else
        {
            if (mParents[i] != nullptr)
            {
                mNodes[i]->Detach();
            }
            else
            {
                // We must be deleting the root node
                OCT_ASSERT(mNodes.size() == 1);
                OCT_ASSERT(GetWorld()->GetRootNode() == mNodes[i]);
                GetWorld()->SetRootNode(nullptr);
            }

            ActionManager::Get()->ExileNode(mNodes[i]);
        }
    }
}

void ActionDeleteNodes::Reverse()
{
    for (uint32_t i = 0; i < mNodes.size(); ++i)
    {
        ActionManager::Get()->RestoreExiledNode(mNodes[i]);

        if (mParents[i] != nullptr)
        {
            if (mBoneIndices[i] >= 0 &&
                mParents[i]->As<SkeletalMesh3D>() &&
                mNodes[i]->As<Node3D>())
            {
                // This node was attached to a bone!
                mNodes[i]->As<Node3D>()->AttachToBone(
                    mParents[i]->As<SkeletalMesh3D>(),
                    mBoneIndices[i], 
                    false, 
                    mChildIndices[i]);
            }
            else
            {
                // Normal attachment
                mParents[i]->AddChild(mNodes[i], mChildIndices[i]);
            }
            // TODO: Support attaching to the correct bone. 
            // Probably need to add extra parameter to Attach() to include child index.
        }
        else
        {
            // Must have deleted the root node.
            OCT_ASSERT(mNodes.size() == 1);
            OCT_ASSERT(GetWorld()->GetRootNode() == nullptr);
            GetWorld()->SetRootNode(mNodes[i]);
        }
    }
}

ActionAttachNode::ActionAttachNode(Node* node, Node* newParent, int32_t childIndex, int32_t boneIndex)
{
    mNode = node;
    mNewParent = newParent;
    mPrevParent = node->GetParent();
    mChildIndex = childIndex;
    mPrevChildIndex = node->GetParent() ? node->GetParent()->FindChildIndex(node) : -1;
    mBoneIndex = boneIndex;
    mPrevBoneIndex = node->IsNode3D() ? node->As<Node3D>()->GetParentBoneIndex() : -1;
    OCT_ASSERT(mNode);
    OCT_ASSERT(mNewParent);
}

void ActionAttachNode::Execute()
{
    if (mBoneIndex >= 0 &&
        mNewParent != nullptr &&
        mNewParent->As<SkeletalMesh3D>() &&
        mNode->As<Node3D>())
    {
        Node3D* node3d = mNode->As<Node3D>();
        SkeletalMesh3D* skParent = mNewParent->As<SkeletalMesh3D>();

        node3d->AttachToBone(skParent, mBoneIndex, true, mChildIndex);
    }
    else
    {
       mNode->Attach(mNewParent, true, mChildIndex);
    }
}

void ActionAttachNode::Reverse()
{
    if (mPrevBoneIndex >= 0 &&
        mPrevParent != nullptr &&
        mPrevParent->As<SkeletalMesh3D>() &&
        mNode->As<Node3D>())
    {
        Node3D* node3d = mNode->As<Node3D>();
        SkeletalMesh3D* skParent = mPrevParent->As<SkeletalMesh3D>();
        node3d->AttachToBone(skParent, mPrevBoneIndex, true, mPrevChildIndex);
    }
    else
    {
        mNode->Attach(mPrevParent, true, mPrevChildIndex);
    }
}

ActionSetRootNode::ActionSetRootNode(Node* newRoot)
{
    mNewRoot = newRoot;
    mOldRoot = GetWorld()->GetRootNode();
    mNewRootParent = mNewRoot->GetParent();
    mNewRootChildIndex = mNewRootParent ? mNewRootParent->FindChildIndex(mNewRoot) : -1;

    OCT_ASSERT(mNewRoot != mOldRoot);
    OCT_ASSERT(mNewRoot != nullptr);
    OCT_ASSERT(mOldRoot != nullptr);
    OCT_ASSERT(mNewRootParent != nullptr && mNewRootChildIndex != -1);
}

void ActionSetRootNode::Execute()
{
    mNewRoot->Detach(true);
    GetWorld()->SetRootNode(mNewRoot);
    mOldRoot->Attach(mNewRoot, true);
}

void ActionSetRootNode::Reverse()
{
    mOldRoot->Detach(true);
    GetWorld()->SetRootNode(mOldRoot);
    mNewRoot->Attach(mNewRootParent, true, mNewRootChildIndex);
}

ActionSetAbsoluteRotation::ActionSetAbsoluteRotation(Node3D* node, glm::quat rot)
{
    mNode = node;
    mNewRotation = rot;
    mPrevRotation = node->GetAbsoluteRotationQuat();
    OCT_ASSERT(mNode);
}

void ActionSetAbsoluteRotation::Execute()
{
    mNode->SetAbsoluteRotation(mNewRotation);
}

void ActionSetAbsoluteRotation::Reverse()
{
    mNode->SetAbsoluteRotation(mPrevRotation);
}

ActionSetAbsolutePosition::ActionSetAbsolutePosition(Node3D* node, glm::vec3 pos)
{
    mNode = node;
    mNewPosition = pos;
    mPrevPosition = node->GetAbsolutePosition();
    OCT_ASSERT(mNode);
}

void ActionSetAbsolutePosition::Execute()
{
    mNode->SetAbsolutePosition(mNewPosition);
}

void ActionSetAbsolutePosition::Reverse()
{
    mNode->SetAbsolutePosition(mPrevPosition);
}

ActionSetAbsoluteScale::ActionSetAbsoluteScale(Node3D* node, glm::vec3 scale)
{
    mNode = node;
    mNewScale = scale;
    mPrevScale = node->GetAbsoluteScale();
    OCT_ASSERT(mNode);
}

void ActionSetAbsoluteScale::Execute()
{
    mNode->SetAbsoluteScale(mNewScale);
}

void ActionSetAbsoluteScale::Reverse()
{
    mNode->SetAbsoluteScale(mPrevScale);
}

#endif
