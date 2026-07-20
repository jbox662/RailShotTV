#pragma once

#include <QDialog>
#include <QString>

class QListWidget;
class QCheckBox;
class QSlider;
class QLabel;
class QStackedWidget;

namespace railshot {
class EngineController;

/// OBS-style Filters dialog: filter stack + chroma/blur settings wired to compositor.
class FiltersDialog : public QDialog {
    Q_OBJECT
public:
    FiltersDialog(EngineController* engine, const QString& sourceId, QWidget* parent = nullptr);

private:
    void reload();
    void saveCurrent();
    void syncLegacyKeys();
    void onAddFilter();
    void onRemoveFilter();
    void onSelectionChanged();

    EngineController* m_engine = nullptr;
    QString m_sourceId;
    QListWidget* m_list = nullptr;
    QStackedWidget* m_stack = nullptr;
    QWidget* m_chromaPage = nullptr;
    QWidget* m_blurPage = nullptr;
    QWidget* m_emptyPage = nullptr;
    QCheckBox* m_chromaEnabled = nullptr;
    QSlider* m_chromaSim = nullptr;
    QCheckBox* m_blurEnabled = nullptr;
    QSlider* m_blurAmount = nullptr;
    QLabel* m_hint = nullptr;
    bool m_loading = false;
};

} // namespace railshot
