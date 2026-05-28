#include "app/settingsdialog.h"

#include "config/settings.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QVBoxLayout>

namespace kiptv {

SettingsDialog::SettingsDialog(Settings &settings, QWidget *parent)
    : QDialog(parent)
    , m_settings(settings)
{
    setWindowTitle(i18nc("@title:window", "Settings"));

    m_container = new QComboBox(this);
    m_container->addItem(QStringLiteral("ts"));
    m_container->addItem(QStringLiteral("m3u8"));
    const int idx = m_container->findText(settings.liveContainer());
    m_container->setCurrentIndex(idx >= 0 ? idx : 0);

    m_minimizeToTray = new QCheckBox(i18n("Minimize to system tray on close"), this);
    m_minimizeToTray->setChecked(settings.minimizeToTray());

    m_networkCaching = new QCheckBox(i18n("Enable network stream cache"), this);
    m_networkCaching->setChecked(settings.networkCaching());

    m_volume = new QSpinBox(this);
    m_volume->setRange(0, 150);
    m_volume->setSuffix(QStringLiteral(" %"));
    m_volume->setValue(settings.lastVolume());

    auto *form = new QFormLayout;
    form->addRow(i18n("Live stream format:"), m_container);
    form->addRow(i18n("Default volume:"), m_volume);
    form->addRow(QString(), m_networkCaching);
    form->addRow(QString(), m_minimizeToTray);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(this, &QDialog::accepted, this, &SettingsDialog::apply);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void SettingsDialog::apply()
{
    m_settings.setLiveContainer(m_container->currentText());
    m_settings.setMinimizeToTray(m_minimizeToTray->isChecked());
    m_settings.setNetworkCaching(m_networkCaching->isChecked());
    m_settings.setLastVolume(m_volume->value());
    m_settings.save();
}

} // namespace kiptv
