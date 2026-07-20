#pragma once

#include <QWidget>
#include <QImage>
#include <QRectF>
#include <QColor>

struct ID3D11Texture2D;
struct IDXGISwapChain1;
struct ID3D11RenderTargetView;

namespace railshot {

class D3D11Device;

/// Selection / transform chrome drawn on the preview swap chain (canvas space).
struct PreviewEditChrome {
    bool visible = false;
    bool cropping = false;   // Alt-crop active — emphasize crop edges
    QRectF rect;             // normalized 0..1 canvas rect (axis-aligned bounds)
    double rotationDeg = 0.0;
    double cropLeft = 0.0;
    double cropRight = 0.0;
    double cropTop = 0.0;
    double cropBottom = 0.0;
    QColor color{0x22, 0xC5, 0x5E};
};

/// Qt widget that presents a D3D11 texture via DXGI swap chain (HWND).
class PreviewSurface : public QWidget {
    Q_OBJECT
public:
    explicit PreviewSurface(QWidget* parent = nullptr);
    ~PreviewSurface() override;

    void setDevice(D3D11Device* device);
    void presentTexture(ID3D11Texture2D* texture);
    void setLabel(const QString& label, const QColor& color);
    void setEmptyMessage(const QString& msg);
    void setEditChrome(const PreviewEditChrome& chrome);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    QPaintEngine* paintEngine() const override { return nullptr; }

private:
    bool ensureSwapChain(unsigned width, unsigned height);
    void releaseSwapChain();
    void drawEditChrome();

    D3D11Device* m_device = nullptr;
    IDXGISwapChain1* m_swap = nullptr;
    ID3D11RenderTargetView* m_rtv = nullptr;
    unsigned m_swapW = 0;
    unsigned m_swapH = 0;
    QString m_label;
    QColor m_labelColor{0x22, 0xC5, 0x5E};
    QString m_empty = QStringLiteral("No Signal");
    bool m_hasFrame = false;
    PreviewEditChrome m_chrome;
};

} // namespace railshot
