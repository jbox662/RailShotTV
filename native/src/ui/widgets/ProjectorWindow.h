#pragma once

#include <QMainWindow>
#include <QString>

class QTimer;
class QLabel;

namespace railshot {
class EngineController;
class PreviewSurface;

enum class ProjectorKind {
    Preview,
    Program,
};

/// OBS-style projector: floating/fullscreen window showing Preview or Program.
class ProjectorWindow : public QMainWindow {
    Q_OBJECT
public:
    ProjectorWindow(EngineController* engine, ProjectorKind kind, QWidget* parent = nullptr);
    ~ProjectorWindow() override;

    ProjectorKind kind() const { return m_kind; }
    void setAlwaysOnTop(bool on);
    bool alwaysOnTop() const { return m_alwaysOnTop; }

    /// Open windowed projector (non-modal, independent lifetime).
    static ProjectorWindow* openWindowed(EngineController* engine, ProjectorKind kind, QWidget* parent = nullptr);
    /// Open fullscreen on the screen that contains `anchor` (or primary).
    static ProjectorWindow* openFullscreen(EngineController* engine, ProjectorKind kind,
                                           QWidget* anchor = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void tick();

private:
    void rebuildTitle();

    EngineController* m_engine = nullptr;
    ProjectorKind m_kind = ProjectorKind::Preview;
    PreviewSurface* m_surface = nullptr;
    QTimer* m_timer = nullptr;
    bool m_alwaysOnTop = false;
    bool m_fullscreen = false;
};

} // namespace railshot
