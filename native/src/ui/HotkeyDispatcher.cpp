#include "ui/HotkeyDispatcher.h"
#include "core/EngineController.h"
#include "scoreboard/ScoreboardModel.h"
#include "audio/AudioGraph.h"
#include <QKeyEvent>
#include <QJsonObject>
#include <utility>

namespace railshot {

HotkeyDispatcher::HotkeyDispatcher(EngineController* engine, QObject* parent)
    : QObject(parent), m_engine(engine)
{
    reload();
    if (engine && engine->settings()) {
        connect(engine->settings(), &SettingsStore::settingsChanged, this, &HotkeyDispatcher::reload);
    }
}

QKeySequence HotkeyDispatcher::sequenceFor(const QString& binding)
{
    if (binding.isEmpty()) return {};
    if (binding.compare(QStringLiteral("Space"), Qt::CaseInsensitive) == 0)
        return QKeySequence(Qt::Key_Space);
    if (binding.compare(QStringLiteral("Return"), Qt::CaseInsensitive) == 0
        || binding.compare(QStringLiteral("Enter"), Qt::CaseInsensitive) == 0)
        return QKeySequence(Qt::Key_Return);
    return QKeySequence(binding);
}

void HotkeyDispatcher::reload()
{
    m_seqToAction.clear();
    if (!m_engine || !m_engine->settings()) return;
    const QJsonObject keys = m_engine->settings()->hotkeys();
    for (auto it = keys.begin(); it != keys.end(); ++it) {
        const QKeySequence seq = sequenceFor(it.value().toString());
        if (!seq.isEmpty())
            m_seqToAction.insert(seq.toString(QKeySequence::PortableText), it.key());
    }
}

bool HotkeyDispatcher::handleKey(QKeyEvent* event)
{
    if (!event || event->isAutoRepeat()) return false;
    // Don't steal keys from line edits etc. — MainWindow should only call when appropriate.
    const QKeySequence seq(event->key() | event->modifiers());
    const QString key = seq.toString(QKeySequence::PortableText);
    const auto it = m_seqToAction.constFind(key);
    if (it == m_seqToAction.cend()) {
        // Try key-only without modifiers for digit scenes
        const QKeySequence bare(event->key());
        const auto it2 = m_seqToAction.constFind(bare.toString(QKeySequence::PortableText));
        if (it2 == m_seqToAction.cend()) return false;
        dispatch(*it2);
        return true;
    }
    dispatch(*it);
    return true;
}

void HotkeyDispatcher::dispatch(const QString& action)
{
    if (!m_engine) return;
    if (action == QLatin1String("go")) {
        m_engine->go();
        return;
    }
    if (action.startsWith(QLatin1String("scene"))) {
        bool ok = false;
        const int idx = action.mid(5).toInt(&ok) - 1;
        if (!ok || idx < 0) return;
        auto p = m_engine->projectSnapshot();
        if (idx < p.scenes.size())
            m_engine->setPreviewScene(p.scenes[idx].id);
        return;
    }
    if (action == QLatin1String("streamToggle")) {
        if (m_engine->telemetrySnapshot().streaming)
            m_engine->stopStreaming();
        else {
            const auto p = m_engine->projectSnapshot();
            if (!p.streamTargets.isEmpty()) {
                QString err;
                m_engine->startStreaming(p.streamTargets.first().id, &err);
            }
        }
        return;
    }
    if (action == QLatin1String("recordToggle")) {
        if (m_engine->telemetrySnapshot().recording)
            m_engine->stopRecording();
        else {
            QString err;
            m_engine->startRecording(&err);
        }
        return;
    }
    if (action == QLatin1String("saveReplay")) {
        QString err;
        m_engine->saveReplay(&err);
        return;
    }
    if (action == QLatin1String("muteMic")) {
        if (!m_engine->audio()) return;
        auto st = m_engine->audio()->channelState(QStringLiteral("mic"));
        if (st.id.isEmpty()) {
            const auto chans = m_engine->audio()->channels();
            for (const auto& c : chans) {
                if (c.name.contains(QStringLiteral("Mic"), Qt::CaseInsensitive)
                    || c.id.contains(QStringLiteral("mic"), Qt::CaseInsensitive)) {
                    st = c;
                    break;
                }
            }
        }
        if (!st.id.isEmpty()) {
            st.muted = !st.muted;
            m_engine->audio()->setChannelState(st.id, st);
        }
        return;
    }
    auto* sb = m_engine->scoreboard();
    if (!sb) return;
    if (action == QLatin1String("scoreAPlus")) sb->incrementA(1);
    else if (action == QLatin1String("scoreAMinus")) sb->incrementA(-1);
    else if (action == QLatin1String("scoreBPlus")) sb->incrementB(1);
    else if (action == QLatin1String("scoreBMinus")) sb->incrementB(-1);
    else if (action == QLatin1String("scoreReset")) sb->resetScores();
    else if (action == QLatin1String("scoreSwap")) {
        auto s = sb->state();
        std::swap(s.playerA, s.playerB);
        std::swap(s.scoreA, s.scoreB);
        sb->setState(s);
    }
}

} // namespace railshot
