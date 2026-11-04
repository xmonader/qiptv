#include "core/singleinstance.h"

#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QThread>

namespace qiptv {

namespace {
constexpr int kConnectTimeoutMs = 500;
constexpr int kConnectAttempts = 10; // primary may still be starting up
const QByteArray kActivate = QByteArrayLiteral("activate\n");
} // namespace

SingleInstance::SingleInstance(const QString &dir, const QString &key, QObject *parent)
    : QObject(parent)
    , m_lockPath(QDir(dir).filePath(key + QStringLiteral(".lock")))
    , m_socketPath(QDir(dir).filePath(key + QStringLiteral(".sock")))
{
}

SingleInstance::~SingleInstance() = default;

bool SingleInstance::tryBecomePrimary()
{
    auto lock = std::make_unique<QLockFile>(m_lockPath);
    lock->setStaleLockTime(0); // only PID liveness decides staleness
    if (!lock->tryLock(0)) {
        if (lock->error() == QLockFile::LockFailedError) {
            notifyPrimary();
            return false;
        }
        // Lock dir unusable (permissions, missing runtime dir): don't refuse
        // to start over a convenience feature.
        qWarning("qiptv: single-instance lock unavailable (%s); continuing",
                 qPrintable(m_lockPath));
        return true;
    }
    m_lock = std::move(lock);

    // We hold the lock, so any socket file left behind is stale.
    QLocalServer::removeServer(m_socketPath);
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    if (!m_server->listen(m_socketPath)) {
        qWarning("qiptv: cannot listen on %s: %s", qPrintable(m_socketPath),
                 qPrintable(m_server->errorString()));
        return true; // still primary; later launches just won't raise us
    }
    connect(m_server, &QLocalServer::newConnection, this, &SingleInstance::onNewConnection);
    return true;
}

bool SingleInstance::notifyPrimary() const
{
    for (int attempt = 0; attempt < kConnectAttempts; ++attempt) {
        QLocalSocket socket;
        socket.connectToServer(m_socketPath);
        if (socket.waitForConnected(kConnectTimeoutMs)) {
            socket.write(kActivate);
            const bool ok = socket.waitForBytesWritten(kConnectTimeoutMs);
            socket.disconnectFromServer();
            return ok;
        }
        QThread::msleep(100);
    }
    qWarning("qiptv: another instance is running but did not respond");
    return false;
}

void SingleInstance::onNewConnection()
{
    while (QLocalSocket *socket = m_server->nextPendingConnection()) {
        connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        connect(socket, &QLocalSocket::readyRead, this, [this, socket] {
            if (socket->readAll().contains(kActivate.trimmed())) {
                Q_EMIT activationRequested();
            }
        });
    }
}

} // namespace qiptv
