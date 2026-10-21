#include "browse/seriesdetail.h"

#include "api/xtreamclient.h"
#include "core/imageloader.h"


#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

namespace kiptv {

namespace {
constexpr int EpisodeIndexRole = Qt::UserRole + 1; // index into m_info.episodes
}

SeriesDetail::SeriesDetail(XtreamClient *client, ImageLoader *images, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
    , m_images(images)
{
    m_back = new QPushButton(QIcon::fromTheme(QStringLiteral("go-previous")), tr("Back"), this);
    m_back->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

    m_poster = new QLabel(this);
    m_poster->setFixedSize(160, 240);
    m_poster->setScaledContents(true);
    m_poster->setPixmap(QIcon::fromTheme(QStringLiteral("media-playlist-shuffle")).pixmap(160, 240));

    m_title = new QLabel(this);
    m_title->setTextFormat(Qt::RichText);
    m_title->setWordWrap(true);
    m_plot = new QLabel(this);
    m_plot->setWordWrap(true);
    m_plot->setAlignment(Qt::AlignTop);

    m_seasonBox = new QComboBox(this);
    m_episodes = new QListWidget(this);

    auto *infoCol = new QVBoxLayout;
    infoCol->addWidget(m_title);
    infoCol->addWidget(m_plot, 1);
    infoCol->addWidget(new QLabel(tr("Season:"), this));
    infoCol->addWidget(m_seasonBox);

    auto *topRow = new QHBoxLayout;
    topRow->addWidget(m_poster);
    topRow->addLayout(infoCol, 1);

    auto *root = new QVBoxLayout(this);
    root->addWidget(m_back);
    root->addLayout(topRow);
    root->addWidget(m_episodes, 1);

    connect(m_back, &QPushButton::clicked, this, &SeriesDetail::backRequested);
    connect(m_seasonBox, &QComboBox::currentIndexChanged, this, &SeriesDetail::onSeasonChanged);
    connect(m_episodes, &QListWidget::itemActivated, this, &SeriesDetail::onEpisodeActivated);
    connect(m_client, &XtreamClient::seriesInfoReady, this, &SeriesDetail::onInfoReady);
    connect(m_images, &ImageLoader::loaded, this, &SeriesDetail::onImageLoaded);
}

void SeriesDetail::open(const QString &seriesId, const QString &name)
{
    m_seriesId = seriesId;
    m_name = name;
    m_info = SeriesInfo();
    m_title->setText(QStringLiteral("<h2>%1</h2>").arg(name.toHtmlEscaped()));
    m_plot->clear();
    m_seasonBox->clear();
    m_episodes->clear();
    m_client->fetchSeriesInfo(seriesId);
}

void SeriesDetail::onInfoReady(const SeriesInfo &info)
{
    m_info = info;
    if (!info.plot.isEmpty()) {
        m_plot->setText(info.plot);
    }
    m_posterUrl = info.cover;
    if (!m_posterUrl.isEmpty()) {
        m_images->load(m_posterUrl);
    }

    QSignalBlocker block(m_seasonBox);
    m_seasonBox->clear();
    const QVector<int> seasons = info.seasons();
    for (int s : seasons) {
        m_seasonBox->addItem(tr("Season %1").arg(s), s);
    }
    block.unblock();
    if (m_seasonBox->count() > 0) {
        m_seasonBox->setCurrentIndex(0);
        onSeasonChanged();
    }
}

void SeriesDetail::onSeasonChanged()
{
    m_episodes->clear();
    if (m_seasonBox->currentIndex() < 0) {
        return;
    }
    const int season = m_seasonBox->currentData().toInt();
    for (int i = 0; i < m_info.episodes.size(); ++i) {
        const Episode &e = m_info.episodes.at(i);
        if (e.season != season) {
            continue;
        }
        const QString label = e.title.isEmpty()
            ? tr("Episode %1").arg(e.episodeNum)
            : tr("%1. %2").arg(e.episodeNum).arg(e.title);
        auto *item = new QListWidgetItem(QIcon::fromTheme(QStringLiteral("video-x-generic")), label);
        item->setData(EpisodeIndexRole, i);
        m_episodes->addItem(item);
    }
}

void SeriesDetail::onEpisodeActivated(QListWidgetItem *item)
{
    if (!item) {
        return;
    }
    const int idx = item->data(EpisodeIndexRole).toInt();
    if (idx < 0 || idx >= m_info.episodes.size()) {
        return;
    }
    const Episode &e = m_info.episodes.at(idx);
    const QString url = m_client->urls().seriesUrl(e.id, e.containerExtension);
    const QString title = tr("%1 — S%2E%3").arg(m_name).arg(e.season).arg(e.episodeNum);
    Q_EMIT playEpisode(url, title);
}

void SeriesDetail::onImageLoaded(const QString &url, const QImage &image)
{
    if (url == m_posterUrl && !image.isNull()) {
        m_poster->setPixmap(QPixmap::fromImage(image));
    }
}

} // namespace kiptv
