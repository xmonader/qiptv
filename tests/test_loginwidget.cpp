#include "auth/loginwidget.h"

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QtTest>

using namespace kiptv;

// Widget-level behaviour: validation, signal emission, profile prefill.
// Runs under the offscreen platform (set via the test's ENVIRONMENT).
class LoginWidgetTest : public QObject {
    Q_OBJECT
private:
    static QPushButton *loginButton(LoginWidget *w)
    {
        const auto buttons = w->findChildren<QPushButton *>();
        return buttons.isEmpty() ? nullptr : buttons.first();
    }

private Q_SLOTS:
    void invalidInputShowsErrorAndNoSignal()
    {
        LoginWidget w;
        int emitted = 0;
        connect(&w, &LoginWidget::loginRequested, [&](const Profile &) { ++emitted; });

        QPushButton *btn = loginButton(&w);
        QVERIFY(btn);
        btn->click(); // all fields empty
        QCOMPARE(emitted, 0);
    }

    void validInputEmitsProfile()
    {
        LoginWidget w;
        Profile got;
        int emitted = 0;
        connect(&w, &LoginWidget::loginRequested, [&](const Profile &p) {
            got = p;
            ++emitted;
        });

        const auto edits = w.findChildren<QLineEdit *>();
        // Order of construction: name, server, user, pass.
        QVERIFY(edits.size() >= 4);
        edits[0]->setText("Home");
        edits[1]->setText("http://x.tv:8080");
        edits[2]->setText("bob");
        edits[3]->setText("secret");

        loginButton(&w)->click();
        QCOMPARE(emitted, 1);
        QCOMPARE(got.name, QStringLiteral("Home"));
        QCOMPARE(got.serverUrl, QStringLiteral("http://x.tv:8080"));
        QCOMPARE(got.username, QStringLiteral("bob"));
        QCOMPARE(got.password, QStringLiteral("secret"));
    }

    void selectingSavedProfileFillsFields()
    {
        LoginWidget w;
        Profile p;
        p.id = "id-1";
        p.name = "Saved";
        p.serverUrl = "http://saved.tv";
        p.username = "alice";
        p.password = "pw";
        w.setProfiles({p}, "id-1");

        const auto edits = w.findChildren<QLineEdit *>();
        QCOMPARE(edits[1]->text(), QStringLiteral("http://saved.tv"));
        QCOMPARE(edits[2]->text(), QStringLiteral("alice"));

        // The emitted profile should carry the saved id (an update, not new).
        Profile got;
        connect(&w, &LoginWidget::loginRequested, [&](const Profile &x) { got = x; });
        loginButton(&w)->click();
        QCOMPARE(got.id, QStringLiteral("id-1"));
    }

    void newProfileEntryHasEmptyId()
    {
        LoginWidget w;
        Profile saved;
        saved.id = "id-1";
        saved.name = "Saved";
        saved.serverUrl = "http://saved.tv";
        saved.username = "alice";
        w.setProfiles({saved}, QString()); // select "New profile…"

        const auto edits = w.findChildren<QLineEdit *>();
        edits[1]->setText("http://new.tv");
        edits[2]->setText("carol");

        Profile got;
        connect(&w, &LoginWidget::loginRequested, [&](const Profile &x) { got = x; });
        loginButton(&w)->click();
        QVERIFY(got.id.isEmpty()); // brand-new profile
        QCOMPARE(got.serverUrl, QStringLiteral("http://new.tv"));
    }
};

QTEST_MAIN(LoginWidgetTest)
#include "test_loginwidget.moc"
