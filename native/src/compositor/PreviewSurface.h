#pragma once

#include <QWidget>
#include <QImage>

struct ID3D11Texture2D;
struct IDXGISwapChain1;
struct ID3D11RenderTargetView;

namespace railshot {

class D3D11Device;

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

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    QPaintEngine* paintEngine() const override { return nullptr; }

private:
    bool ensureSwapChain(unsigned width, unsigned height);
    void releaseSwapChain();

    D3D11Device* m_device = nullptr;
    IDXGISwapChain1* m_swap = nullptr;
    ID3D11RenderTargetView* m_rtv = nullptr;
    unsigned m_swapW = 0;
    unsigned m_swapH = 0;
    QString m_label;
    QColor m_labelColor{0x22, 0xC5, 0x5E};
    QString m_empty = QStringLiteral("No Signal");
    bool m_hasFrame = false;
};

} // namespace railshot
