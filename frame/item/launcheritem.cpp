/*
 * Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "launcheritem.h"
#include "themeappicon.h"
#include "utils.h"
#include "../widgets/tipswidget.h"

#include <QPainter>
#include <QProcess>
#include <QMouseEvent>
#include <QApplication>
#include <QGSettings>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <DDBusSender>
#include <QDBusPendingReply>

DCORE_USE_NAMESPACE

LauncherItem::LauncherItem(QWidget *parent)
    : DockItem(parent)
    , m_gsettings(Utils::ModuleSettingsPtr("launcher", QByteArray(), this))
{
    if (m_gsettings) {
        connect(m_gsettings, &QGSettings::changed, this, &LauncherItem::onGSettingsChanged);
    }
}

void LauncherItem::refreshIcon()
{
    const int iconSize = qMin(width(), height());
    if (DockDisplayMode == Efficient) {
        ThemeAppIcon::getIcon(m_icon, "deepin-launcher", iconSize * 0.7);
    } else {
        ThemeAppIcon::getIcon(m_icon, "deepin-launcher", iconSize * 0.8);
    }

    update();
}

void LauncherItem::paintEvent(QPaintEvent *e)
{
    DockItem::paintEvent(e);

    if (!isVisible())
        return;

    QPainter painter(this);

    const auto ratio = devicePixelRatioF();
    const int iconX = rect().center().x() - m_icon.rect().center().x() / ratio;
    const int iconY = rect().center().y() - m_icon.rect().center().y() / ratio;

    painter.drawPixmap(iconX, iconY, m_icon);
}

void LauncherItem::resizeEvent(QResizeEvent *e)
{
    DockItem::resizeEvent(e);

    refreshIcon();
}

void LauncherItem::mousePressEvent(QMouseEvent *e)
{
    if (checkGSettingsControl()) {
        return;
    }

    hidePopup();

    return QWidget::mousePressEvent(e);
}

void LauncherItem::mouseReleaseEvent(QMouseEvent *e)
{
    if (checkGSettingsControl()) {
        return;
    }

    if (e->button() != Qt::LeftButton)
        return;

    DDBusSender dbusSender = DDBusSender()
            .service("com.deepin.dde.Launcher")
            .path("/com/deepin/dde/Launcher")
            .interface("com.deepin.dde.Launcher");

    QDBusPendingReply<bool> visibleReply = dbusSender.property("Visible").get();
    if (!visibleReply.value())
       dbusSender.method("Show").call();
}

QWidget *LauncherItem::popupTips()
{
    if (checkGSettingsControl()) {
        return nullptr;
    }

    m_tips.reset(new TipsWidget(this));
    m_tips->setVisible(false);
    m_tips->setText(tr("Launcher"));
    m_tips->setObjectName("launcher");
    return m_tips.get();
}

void LauncherItem::onGSettingsChanged(const QString& key) {
    if (key != "enable") {
        return;
    }

    if (m_gsettings && m_gsettings->keys().contains("enable")) {
        setVisible(m_gsettings->get("enable").toBool());
    }
}

bool LauncherItem::checkGSettingsControl() const
{
    return m_gsettings && m_gsettings->keys().contains("control")
            && m_gsettings->get("control").toBool();
}

void LauncherItem::showEvent(QShowEvent* event) {
    QTimer::singleShot(0, this, [=] {
        onGSettingsChanged("enable");
    });

    return DockItem::showEvent(event);
}
