#pragma once

#include "core/Types.h"
#include "capture/FrameBus.h"
#include <QObject>
#include <QImage>
#include <QElapsedTimer>
#include <QSet>
#include <QString>
#include <QHash>
#include <memory>

struct ID3D11Texture2D;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D11SamplerState;
struct ID3D11BlendState;

namespace railshot {

class D3D11Device;
class Project;

class D3D11Compositor : public QObject {
    Q_OBJECT
public:
    explicit D3D11Compositor(D3D11Device* device, QObject* parent = nullptr);
    ~D3D11Compositor() override;

    bool initialize(int width, int height, QString* error = nullptr);
    void resize(int width, int height);
    void shutdown();

    ID3D11Texture2D* previewTexture() const { return m_previewTex; }
    ID3D11Texture2D* programTexture() const { return m_programTex; }

    /// Compose a scene. Pass project for Scene/Group nested resolution.
    bool compose(const SceneItem& scene, FrameBus& bus, bool toProgram,
                 float transitionMix = 1.0f, float clearR = 0.02f, float clearG = 0.02f,
                 float clearB = 0.03f, const Project* project = nullptr);

    bool captureProgramHold();
    void clearProgramHold();
    bool hasProgramHold() const { return m_holdTex != nullptr; }
    void blendProgramHold(float progress, TransitionType type);
    void setWipeDirection(int direction);
    int wipeDirection() const { return m_wipeDirection; }

    /// Fullscreen overlay (stinger) using source alpha.
    void drawFullscreenOverlay(ID3D11Texture2D* tex, float opacity = 1.0f, bool toProgram = true);

    QImage readbackProgram() const;
    QImage readbackPreview() const;
    QImage readbackTexture(ID3D11Texture2D* tex) const;

    int width() const { return m_width; }
    int height() const { return m_height; }

signals:
    void frameComposed(bool program);

private:
    struct MaskEntry {
        ID3D11Texture2D* tex = nullptr;
        ID3D11ShaderResourceView* srv = nullptr;
    };

    bool createTargets(QString* error);
    bool createPipeline(QString* error);
    bool ensureNestTargets();
    void clearTarget(ID3D11RenderTargetView* rtv, float r, float g, float b, float a);
    void drawSource(const SourceItem& src, FrameBus& bus, ID3D11RenderTargetView* rtv);
    void drawTexture(ID3D11Texture2D* tex, const Transform& xf, float opacityMul, ID3D11RenderTargetView* rtv);
    void drawSceneOrGroup(const SourceItem& src, FrameBus& bus, ID3D11RenderTargetView* rtv);
    Transform combineTransform(const Transform& parent, const Transform& child) const;
    QSet<QString> groupedChildIds(const SceneItem& scene) const;
    ID3D11ShaderResourceView* ensureMaskSrv(const QString& path);
    void clearMaskCache();

    D3D11Device* m_device = nullptr;
    int m_width = 1920;
    int m_height = 1080;
    const Project* m_project = nullptr;
    int m_nestDepth = 0;

    ID3D11Texture2D* m_previewTex = nullptr;
    ID3D11Texture2D* m_programTex = nullptr;
    ID3D11Texture2D* m_holdTex = nullptr;
    ID3D11Texture2D* m_transScratch = nullptr;
    ID3D11Texture2D* m_nestTex = nullptr;
    ID3D11RenderTargetView* m_previewRtv = nullptr;
    ID3D11RenderTargetView* m_programRtv = nullptr;
    ID3D11RenderTargetView* m_nestRtv = nullptr;

    ID3D11VertexShader* m_vs = nullptr;
    ID3D11PixelShader* m_ps = nullptr;
    ID3D11PixelShader* m_transPs = nullptr;
    ID3D11InputLayout* m_layout = nullptr;
    ID3D11Buffer* m_vb = nullptr;
    ID3D11Buffer* m_cb = nullptr;
    ID3D11Buffer* m_transCb = nullptr;
    ID3D11SamplerState* m_sampler = nullptr;
    ID3D11BlendState* m_blend = nullptr;
    ID3D11BlendState* m_blendOpaque = nullptr;
    int m_wipeDirection = 0;
    QElapsedTimer m_fxClock;
    QHash<QString, MaskEntry> m_maskCache;
};

} // namespace railshot
