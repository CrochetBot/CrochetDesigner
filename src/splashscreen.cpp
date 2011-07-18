/*************************************************\
| Copyright (c) 2011 Stitch Works Software        |
| Brian C. Milco <brian@stitchworkssoftware.com>  |
\*************************************************/
#include "splashscreen.h"

#include <QPainter>
#include <QBitmap>
#include "appinfo.h"

#include <QDebug>

SplashScreen::SplashScreen()
{
    QPixmap p(":/images/splashscreen.png");
    this->setPixmap(p);
    this->setMask(p.mask());
}

void SplashScreen::drawContents (QPainter *painter)
{
    QRect rect = QRect(QPoint(0,0), this->pixmap().size());
    painter->drawPixmap(rect, this->pixmap());

    QFont origFont = painter->font();
/*
    QFont titleFont = QFont(origFont.family());
    titleFont.setPointSize(50);    

    painter->setFont(titleFont);
    painter->drawText(QRect(25, 200, 400, 100), AppInfo::inst()->appName);

    QFont byFont = QFont(origFont.family());
    byFont.setPointSize(30);
    painter->setFont(byFont);
    painter->drawText(QRect(25, 125, 400, 100), AppInfo::inst()->appOrg);
 */   
    painter->setFont(origFont);
    QString version = QString("Version: %1").arg(AppInfo::inst()->appVersionShort);
    painter->drawText(QRectF(75, 280, 250, 50), version);
    painter->drawText(QRectF(75, 295, 250, 50), mMessage);
}

void SplashScreen::showMessage (const QString &message)
{
    if(message != mMessage) {
        mMessage = message;
        this->repaint();
    }
}
