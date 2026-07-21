#pragma once
#include <QDialog>

namespace railshot {
class EngineController;

class ShortcutsOverlay : public QDialog {
    Q_OBJECT
public:
    explicit ShortcutsOverlay(EngineController* engine, QWidget* parent = nullptr);
};

} // namespace railshot
