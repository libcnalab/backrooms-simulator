#pragma once

#include "World.hpp"
#include "Materials.hpp"

#include <array>
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffectInstance.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"

#include "CnaEngine/Diagnostics/FrameStatistics.hpp"
#include "CnaEngine/Graphics/Camera.hpp"
#include "CnaEngine/Graphics/ForwardRenderer.hpp"
#include "CnaEngine/Graphics/GpuMesh.hpp"
#include "CnaEngine/Graphics/Material.hpp"
#include "CnaEngine/Graphics/RenderQueue.hpp"
#include "CnaEngine/Graphics/ShadowMap.hpp"
#include "CnaEngine/Input/ActionMap.hpp"

namespace Backrooms {

class BackroomsGame final : public Microsoft::Xna::Framework::Game {
public:
    static constexpr double kDefaultWalkSpeed=2.4;
    static constexpr double kDefaultRunSpeed=4.8;
    static constexpr float kDefaultVerticalFov=60.0f;

    explicit BackroomsGame(std::uint64_t seed, bool streamTest = false,
                           int startLevel = 0, double startX = 2.5,
                           double startZ = 2.5, double walkSpeed = kDefaultWalkSpeed,
                           double runSpeed = kDefaultRunSpeed,
                           double streamTestMetres = 9600.0,
                           float verticalFovDegrees = kDefaultVerticalFov,
                           int multiSampleCount = 4, int streamTestLoops = 1);
    ~BackroomsGame() override;
    // Deterministic capture: once the 5 by 5 neighborhood is loaded, draw one
    // frame with creatures posed at `seconds`, save it as PNG and exit. No
    // input, mouse capture or wall-clock time reaches the frame.
    void RequestCapture(std::string path, float seconds);
    void SetView(float yawDegrees, float pitchDegrees);
    // Creatures' shadows from the ceiling light (on unless switched off here
    // or in the settings; devices that cannot sample a shadow map keep the
    // contact spots).
    void SetShadows(bool enabled) { shadows_=enabled; }
    const std::string& GetTypeName() const override;
    void Initialize() override;
    void LoadContent() override;
    void Update(Microsoft::Xna::Framework::GameTime& time) override;
    void Draw(const Microsoft::Xna::Framework::GameTime& time) override;

private:
    struct Chunk {
        std::array<std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer>,
                   kMaterialCount> vertices;
        std::array<int, kMaterialCount> materialTriangles{};
        // The used part of each pooled buffer, as the engine draws it.
        std::array<CnaEngine::Graphics::GpuMesh, kMaterialCount> meshes;
        std::vector<EntitySpawn> entities;
        Microsoft::Xna::Framework::BoundingBox bounds;
        int triangles = 0;
        double buildMs = 0;
    };

    Microsoft::Xna::Framework::GraphicsDeviceManager graphics_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> effect_;
    std::unique_ptr<Materials> materials_;
    // Chunks are drawn by cna-engine's forward renderer; creatures stay a game pass.
    std::unique_ptr<CnaEngine::Graphics::ForwardRenderer> renderer_;
    std::array<CnaEngine::Graphics::Material, kMaterialCount> chunkMaterials_;
    CnaEngine::Graphics::RenderQueue queue_;
    CnaEngine::Graphics::Camera camera_;
    struct EntityMesh {
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> vertices;
        int triangles=0;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer> shadowVertices;
        int shadowTriangles=0;
        // The body as the engine's mesh, for the shadow map.
        CnaEngine::Graphics::GpuMesh gpu;
    };
    std::array<EntityMesh,kEntityKindCount> entityMeshes_;
    // This frame's creatures in view range, posed; placed before the shadow
    // map is drawn, which has to happen before the frame is cleared.
    struct PlacedCreature {
        const EntityMesh* model=nullptr;
        Microsoft::Xna::Framework::Matrix world;
        float opacity=1;
        double distance=0;
        bool casts=false;
    };
    std::vector<PlacedCreature> creatures_;
    std::unique_ptr<CnaEngine::Graphics::ShadowMap> shadowMap_;
    std::vector<CnaEngine::Graphics::RenderItem> casters_;
    CnaEngine::Graphics::Material casterMaterial_;
    bool shadows_ = true;
    Microsoft::Xna::Framework::Graphics::BlendState entityDepthOnlyBlend_;
    double nearestEntityDistance_ = 0;
    float nearestEntityOpacity_ = 0;
    std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> humSound_;
    std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffectInstance> hum_;
    std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> step_;
    std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> carpetStep_;
    std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> transition_;
    std::array<std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect>,
               kEntityKindCount> entitySounds_;
    double entitySoundDelay_=5;
    unsigned entitySoundCount_=0;
    bool entitySoundWarningShown_=false;
    std::map<ChunkCoord, Chunk> chunks_;
    std::vector<std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer>> spareVertices_;
    LevelCatalog levelCatalog_;
    RoomLayoutCache roomLayouts_;
    WorldConfig world_;
    // Keys and buttons as engine actions and axes; mouse look reads the
    // relative motion CNA reports directly.
    CnaEngine::Input::ActionMap controls_;
    CnaEngine::Input::AxisId moveAxis_ = 0, strafeAxis_ = 0;
    CnaEngine::Input::ActionId runToggle_ = 0, releaseMouse_ = 0, respawn_ = 0, recapture_ = 0;
    double x_ = 2.5, z_ = 2.5;
    float yaw_ = 1.5707963f, pitch_ = 0;
    float verticalFovDegrees_ = kDefaultVerticalFov;
    bool captured_ = false;
    bool running_ = false;
    double walkSpeed_ = kDefaultWalkSpeed;
    double runSpeed_ = kDefaultRunSpeed;
    bool insidePortal_ = false;
    int updateCount_ = 0;
    // Start-to-start draw intervals over the last 120 frames.
    CnaEngine::Diagnostics::FrameStatistics drawIntervals_{120};
    std::chrono::steady_clock::time_point lastDraw_{};
    double statsTime_ = 0;
    int drawnChunks_ = 0, drawnTriangles_ = 0;
    double drawWorkMs_ = 0;
    double lastBuildMs_ = 0;
    double peakBuildMs_ = 0;
    double stepDistance_ = 0;
    unsigned stepCount_ = 0;
    bool stepWarningShown_ = false;
    bool streamTest_ = false;
    double streamTestMetres_ = 9600.0;
    int streamTestLoops_ = 1;
    double streamTestTime_ = 0;
    int bufferCreations_ = 0;
    int bufferReuses_ = 0;
    std::string capturePath_;
    float captureSeconds_ = 0;
    bool captureDone_ = false;

    void BuildChunk(ChunkCoord coord);
    void BuildEntityMesh();
    void PlaceCreatures(float seconds);
    bool RenderShadowMap();
    void UpdateEntityAudio(float seconds,double elapsed);
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::VertexBuffer>
        AcquireBuffer(int vertexCount);
    void RetireChunk(Chunk& chunk);
    void Stream();
    void Transition(int level);
    void UpdateTitle(double elapsed);
    void RecordDrawInterval();
    void SaveCapture();
};

} // namespace Backrooms
