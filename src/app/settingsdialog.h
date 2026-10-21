#pragma once

#include <QDialog>

class QComboBox;
class QCheckBox;
class QSpinBox;

namespace qiptv {

class Settings;

/**
 * Preferences dialog. Loads from the given Settings on construction and, when
 * accepted, writes the edited values back and saves.
 */
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(Settings &settings, QWidget *parent = nullptr);

private:
    void apply();

    Settings &m_settings;
    QComboBox *m_container = nullptr;
    QCheckBox *m_minimizeToTray = nullptr;
    QCheckBox *m_networkCaching = nullptr;
    QSpinBox *m_volume = nullptr;
};

} // namespace qiptv
