#pragma once
#include <QWidget>
#include <QPushButton>
#include <QVector>
#include <QLabel>

namespace railshot {
class EngineController;

class TransitionPanel : public QWidget {
    Q_OBJECT
public:
    explicit TransitionPanel(EngineController* engine, QWidget* parent = nullptr);

private:
    void restyleTypes();
    void refreshScenePad();
    void updateGoArmed();

    EngineController* m_engine = nullptr;
    QPushButton* m_go = nullptr;
    QLabel* m_activeLabel = nullptr;
    QString m_active = QStringLiteral("Cut");
    QVector<QPushButton*> m_typeButtons;
    QVector<QPushButton*> m_scenePad;
};

} // namespace railshot
