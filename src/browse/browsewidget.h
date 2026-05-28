#pragma once

#include "models/category.h"

#include <QHash>
#include <QWidget>

class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QLabel;

namespace kiptv {

class XtreamClient;
class ImageLoader;

/**
 * The post-login browser: a section rail (Live / Movies / Series), a category
 * list, and a content view (channel list or poster grid). Talks to the
 * XtreamClient for data and the ImageLoader for posters/logos.
 *
 * Activating a content item emits playRequested (live/movie, ready to play) or
 * openSeriesRequested (series, which the window opens in the detail view).
 */
class BrowseWidget : public QWidget {
    Q_OBJECT
public:
    enum Section { Live = 0, Movies, Series };

    BrowseWidget(XtreamClient *client, ImageLoader *images, QWidget *parent = nullptr);

    /// Clear caches and fetch categories for the current section.
    void reload();

Q_SIGNALS:
    void playRequested(const QString &url, const QString &title);
    void openSeriesRequested(const QString &seriesId, const QString &name);

private:
    void onSectionChanged();
    void onCategoryChanged();
    void onContentActivated(QListWidgetItem *item);
    void applyFilter(const QString &text);

    void populateCategories(const QVector<Category> &cats);
    void requestCategories(Section s);

    void onImageLoaded(const QString &url, const QImage &image);

    Section currentSection() const;

    QListWidget *m_sections = nullptr;
    QLineEdit *m_search = nullptr;
    QListWidget *m_categories = nullptr;
    QListWidget *m_content = nullptr;
    QLabel *m_status = nullptr;

    XtreamClient *m_client = nullptr;
    ImageLoader *m_images = nullptr;

    QHash<int, QVector<Category>> m_catCache; // section -> categories
};

} // namespace kiptv
