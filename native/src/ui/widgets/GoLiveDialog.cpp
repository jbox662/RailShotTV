#include "ui/widgets/GoLiveDialog.h"
#include "core/EngineController.h"
#include "core/SecretStore.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QTimer>

namespace railshot {

GoLiveDialog::GoLiveDialog(EngineController* engine, QWidget* parent)
    : QDialog(parent), m_engine(engine)
{
    setWindowTitle(QStringLiteral("Go Live — RailShotTV"));
    setMinimumWidth(420);
    auto* col = new QVBoxLayout(this);

    col->addWidget(new QLabel(QStringLiteral("Pre-stream checklist"), this));
    auto* checks = new QLabel(QStringLiteral(
        "• Audio devices\n• Active scene\n• Video encoder\n• Network\n• Stream key"), this);
    col->addWidget(checks);

    m_platform = new QComboBox(this);
    m_platform->addItems({QStringLiteral("YouTube"), QStringLiteral("Twitch"),
                          QStringLiteral("Facebook"), QStringLiteral("Custom")});
    col->addWidget(m_platform);

    m_url = new QLineEdit(this);
    m_url->setPlaceholderText(QStringLiteral("rtmp://a.rtmp.youtube.com/live2"));
    col->addWidget(m_url);

    m_key = new QLineEdit(this);
    m_key->setEchoMode(QLineEdit::Password);
    m_key->setPlaceholderText(QStringLiteral("Stream key"));
    col->addWidget(m_key);

    auto* bar = new QProgressBar(this);
    bar->setRange(0, 5);
    bar->setValue(0);
    col->addWidget(bar);

    auto* row = new QHBoxLayout();
    auto* cancel = new QPushButton(QStringLiteral("Cancel"), this);
    auto* go = new QPushButton(QStringLiteral("GO LIVE"), this);
    go->setObjectName(QStringLiteral("streamButton"));
    row->addWidget(cancel);
    row->addWidget(go);
    col->addLayout(row);

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(go, &QPushButton::clicked, this, [this, bar] {
        // Simulated checklist progress then real start
        bar->setValue(0);
        auto* t = new QTimer(this);
        connect(t, &QTimer::timeout, this, [this, bar, t] {
            bar->setValue(bar->value() + 1);
            if (bar->value() >= 5) {
                t->stop();
                t->deleteLater();
                StreamTarget target;
                target.id = newId(QStringLiteral("tgt"));
                target.platform = m_platform->currentText().toLower();
                target.name = m_platform->currentText();
                target.rtmpUrl = m_url->text().trimmed();
                target.streamKeySecretId = SecretStore::makeStreamKeyId(target.id);
                if (!m_key->text().isEmpty())
                    SecretStore::store(target.streamKeySecretId, m_key->text());

                m_engine->sceneGraph()->mutate([&](Project& p) {
                    p.streamTargets.append(target);
                    p.output = p.output.width > 0 ? p.output : OutputProfile{};
                });

                QString err;
                if (!m_engine->startStreaming(target.id, &err)) {
                    QMessageBox::warning(this, QStringLiteral("Go Live"), err);
                    return;
                }
                accept();
            }
        });
        t->start(200);
    });

    connect(m_platform, &QComboBox::currentTextChanged, this, [this](const QString& p) {
        if (p == QLatin1String("YouTube"))
            m_url->setText(QStringLiteral("rtmp://a.rtmp.youtube.com/live2"));
        else if (p == QLatin1String("Twitch"))
            m_url->setText(QStringLiteral("rtmp://live.twitch.tv/app"));
        else if (p == QLatin1String("Facebook"))
            m_url->setText(QStringLiteral("rtmps://live-api-s.facebook.com:443/rtmp"));
        else
            m_url->clear();
    });
}

} // namespace railshot
