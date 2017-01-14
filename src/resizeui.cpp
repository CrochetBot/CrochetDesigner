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
 MERCHANTABILITY or FITNQTabWidgetESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Crochet Charts. If not, see <http://www.gnu.org/licenses/>.

 \****************************************************************************/
#include "resizeui.h"
#include "ui_resize.h"
#include "crochettab.h"

ResizeUI::ResizeUI(QTabWidget* tabWidget, QWidget* parent)
	: QDockWidget(parent),
	mTabWidget(tabWidget),
	ui(new Ui::ResizeDialog)
{
    ui->setupUi(this);
    setVisible(false);
    setFloating(true);
	
	connect(this, SIGNAL(visibilityChanged(bool)), SLOT(updateContent()));	
	connect(ui->topBox, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	connect(ui->bottomBox, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	connect(ui->leftBox, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	connect(ui->rightBox, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	connect(ui->clampButton, SIGNAL(pressed()), this, SLOT(clampPressed()));
}

ResizeUI::~ResizeUI()
{
	delete ui;
}

void ResizeUI::updateContent()
{
	CrochetTab* currentTab = qobject_cast<CrochetTab*>(mTabWidget->currentWidget());
	if (currentTab != NULL)
	{
		QRectF sceneRect = currentTab->scene()->sceneRect();
		setContent(-sceneRect.y(), sceneRect.height() + sceneRect.y(), -sceneRect.x(), sceneRect.width() + sceneRect.x());
	}
}

void ResizeUI::updateContent(int index)
{
	CrochetTab* currentTab = qobject_cast<CrochetTab*>(mTabWidget->widget(index));
	if (currentTab != NULL)
	{
		QRectF sceneRect = currentTab->scene()->sceneRect();
		setContent(-sceneRect.y(), sceneRect.height() + sceneRect.y(), -sceneRect.x(), sceneRect.width() + sceneRect.x());
	}
}

void ResizeUI::setContent(qreal top, qreal bottom, qreal left, qreal right)
{
	disconnect(ui->topBox, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	disconnect(ui->bottomBox, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	disconnect(ui->leftBox, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	disconnect(ui->rightBox, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	ui->topBox->setValue(top);
	ui->bottomBox->setValue(bottom);
	ui->leftBox->setValue(left);
	ui->rightBox->setValue(right);
	connect(ui->topBox, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	connect(ui->bottomBox, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	connect(ui->leftBox, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	connect(ui->rightBox, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
}

void ResizeUI::valueChanged(int value)
{
	Q_UNUSED(value);
	sendResize();
}

void ResizeUI::clampPressed()
{	
	CrochetTab* currentTab = qobject_cast<CrochetTab*>(mTabWidget->currentWidget());
	if (currentTab != NULL)
	{
		QList<QGraphicsItem*> selection = currentTab->scene()->items();
		if (selection.length() == 0)
			return;
			
		QRectF ibr = ibr = selection.first()->sceneBoundingRect();
		foreach (QGraphicsItem* i, selection) {
			if (i->isVisible() && i->isEnabled())
				ibr = ibr.united(i->sceneBoundingRect());
		}

		ibr.setTop(ibr.top() - SCENE_CLAMP_BORDER_SIZE - 10);
		ibr.setBottom(ibr.bottom() + SCENE_CLAMP_BORDER_SIZE + 10);
		ibr.setLeft(ibr.left() - SCENE_CLAMP_BORDER_SIZE - 10);
		ibr.setRight(ibr.right() + SCENE_CLAMP_BORDER_SIZE + 10);
		
		emit resize(ibr);
		updateContent();
	}
}

void ResizeUI::sendResize()
{
	emit resize(QRectF(-ui->leftBox->value(), -ui->topBox->value(),ui->rightBox->value() + ui->leftBox->value(), ui->topBox->value() + ui->bottomBox->value()));
}