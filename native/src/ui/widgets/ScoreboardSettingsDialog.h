#pragma once
#include <QDialog>
#include <QColor>
#include <QImage>
#include <QJsonObject>
#include <QVector>

class QLabel;
class QComboBox;
class QPushButton;
class QButtonGroup;
class QFrame;
class QScrollArea;

namespace railshot {
class EngineController;

/// Preset gallery, colors, layout/theme, and sport for the on-air scoreboard.
class ScoreboardSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit ScoreboardSettingsDialog(EngineController* engine, QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void refreshPreview();
    void paintPreviewLabel();
    void rescalePresetThumbs();
    void setSelectedPreset(int index);
    void styleSwatch(QPushButton* btn, const QColor& c);
    void syncHexLabels();
    QColor pickColor(const QColor& current, const QString& title);
    QJsonObject previewState() const;

    EngineController* m_engine = nullptr;
    QLabel* m_preview = nullptr;
    QScrollArea* m_presetScroll = nullptr;
    QButtonGroup* m_sportGroup = nullptr;
    QComboBox* m_layoutBox = nullptr;
    QComboBox* m_themeBox = nullptr;
    QPushButton* m_colorABtn = nullptr;
    QPushButton* m_colorBBtn = nullptr;
    QPushButton* m_textColorBtn = nullptr;
    QPushButton* m_bgColorBtn = nullptr;
    QLabel* m_colorAHex = nullptr;
    QLabel* m_colorBHex = nullptr;
    QLabel* m_textHex = nullptr;
    QLabel* m_bgHex = nullptr;
    QVector<QFrame*> m_presetCards;
    QVector<QLabel*> m_presetThumbs;
    int m_selectedPreset = -1;
    QColor m_colorA;
    QColor m_colorB;
    QColor m_textColor;
    QColor m_bgColor;
    bool m_useCustomText = false;
    bool m_useCustomBg = false;
    QImage m_previewImg;
};

} // namespace railshot
