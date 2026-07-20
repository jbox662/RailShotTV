#pragma once
#include <QObject>
#include <QKeySequence>
#include <QHash>

class QKeyEvent;

namespace railshot {
class EngineController;

/// Maps SettingsStore hotkeys to engine/live-ops actions.
class HotkeyDispatcher : public QObject {
    Q_OBJECT
public:
    explicit HotkeyDispatcher(EngineController* engine, QObject* parent = nullptr);
    void reload();
    bool handleKey(QKeyEvent* event);

    static QKeySequence sequenceFor(const QString& binding);

private:
    void dispatch(const QString& action);

    EngineController* m_engine = nullptr;
    QHash<QString, QString> m_seqToAction; // normalized sequence -> action id
};

} // namespace railshot
