#pragma once

#include <QApplication>
#include <memory>

namespace railshot {

class EngineController;
class MainWindow;

class Application {
public:
    Application(int& argc, char** argv);
    ~Application();

    int run();

private:
    bool bootstrap(QString* error);

    std::unique_ptr<QApplication> m_app;
    std::unique_ptr<EngineController> m_engine;
    std::unique_ptr<MainWindow> m_window;
};

} // namespace railshot
