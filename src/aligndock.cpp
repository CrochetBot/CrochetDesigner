/****************************************************************************\
 Copyright (c) 2010-2014 Stitch Works Software
 Brian C. Milco <bcmilco@gmail.com>

 This file is part of Crochet Charts.

 Crochet Charts is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Crochet Charts is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Crochet Charts. If not, see <http://www.gnu.org/licenses/>.

 \****************************************************************************/
#include "aligndock.h"
#include "ui_aligndock.h"

AlignDock::AlignDock(QWidget* parent)
    : QDockWidget(parent)
    , ui(new Ui::AlignDock)
{
    ui->setupUi(this);
    setVisible(false);
    setFloating(true);
    setObjectName("alignDock");

    connect(ui->alignBottom, SIGNAL(clicked(bool)), SLOT(generateAlignment()));
    connect(ui->alignCenterH, SIGNAL(clicked(bool)), SLOT(generateAlignment()));
    connect(ui->alignCenterV, SIGNAL(clicked(bool)), SLOT(generateAlignment()));
    connect(ui->alignLeft, SIGNAL(clicked(bool)), SLOT(generateAlignment()));
    connect(ui->alignRight, SIGNAL(clicked(bool)), SLOT(generateAlignment()));
    connect(ui->alignTop, SIGNAL(clicked(bool)), SLOT(generateAlignment()));
    connect(ui->alignToPath, SIGNAL(clicked(bool)), SLOT(generateAlignment()));

    connect(ui->distributeBottom, SIGNAL(clicked(bool)), SLOT(generateDistribution()));
    connect(ui->distributeCenterH, SIGNAL(clicked(bool)), SLOT(generateDistribution()));
    connect(ui->distributeCenterV, SIGNAL(clicked(bool)), SLOT(generateDistribution()));
    connect(ui->distributeLeft, SIGNAL(clicked(bool)), SLOT(generateDistribution()));
    connect(ui->distributeRight, SIGNAL(clicked(bool)), SLOT(generateDistribution()));
    connect(ui->distributeTop, SIGNAL(clicked(bool)), SLOT(generateDistribution()));
    connect(ui->distributeToPath, SIGNAL(clicked(bool)), SLOT(generateDistribution()));

    ui->alignToPath->setVisible(false);
    ui->distributeToPath->setVisible(false);
}

AlignDock::~AlignDock()
{
    delete ui;
}

void
AlignDock::generateAlignment()
{
    int style = 1;

    QToolButton* button = qobject_cast<QToolButton*>(sender());

    if (button == ui->alignLeft)
        style = 1;
    else if (button == ui->alignCenterV)
        style = 2;
    else if (button == ui->alignRight)
        style = 3;
    else if (button == ui->alignTop)
        style = 4;
    else if (button == ui->alignCenterH)
        style = 5;
    else if (button == ui->alignBottom)
        style = 6;
    else if (button == ui->alignToPath)
        style = 7;

    emit align(style);
}

void
AlignDock::generateDistribution()
{
    int style = 1;

    QToolButton* button = qobject_cast<QToolButton*>(sender());

    if (button == ui->distributeLeft)
        style = 1;
    else if (button == ui->distributeCenterH)
        style = 2;
    else if (button == ui->distributeRight)
        style = 3;
    else if (button == ui->distributeTop)
        style = 4;
    else if (button == ui->distributeCenterV)
        style = 5;
    else if (button == ui->distributeBottom)
        style = 6;
    else if (button == ui->distributeToPath)
        style = 7;

    emit distribute(style);
}
