#include "auth/loginwidget.h"


#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace qiptv {

LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->addStretch();

    auto *title = new QLabel(tr("<h2>Connect to your IPTV provider</h2>"), this);
    title->setAlignment(Qt::AlignCenter);
    outer->addWidget(title);

    m_profileBox = new QComboBox(this);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("e.g. Home"));
    m_serverEdit = new QLineEdit(this);
    m_serverEdit->setPlaceholderText(tr("http://example.com:8080"));
    m_userEdit = new QLineEdit(this);
    m_passEdit = new QLineEdit(this);
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_saveCheck = new QCheckBox(tr("Save this profile"), this);
    m_saveCheck->setChecked(true);

    auto *form = new QFormLayout;
    form->addRow(tr("Profile:"), m_profileBox);
    form->addRow(tr("Name:"), m_nameEdit);
    form->addRow(tr("Server URL:"), m_serverEdit);
    form->addRow(tr("Username:"), m_userEdit);
    form->addRow(tr("Password:"), m_passEdit);
    form->addRow(QString(), m_saveCheck);

    // Keep the form from stretching edge to edge on wide windows.
    auto *formWrap = new QHBoxLayout;
    formWrap->addStretch();
    auto *formContainer = new QWidget(this);
    formContainer->setLayout(form);
    formContainer->setMaximumWidth(480);
    formWrap->addWidget(formContainer);
    formWrap->addStretch();
    outer->addLayout(formWrap);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet(QStringLiteral("color: palette(link-visited);"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    outer->addWidget(m_errorLabel);

    m_loginButton = new QPushButton(tr("Log in"), this);
    m_loginButton->setDefault(true);
    auto *btnWrap = new QHBoxLayout;
    btnWrap->addStretch();
    btnWrap->addWidget(m_loginButton);
    btnWrap->addStretch();
    outer->addLayout(btnWrap);
    outer->addStretch();

    connect(m_profileBox, &QComboBox::currentIndexChanged, this, &LoginWidget::onProfileSelected);
    connect(m_loginButton, &QPushButton::clicked, this, &LoginWidget::onLoginClicked);
    connect(m_passEdit, &QLineEdit::returnPressed, this, &LoginWidget::onLoginClicked);
}

void LoginWidget::setProfiles(const QVector<Profile> &profiles, const QString &selectedId)
{
    m_profiles = profiles;
    QSignalBlocker block(m_profileBox);
    m_profileBox->clear();
    m_profileBox->addItem(tr("New profile…"), QString());
    int selectIndex = 0;
    for (const Profile &p : profiles) {
        m_profileBox->addItem(p.name.isEmpty() ? p.serverUrl : p.name, p.id);
        if (p.id == selectedId) {
            selectIndex = m_profileBox->count() - 1;
        }
    }
    m_profileBox->setCurrentIndex(selectIndex);
    onProfileSelected(selectIndex);
}

void LoginWidget::onProfileSelected(int index)
{
    const QString id = m_profileBox->itemData(index).toString();
    if (id.isEmpty()) {
        m_nameEdit->clear();
        m_serverEdit->clear();
        m_userEdit->clear();
        m_passEdit->clear();
        return;
    }
    for (const Profile &p : std::as_const(m_profiles)) {
        if (p.id == id) {
            m_nameEdit->setText(p.name);
            m_serverEdit->setText(p.serverUrl);
            m_userEdit->setText(p.username);
            m_passEdit->setText(p.password);
            break;
        }
    }
}

Profile LoginWidget::currentInput() const
{
    Profile p;
    p.id = m_profileBox->currentData().toString();
    p.name = m_nameEdit->text().trimmed();
    p.serverUrl = m_serverEdit->text().trimmed();
    p.username = m_userEdit->text().trimmed();
    p.password = m_passEdit->text();
    if (p.name.isEmpty()) {
        p.name = p.serverUrl; // sensible default label
    }
    return p;
}

bool LoginWidget::saveRequested() const
{
    return m_saveCheck->isChecked();
}

void LoginWidget::onLoginClicked()
{
    const Profile p = currentInput();
    if (!p.isValid()) {
        showError(tr("Please enter a server URL and username."));
        return;
    }
    clearError();
    Q_EMIT loginRequested(p);
}

void LoginWidget::setBusy(bool busy)
{
    m_loginButton->setEnabled(!busy);
    m_loginButton->setText(busy ? tr("Connecting…") : tr("Log in"));
}

void LoginWidget::showError(const QString &message)
{
    m_errorLabel->setText(message);
    m_errorLabel->setVisible(!message.isEmpty());
}

void LoginWidget::clearError()
{
    m_errorLabel->clear();
    m_errorLabel->hide();
}

} // namespace qiptv
