/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QMPlay2Extensions.hpp>

#include <NetworkAccess.hpp>

#include <QTreeWidget>
#include <QPointer>
#include <QMutex>

class NetworkReply;
class QProgressBar;
class QActionGroup;
class QToolButton;
class QCompleter;
class YouTubeDL;
class QSpinBox;
class LineEdit;
class QLabel;
class QMenu;

/**/

class ResultsYoutube final : public QTreeWidget
{
    Q_OBJECT
public:
    ResultsYoutube();
    ~ResultsYoutube();

private:
    void playOrEnqueue(const QString &param, QTreeWidgetItem *tWI, const QString &addrParam = QString());

    QMenu *menu;

private slots:
    void playEntry(QTreeWidgetItem *tWI);

    void openPage();
    void copyPageURL();

    void contextMenu(const QPoint &p);
};

/**/

class PageSwitcher final : public QWidget
{
    Q_OBJECT
public:
    PageSwitcher(QWidget *youTubeW);

    QToolButton *prevB, *nextB;
    QSpinBox *currPageB;
};

/**/

using ItagNames = QPair<QStringList, QList<int>>;

class YouTube final : public QWidget, public QMPlay2Extensions
{
    Q_OBJECT

public:
    YouTube(Module &module);
    ~YouTube();

    bool set() override;

    DockWidget *getDockWidget() override;

    bool canConvertAddress() const override;

    QString matchAddress(const QString &url) const override;
    QList<AddressPrefix> addressPrefixList(bool) const override;
    void convertAddress(const QString &, const QString &, const QString &, QString *, QString *, QIcon *, QString *, IOController<> *ioCtrl) override;

    QVector<QAction *> getActions(const QString &, double, const QString &, const QString &, const QString &) override;

    inline QString getYtDlPath() const;

private slots:
    void next();
    void prev();
    void chPage();

    void searchTextEdited(const QString &text);
    void search();

    void netFinished(NetworkReply *reply);

    void searchMenu();

private:
    void setItags(int qualityIdx);

    void deleteReplies();

    void setAutocomplete(const QByteArray &data);
    void setSearchResults(QString data);

    QStringList getYouTubeVideo(const QString &param, const QString &url, IOController<YouTubeDL> &youTubeDL);

    void preparePlaylist(const QString &data, QTreeWidgetItem *tWI);

    DockWidget *dw;

    QIcon youtubeIcon, videoIcon;

    LineEdit *searchE;
    QToolButton *searchB;
    ResultsYoutube *resultsW;
    QProgressBar *progressB;
    PageSwitcher *pageSwitcher;

    QString lastTitle;
    QCompleter *completer;
    int currPage;

    QPointer<NetworkReply> autocompleteReply, searchReply;
    QList<NetworkReply *> linkReplies, imageReplies;
    NetworkAccess net;

    QString youtubedl;
    bool m_allowSubtitles;

    QActionGroup *m_qualityGroup = nullptr, *m_sortByGroup = nullptr;

    int m_sortByIdx = 0;

    QMutex m_itagsMutex;
    QList<int> m_videoItags, m_audioItags;
};

#define YouTubeName "YouTube Browser"
