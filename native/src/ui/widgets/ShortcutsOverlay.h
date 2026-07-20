#pragma once
#include <QDialog>

namespace railshot {

class ShortcutsOverlay : public QDialog {
    Q_OBJECT
public:
    explicit ShortcutsOverlay(QWidget* parent = nullptr);
};

} // namespace railshot
