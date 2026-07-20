#pragma once

#include "core/Types.h"
#include "capture/FrameBus.h"
#include <QObject>
#include <QImage>
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

    /// Compose a scene into the given bus (preview or program).
    bool compose(const SceneItem& scene, FrameBus& bus, bool toProgram,
                 float transitionMix = 1.0f);

    /// Readback program frame for encoder (BGRA). Avoid in steady UI path.
    QImage readbackProgram() const;

    int width() const { return m_width; }
    int height() const { return m_height; }

signals:
    void frameComposed(bool program);

private:
    bool createTargets(QString* error);
    bool createPipeline(QString* error);
    void clearTarget(ID3D11RenderTargetView* rtv, float r, float g, float b, float a);
    void drawSource(const SourceItem& src, FrameBus& bus, ID3D11RenderTargetView* rtv);

    D3D11Device* m_device = nullptr;
    int m_width = 1920;
    int m_height = 1080;

    ID3D11Texture2D* m_previewTex = nullptr;
    ID3D11Texture2D* m_programTex = nullptr;
    ID3D11RenderTargetView* m_previewRtv = nullptr;
    ID3D11RenderTargetView* m_programRtv = nullptr;

    ID3D11VertexShader* m_vs = nullptr;
    ID3D11PixelShader* m_ps = nullptr;
    ID3D11InputLayout* m_layout = nullptr;
    ID3D11Buffer* m_vb = nullptr;
    ID3D11Buffer* m_cb = nullptr;
    ID3D11SamplerState* m_sampler = nullptr;
    ID3D11BlendState* m_blend = nullptr;
};

} // namespace railshot
