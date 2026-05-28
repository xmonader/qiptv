#include "app/mainwindow.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <KStandardAction>

#include <QLabel>
#include <QStackedWidget>

namespace kiptv {

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
{
    m_stack = new QStackedWidget(this);

    auto *welcome = new QLabel(
        i18n("kiptv\n\nNative KDE IPTV client (Xtream Codes).\nLogin screen arrives in Phase 3."),
        this);
    welcome->setAlignment(Qt::AlignCenter);
    welcome->setWordWrap(true);
    m_stack->addWidget(welcome);

    setCentralWidget(m_stack);
    setupActions();

    // setupGUI wires the standard menus/toolbars. ToolBar | Keys | Save |
    // Create builds a default UI even before we ship a kiptvui.rc file.
    setupGUI(Default, QStringLiteral("kiptvui.rc"));
}

MainWindow::~MainWindow() = default;

void MainWindow::setupActions()
{
    KStandardAction::quit(this, &QWidget::close, actionCollection());
}

} // namespace kiptv
