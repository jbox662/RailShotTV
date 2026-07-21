#include "ui/widgets/ShortcutsOverlay.h"
#include "ui/HotkeyDispatcher.h"
#include "core/EngineController.h"
#include "core/SettingsStore.h"
#include "ui/Motion.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QScrollArea>
#include <QJsonObject>

namespace railshot {

namespace {
QWidget* section(const QString& title, const QVector<QPair<QString, QString>>& rows, QWidget* parent)
{
    auto* box = new QWidget(parent);
    auto* lay = new QVBoxLayout(box);
    lay->setContentsMargins(0, 0, 0, 12);
    lay->setSpacing(6);
    auto* h = new QLabel(title, box);
    h->setStyleSheet(QStringLiteral(
        "color:#3A6AFF; font-weight:800; font-size:10px; letter-spacing:1.5px; background:transparent;"));
    lay->addWidget(h);
    for (const auto& r : rows) {
        auto* row = new QHBoxLayout();
        auto* key = new QLabel(r.first, box);
        key->setStyleSheet(QStringLiteral(
            "font-family:'JetBrains Mono'; font-size:11px; color:#D0D2D8;"
            "background:#1E2128; border:1px solid #2A2D35; border-radius:3px; padding:4px 8px;"));
        auto* action = new QLabel(r.second, box);
        action->setStyleSheet(QStringLiteral("color:#A0A8B8; font-size:12px; background:transparent;"));
        row->addWidget(key);
        row->addWidget(action, 1);
        lay->addLayout(row);
    }
    return box;
}
} // namespace

ShortcutsOverlay::ShortcutsOverlay(EngineController* engine, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Keyboard Shortcuts"));
    setModal(true);
    setFixedWidth(560);
    setStyleSheet(QStringLiteral(
        "QDialog{background:#141619;border:1px solid #3A3D45;}"
        "QLabel{background:transparent;}"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QWidget(this);
    header->setFixedHeight(44);
    header->setStyleSheet(QStringLiteral("background:#0F1114; border-bottom:1px solid #2A2D35;"));
    auto* h = new QHBoxLayout(header);
    h->setContentsMargins(16, 0, 12, 0);
    auto* title = new QLabel(QStringLiteral("KEYBOARD SHORTCUTS"), header);
    title->setStyleSheet(QStringLiteral(
        "font-family:'Bebas Neue'; font-size:18px; letter-spacing:1px; color:#F0F0F0;"));
    auto* close = new QPushButton(QStringLiteral("✕"), header);
    close->setFixedSize(28, 24);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    h->addWidget(title);
    h->addStretch();
    h->addWidget(close);
    root->addWidget(header);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* body = new QWidget;
    auto* lay = new QVBoxLayout(body);
    lay->setContentsMargins(20, 16, 20, 16);
    lay->setSpacing(8);

    QJsonObject keys = engine && engine->settings() ? engine->settings()->hotkeys() : QJsonObject{};

    auto rowsFor = [&](const QStringList& actions) {
        QVector<QPair<QString, QString>> rows;
        for (const QString& a : actions) {
            const QString binding = keys.value(a).toString();
            const QKeySequence seq = HotkeyDispatcher::sequenceFor(binding);
            const QString keyText = seq.isEmpty()
                ? QStringLiteral("—")
                : seq.toString(QKeySequence::NativeText);
            rows.push_back({keyText, HotkeyDispatcher::labelForAction(a)});
        }
        return rows;
    };

    lay->addWidget(section(QStringLiteral("TRANSITIONS / SCENES"), rowsFor({
        QStringLiteral("go"), QStringLiteral("scene1"), QStringLiteral("scene2"),
        QStringLiteral("scene3"), QStringLiteral("scene4"), QStringLiteral("scene5"),
        QStringLiteral("scene6"), QStringLiteral("scene7"), QStringLiteral("scene8"),
    }), body));
    lay->addWidget(section(QStringLiteral("STREAM / RECORD"), rowsFor({
        QStringLiteral("streamToggle"), QStringLiteral("recordToggle"), QStringLiteral("saveReplay"),
        QStringLiteral("screenshotPreview"), QStringLiteral("screenshotProgram"),
    }), body));
    lay->addWidget(section(QStringLiteral("AUDIO / SCOREBOARD"), rowsFor({
        QStringLiteral("muteMic"), QStringLiteral("muteDesktop"),
        QStringLiteral("scoreAPlus"), QStringLiteral("scoreAMinus"),
        QStringLiteral("scoreBPlus"), QStringLiteral("scoreBMinus"),
        QStringLiteral("scoreReset"), QStringLiteral("scoreSwap"),
    }), body));
    lay->addWidget(section(QStringLiteral("UI"), {
        {QStringLiteral("Shift+?"), QStringLiteral("This shortcuts overlay")},
        {keys.value(QStringLiteral("fullscreen")).toString(QStringLiteral("F")),
         HotkeyDispatcher::labelForAction(QStringLiteral("fullscreen"))},
        {QStringLiteral("Ctrl+S"), QStringLiteral("Save project")},
        {QStringLiteral("Ctrl+O"), QStringLiteral("Open project")},
        {QStringLiteral("Esc"), QStringLiteral("Close overlay / dialogs")},
    }, body));
    lay->addStretch();
    scroll->setWidget(body);
    root->addWidget(scroll, 1);
    motion::playModalEnter(this);
}

} // namespace railshot
