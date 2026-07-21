#pragma once
#include <QDialog>
#include <QColor>
#include <QImage>
#include <QJsonObject>

class QLabel;
class QComboBox;
class QPushButton;
class QButtonGroup;
class QFrame;

namespace railshot {
class EngineController;

/// Preset gallery, colors, layout/theme, and sport for the on-air scoreboard.
class ScoreboardSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit ScoreboardSettingsDialog(EngineController* engine, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void refreshPreview();
    void applyColorButton(QPushButton* btn, const QColor& c, const QString& label);
    QColor pickColor(const QColor& current, const QString& title);
    QJsonObject previewState() const;

    EngineController* m_engine = nullptr;
    QLabel* m_preview = nullptr;
    QFrame* m_previewCard = nullptr;
    QButtonGroup* m_presetGroup = nullptr;
    QButtonGroup* m_sportGroup = nullptr;
    QComboBox* m_layoutBox = nullptr;
    QComboBox* m_themeBox = nullptr;
    QPushButton* m_colorABtn = nullptr;
    QPushButton* m_colorBBtn = nullptr;
    QPushButton* m_textColorBtn = nullptr;
    QPushButton* m_bgColorBtn = nullptr;
    QColor m_colorA;
    QColor m_colorB;
    QColor m_textColor;
    QColor m_bgColor;
    bool m_useCustomText = false;
    bool m_useCustomBg = false;
    QImage m_previewImg;
};

} // namespace railshot
