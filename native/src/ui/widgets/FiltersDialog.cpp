#include "ui/widgets/FiltersDialog.h"
#include "core/EngineController.h"
#include "core/SceneGraph.h"
#include "core/Types.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QStackedWidget>
#include <QFormLayout>
#include <QMenu>
#include <QCursor>
#include <QJsonArray>
#include <QJsonObject>

namespace railshot {

QJsonArray FiltersDialog::s_clipboard{};
bool FiltersDialog::s_hasClipboard = false;

namespace {
QString filterTypeLabel(const QString& type)
{
    if (type == QLatin1String("chroma_key")) return QStringLiteral("Chroma Key");
    if (type == QLatin1String("blur")) return QStringLiteral("Blur");
    if (type == QLatin1String("color_correction")) return QStringLiteral("Color Correction");
    return type;
}

QJsonArray remappedFilters(const QJsonArray& in)
{
    QJsonArray out;
    for (const auto& v : in) {
        QJsonObject o = v.toObject();
        o.insert(QStringLiteral("id"), newId(QStringLiteral("flt")));
        out.append(o);
    }
    return out;
}
} // namespace

FiltersDialog::FiltersDialog(EngineController* engine, const QString& sourceId, QWidget* parent)
    : QDialog(parent), m_engine(engine), m_sourceId(sourceId)
{
    setWindowTitle(QStringLiteral("Filters"));
    setMinimumSize(560, 400);
    setStyleSheet(QStringLiteral(
        "QDialog{background:#0F1114;}"
        "QLabel{color:#C8CCD4; font-family:'DM Sans';}"
        "QListWidget{background:#0A0C0F; border:1px solid #2A2D35; color:#E0E2E8;}"
        "QListWidget::item:selected{background:#1A2A3A; color:#7AB8FF;}"
        "QPushButton{background:#1A1E26; border:1px solid #5A5E68; border-radius:3px;"
        "  color:#E0E2E8; font-weight:700; padding:4px 10px;}"
        "QPushButton:hover{border-color:#4F9EFF;}"
        "QPushButton:disabled{color:#505860; border-color:#2A2D35;}"
        "QCheckBox{color:#E0E2E8;}"));

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(10);

    auto* left = new QVBoxLayout();
    m_list = new QListWidget(this);
    left->addWidget(m_list, 1);

    auto* tools = new QHBoxLayout();
    auto* addBtn = new QPushButton(QStringLiteral("+"), this);
    auto* remBtn = new QPushButton(QStringLiteral("\u2212"), this);
    m_upBtn = new QPushButton(QStringLiteral("▲"), this);
    m_downBtn = new QPushButton(QStringLiteral("▼"), this);
    addBtn->setFixedWidth(32);
    remBtn->setFixedWidth(32);
    m_upBtn->setFixedWidth(32);
    m_downBtn->setFixedWidth(32);
    m_upBtn->setToolTip(QStringLiteral("Move filter up"));
    m_downBtn->setToolTip(QStringLiteral("Move filter down"));
    tools->addWidget(addBtn);
    tools->addWidget(remBtn);
    tools->addSpacing(6);
    tools->addWidget(m_upBtn);
    tools->addWidget(m_downBtn);
    tools->addStretch();
    left->addLayout(tools);
    root->addLayout(left, 1);

    auto* right = new QVBoxLayout();
    m_hint = new QLabel(QStringLiteral("Select or add a filter"), this);
    right->addWidget(m_hint);

    m_stack = new QStackedWidget(this);
    m_emptyPage = new QWidget(m_stack);
    m_stack->addWidget(m_emptyPage);

    m_chromaPage = new QWidget(m_stack);
    {
        auto* form = new QFormLayout(m_chromaPage);
        m_chromaEnabled = new QCheckBox(QStringLiteral("Enabled"), m_chromaPage);
        m_chromaSim = new QSlider(Qt::Horizontal, m_chromaPage);
        m_chromaSim->setRange(5, 95);
        form->addRow(m_chromaEnabled);
        form->addRow(QStringLiteral("Similarity"), m_chromaSim);
        form->addRow(new QLabel(QStringLiteral("Green screen key — live in compositor."), m_chromaPage));
    }
    m_stack->addWidget(m_chromaPage);

    m_blurPage = new QWidget(m_stack);
    {
        auto* form = new QFormLayout(m_blurPage);
        m_blurEnabled = new QCheckBox(QStringLiteral("Enabled"), m_blurPage);
        m_blurAmount = new QSlider(Qt::Horizontal, m_blurPage);
        m_blurAmount->setRange(0, 100);
        form->addRow(m_blurEnabled);
        form->addRow(QStringLiteral("Amount"), m_blurAmount);
    }
    m_stack->addWidget(m_blurPage);

    m_colorPage = new QWidget(m_stack);
    {
        auto* form = new QFormLayout(m_colorPage);
        m_colorEnabled = new QCheckBox(QStringLiteral("Enabled"), m_colorPage);
        m_brightness = new QSlider(Qt::Horizontal, m_colorPage);
        m_brightness->setRange(-100, 100);
        m_contrast = new QSlider(Qt::Horizontal, m_colorPage);
        m_contrast->setRange(-100, 100);
        m_saturation = new QSlider(Qt::Horizontal, m_colorPage);
        m_saturation->setRange(-100, 100);
        form->addRow(m_colorEnabled);
        form->addRow(QStringLiteral("Brightness"), m_brightness);
        form->addRow(QStringLiteral("Contrast"), m_contrast);
        form->addRow(QStringLiteral("Saturation"), m_saturation);
        form->addRow(new QLabel(QStringLiteral("OBS Color Correction — live in compositor."), m_colorPage));
    }
    m_stack->addWidget(m_colorPage);
    right->addWidget(m_stack, 1);

    auto* closeRow = new QHBoxLayout();
    closeRow->addStretch();
    auto* closeBtn = new QPushButton(QStringLiteral("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    closeRow->addWidget(closeBtn);
    right->addLayout(closeRow);
    root->addLayout(right, 2);

    connect(addBtn, &QPushButton::clicked, this, &FiltersDialog::onAddFilter);
    connect(remBtn, &QPushButton::clicked, this, &FiltersDialog::onRemoveFilter);
    connect(m_upBtn, &QPushButton::clicked, this, &FiltersDialog::onMoveUp);
    connect(m_downBtn, &QPushButton::clicked, this, &FiltersDialog::onMoveDown);
    connect(m_list, &QListWidget::currentRowChanged, this, &FiltersDialog::onSelectionChanged);
    connect(m_chromaEnabled, &QCheckBox::toggled, this, &FiltersDialog::saveCurrent);
    connect(m_chromaSim, &QSlider::valueChanged, this, &FiltersDialog::saveCurrent);
    connect(m_blurEnabled, &QCheckBox::toggled, this, &FiltersDialog::saveCurrent);
    connect(m_blurAmount, &QSlider::valueChanged, this, &FiltersDialog::saveCurrent);
    connect(m_colorEnabled, &QCheckBox::toggled, this, &FiltersDialog::saveCurrent);
    connect(m_brightness, &QSlider::valueChanged, this, &FiltersDialog::saveCurrent);
    connect(m_contrast, &QSlider::valueChanged, this, &FiltersDialog::saveCurrent);
    connect(m_saturation, &QSlider::valueChanged, this, &FiltersDialog::saveCurrent);

    reload();
}

void FiltersDialog::copyFilters(const QJsonArray& filters)
{
    s_clipboard = filters;
    s_hasClipboard = !filters.isEmpty();
}

bool FiltersDialog::hasFilterClipboard() { return s_hasClipboard && !s_clipboard.isEmpty(); }

QJsonArray FiltersDialog::filterClipboard() { return s_clipboard; }

void FiltersDialog::pasteOnto(EngineController* engine, const QString& sourceId)
{
    if (!engine || sourceId.isEmpty() || !hasFilterClipboard()) return;
    const QJsonArray pasted = remappedFilters(s_clipboard);
    engine->sceneGraph()->mutate([&](Project& p) {
        if (auto* s = p.findSourceAnywhere(sourceId)) {
            s->settings.insert(QStringLiteral("filters"), pasted);
            bool chroma = false;
            int sim = 40;
            int blur = 0;
            int bri = 0;
            int con = 0;
            int sat = 0;
            for (const auto& v : pasted) {
                const auto o = v.toObject();
                if (!o.value(QStringLiteral("enabled")).toBool(true)) continue;
                const QString type = o.value(QStringLiteral("type")).toString();
                if (type == QLatin1String("chroma_key")) {
                    chroma = true;
                    sim = o.value(QStringLiteral("similarity")).toInt(40);
                } else if (type == QLatin1String("blur")) {
                    blur = qMax(blur, o.value(QStringLiteral("amount")).toInt(0));
                } else if (type == QLatin1String("color_correction")) {
                    bri = o.value(QStringLiteral("brightness")).toInt(0);
                    con = o.value(QStringLiteral("contrast")).toInt(0);
                    sat = o.value(QStringLiteral("saturation")).toInt(0);
                }
            }
            s->settings.insert(QStringLiteral("chromaKey"), chroma);
            s->settings.insert(QStringLiteral("chromaSimilarity"), sim);
            s->settings.insert(QStringLiteral("blur"), blur);
            s->settings.insert(QStringLiteral("brightness"), bri);
            s->settings.insert(QStringLiteral("contrast"), con);
            s->settings.insert(QStringLiteral("saturation"), sat);
        }
    });
}

void FiltersDialog::reload()
{
    m_loading = true;
    m_list->clear();
    const auto* src = m_engine->projectSnapshot().findSourceAnywhere(m_sourceId);
    if (!src) {
        m_loading = false;
        return;
    }
    setWindowTitle(QStringLiteral("Filters — %1").arg(src->name));

    QJsonArray filters = src->settings.value(QStringLiteral("filters")).toArray();
    if (filters.isEmpty()) {
        if (src->settings.value(QStringLiteral("chromaKey")).toBool()) {
            QJsonObject f;
            f.insert(QStringLiteral("id"), newId(QStringLiteral("flt")));
            f.insert(QStringLiteral("type"), QStringLiteral("chroma_key"));
            f.insert(QStringLiteral("enabled"), true);
            f.insert(QStringLiteral("similarity"),
                     src->settings.value(QStringLiteral("chromaSimilarity")).toInt(40));
            filters.append(f);
        }
        if (src->settings.value(QStringLiteral("blur")).toInt(0) > 0) {
            QJsonObject f;
            f.insert(QStringLiteral("id"), newId(QStringLiteral("flt")));
            f.insert(QStringLiteral("type"), QStringLiteral("blur"));
            f.insert(QStringLiteral("enabled"), true);
            f.insert(QStringLiteral("amount"), src->settings.value(QStringLiteral("blur")).toInt(0));
            filters.append(f);
        }
        const int bri = src->settings.value(QStringLiteral("brightness")).toInt(0);
        const int con = src->settings.value(QStringLiteral("contrast")).toInt(0);
        const int sat = src->settings.value(QStringLiteral("saturation")).toInt(0);
        if (bri != 0 || con != 0 || sat != 0) {
            QJsonObject f;
            f.insert(QStringLiteral("id"), newId(QStringLiteral("flt")));
            f.insert(QStringLiteral("type"), QStringLiteral("color_correction"));
            f.insert(QStringLiteral("enabled"), true);
            f.insert(QStringLiteral("brightness"), bri);
            f.insert(QStringLiteral("contrast"), con);
            f.insert(QStringLiteral("saturation"), sat);
            filters.append(f);
        }
        if (!filters.isEmpty()) {
            m_engine->sceneGraph()->mutate([this, filters](Project& p) {
                if (auto* s = p.findSourceAnywhere(m_sourceId))
                    s->settings.insert(QStringLiteral("filters"), filters);
            });
        }
    }

    for (const auto& v : filters) {
        const auto o = v.toObject();
        const QString type = o.value(QStringLiteral("type")).toString();
        const bool en = o.value(QStringLiteral("enabled")).toBool(true);
        auto* item = new QListWidgetItem(
            QStringLiteral("%1%2").arg(en ? QString() : QStringLiteral("[off] "), filterTypeLabel(type)),
            m_list);
        item->setData(Qt::UserRole, o.value(QStringLiteral("id")).toString());
        item->setData(Qt::UserRole + 1, type);
        item->setCheckState(en ? Qt::Checked : Qt::Unchecked);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    }
    m_loading = false;

    // Toggle enable from list checkbox
    disconnect(m_list, &QListWidget::itemChanged, nullptr, nullptr);
    connect(m_list, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
        if (m_loading || !item) return;
        const QString id = item->data(Qt::UserRole).toString();
        const bool en = item->checkState() == Qt::Checked;
        m_engine->sceneGraph()->mutate([this, id, en](Project& p) {
            if (auto* s = p.findSourceAnywhere(m_sourceId)) {
                auto arr = s->settings.value(QStringLiteral("filters")).toArray();
                for (int i = 0; i < arr.size(); ++i) {
                    auto o = arr.at(i).toObject();
                    if (o.value(QStringLiteral("id")).toString() != id) continue;
                    o.insert(QStringLiteral("enabled"), en);
                    arr.replace(i, o);
                    break;
                }
                s->settings.insert(QStringLiteral("filters"), arr);
            }
        });
        syncLegacyKeys();
        const int keep = m_list->row(item);
        reload();
        if (keep >= 0 && keep < m_list->count())
            m_list->setCurrentRow(keep);
    });

    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
    else
        onSelectionChanged();
}

void FiltersDialog::onAddFilter()
{
    QMenu menu(this);
    auto* color = menu.addAction(QStringLiteral("Color Correction"));
    auto* chroma = menu.addAction(QStringLiteral("Chroma Key"));
    auto* blur = menu.addAction(QStringLiteral("Blur"));
    auto* chosen = menu.exec(QCursor::pos());
    if (!chosen) return;

    QJsonObject f;
    f.insert(QStringLiteral("id"), newId(QStringLiteral("flt")));
    f.insert(QStringLiteral("enabled"), true);
    if (chosen == color) {
        f.insert(QStringLiteral("type"), QStringLiteral("color_correction"));
        f.insert(QStringLiteral("brightness"), 0);
        f.insert(QStringLiteral("contrast"), 0);
        f.insert(QStringLiteral("saturation"), 0);
    } else if (chosen == chroma) {
        f.insert(QStringLiteral("type"), QStringLiteral("chroma_key"));
        f.insert(QStringLiteral("similarity"), 40);
    } else {
        f.insert(QStringLiteral("type"), QStringLiteral("blur"));
        f.insert(QStringLiteral("amount"), 25);
    }

    m_engine->sceneGraph()->mutate([this, f](Project& p) {
        if (auto* s = p.findSourceAnywhere(m_sourceId)) {
            auto arr = s->settings.value(QStringLiteral("filters")).toArray();
            arr.append(f);
            s->settings.insert(QStringLiteral("filters"), arr);
        }
    });
    syncLegacyKeys();
    reload();
    m_list->setCurrentRow(m_list->count() - 1);
}

void FiltersDialog::onRemoveFilter()
{
    if (!m_list->currentItem()) return;
    const QString id = m_list->currentItem()->data(Qt::UserRole).toString();
    m_engine->sceneGraph()->mutate([this, id](Project& p) {
        if (auto* s = p.findSourceAnywhere(m_sourceId)) {
            auto arr = s->settings.value(QStringLiteral("filters")).toArray();
            QJsonArray next;
            for (const auto& v : arr) {
                if (v.toObject().value(QStringLiteral("id")).toString() != id)
                    next.append(v);
            }
            s->settings.insert(QStringLiteral("filters"), next);
        }
    });
    syncLegacyKeys();
    reload();
}

void FiltersDialog::onMoveUp() { moveFilter(-1); }
void FiltersDialog::onMoveDown() { moveFilter(1); }

void FiltersDialog::moveFilter(int delta)
{
    const int row = m_list->currentRow();
    if (row < 0) return;
    const int dest = row + delta;
    if (dest < 0 || dest >= m_list->count()) return;
    const QString id = m_list->item(row)->data(Qt::UserRole).toString();

    m_engine->sceneGraph()->mutate([this, id, delta](Project& p) {
        if (auto* s = p.findSourceAnywhere(m_sourceId)) {
            auto arr = s->settings.value(QStringLiteral("filters")).toArray();
            int idx = -1;
            for (int i = 0; i < arr.size(); ++i) {
                if (arr.at(i).toObject().value(QStringLiteral("id")).toString() == id) {
                    idx = i;
                    break;
                }
            }
            if (idx < 0) return;
            const int destIdx = idx + delta;
            if (destIdx < 0 || destIdx >= arr.size()) return;
            const QJsonValue a = arr.at(idx);
            const QJsonValue b = arr.at(destIdx);
            arr.replace(idx, b);
            arr.replace(destIdx, a);
            s->settings.insert(QStringLiteral("filters"), arr);
        }
    });
    reload();
    m_list->setCurrentRow(dest);
}

void FiltersDialog::onSelectionChanged()
{
    m_loading = true;
    const int row = m_list->currentRow();
    m_upBtn->setEnabled(row > 0);
    m_downBtn->setEnabled(row >= 0 && row < m_list->count() - 1);
    if (row < 0) {
        m_stack->setCurrentWidget(m_emptyPage);
        m_hint->setText(QStringLiteral("Select or add a filter"));
        m_loading = false;
        return;
    }
    const QString type = m_list->item(row)->data(Qt::UserRole + 1).toString();
    const QString id = m_list->item(row)->data(Qt::UserRole).toString();
    const auto* src = m_engine->projectSnapshot().findSourceAnywhere(m_sourceId);
    QJsonObject f;
    if (src) {
        for (const auto& v : src->settings.value(QStringLiteral("filters")).toArray()) {
            if (v.toObject().value(QStringLiteral("id")).toString() == id) {
                f = v.toObject();
                break;
            }
        }
    }
    m_hint->setText(filterTypeLabel(type));
    if (type == QLatin1String("chroma_key")) {
        m_chromaEnabled->setChecked(f.value(QStringLiteral("enabled")).toBool(true));
        m_chromaSim->setValue(f.value(QStringLiteral("similarity")).toInt(40));
        m_stack->setCurrentWidget(m_chromaPage);
    } else if (type == QLatin1String("blur")) {
        m_blurEnabled->setChecked(f.value(QStringLiteral("enabled")).toBool(true));
        m_blurAmount->setValue(f.value(QStringLiteral("amount")).toInt(0));
        m_stack->setCurrentWidget(m_blurPage);
    } else if (type == QLatin1String("color_correction")) {
        m_colorEnabled->setChecked(f.value(QStringLiteral("enabled")).toBool(true));
        m_brightness->setValue(f.value(QStringLiteral("brightness")).toInt(0));
        m_contrast->setValue(f.value(QStringLiteral("contrast")).toInt(0));
        m_saturation->setValue(f.value(QStringLiteral("saturation")).toInt(0));
        m_stack->setCurrentWidget(m_colorPage);
    } else {
        m_stack->setCurrentWidget(m_emptyPage);
    }
    m_loading = false;
}

void FiltersDialog::saveCurrent()
{
    if (m_loading) return;
    const int row = m_list->currentRow();
    if (row < 0) return;
    const QString id = m_list->item(row)->data(Qt::UserRole).toString();
    const QString type = m_list->item(row)->data(Qt::UserRole + 1).toString();

    m_engine->sceneGraph()->mutate([this, id, type](Project& p) {
        if (auto* s = p.findSourceAnywhere(m_sourceId)) {
            auto arr = s->settings.value(QStringLiteral("filters")).toArray();
            for (int i = 0; i < arr.size(); ++i) {
                auto o = arr.at(i).toObject();
                if (o.value(QStringLiteral("id")).toString() != id) continue;
                if (type == QLatin1String("chroma_key")) {
                    o.insert(QStringLiteral("enabled"), m_chromaEnabled->isChecked());
                    o.insert(QStringLiteral("similarity"), m_chromaSim->value());
                } else if (type == QLatin1String("blur")) {
                    o.insert(QStringLiteral("enabled"), m_blurEnabled->isChecked());
                    o.insert(QStringLiteral("amount"), m_blurAmount->value());
                } else if (type == QLatin1String("color_correction")) {
                    o.insert(QStringLiteral("enabled"), m_colorEnabled->isChecked());
                    o.insert(QStringLiteral("brightness"), m_brightness->value());
                    o.insert(QStringLiteral("contrast"), m_contrast->value());
                    o.insert(QStringLiteral("saturation"), m_saturation->value());
                }
                arr.replace(i, o);
                break;
            }
            s->settings.insert(QStringLiteral("filters"), arr);
        }
    });
    syncLegacyKeys();
    const int keep = row;
    reload();
    if (keep >= 0 && keep < m_list->count())
        m_list->setCurrentRow(keep);
}

void FiltersDialog::syncLegacyKeys()
{
    m_engine->sceneGraph()->mutate([this](Project& p) {
        if (auto* s = p.findSourceAnywhere(m_sourceId)) {
            bool chroma = false;
            int sim = 40;
            int blur = 0;
            int bri = 0;
            int con = 0;
            int sat = 0;
            for (const auto& v : s->settings.value(QStringLiteral("filters")).toArray()) {
                const auto o = v.toObject();
                if (!o.value(QStringLiteral("enabled")).toBool(true)) continue;
                const QString type = o.value(QStringLiteral("type")).toString();
                if (type == QLatin1String("chroma_key")) {
                    chroma = true;
                    sim = o.value(QStringLiteral("similarity")).toInt(40);
                } else if (type == QLatin1String("blur")) {
                    blur = qMax(blur, o.value(QStringLiteral("amount")).toInt(0));
                } else if (type == QLatin1String("color_correction")) {
                    bri = o.value(QStringLiteral("brightness")).toInt(0);
                    con = o.value(QStringLiteral("contrast")).toInt(0);
                    sat = o.value(QStringLiteral("saturation")).toInt(0);
                }
            }
            s->settings.insert(QStringLiteral("chromaKey"), chroma);
            s->settings.insert(QStringLiteral("chromaSimilarity"), sim);
            s->settings.insert(QStringLiteral("blur"), blur);
            s->settings.insert(QStringLiteral("brightness"), bri);
            s->settings.insert(QStringLiteral("contrast"), con);
            s->settings.insert(QStringLiteral("saturation"), sat);
        }
    });
}

} // namespace railshot
