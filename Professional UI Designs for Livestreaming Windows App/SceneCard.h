#pragma once
#include <QWidget>
#include <QString>

/**
 * SceneCard
 *
 * A 100×70px clickable card representing a single scene in the scene switcher.
 *
 * Visual spec:
 *   - Background:  #1E2640  (inactive) / rgba(79,158,255,0.09) (active)
 *   - Border:      1px #2A3350 (inactive) / 1px rgba(79,158,255,0.31) (active)
 *   - Border-radius: 6px
 *   - Thumbnail area: 100×52px — filled with a QPixmap preview or a grey placeholder
 *   - Scene name: 11px DM Sans, bottom-aligned, centered
 *   - LIVE badge: 18×12px orange pill in top-left corner (only when active)
 *
 * Emits clicked(int sceneIndex) when clicked.
 */
class SceneCard : public QWidget
{
    Q_OBJECT

public:
    explicit SceneCard(int index, const QString &name, QWidget *parent = nullptr);

    void setActive(bool active);
    void setThumbnail(const QPixmap &pixmap);
    void setName(const QString &name);

    bool isActive() const { return m_active; }
    int  sceneIndex() const { return m_index; }

    QSize sizeHint() const override { return QSize(100, 70); }
    QSize minimumSizeHint() const override { return QSize(80, 56); }

signals:
    void clicked(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    int     m_index  = 0;
    QString m_name;
    QPixmap m_thumbnail;
    bool    m_active  = false;
    bool    m_hovered = false;
};
