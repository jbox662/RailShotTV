#pragma once

#include "core/Types.h"
#include <QWidget>
#include <QJsonObject>

namespace railshot {
class EngineController;

/// Base for the leading "Source" tab in Properties.
class SourcePropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit SourcePropertiesPanel(EngineController* engine, QWidget* parent = nullptr)
        : QWidget(parent), m_engine(engine)
    {
    }

    virtual void loadFrom(const SourceItem& src) = 0;
    /// Merge panel fields into settings (preserves unrelated keys).
    virtual void applyTo(QJsonObject& settings) const = 0;
    /// Reset type-specific keys to defaults (does not clear FX/audio unless owned by panel).
    virtual void resetDefaults(QJsonObject& settings) const = 0;

signals:
    void settingsEdited();

protected:
    EngineController* m_engine = nullptr;
};

SourcePropertiesPanel* createSourcePropertiesPanel(SourceType type, EngineController* engine,
                                                   QWidget* parent = nullptr);

} // namespace railshot
