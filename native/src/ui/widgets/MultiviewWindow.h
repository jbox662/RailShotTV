#pragma once

#include <QMainWindow>
#include <QString>

class QTimer;
class QGridLayout;
class QEvent;

namespace railshot {
class EngineController;
class PreviewSurface;

/// OBS-style Multiview: scene grid + live Preview/Program cells.
class MultiviewWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MultiviewWindow(EngineController* engine, QWidget* parent = nullptr);
    ~MultiviewWindow() override;

    static MultiviewWindow* openWindowed(EngineController* engine, QWidget* parent = nullptr);
    static MultiviewWindow* openFullscreen(EngineController* engine, QWidget* anchor = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void tick();
    void rebuildSceneGrid();

private:
    EngineController* m_engine = nullptr;
    PreviewSurface* m_preview = nullptr;
    PreviewSurface* m_program = nullptr;
    QWidget* m_gridHost = nullptr;
    QGridLayout* m_grid = nullptr;
    QTimer* m_timer = nullptr;
    bool m_fullscreen = false;
    QString m_lastPreviewId;
    QString m_lastProgramId;
    int m_lastSceneCount = -1;
};

} // namespace railshot
