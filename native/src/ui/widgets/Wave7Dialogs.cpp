#include "ui/widgets/Wave7Dialogs.h"
#include "core/EngineController.h"
#include "core/SettingsStore.h"
#include "overlays/VirtualCamera.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QJsonObject>
#include <QHash>
#include <optional>

namespace railshot {

namespace {
bool pathMissing(const QString& raw)
{
    if (raw.trimmed().isEmpty()) return true;
    QUrl u(raw);
    if (u.isLocalFile() || raw.startsWith(QLatin1String("file:"), Qt::CaseInsensitive)) {
        const QString local = u.isLocalFile() ? u.toLocalFile() : QUrl::fromUserInput(raw).toLocalFile();
        return local.isEmpty() || !QFileInfo::exists(local);
    }
    // Remote http(s) — not treated as missing files.
    if (raw.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)
        || raw.startsWith(QLatin1String("https://"), Qt::CaseInsensitive))
        return false;
    return !QFileInfo::exists(raw);
}

QString displayPath(const QString& raw)
{
    QUrl u(raw);
    if (u.isLocalFile()) return u.toLocalFile();
    if (raw.startsWith(QLatin1String("file:"), Qt::CaseInsensitive))
        return QUrl::fromUserInput(raw).toLocalFile();
    return raw;
}
} // namespace

QVector<MissingFileEntry> scanMissingFiles(const Project& project)
{
    QVector<MissingFileEntry> out;
    for (const auto& scene : project.scenes) {
        for (const auto& src : scene.sources) {
            if (src.type == SourceType::Image || src.type == SourceType::Media) {
                const QString path = src.settings.value(QStringLiteral("path")).toString();
                if (pathMissing(path)) {
                    MissingFileEntry e;
                    e.sourceId = src.id;
                    e.sourceName = src.name;
                    e.sceneName = scene.name;
                    e.settingKey = QStringLiteral("path");
                    e.originalPath = displayPath(path);
                    e.newPath = e.originalPath;
                    out.push_back(e);
                }
            } else if (src.type == SourceType::Browser) {
                const QString url = src.settings.value(QStringLiteral("url")).toString();
                // Only local HTML / file URLs
                if (!url.startsWith(QLatin1String("http"), Qt::CaseInsensitive) && pathMissing(url)) {
                    MissingFileEntry e;
                    e.sourceId = src.id;
                    e.sourceName = src.name;
                    e.sceneName = scene.name;
                    e.settingKey = QStringLiteral("url");
                    e.originalPath = displayPath(url);
                    e.newPath = e.originalPath;
                    out.push_back(e);
                }
            }
        }
    }
    return out;
}

MissingFilesDialog::MissingFilesDialog(EngineController* engine, QVector<MissingFileEntry> entries,
                                       QWidget* parent)
    : QDialog(parent), m_engine(engine), m_entries(std::move(entries))
{
    setWindowTitle(QStringLiteral("Missing Files"));
    resize(720, 360);
    auto* root = new QVBoxLayout(this);
    root->addWidget(new QLabel(
        QStringLiteral("These sources reference files that could not be found. "
                       "Browse individually or Search Folder to rematch by filename."),
        this));

    m_table = new QTableWidget(m_entries.size(), 4, this);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("Source"), QStringLiteral("Scene"), QStringLiteral("Missing Path"),
         QStringLiteral("New Path")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    for (int i = 0; i < m_entries.size(); ++i) {
        m_table->setItem(i, 0, new QTableWidgetItem(m_entries[i].sourceName));
        m_table->setItem(i, 1, new QTableWidgetItem(m_entries[i].sceneName));
        auto* miss = new QTableWidgetItem(m_entries[i].originalPath);
        miss->setFlags(miss->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(i, 2, miss);
        m_table->setItem(i, 3, new QTableWidgetItem(m_entries[i].newPath));
    }
    root->addWidget(m_table, 1);

    auto* row = new QHBoxLayout();
    auto* browse = new QPushButton(QStringLiteral("Browse…"), this);
    auto* search = new QPushButton(QStringLiteral("Search Folder…"), this);
    row->addWidget(browse);
    row->addWidget(search);
    row->addStretch();
    root->addLayout(row);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Apply"));
    root->addWidget(buttons);

    connect(browse, &QPushButton::clicked, this, [this] {
        const int r = m_table->currentRow();
        if (r >= 0) browseRow(r);
    });
    connect(search, &QPushButton::clicked, this, &MissingFilesDialog::searchFolder);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        applyRemaps();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void MissingFilesDialog::browseRow(int row)
{
    if (row < 0 || row >= m_entries.size()) return;
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Locate File"),
                                                      m_entries[row].originalPath);
    if (path.isEmpty()) return;
    m_entries[row].newPath = path;
    m_table->item(row, 3)->setText(path);
}

void MissingFilesDialog::searchFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Search Folder"));
    if (dir.isEmpty()) return;
    QDir root(dir);
    const QFileInfoList files = root.entryInfoList(QDir::Files | QDir::NoSymLinks, QDir::Name);
    QHash<QString, QString> byName;
    for (const auto& fi : files)
        byName.insert(fi.fileName().toLower(), fi.absoluteFilePath());

    int matched = 0;
    for (int i = 0; i < m_entries.size(); ++i) {
        const QString name = QFileInfo(m_entries[i].originalPath).fileName().toLower();
        if (name.isEmpty()) continue;
        auto it = byName.constFind(name);
        if (it == byName.cend()) continue;
        m_entries[i].newPath = *it;
        m_table->item(i, 3)->setText(*it);
        ++matched;
    }
    QMessageBox::information(this, QStringLiteral("Search Folder"),
                             QStringLiteral("Matched %1 of %2 missing file(s).")
                                 .arg(matched).arg(m_entries.size()));
}

int MissingFilesDialog::applyRemaps()
{
    if (!m_engine) return 0;
    int n = 0;
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_table->item(i, 3))
            m_entries[i].newPath = m_table->item(i, 3)->text().trimmed();
        auto& e = m_entries[i];
        if (e.newPath.isEmpty() || e.newPath == e.originalPath) continue;
        if (!QFileInfo::exists(e.newPath)) continue;

        auto srcOpt = [&]() -> std::optional<SourceItem> {
            const auto p = m_engine->projectSnapshot();
            for (const auto& sc : p.scenes)
                for (const auto& s : sc.sources)
                    if (s.id == e.sourceId) return s;
            return std::nullopt;
        }();
        if (!srcOpt) continue;
        QJsonObject settings = srcOpt->settings;
        if (e.settingKey == QLatin1String("url"))
            settings.insert(QStringLiteral("url"), QUrl::fromLocalFile(e.newPath).toString());
        else
            settings.insert(QStringLiteral("path"), e.newPath);
        m_engine->updateSourceSettings(e.sourceId, settings);
        ++n;
    }
    return n;
}

VCamConfigDialog::VCamConfigDialog(EngineController* engine, QWidget* parent)
    : QDialog(parent), m_engine(engine)
{
    setWindowTitle(QStringLiteral("Virtual Camera"));
    resize(420, 220);
    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    m_status = new QLabel(this);
    m_res = new QLabel(this);
    auto* name = new QLabel(QStringLiteral("RailShot Virtual Camera"), this);
    form->addRow(QStringLiteral("Device name"), name);
    form->addRow(QStringLiteral("Output"), new QLabel(QStringLiteral("Program (canvas)"), this));
    form->addRow(QStringLiteral("Resolution"), m_res);
    form->addRow(QStringLiteral("Status"), m_status);
    root->addLayout(form);

    m_startOnLaunch = new QCheckBox(QStringLiteral("Start Virtual Camera when RailShot launches"), this);
    if (engine && engine->settings())
        m_startOnLaunch->setChecked(
            engine->settings()->uiState().value(QStringLiteral("vcamStartOnLaunch")).toBool(false));
    root->addWidget(m_startOnLaunch);

    auto* note = new QLabel(
        QStringLiteral("External Output uses the same Virtual Camera feed. "
                       "Other apps see it as a webcam after Start."),
        this);
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("color:#8892A4; font-size:11px;"));
    root->addWidget(note);

    auto* row = new QHBoxLayout();
    auto* toggle = new QPushButton(this);
    auto* close = new QPushButton(QStringLiteral("Close"), this);
    row->addWidget(toggle);
    row->addStretch();
    row->addWidget(close);
    root->addLayout(row);

    auto syncBtn = [this, toggle] {
        const bool on = m_engine && m_engine->virtualCamera() && m_engine->virtualCamera()->isRunning();
        toggle->setText(on ? QStringLiteral("Stop Virtual Camera")
                           : QStringLiteral("Start Virtual Camera"));
    };
    refresh();
    syncBtn();

    connect(toggle, &QPushButton::clicked, this, [this, syncBtn] {
        this->toggle();
        refresh();
        syncBtn();
    });
    connect(close, &QPushButton::clicked, this, [this] {
        if (m_engine && m_engine->settings()) {
            auto ui = m_engine->settings()->uiState();
            ui.insert(QStringLiteral("vcamStartOnLaunch"), m_startOnLaunch->isChecked());
            m_engine->settings()->setUiState(ui);
            m_engine->settings()->sync();
        }
        accept();
    });
    if (engine && engine->virtualCamera()) {
        connect(engine->virtualCamera(), &VirtualCamera::started, this, [this, syncBtn] {
            refresh();
            syncBtn();
        });
        connect(engine->virtualCamera(), &VirtualCamera::stopped, this, [this, syncBtn] {
            refresh();
            syncBtn();
        });
    }
}

void VCamConfigDialog::refresh()
{
    if (!m_engine) return;
    const auto profile = m_engine->settings() ? m_engine->settings()->outputProfile() : OutputProfile{};
    m_res->setText(QStringLiteral("%1 × %2").arg(profile.width).arg(profile.height));
    const bool on = m_engine->virtualCamera() && m_engine->virtualCamera()->isRunning();
    m_status->setText(on ? QStringLiteral("Running") : QStringLiteral("Stopped"));
}

void VCamConfigDialog::toggle()
{
    if (!m_engine) return;
    QString err;
    const bool on = m_engine->virtualCamera() && m_engine->virtualCamera()->isRunning();
    if (!m_engine->setVirtualCameraEnabled(!on, &err) && !err.isEmpty())
        QMessageBox::warning(this, QStringLiteral("Virtual Camera"), err);
}

} // namespace railshot
