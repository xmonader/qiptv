#pragma once

#include <KXmlGuiWindow>

class QStackedWidget;

namespace kiptv {

/**
 * Main application window. KXmlGuiWindow gives us KDE-standard menus,
 * toolbars, and action management via KActionCollection.
 *
 * In Phase 1 this is a skeleton: a central placeholder and standard
 * quit action. Later phases add the navigation, browsing views, and
 * player as central widgets in a QStackedWidget.
 */
class MainWindow : public KXmlGuiWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupActions();

    QStackedWidget *m_stack = nullptr;
};

} // namespace kiptv
