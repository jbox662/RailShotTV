#include "ui/widgets/ExtraBrowserPanel.h"
#include "core/EngineController.h"
#include "core/Types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QLabel>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QJsonObject>

namespace railshot {

ExtraBrowserPanel::ExtraBrowserPanel(EngineController* engine, const QString& panelId,
                                     const QString& title, const QString& url, QWidget* parent)
    : QWidget(parent), m_engine(engine), m_id(panelId), m_title(title)
{
    setObjectName(QStringLiteral("extraBrowserPanel"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->setSpacing(4);

    auto* bar = new QHBoxLayout();
    m_url = new QLineEdit(this);
    m_url->setPlaceholderText(QStringLiteral("https://… or local .html path"));
    m_url->setText(url);
    auto* go = new QPushButton(QStringLiteral("Go"), this);
    auto* reload = new QPushButton(QStringLiteral("Reload"), this);
    auto* external = new QPushButton(QStringLiteral("System"), this);
    external->setToolTip(QStringLiteral("Open in system browser (full JS/CSS)"));
    auto* asSource = new QPushButton(QStringLiteral("+ Source"), this);
    asSource->setToolTip(QStringLiteral("Add as Browser source on the Preview scene"));
    auto* remove = new QPushButton(QStringLiteral("✕"), this);
    remove->setFixedWidth(28);
    remove->setToolTip(QStringLiteral("Remove this browser panel"));
    bar->addWidget(m_url, 1);
    bar->addWidget(go);
    bar->addWidget(reload);
    bar->addWidget(external);
    bar->addWidget(asSource);
    bar->addWidget(remove);
    root->addLayout(bar);

    m_hint = new QLabel(
        QStringLiteral("Dock preview uses a simple HTML viewer. Use System for modern sites, or + Source for the compositor."),
        this);
    m_hint->setWordWrap(true);
    m_hint->setStyleSheet(QStringLiteral("color:#606878; font-size:10px;"));
    root->addWidget(m_hint);

    m_view = new QTextBrowser(this);
    m_view->setOpenExternalLinks(true);
    m_view->setStyleSheet(QStringLiteral("QTextBrowser{background:#0C0E12; color:#D0D2D8; border:1px solid #2A2D35;}"));
    root->addWidget(m_view, 1);

    connect(go, &QPushButton::clicked, this, &ExtraBrowserPanel::navigate);
    connect(reload, &QPushButton::clicked, this, &ExtraBrowserPanel::reload);
    connect(external, &QPushButton::clicked, this, &ExtraBrowserPanel::openExternal);
    connect(asSource, &QPushButton::clicked, this, &ExtraBrowserPanel::addAsSource);
    connect(remove, &QPushButton::clicked, this, [this] { emit removeRequested(m_id); });
    connect(m_url, &QLineEdit::returnPressed, this, &ExtraBrowserPanel::navigate);

    if (!url.isEmpty())
        navigateTo(url);
}

QString ExtraBrowserPanel::currentUrl() const
{
    return m_url ? m_url->text().trimmed() : QString();
}

void ExtraBrowserPanel::navigateTo(const QString& url)
{
    if (m_url) m_url->setText(url);
    navigate();
}

void ExtraBrowserPanel::reload()
{
    navigate();
}

void ExtraBrowserPanel::navigate()
{
    const QString text = currentUrl();
    emit urlChanged(m_id, text);
    if (text.isEmpty()) {
        m_view->setHtml(QStringLiteral("<p style='color:#8892A4'>Enter a URL or local HTML path.</p>"));
        return;
    }
    QUrl u = QUrl::fromUserInput(text);
    if (u.isLocalFile() || text.endsWith(QLatin1String(".html"), Qt::CaseInsensitive)
        || text.endsWith(QLatin1String(".htm"), Qt::CaseInsensitive)) {
        const QString path = u.isLocalFile() ? u.toLocalFile() : text;
        if (QFileInfo::exists(path)) {
            m_view->setSource(QUrl::fromLocalFile(path));
            return;
        }
    }
    if (u.scheme().startsWith(QLatin1String("http"))) {
        m_view->setHtml(QStringLiteral(
            "<div style='font-family:Segoe UI;padding:16px;color:#A0A8B8'>"
            "<p><b style='color:#F0F0F0'>%1</b></p>"
            "<p>Remote pages with modern JS need the system browser or a Browser source.</p>"
            "<p><a href=\"%1\">Open here (basic)</a> — or use <b>System</b> / <b>+ Source</b>.</p>"
            "</div>").arg(text.toHtmlEscaped()));
        // Still try QTextBrowser load for simple pages
        m_view->setSource(u);
        return;
    }
    m_view->setSource(u);
}

void ExtraBrowserPanel::openExternal()
{
    const QString text = currentUrl();
    if (text.isEmpty()) return;
    QDesktopServices::openUrl(QUrl::fromUserInput(text));
}

void ExtraBrowserPanel::addAsSource()
{
    if (!m_engine) return;
    const QString text = currentUrl();
    if (text.isEmpty()) return;
    QJsonObject settings;
    settings.insert(QStringLiteral("url"), text);
    settings.insert(QStringLiteral("width"), 1280);
    settings.insert(QStringLiteral("height"), 720);
    settings.insert(QStringLiteral("fps"), 30);
    const QString name = m_title.isEmpty() ? QStringLiteral("Browser Panel") : m_title;
    const QString id = m_engine->addSource(SourceType::Browser, name, settings);
    m_engine->setSelectedSourceId(id);
}

} // namespace railshot
