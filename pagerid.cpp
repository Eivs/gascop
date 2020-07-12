/*
 *  Gascop
 *
 *  Copyright (C) 2011-2014 Clive Cooper.
 *
 *  Gascop is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Gascop is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Clive Cooper,
 *
 */
 
#include <QtGui>
#include <QApplication>

#include "pagerid.h"

Pagerid::Pagerid(QWidget *parent)
	: QDialog(parent)
{
  setupUi(this); // this sets up GUI

  // signals/slots mechanism
  connect(pbDone, SIGNAL(clicked()), this, SLOT(pbDoneClick()));
  connect(pbCancel, SIGNAL(clicked()), this, SLOT(pbCancelClick()));
  connect(lwPagerIDs, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(pagerIDsDoubleClicked(QListWidgetItem *)));
  lwPagerIDs->installEventFilter(this);
}

void Pagerid::showEvent(QShowEvent *event)
{
  event->setAccepted(true);

}

void Pagerid::closeEvent(QCloseEvent *event)
{
  event->setAccepted(true);
}

void Pagerid::pbDoneClick()
{
  accept();
  close();
}

void Pagerid::pbCancelClick()
{
  reject();
  close();
}

void Pagerid::pagerIDsDoubleClicked(QListWidgetItem *)
{
  pbDoneClick();
}

bool Pagerid::eventFilter(QObject * target, QEvent * event)
{
  if(target == lwPagerIDs)
  {
    if(event->type() == QEvent::KeyPress)					// make sure it is a keypress event
    {
      int key = static_cast<QKeyEvent*>(event)->key();
      if(key == Qt::Key_Return)                             // So the user can just press enter on the prev pages list
      {
        pagerIDsDoubleClicked(lwPagerIDs->currentItem());
        return(true);
      }
    }
  }
  return(false);
}
