#include <QtTest>

namespace kiptv {
const char *coreVersion();
}

// Phase 1 smoke test: proves the core library builds, links, and the
// QtTest harness runs. Real coverage (API client, JSON parsing) lands
// in Phase 2.
class SmokeTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void coreVersionIsSet()
    {
        QCOMPARE(QString::fromLatin1(kiptv::coreVersion()), QStringLiteral("0.1.0"));
    }
};

QTEST_MAIN(SmokeTest)
#include "test_smoke.moc"
