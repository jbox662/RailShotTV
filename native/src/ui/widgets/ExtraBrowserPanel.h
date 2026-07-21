#pragma once

#include <QWidget>
#include <QString>

class QLineEdit;
class QTextBrowser;
class QLabel;

namespace railshot {
class EngineController;

/// OBS-style custom browser dock panel (lightweight: QTextBrowser + system browser).
class ExtraBrowserPanel : public QWidget {
    Q_OBJECT
public:
    ExtraBrowserPanel(EngineController* engine, const QString& panelId,
                      const QString& title, const QString& url, QWidget* parent = nullptr);

    QString panelId() const { return m_id; }
    QString titleText() const { return m_title; }
    QString currentUrl() const;

signals:
    void urlChanged(const QString& panelId, const QString& url);
    void titleEditRequested(const QString& panelId);
    void removeRequested(const QString& panelId);

public slots:
    void navigateTo(const QString& url);
    void reload();

private:
    void navigate();
    void openExternal();
    void addAsSource();

    EngineController* m_engine = nullptr;
    QString m_id;
    QString m_title;
    QLineEdit* m_url = nullptr;
    QTextBrowser* m_view = nullptr;
    QLabel* m_hint = nullptr;
};

} // namespace railshot
