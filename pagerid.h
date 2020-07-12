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
 
#ifndef PAGERID_H
#define PAGERID_H

#include "ui_pagerid.h"

class Pagerid : public QDialog, public Ui::Pagerid
{
  Q_OBJECT

  public:
    Pagerid(QWidget *parent = 0);

  protected:
    void showEvent(QShowEvent *event);
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject * target, QEvent * event);

  public slots:
    void pbDoneClick();
    void pbCancelClick();
    void pagerIDsDoubleClicked(QListWidgetItem * item);

  private:
    Ui::Pagerid ui;

};

#endif // PAGERID_H
