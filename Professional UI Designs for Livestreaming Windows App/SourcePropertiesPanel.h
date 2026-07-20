#pragma once
// SourcePropertiesPanel — dynamic properties panel for the Scene Editor.
//
// Displays and edits the live settings of a selected OBS source item:
//   • Header: source name, type label, visibility toggle, lock toggle
//   • Transform section: Position X/Y, Scale X/Y, Rotation, Opacity
//   • Crop section: Top / Right / Bottom / Left
//   • Source Settings section: key→value rows from obs_source_get_settings()
//     (strings → QLineEdit, booleans → QCheckBox, numbers → QDoubleSpinBox)
//   • Audio section (shown only for audio-capable sources): Volume slider
//
// Usage:
//   panel->loadSource(itemId);   // call when selection changes
//   panel->clearSource();        // call when nothing is selected
//
// All edits are applied immediately via OBSSourceManager.

#include <QWidget>
#include <QScrollArea>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMap>
#include <QString>

class SourcePropertiesPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SourcePropertiesPanel(QWidget *parent = nullptr);

    /** Load settings for the source item with the given scene-item ID. */
    void loadSource(int itemId);

    /** Clear the panel (show "No source selected" placeholder). */
    void clearSource();

private slots:
    void onPosXChanged(int v);
    void onPosYChanged(int v);
    void onScaleXChanged(double v);
    void onScaleYChanged(double v);
    void onRotationChanged(double v);
    void onOpacityChanged(int v);
    void onCropChanged();
    void onVisibilityToggled(bool v);
    void onLockToggled(bool v);
    void onVolumeChanged(int v);
    void onSettingStringChanged(const QString &key, const QString &value);
    void onSettingBoolChanged(const QString &key, bool value);
    void onSettingNumberChanged(const QString &key, double value);

private:
    void buildUi();
    void buildHeader(QVBoxLayout *root);
    void buildTransformSection(QVBoxLayout *root);
    void buildCropSection(QVBoxLayout *root);
    void buildAudioSection(QVBoxLayout *root);
    void buildSettingsSection(QVBoxLayout *root);

    void populateTransform();
    void populateCrop();
    void populateAudio();
    void populateSettings();
    void rebuildSettingsRows();

    QWidget *sectionHeader(const QString &title, const QString &accentColor);
    QFrame  *hSep();

    // ── State ─────────────────────────────────────────────────────────────────
    int  m_itemId = -1;   // currently selected scene-item ID (-1 = none)

    // ── Header widgets ────────────────────────────────────────────────────────
    QLabel      *m_nameLabel    = nullptr;
    QLabel      *m_typeLabel    = nullptr;
    QCheckBox   *m_visCheck     = nullptr;
    QCheckBox   *m_lockCheck    = nullptr;

    // ── Transform widgets ─────────────────────────────────────────────────────
    QSpinBox       *m_posX      = nullptr;
    QSpinBox       *m_posY      = nullptr;
    QDoubleSpinBox *m_scaleX    = nullptr;
    QDoubleSpinBox *m_scaleY    = nullptr;
    QDoubleSpinBox *m_rotation  = nullptr;
    QSlider        *m_opacity   = nullptr;
    QLabel         *m_opacityLbl= nullptr;

    // ── Crop widgets ──────────────────────────────────────────────────────────
    QSpinBox *m_cropTop    = nullptr;
    QSpinBox *m_cropRight  = nullptr;
    QSpinBox *m_cropBottom = nullptr;
    QSpinBox *m_cropLeft   = nullptr;

    // ── Audio widgets ─────────────────────────────────────────────────────────
    QSlider *m_volumeSlider = nullptr;
    QLabel  *m_volumeLabel  = nullptr;
    QWidget *m_audioSection = nullptr;

    // ── Dynamic settings section ──────────────────────────────────────────────
    QWidget    *m_settingsSection  = nullptr;
    QVBoxLayout*m_settingsLayout   = nullptr;

    // ── Placeholder ───────────────────────────────────────────────────────────
    QWidget *m_placeholder = nullptr;
    QWidget *m_content     = nullptr;
};
