#pragma once

#include <QDialog>
#include <QString>
#include <QJsonArray>
#include <QColor>

class QListWidget;
class QCheckBox;
class QSlider;
class QLabel;
class QStackedWidget;
class QPushButton;
class QComboBox;
class QLineEdit;

namespace railshot {
class EngineController;

/// OBS-style Filters dialog: video effect stack + copy/paste.
class FiltersDialog : public QDialog {
    Q_OBJECT
public:
    FiltersDialog(EngineController* engine, const QString& sourceId, QWidget* parent = nullptr);

    static void copyFilters(const QJsonArray& filters);
    static bool hasFilterClipboard();
    static QJsonArray filterClipboard();
    static void pasteOnto(EngineController* engine, const QString& sourceId);

private:
    void reload();
    void saveCurrent();
    void syncLegacyKeys();
    void onAddFilter();
    void onRemoveFilter();
    void onMoveUp();
    void onMoveDown();
    void onSelectionChanged();
    void moveFilter(int delta);
    void pickKeyColor();
    void updateKeyColorButton();

    EngineController* m_engine = nullptr;
    QString m_sourceId;
    QListWidget* m_list = nullptr;
    QStackedWidget* m_stack = nullptr;
    QWidget* m_chromaPage = nullptr;
    QWidget* m_colorKeyPage = nullptr;
    QWidget* m_lumaPage = nullptr;
    QWidget* m_maskPage = nullptr;
    QWidget* m_lutPage = nullptr;
    QWidget* m_blurPage = nullptr;
    QWidget* m_colorPage = nullptr;
    QWidget* m_cropPage = nullptr;
    QWidget* m_scrollPage = nullptr;
    QWidget* m_sharpenPage = nullptr;
    QWidget* m_emptyPage = nullptr;

    QCheckBox* m_chromaEnabled = nullptr;
    QComboBox* m_chromaType = nullptr;
    QPushButton* m_chromaColorBtn = nullptr;
    QSlider* m_chromaSim = nullptr;
    QSlider* m_chromaSmooth = nullptr;
    QColor m_chromaCustomColor{0, 255, 0};

    QCheckBox* m_colorKeyEnabled = nullptr;
    QComboBox* m_colorKeyType = nullptr;
    QPushButton* m_colorKeyColorBtn = nullptr;
    QSlider* m_colorKeySim = nullptr;
    QSlider* m_colorKeySmooth = nullptr;
    QColor m_colorKeyCustomColor{0, 255, 0};

    QCheckBox* m_lumaEnabled = nullptr;
    QSlider* m_lumaMin = nullptr;
    QSlider* m_lumaMax = nullptr;
    QSlider* m_lumaSmooth = nullptr;

    QCheckBox* m_maskEnabled = nullptr;
    QLineEdit* m_maskPath = nullptr;
    QSlider* m_maskOpacity = nullptr;
    QCheckBox* m_maskInvert = nullptr;

    QCheckBox* m_lutEnabled = nullptr;
    QLineEdit* m_lutPath = nullptr;
    QSlider* m_lutAmount = nullptr;

    QCheckBox* m_blurEnabled = nullptr;
    QSlider* m_blurAmount = nullptr;
    QCheckBox* m_colorEnabled = nullptr;
    QSlider* m_brightness = nullptr;
    QSlider* m_contrast = nullptr;
    QSlider* m_saturation = nullptr;
    QCheckBox* m_cropEnabled = nullptr;
    QSlider* m_cropL = nullptr;
    QSlider* m_cropR = nullptr;
    QSlider* m_cropT = nullptr;
    QSlider* m_cropB = nullptr;
    QCheckBox* m_cropPad = nullptr;
    QCheckBox* m_scrollEnabled = nullptr;
    QSlider* m_scrollX = nullptr;
    QSlider* m_scrollY = nullptr;
    QCheckBox* m_sharpenEnabled = nullptr;
    QSlider* m_sharpenAmount = nullptr;
    QLabel* m_hint = nullptr;
    QPushButton* m_upBtn = nullptr;
    QPushButton* m_downBtn = nullptr;
    bool m_loading = false;
    /// Which key color button is being edited: "chroma" or "color_key"
    QString m_pickingKeyFor;

    static QJsonArray s_clipboard;
    static bool s_hasClipboard;
};

} // namespace railshot
