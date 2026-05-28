#pragma once

#include "config/profile.h"

#include <QWidget>
#include <QVector>

class QComboBox;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QLabel;

namespace kiptv {

/**
 * Login form: pick a saved profile or enter new credentials.
 *
 * Emits loginRequested() with the entered Profile (id empty for a brand-new
 * one). The owning window performs the actual authentication and reports
 * back via showError()/setBusy().
 */
class LoginWidget : public QWidget {
    Q_OBJECT
public:
    explicit LoginWidget(QWidget *parent = nullptr);

    /// Populate the saved-profile selector. selectedId is pre-selected.
    void setProfiles(const QVector<Profile> &profiles, const QString &selectedId = QString());

    /// Whether "Save this profile" is checked.
    bool saveRequested() const;

    void setBusy(bool busy);
    void showError(const QString &message);
    void clearError();

Q_SIGNALS:
    void loginRequested(const kiptv::Profile &profile);

private:
    void onProfileSelected(int index);
    void onLoginClicked();
    Profile currentInput() const;

    QComboBox *m_profileBox = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_serverEdit = nullptr;
    QLineEdit *m_userEdit = nullptr;
    QLineEdit *m_passEdit = nullptr;
    QCheckBox *m_saveCheck = nullptr;
    QPushButton *m_loginButton = nullptr;
    QLabel *m_errorLabel = nullptr;

    QVector<Profile> m_profiles;
};

} // namespace kiptv
