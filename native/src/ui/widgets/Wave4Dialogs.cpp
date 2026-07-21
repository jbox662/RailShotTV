#include "ui/widgets/Wave4Dialogs.h"
#include "core/EngineController.h"
#include "core/SettingsStore.h"
#include "core/SceneGraph.h"
#include "core/ProfileCollectionStore.h"
#include "core/Logger.h"
#include "core/Types.h"
#include "compositor/D3D11Compositor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QTimer>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QScrollBar>
#include <QStandardPaths>

namespace railshot {

namespace {
QString dialogStyle()
{
    return QStringLiteral(
        "QDialog{background:#0F1114;}"
        "QLabel{color:#C8CCD4; font-family:'DM Sans';}"
        "QListWidget{background:#0A0C0F; border:1px solid #2A2D35; color:#E0E2E8;}"
        "QListWidget::item:selected{background:#1A2A3A; color:#7AB8FF;}"
        "QLineEdit,QPlainTextEdit{background:#0A0C0F; border:1px solid #2A2D35; color:#E0E2E8;}"
        "QPushButton{background:#1A1E26; border:1px solid #5A5E68; border-radius:3px;"
        "  color:#E0E2E8; font-weight:700; padding:5px 12px;}"
        "QPushButton:hover{border-color:#4F9EFF;}");
}
} // namespace

// ── Profiles ───────────────────────────────────────────────────────────────

ProfilesDialog::ProfilesDialog(EngineController* engine, QWidget* parent)
    : QDialog(parent), m_engine(engine)
{
    setWindowTitle(QStringLiteral("Profiles"));
    setMinimumSize(480, 360);
    setStyleSheet(dialogStyle());

    ensureDefaultProfile(engine ? engine->settings()->outputProfile() : OutputProfile{});

    auto* root = new QVBoxLayout(this);
    m_list = new QListWidget(this);
    root->addWidget(m_list, 1);
    m_detail = new QLabel(this);
    m_detail->setWordWrap(true);
    root->addWidget(m_detail);

    auto* row = new QHBoxLayout();
    auto* neu = new QPushButton(QStringLiteral("New"), this);
    auto* dup = new QPushButton(QStringLiteral("Duplicate"), this);
    auto* ren = new QPushButton(QStringLiteral("Rename"), this);
    auto* rem = new QPushButton(QStringLiteral("Remove"), this);
    auto* apply = new QPushButton(QStringLiteral("Apply"), this);
    apply->setObjectName(QStringLiteral("primaryButton"));
    row->addWidget(neu);
    row->addWidget(dup);
    row->addWidget(ren);
    row->addWidget(rem);
    row->addStretch();
    row->addWidget(apply);
    root->addLayout(row);

    connect(neu, &QPushButton::clicked, this, &ProfilesDialog::onNew);
    connect(dup, &QPushButton::clicked, this, &ProfilesDialog::onDuplicate);
    connect(ren, &QPushButton::clicked, this, &ProfilesDialog::onRename);
    connect(rem, &QPushButton::clicked, this, &ProfilesDialog::onRemove);
    connect(apply, &QPushButton::clicked, this, &ProfilesDialog::onApply);
    connect(m_list, &QListWidget::currentTextChanged, this, [this](const QString& name) {
        auto p = loadNamedProfile(name);
        if (!p) {
            m_detail->clear();
            return;
        }
        m_detail->setText(QStringLiteral("%1×%2 @ %3 fps · %4 kbps · %5")
                              .arg(p->profile.width)
                              .arg(p->profile.height)
                              .arg(p->profile.fps, 0, 'f', 2)
                              .arg(p->profile.videoBitrateKbps)
                              .arg(p->profile.encoderPreference));
    });
    reload();
}

QString ProfilesDialog::selectedName() const
{
    auto* item = m_list->currentItem();
    return item ? item->text() : QString();
}

void ProfilesDialog::reload()
{
    const QString cur = currentProfileName();
    m_list->clear();
    for (const auto& n : listProfileNames()) {
        m_list->addItem(n);
        if (n == cur)
            m_list->setCurrentRow(m_list->count() - 1);
    }
    if (m_list->currentRow() < 0 && m_list->count() > 0)
        m_list->setCurrentRow(0);
}

void ProfilesDialog::onNew()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("New Profile"),
                                               QStringLiteral("Name:"), QLineEdit::Normal,
                                               QStringLiteral("New Profile"), &ok).trimmed();
    if (!ok || name.isEmpty() || !m_engine) return;
    NamedOutputProfile p;
    p.name = name;
    p.profile = m_engine->settings()->outputProfile();
    if (p.profile.width <= 0)
        p.profile = m_engine->projectSnapshot().output;
    QString err;
    if (!saveNamedProfile(p, &err)) {
        QMessageBox::warning(this, QStringLiteral("Profiles"), err);
        return;
    }
    setCurrentProfileName(name);
    reload();
}

void ProfilesDialog::onDuplicate()
{
    const QString from = selectedName();
    if (from.isEmpty()) return;
    bool ok = false;
    const QString to = QInputDialog::getText(this, QStringLiteral("Duplicate Profile"),
                                             QStringLiteral("New name:"), QLineEdit::Normal,
                                             from + QStringLiteral(" Copy"), &ok).trimmed();
    if (!ok || to.isEmpty()) return;
    QString err;
    if (!duplicateNamedProfile(from, to, &err)) {
        QMessageBox::warning(this, QStringLiteral("Profiles"), err);
        return;
    }
    reload();
}

void ProfilesDialog::onRename()
{
    const QString from = selectedName();
    if (from.isEmpty()) return;
    bool ok = false;
    const QString to = QInputDialog::getText(this, QStringLiteral("Rename Profile"),
                                             QStringLiteral("New name:"), QLineEdit::Normal,
                                             from, &ok).trimmed();
    if (!ok || to.isEmpty() || to == from) return;
    auto p = loadNamedProfile(from);
    if (!p) return;
    QString err;
    p->name = to;
    if (!saveNamedProfile(*p, &err) || !deleteNamedProfile(from, &err)) {
        QMessageBox::warning(this, QStringLiteral("Profiles"), err);
        return;
    }
    if (currentProfileName() == from)
        setCurrentProfileName(to);
    reload();
}

void ProfilesDialog::onRemove()
{
    const QString name = selectedName();
    if (name.isEmpty()) return;
    if (listProfileNames().size() <= 1) {
        QMessageBox::information(this, QStringLiteral("Profiles"),
                                 QStringLiteral("Keep at least one profile."));
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("Remove Profile"),
                              QStringLiteral("Delete profile \"%1\"?").arg(name))
        != QMessageBox::Yes)
        return;
    QString err;
    deleteNamedProfile(name, &err);
    reload();
}

void ProfilesDialog::onApply()
{
    const QString name = selectedName();
    if (name.isEmpty() || !m_engine) return;
    auto p = loadNamedProfile(name);
    if (!p) return;
    m_engine->settings()->setOutputProfile(p->profile);
    m_engine->sceneGraph()->mutate([&](Project& proj) {
        proj.output = p->profile;
    });
    if (m_engine->compositor() && p->profile.width > 0 && p->profile.height > 0)
        m_engine->compositor()->resize(p->profile.width, p->profile.height);
    setCurrentProfileName(name);
    emit profileApplied(name);
    accept();
}

// ── Scene collections ──────────────────────────────────────────────────────

SceneCollectionsDialog::SceneCollectionsDialog(EngineController* engine, QWidget* parent)
    : QDialog(parent), m_engine(engine)
{
    setWindowTitle(QStringLiteral("Scene Collections"));
    setMinimumSize(480, 360);
    setStyleSheet(dialogStyle());

    auto* root = new QVBoxLayout(this);
    root->addWidget(new QLabel(QStringLiteral(
        "Collections are full project snapshots (scenes, sources, output). "
        "Switching saves the current collection first."), this));
    m_list = new QListWidget(this);
    root->addWidget(m_list, 1);

    auto* row = new QHBoxLayout();
    auto* neu = new QPushButton(QStringLiteral("New"), this);
    auto* dup = new QPushButton(QStringLiteral("Duplicate"), this);
    auto* ren = new QPushButton(QStringLiteral("Rename"), this);
    auto* rem = new QPushButton(QStringLiteral("Remove"), this);
    auto* sw = new QPushButton(QStringLiteral("Switch To"), this);
    row->addWidget(neu);
    row->addWidget(dup);
    row->addWidget(ren);
    row->addWidget(rem);
    row->addStretch();
    row->addWidget(sw);
    root->addLayout(row);

    connect(neu, &QPushButton::clicked, this, &SceneCollectionsDialog::onNew);
    connect(dup, &QPushButton::clicked, this, &SceneCollectionsDialog::onDuplicate);
    connect(ren, &QPushButton::clicked, this, &SceneCollectionsDialog::onRename);
    connect(rem, &QPushButton::clicked, this, &SceneCollectionsDialog::onRemove);
    connect(sw, &QPushButton::clicked, this, &SceneCollectionsDialog::onSwitch);
    reload();
}

QString SceneCollectionsDialog::selectedName() const
{
    auto* item = m_list->currentItem();
    return item ? item->text() : QString();
}

void SceneCollectionsDialog::reload()
{
    const QString cur = currentCollectionName();
    m_list->clear();
    for (const auto& n : listCollectionNames()) {
        auto* item = new QListWidgetItem(n, m_list);
        if (n == cur)
            item->setText(n + QStringLiteral("  (current)"));
    }
    // Store bare name in UserRole
    for (int i = 0; i < m_list->count(); ++i) {
        auto* item = m_list->item(i);
        QString n = item->text();
        n.remove(QStringLiteral("  (current)"));
        item->setData(Qt::UserRole, n);
        if (n == cur)
            m_list->setCurrentRow(i);
    }
    if (m_list->currentRow() < 0 && m_list->count() > 0)
        m_list->setCurrentRow(0);
}

void SceneCollectionsDialog::onNew()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("New Collection"),
                                               QStringLiteral("Name:"), QLineEdit::Normal,
                                               QStringLiteral("Untitled"), &ok).trimmed();
    if (!ok || name.isEmpty() || !m_engine) return;
    QString err;
    if (!m_engine->saveProject(collectionFilePath(name), &err)) {
        QMessageBox::warning(this, QStringLiteral("Collections"), err);
        return;
    }
    setCurrentCollectionName(name);
    reload();
    emit collectionSwitchRequested(name);
}

void SceneCollectionsDialog::onDuplicate()
{
    const QString from = m_list->currentItem()
                             ? m_list->currentItem()->data(Qt::UserRole).toString()
                             : QString();
    if (from.isEmpty()) return;
    bool ok = false;
    const QString to = QInputDialog::getText(this, QStringLiteral("Duplicate Collection"),
                                             QStringLiteral("New name:"), QLineEdit::Normal,
                                             from + QStringLiteral(" Copy"), &ok).trimmed();
    if (!ok || to.isEmpty()) return;
    QString err;
    if (!duplicateCollection(from, to, &err)) {
        QMessageBox::warning(this, QStringLiteral("Collections"), err);
        return;
    }
    reload();
}

void SceneCollectionsDialog::onRename()
{
    const QString from = m_list->currentItem()
                             ? m_list->currentItem()->data(Qt::UserRole).toString()
                             : QString();
    if (from.isEmpty()) return;
    bool ok = false;
    const QString to = QInputDialog::getText(this, QStringLiteral("Rename Collection"),
                                             QStringLiteral("New name:"), QLineEdit::Normal,
                                             from, &ok).trimmed();
    if (!ok || to.isEmpty() || to == from) return;
    QString err;
    if (!renameCollection(from, to, &err)) {
        QMessageBox::warning(this, QStringLiteral("Collections"), err);
        return;
    }
    reload();
}

void SceneCollectionsDialog::onRemove()
{
    const QString name = m_list->currentItem()
                             ? m_list->currentItem()->data(Qt::UserRole).toString()
                             : QString();
    if (name.isEmpty()) return;
    if (listCollectionNames().size() <= 1) {
        QMessageBox::information(this, QStringLiteral("Collections"),
                                 QStringLiteral("Keep at least one collection, or create another first."));
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("Remove Collection"),
                              QStringLiteral("Delete collection \"%1\"?").arg(name))
        != QMessageBox::Yes)
        return;
    QString err;
    deleteCollection(name, &err);
    reload();
}

void SceneCollectionsDialog::onSwitch()
{
    const QString name = m_list->currentItem()
                             ? m_list->currentItem()->data(Qt::UserRole).toString()
                             : QString();
    if (name.isEmpty()) return;
    emit collectionSwitchRequested(name);
    accept();
}

// ── Remux ──────────────────────────────────────────────────────────────────

RemuxDialog::RemuxDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Remux Recordings"));
    setMinimumSize(560, 220);
    setStyleSheet(dialogStyle());

    auto* root = new QVBoxLayout(this);
    root->addWidget(new QLabel(QStringLiteral(
        "Lossless remux (e.g. MKV → MP4) using FFmpeg. Requires ffmpeg on PATH."), this));

    auto* inRow = new QHBoxLayout();
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText(QStringLiteral("Input media file…"));
    auto* inBtn = new QPushButton(QStringLiteral("Browse…"), this);
    inRow->addWidget(m_input, 1);
    inRow->addWidget(inBtn);
    root->addLayout(inRow);

    auto* outRow = new QHBoxLayout();
    m_output = new QLineEdit(this);
    m_output->setPlaceholderText(QStringLiteral("Output file…"));
    auto* outBtn = new QPushButton(QStringLiteral("Browse…"), this);
    outRow->addWidget(m_output, 1);
    outRow->addWidget(outBtn);
    root->addLayout(outRow);

    m_status = new QLabel(QStringLiteral("Idle"), this);
    root->addWidget(m_status);

    auto* row = new QHBoxLayout();
    row->addStretch();
    m_go = new QPushButton(QStringLiteral("Remux"), this);
    auto* closeBtn = new QPushButton(QStringLiteral("Close"), this);
    row->addWidget(m_go);
    row->addWidget(closeBtn);
    root->addLayout(row);

    m_proc = new QProcess(this);
    connect(inBtn, &QPushButton::clicked, this, &RemuxDialog::browseInput);
    connect(outBtn, &QPushButton::clicked, this, &RemuxDialog::browseOutput);
    connect(m_go, &QPushButton::clicked, this, &RemuxDialog::startRemux);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus) { onProcFinished(code); });
}

void RemuxDialog::browseInput()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Input"), QString(),
        QStringLiteral("Media (*.mkv *.mp4 *.mov *.flv *.ts *.m4v);;All (*.*)"));
    if (path.isEmpty()) return;
    m_input->setText(path);
    if (m_output->text().isEmpty()) {
        QFileInfo fi(path);
        QString out = fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName() + QStringLiteral(".mp4");
        if (out == path)
            out = fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName() + QStringLiteral("_remux.mp4");
        m_output->setText(out);
    }
}

void RemuxDialog::browseOutput()
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Output"), m_output->text(),
        QStringLiteral("MP4 (*.mp4);;MKV (*.mkv);;MOV (*.mov);;All (*.*)"));
    if (!path.isEmpty())
        m_output->setText(path);
}

void RemuxDialog::startRemux()
{
    const QString in = m_input->text().trimmed();
    const QString out = m_output->text().trimmed();
    if (in.isEmpty() || out.isEmpty()) {
        m_status->setText(QStringLiteral("Choose input and output paths."));
        return;
    }
    if (!QFileInfo::exists(in)) {
        m_status->setText(QStringLiteral("Input file not found."));
        return;
    }
    if (m_proc->state() != QProcess::NotRunning)
        return;

    m_status->setText(QStringLiteral("Remuxing…"));
    m_go->setEnabled(false);
    m_proc->start(QStringLiteral("ffmpeg"),
                  {QStringLiteral("-y"), QStringLiteral("-i"), in,
                   QStringLiteral("-c"), QStringLiteral("copy"), out});
    if (!m_proc->waitForStarted(3000)) {
        m_status->setText(QStringLiteral(
            "Could not start ffmpeg. Install FFmpeg and ensure it is on PATH."));
        m_go->setEnabled(true);
    }
}

void RemuxDialog::onProcFinished(int exitCode)
{
    m_go->setEnabled(true);
    if (exitCode == 0) {
        m_status->setText(QStringLiteral("Done: %1").arg(m_output->text()));
        Logger::info(QStringLiteral("Remux OK → %1").arg(m_output->text()));
    } else {
        const QString err = QString::fromLocal8Bit(m_proc->readAllStandardError()).right(400);
        m_status->setText(QStringLiteral("Failed (exit %1). %2").arg(exitCode).arg(err.simplified()));
        Logger::error(QStringLiteral("Remux failed: %1").arg(err));
    }
}

// ── Log viewer ─────────────────────────────────────────────────────────────

LogViewerDialog::LogViewerDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Log Viewer"));
    setMinimumSize(720, 480);
    resize(900, 560);
    setStyleSheet(dialogStyle());

    auto* root = new QVBoxLayout(this);
    m_pathLabel = new QLabel(this);
    m_pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    root->addWidget(m_pathLabel);

    m_text = new QPlainTextEdit(this);
    m_text->setReadOnly(true);
    m_text->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_text->setStyleSheet(QStringLiteral(
        "font-family:'JetBrains Mono','Consolas',monospace; font-size:11px;"));
    root->addWidget(m_text, 1);

    auto* row = new QHBoxLayout();
    auto* refreshBtn = new QPushButton(QStringLiteral("Refresh"), this);
    auto* folderBtn = new QPushButton(QStringLiteral("Open Folder"), this);
    auto* fileBtn = new QPushButton(QStringLiteral("Open File"), this);
    auto* closeBtn = new QPushButton(QStringLiteral("Close"), this);
    row->addWidget(refreshBtn);
    row->addWidget(folderBtn);
    row->addWidget(fileBtn);
    row->addStretch();
    row->addWidget(closeBtn);
    root->addLayout(row);

    connect(refreshBtn, &QPushButton::clicked, this, &LogViewerDialog::refresh);
    connect(folderBtn, &QPushButton::clicked, this, &LogViewerDialog::openFolder);
    connect(fileBtn, &QPushButton::clicked, this, &LogViewerDialog::openFile);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &LogViewerDialog::refresh);
    timer->start(2000);
    refresh();
}

void LogViewerDialog::refresh()
{
    const QString path = Logger::logFilePath();
    m_pathLabel->setText(path.isEmpty() ? QStringLiteral("(no log file yet)")
                                        : path);
    if (path.isEmpty() || !QFile::exists(path)) {
        m_text->setPlainText(QStringLiteral("No log file available for this session."));
        return;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_text->setPlainText(f.errorString());
        return;
    }
    const QByteArray data = f.readAll();
    // Keep last ~512KB for UI responsiveness
    const QByteArray slice = data.size() > 512 * 1024 ? data.right(512 * 1024) : data;
    const bool atBottom = m_text->verticalScrollBar()
                              ? m_text->verticalScrollBar()->value()
                                    >= m_text->verticalScrollBar()->maximum() - 8
                              : true;
    m_text->setPlainText(QString::fromUtf8(slice));
    if (atBottom) {
        auto* bar = m_text->verticalScrollBar();
        if (bar) bar->setValue(bar->maximum());
    }
}

void LogViewerDialog::openFolder()
{
    QString path = Logger::logFilePath();
    if (path.isEmpty()) {
        path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
               + QStringLiteral("/logs");
        QDir().mkpath(path);
    } else {
        path = QFileInfo(path).absolutePath();
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void LogViewerDialog::openFile()
{
    const QString path = Logger::logFilePath();
    if (path.isEmpty() || !QFile::exists(path)) {
        QMessageBox::information(this, QStringLiteral("Logs"),
                                 QStringLiteral("No log file for this session yet."));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

} // namespace railshot
