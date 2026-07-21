#pragma once

#include <QWidget>
#include <QString>

class QPushButton;
class QLabel;

namespace railshot {
class EngineController;

/// OBS-style context / source toolbar under Preview (type-specific quick actions).
class SourceContextToolbar : public QWidget {
    Q_OBJECT
public:
    explicit SourceContextToolbar(EngineController* engine, QWidget* parent = nullptr);

public slots:
    void setSourceId(const QString& sourceId);

signals:
    void filtersRequested(const QString& sourceId);
    void transformRequested(const QString& sourceId);
    void propertiesRequested(const QString& sourceId);
    void interactRequested(const QString& sourceId);

private:
    void rebuild();
    QPushButton* addBtn(const QString& text, const QString& tip);

    EngineController* m_engine = nullptr;
    QString m_sourceId;
    QLabel* m_title = nullptr;
    QWidget* m_actions = nullptr;
};

} // namespace railshot
