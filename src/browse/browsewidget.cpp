#include "browse/browsewidget.h"

#include "api/xtreamclient.h"
#include "core/imageloader.h"


#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPixmap>
#include <QVBoxLayout>

namespace qiptv {

namespace {
enum Roles {
    IdRole = Qt::UserRole + 1, // int stream id (live/movie) or string series id
    IconUrlRole,
    ExtRole,  // container extension (movie)
    KindRole, // Section
};

QIcon fallbackIcon(BrowseWidget::Section s)
{
    switch (s) {
    case BrowseWidget::Live:
        return QIcon::fromTheme(QStringLiteral("video-television"));
    case BrowseWidget::Movies:
        return QIcon::fromTheme(QStringLiteral("video-x-generic"));
    case BrowseWidget::Series:
        return QIcon::fromTheme(QStringLiteral("media-playlist-shuffle"));
    }
    return QIcon();
}
} // namespace

BrowseWidget::BrowseWidget(XtreamClient *client, ImageLoader *images, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
    , m_images(images)
{
    m_sections = new QListWidget(this);
    m_sections->setMaximumWidth(160);
    m_sections->addItem(new QListWidgetItem(QIcon::fromTheme(QStringLiteral("video-television")), tr("Live TV")));
    m_sections->addItem(new QListWidgetItem(QIcon::fromTheme(QStringLiteral("video-x-generic")), tr("Movies")));
    m_sections->addItem(new QListWidgetItem(QIcon::fromTheme(QStringLiteral("media-playlist-shuffle")), tr("Series")));

    m_categories = new QListWidget(this);
    m_categories->setMaximumWidth(240);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Filter…"));
    m_search->setClearButtonEnabled(true);

    m_content = new QListWidget(this);
    m_content->setViewMode(QListView::IconMode);
    m_content->setResizeMode(QListView::Adjust);
    m_content->setMovement(QListView::Static);
    m_content->setIconSize(QSize(96, 96));
    m_content->setGridSize(QSize(150, 150));
    m_content->setWordWrap(true);
    m_content->setUniformItemSizes(false);

    m_status = new QLabel(this);
    m_status->setAlignment(Qt::AlignCenter);

    auto *contentCol = new QVBoxLayout;
    contentCol->addWidget(m_search);
    contentCol->addWidget(m_content, 1);
    contentCol->addWidget(m_status);

    auto *root = new QHBoxLayout(this);
    root->addWidget(m_sections);
    root->addWidget(m_categories);
    root->addLayout(contentCol, 1);

    connect(m_sections, &QListWidget::currentRowChanged, this, &BrowseWidget::onSectionChanged);
    connect(m_categories, &QListWidget::itemSelectionChanged, this, &BrowseWidget::onCategoryChanged);
    connect(m_content, &QListWidget::itemActivated, this, &BrowseWidget::onContentActivated);
    connect(m_search, &QLineEdit::textChanged, this, &BrowseWidget::applyFilter);

    connect(m_client, &XtreamClient::liveCategoriesReady, this, [this](const QVector<Category> &c) {
        m_catCache[Live] = c;
        if (currentSection() == Live) {
            populateCategories(c);
        }
    });
    connect(m_client, &XtreamClient::vodCategoriesReady, this, [this](const QVector<Category> &c) {
        m_catCache[Movies] = c;
        if (currentSection() == Movies) {
            populateCategories(c);
        }
    });
    connect(m_client, &XtreamClient::seriesCategoriesReady, this, [this](const QVector<Category> &c) {
        m_catCache[Series] = c;
        if (currentSection() == Series) {
            populateCategories(c);
        }
    });

    connect(m_client, &XtreamClient::liveStreamsReady, this, [this](const QVector<LiveStream> &streams) {
        if (currentSection() != Live) {
            return;
        }
        m_content->clear();
        for (const LiveStream &s : streams) {
            auto *item = new QListWidgetItem(fallbackIcon(Live), s.name);
            item->setData(IdRole, s.streamId);
            item->setData(IconUrlRole, s.streamIcon);
            item->setData(KindRole, int(Live));
            m_content->addItem(item);
            if (!s.streamIcon.isEmpty()) {
                m_images->load(s.streamIcon);
            }
        }
        m_status->setText(tr("%n channel(s)", nullptr, streams.size()));
        applyFilter(m_search->text());
    });

    connect(m_client, &XtreamClient::vodStreamsReady, this, [this](const QVector<VodStream> &streams) {
        if (currentSection() != Movies) {
            return;
        }
        m_content->clear();
        for (const VodStream &s : streams) {
            auto *item = new QListWidgetItem(fallbackIcon(Movies), s.name);
            item->setData(IdRole, s.streamId);
            item->setData(IconUrlRole, s.icon);
            item->setData(ExtRole, s.containerExtension);
            item->setData(KindRole, int(Movies));
            m_content->addItem(item);
            if (!s.icon.isEmpty()) {
                m_images->load(s.icon);
            }
        }
        m_status->setText(tr("%n movie(s)", nullptr, streams.size()));
        applyFilter(m_search->text());
    });

    connect(m_client, &XtreamClient::seriesReady, this, [this](const QVector<qiptv::Series> &series) {
        if (currentSection() != Series) {
            return;
        }
        m_content->clear();
        for (const qiptv::Series &s : series) {
            auto *item = new QListWidgetItem(fallbackIcon(Series), s.name);
            item->setData(IdRole, QString::number(s.seriesId));
            item->setData(IconUrlRole, s.cover);
            item->setData(KindRole, int(Series));
            m_content->addItem(item);
            if (!s.cover.isEmpty()) {
                m_images->load(s.cover);
            }
        }
        m_status->setText(tr("%n series", nullptr, series.size()));
        applyFilter(m_search->text());
    });

    connect(m_images, &ImageLoader::loaded, this, &BrowseWidget::onImageLoaded);

    m_sections->setCurrentRow(0);
}

BrowseWidget::Section BrowseWidget::currentSection() const
{
    return static_cast<Section>(qMax(0, m_sections->currentRow()));
}

void BrowseWidget::reload()
{
    m_catCache.clear();
    requestCategories(currentSection());
}

void BrowseWidget::requestCategories(Section s)
{
    switch (s) {
    case Live:
        m_client->fetchLiveCategories();
        break;
    case Movies:
        m_client->fetchVodCategories();
        break;
    case Series:
        m_client->fetchSeriesCategories();
        break;
    }
}

void BrowseWidget::onSectionChanged()
{
    const Section s = currentSection();
    m_content->clear();
    m_status->clear();
    if (m_catCache.contains(s)) {
        populateCategories(m_catCache.value(s));
    } else {
        m_categories->clear();
        requestCategories(s);
    }
}

void BrowseWidget::populateCategories(const QVector<Category> &cats)
{
    m_categories->clear();
    auto *all = new QListWidgetItem(tr("All"));
    all->setData(Qt::UserRole, QString()); // empty id == all
    m_categories->addItem(all);
    for (const Category &c : cats) {
        auto *item = new QListWidgetItem(c.name);
        item->setData(Qt::UserRole, c.id);
        m_categories->addItem(item);
    }
    m_categories->setCurrentRow(0); // triggers onCategoryChanged -> fetch
}

void BrowseWidget::onCategoryChanged()
{
    QListWidgetItem *item = m_categories->currentItem();
    if (!item) {
        return;
    }
    const QString catId = item->data(Qt::UserRole).toString();
    switch (currentSection()) {
    case Live:
        m_client->fetchLiveStreams(catId);
        break;
    case Movies:
        m_client->fetchVodStreams(catId);
        break;
    case Series:
        m_client->fetchSeries(catId);
        break;
    }
}

PlaylistEntry BrowseWidget::entryFor(const QListWidgetItem *item) const
{
    const int id = item->data(IdRole).toInt();
    if (static_cast<Section>(item->data(KindRole).toInt()) == Live) {
        return {m_client->urls().liveUrl(id, m_liveContainer), item->text()};
    }
    return {m_client->urls().vodUrl(id, item->data(ExtRole).toString()), item->text()};
}

Playlist BrowseWidget::playlistFrom(const QListWidgetItem *activated) const
{
    // Snapshot the visible (not search-filtered) items of the same kind, in
    // display order, so next/previous follows what the user sees.
    const int kind = activated->data(KindRole).toInt();
    QVector<PlaylistEntry> entries;
    int current = -1;
    for (int row = 0; row < m_content->count(); ++row) {
        const QListWidgetItem *it = m_content->item(row);
        if (it == activated) {
            current = static_cast<int>(entries.size());
        } else if (it->isHidden() || it->data(KindRole).toInt() != kind) {
            continue;
        }
        entries.push_back(entryFor(it));
    }
    return Playlist(std::move(entries), current, kind == Live);
}

void BrowseWidget::onContentActivated(QListWidgetItem *item)
{
    if (!item) {
        return;
    }
    const Section kind = static_cast<Section>(item->data(KindRole).toInt());
    switch (kind) {
    case Live:
    case Movies:
        Q_EMIT playRequested(playlistFrom(item));
        break;
    case Series: {
        const QString id = item->data(IdRole).toString();
        Q_EMIT openSeriesRequested(id, item->text());
        break;
    }
    }
}

void BrowseWidget::applyFilter(const QString &text)
{
    const QString needle = text.trimmed();
    for (int i = 0; i < m_content->count(); ++i) {
        QListWidgetItem *item = m_content->item(i);
        item->setHidden(!needle.isEmpty()
                        && !item->text().contains(needle, Qt::CaseInsensitive));
    }
}

void BrowseWidget::onImageLoaded(const QString &url, const QImage &image)
{
    if (image.isNull()) {
        return;
    }
    const QPixmap pix = QPixmap::fromImage(image);
    for (int i = 0; i < m_content->count(); ++i) {
        QListWidgetItem *item = m_content->item(i);
        if (item->data(IconUrlRole).toString() == url) {
            item->setIcon(QIcon(pix));
        }
    }
}

} // namespace qiptv
