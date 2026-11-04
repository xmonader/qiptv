#include "api/xtreamclient.h"
#include "browse/browsewidget.h"
#include "core/imageloader.h"
#include "models/livestream.h"

#include <QListWidget>
#include <QtTest>

using namespace qiptv;

// Poster/logo icons must be stored as thumbnails at the view's icon size, not
// at the decoded size: each list item keeps its icon while the list is shown.
class BrowseWidgetTest : public QObject {
    Q_OBJECT
private:
    // Port 9 (discard) on loopback: requests the widget triggers fail fast.
    static QString iconUrl(const char *name)
    {
        return QStringLiteral("http://127.0.0.1:9/%1.png").arg(QLatin1String(name));
    }

    static LiveStream channel(int id, const QString &icon)
    {
        LiveStream s;
        s.streamId = id;
        s.name = QStringLiteral("ch%1").arg(id);
        s.streamIcon = icon;
        return s;
    }

    static QListWidget *contentList(BrowseWidget &w)
    {
        // Sections, categories, content — the content view is the IconMode one.
        for (QListWidget *l : w.findChildren<QListWidget *>()) {
            if (l->viewMode() == QListView::IconMode) {
                return l;
            }
        }
        return nullptr;
    }

    static QSize largestIconSize(const QListWidgetItem *item)
    {
        QSize best;
        for (const QSize &s : item->icon().availableSizes()) {
            if (s.width() * s.height() > best.width() * best.height()) {
                best = s;
            }
        }
        return best;
    }

private Q_SLOTS:
    void iconsAreThumbnailsAndSharedByUrl()
    {
        XtreamClient client;
        ImageLoader images;
        BrowseWidget w(&client, &images);
        QListWidget *list = contentList(w);
        QVERIFY(list);

        const QString poster = iconUrl("a");
        Q_EMIT client.liveStreamsReady(
            {channel(1, poster), channel(2, poster), channel(3, iconUrl("other"))});
        QCOMPARE(list->count(), 3);
        const QSize fallback = largestIconSize(list->item(2));

        QImage big(320, 480, QImage::Format_RGB32);
        big.fill(Qt::blue);
        Q_EMIT images.loaded(poster, big);

        const QSize iconSize = list->iconSize() * w.devicePixelRatioF();
        for (int i : {0, 1}) {
            const QSize got = largestIconSize(list->item(i));
            QVERIFY2(got.isValid(), "icon was not set");
            QVERIFY2(got.width() <= iconSize.width() && got.height() <= iconSize.height(),
                     qPrintable(QStringLiteral("icon %1x%2 exceeds view icon size")
                                    .arg(got.width()).arg(got.height())));
            QCOMPARE(got, QSize(64, 96)); // 2:3 kept within 96x96
        }
        // Items with a different URL are untouched.
        QCOMPARE(largestIconSize(list->item(2)), fallback);
    }

    void lateImageAfterRelistIsIgnored()
    {
        XtreamClient client;
        ImageLoader images;
        BrowseWidget w(&client, &images);
        QListWidget *list = contentList(w);

        const QString stale = iconUrl("stale");
        Q_EMIT client.liveStreamsReady({channel(1, stale)});
        // Replacing the list deletes the old items; the URL index must not
        // keep dangling pointers to them.
        Q_EMIT client.liveStreamsReady({channel(2, iconUrl("fresh"))});
        QCOMPARE(list->count(), 1);
        const QSize before = largestIconSize(list->item(0));

        QImage img(50, 50, QImage::Format_RGB32);
        img.fill(Qt::red);
        Q_EMIT images.loaded(stale, img); // would be a use-after-free if indexed
        QCOMPARE(largestIconSize(list->item(0)), before);
    }
};

QTEST_MAIN(BrowseWidgetTest)
#include "test_browsewidget.moc"
