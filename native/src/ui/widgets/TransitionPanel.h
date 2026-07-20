#pragma once
#include <QWidget>
#include <QPushButton>

namespace railshot {
class EngineController;

class TransitionPanel : public QWidget {
    Q_OBJECT
public:
    explicit TransitionPanel(EngineController* engine, QWidget* parent = nullptr);
private:
    EngineController* m_engine = nullptr;
    QPushButton* m_go = nullptr;
    QString m_active = QStringLiteral("Cut");
};

} // namespace railshot
