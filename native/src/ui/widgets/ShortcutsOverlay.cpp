#include "ui/widgets/ShortcutsOverlay.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QScrollArea>
#include <QKeyEvent>

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

ShortcutsOverlay::ShortcutsOverlay(QWidget* parent)
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

    lay->addWidget(section(QStringLiteral("TRANSITIONS"), {
        {QStringLiteral("Space / Enter"), QStringLiteral("GO (Preview → Program)")},
        {QStringLiteral("1 – 8"), QStringLiteral("Preview scene N")},
    }, body));
    lay->addWidget(section(QStringLiteral("STREAM"), {
        {QStringLiteral("Ctrl+Shift+S"), QStringLiteral("Toggle Stream / Go Live")},
        {QStringLiteral("Ctrl+Shift+R"), QStringLiteral("Toggle Record")},
    }, body));
    lay->addWidget(section(QStringLiteral("UI"), {
        {QStringLiteral("F"), QStringLiteral("Fullscreen")},
        {QStringLiteral("Shift+?"), QStringLiteral("This shortcuts overlay")},
        {QStringLiteral("Ctrl+S"), QStringLiteral("Save project")},
        {QStringLiteral("Ctrl+O"), QStringLiteral("Open project")},
        {QStringLiteral("Esc"), QStringLiteral("Close overlay / Scene Editor")},
    }, body));
    lay->addStretch();
    scroll->setWidget(body);
    root->addWidget(scroll, 1);
}

} // namespace railshot
