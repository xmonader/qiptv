#pragma once

#include <QObject>
#include <QString>

#include <memory>

class QLocalServer;
class QLockFile;

namespace qiptv {

/**
 * Keeps qiptv to one running instance per user.
 *
 * A QLockFile decides who is primary (it detects stale locks left by a
 * crashed process, so a crash never blocks the next launch). The primary
 * also listens on a local socket; a second launch connects to it to ask the
 * running instance to show its window, then exits.
 *
 * Both files live in @p dir (the user's runtime dir in production).
 */
class SingleInstance : public QObject {
    Q_OBJECT
public:
    SingleInstance(const QString &dir, const QString &key, QObject *parent = nullptr);
    ~SingleInstance() override;

    /// True if this process is now the primary instance. False if another
    /// instance holds the lock; that instance has been asked to activate.
    bool tryBecomePrimary();

    QString socketPath() const { return m_socketPath; }

Q_SIGNALS:
    /// Emitted in the primary when a later launch asks it to come forward.
    void activationRequested();

private:
    bool notifyPrimary() const;
    void onNewConnection();

    QString m_lockPath;
    QString m_socketPath;
    std::unique_ptr<QLockFile> m_lock;
    QLocalServer *m_server = nullptr;
};

} // namespace qiptv
