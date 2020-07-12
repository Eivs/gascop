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
#include <QDebug>
#include <QTcpSocket>

#include "mainwin.h"

#define MainWin_DEBUG

const bool DEBUG = true;
const QString VERSION = "2013.11.08-1";
#define RICPOS(c) (c % 8)
#define QSn QString::number

using namespace std;

Mix_Chunk * SC_sound;
bool isPlayingWav;

void channelDone(int channel);

/*  There is no limit to transmission length but bear in mind a 3 minute transmission requires about 13MB of memory to store the wave data
 *
 *
*/

MainWin::MainWin( QWidget * parent, Qt::WFlags f)
 : QMainWindow(parent, f)
{
  setupUi(this);
  readSettings();                     // Get any user settings at start.
  running = false;                    // This is used in the show event to make sure we only run the code in the show event just once.
  formleft = x();                     // Used to keep the main window where the user wants it.
  formtop = y();
  userPath = QDir::homePath();        // userPath is the path to the users home directory.
  txLength = new QTimer(this);
  watchdog = new QTimer(this);
  watchdog->setInterval(2000);
  verLabel->setText("Ver: " + VERSION);
  userDir = QDir::homePath() + "/.gascop";
  gascopdb = new QStringList();
  pagerIDdb = new QStringList();
  QDir md;
  md.mkdir(userDir);
  if(!QFile::exists(userDir + "/prev-pages.txt"))
  {
    QFile::copy(":/files/prev-pages.txt", userDir + "/prev-pages.txt");
    QFile::setPermissions(userDir + "/prev-pages.txt", QFile::ReadUser | QFile::WriteUser | QFile::ReadGroup | QFile::ReadOther);
  }
  if(!QFile::exists(userDir + "/gascop.db"))
  {
    QFile::copy(":/files/gascop.db", userDir + "/gascop.db");
    QFile::setPermissions(userDir + "/gascop.db", QFile::ReadUser | QFile::WriteUser | QFile::ReadGroup | QFile::ReadOther);
  }
  if(!QFile::exists(userDir + "/pagerid.db"))
  {
    QFile::copy(":/files/pagerid.db", userDir + "/pagerid.db");
    QFile::setPermissions(userDir + "/pagerid.db", QFile::ReadUser | QFile::WriteUser | QFile::ReadGroup | QFile::ReadOther);
  }
  pocsagBuffer = NULL;
  connect(txLength, SIGNAL(timeout()), this, SLOT(txLengthTimeout()));
  connect(watchdog, SIGNAL(timeout()), this, SLOT(watchdogTimeout()));
  connect(pbSend, SIGNAL(clicked()), this, SLOT(sendSinglePage()));
  connect(twMain, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
  connect(lwPrevPages, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(prevPagesDoubleClicked(QListWidgetItem *)));
  connect(tbTune, SIGNAL(clicked()), this, SLOT(tuneupTransmit()));
  connect(tbTuneStop, SIGNAL(clicked()), this, SLOT(tuneupStop()));
  connect(pbStart, SIGNAL(clicked()), this, SLOT(ricSearch()));
  connect(pbAbort, SIGNAL(clicked()), this, SLOT(ricSearchAbort()));
  connect(pbSave, SIGNAL(clicked()), this, SLOT(logSave()));
  connect(pbReload, SIGNAL(clicked()), this, SLOT(logReload()));
  connect(tbNumeric, SIGNAL(clicked()), this, SLOT(numericClicked()));
  connect(tbResetTime, SIGNAL(clicked()), this, SLOT(resetTimeClicked()));
  connect(pocsagLevel, SIGNAL(valueChanged(int)), this, SLOT(dataLevelChanged(int)));
  connect(tbGetPagerID, SIGNAL(clicked()), this, SLOT(getPagerIDClicked()));
  connect(teMessage, SIGNAL(textChanged()), this, SLOT(msgTextChanged()));
  connect(cbPeriod, SIGNAL(currentIndexChanged(QString)), this, SLOT(periodIndexChanged(QString)));
  connect(lwPagerIDs, SIGNAL(currentRowChanged(int)), this, SLOT(pagerIDsRowChanged(int)));
  connect(lwPages, SIGNAL(currentRowChanged(int)), this, SLOT(pagesRowChanged(int)));
  connect(pbApply, SIGNAL(clicked()), this, SLOT(applySettings()));
  connect(pbKeyTx, SIGNAL(clicked()), this, SLOT(keyTxClicked()));
  // Below are all to do with detecting a change in the PagerIDs info
  connect(leRic_2, SIGNAL(textChanged(QString)), this, SLOT(pagerIDstextChanged(QString)));
  connect(leName_2, SIGNAL(textChanged(QString)), this, SLOT(pagerIDstextChanged(QString)));
  connect(leFrequency, SIGNAL(textChanged(QString)), this, SLOT(pagerIDstextChanged(QString)));
  connect(sbTx_2, SIGNAL(valueChanged(int)), this, SLOT(pagerIDsvalChanged(int)));
  connect(sbMaxMsgLen, SIGNAL(valueChanged(int)), this, SLOT(pagerIDsvalChanged(int)));
  connect(tbFuncTone1_2, SIGNAL(clicked()), this, SLOT(pagerIDsChange()));
  connect(tbFuncTone2_2, SIGNAL(clicked()), this, SLOT(pagerIDsChange()));
  connect(tbFuncNumeric_2, SIGNAL(clicked()), this, SLOT(pagerIDsChange()));
  connect(tbFuncAlpha_2, SIGNAL(clicked()), this, SLOT(pagerIDsChange()));
  connect(tbAlpha_2, SIGNAL(clicked()), this, SLOT(pagerIDsChange()));
  connect(tbNumeric_2, SIGNAL(clicked()), this, SLOT(pagerIDsChange()));
  connect(tb512_2, SIGNAL(clicked()), this, SLOT(pagerIDsChange()));
  connect(tb1200_2, SIGNAL(clicked()), this, SLOT(pagerIDsChange()));
  connect(tb2400_2, SIGNAL(clicked()), this, SLOT(pagerIDsChange()));
  //=================================================================
  connect(pbAddEntry, SIGNAL(clicked()), this, SLOT(pagerIDsAddEntry()));
  connect(pbUpdateEntry, SIGNAL(clicked()), this, SLOT(pagerIDsUpdateEntry()));
  connect(pbClearAll, SIGNAL(clicked()), this, SLOT(pagerIDsClearAll()));
  connect(pbDeleteEntry, SIGNAL(clicked()), this, SLOT(pagerIDsDeleteEntry()));
  connect(pbDeleteEntry_3, SIGNAL(clicked()), this, SLOT(pagesDeleteEntry()));

  teMessage->installEventFilter(this);
  lwPrevPages->installEventFilter(this);
  leRic->installEventFilter(this);
  tbGetPagerID->installEventFilter(this);
  pbSend->installEventFilter(this);
}

// This allows me to do seconds, milliseconds and microseconds sleeping : gotsleep::msleep(100); for 100 milliseconds
class gotoSleep : public QThread
{
  public:
    static void sleep(unsigned long secs)
    {
      QThread::sleep(secs);
    }
    static void msleep(unsigned long msecs)
    {
      QThread::msleep(msecs);
    }
    static void usleep(unsigned long usecs)
    {
      QThread::usleep(usecs);
    }
};

void MainWin::showEvent(QShowEvent *)  // The main window is drawn
{
  if(!running)
  {
    running = true;                   // All show events after the first one will do nothing because this will be true.
    formleft = x();                   // Get the position of the main window, if the window is never moved then these values do not get updated.
    formtop = y();
    readSettings();
    preambleCW = 0xaaaaaaaa;          // Fixed value preamble
    dataLevelChanged(pocsagLevel->value());
    dtSendTime->setDateTime(QDateTime::currentDateTime());
    applySettings();
    currentBaudRate = 1200;
    currentTx = 1;
    tbTuneStop->setEnabled(false);
    singlePage = false;
    isPlayingWav = false;
    isRicSearching =false;
    testMode = false;                                       // For testing only, set to true and sent pages do not get deleted from the database
    logReload();                                            // Initially load the log file using the reload function
    openDB();
    loadPrevPages();
    tbGetPagerID->setFocus();                              // set the focus on the RIC entry edit
    pagesDbLastModified = getLastModified(userDir + "/gascop.db");
    isAutoPagingActive = true;
    txComplete = true;
    displayMessages("Gascop has started.");
    watchdogTimeout();
  }
}

void MainWin::moveEvent(QMoveEvent *)                       // The main window is being moved.
{
 formleft = x(); // The position of the main window is kept so the next time the application is run it can be put back to where the user left it.
 formtop = y();
}

void MainWin::closeEvent(QCloseEvent *)                     // The main window is closing.
{
  watchdog->stop();
  displayMessages("Gascop has closed.");
  writeSettings();                                          // Save the user settings before closing.
}

// Open the databases
void MainWin::openDB()
{
  if(QFile::exists(userDir + "/gascop.db"))
  {
    readTextFile(userDir + "/gascop.db", gascopdb);
    readTextFile(userDir + "/pagerid.db", pagerIDdb);
    populatePagerIDsEditor();
    if(lwPagerIDs->count() > 0)
    {
      lwPagerIDs->setCurrentRow(0);
      pagerIDsListToEditor(0);
      pbDeleteEntry->setEnabled(true);
      pbClearAll->setEnabled(true);
      pbUpdateEntry->setEnabled(false);
      pbAddEntry->setEnabled(true);
    }
    populatePagerMsgsEditor();
    if(lwPages->count() > 0)
    {
      lwPages->setCurrentRow(0);
      pagesListToEditor(0);
    }
  }
}

// Add a new entry to the Pager IDs database
void MainWin::pagerIDsAddEntry()
{
  int cr = lwPagerIDs->currentRow();
  pagerIDsEditorToStruct();
  writeToPagerIDDatabase(-1);
  populatePagerIDsEditor();
  if((cr >= 0) && (cr < lwPagerIDs->count()))
  {
    lwPagerIDs->setCurrentRow(cr);
    lwPagerIDs->setFocus();
  }
}

// Update an existing entry in the Pager IDs database
void MainWin::pagerIDsUpdateEntry()
{
  int cr = lwPagerIDs->currentRow();
  pagerIDsEditorToStruct();
  writeToPagerIDDatabase(lwPagerIDs->currentRow());
  populatePagerIDsEditor();
  if((cr >= 0) && (cr < lwPagerIDs->count()))
  {
    lwPagerIDs->setCurrentRow(cr);
    lwPagerIDs->setFocus();
  }
}

// Delete an entry from the pager ID database file
void MainWin::pagerIDsDeleteEntry()
{
  int cr = lwPagerIDs->currentRow();
  if((cr >= 0) && (cr < lwPagerIDs->count()))
  {
    pagerIDdb->takeAt(lwPagerIDs->currentRow());
    writeTextFile(userDir + "/pagerid.db", pagerIDdb);
    populatePagerIDsEditor();
    if((cr >= 0) && (cr < lwPagerIDs->count()))
      lwPagerIDs->setCurrentRow(cr);
    else if(lwPagerIDs->count())
      lwPagerIDs->setCurrentRow(lwPagerIDs->count() - 1);
    lwPagerIDs->setFocus();
  }
}

void MainWin::pagesDeleteEntry()
{
  int cr = lwPages->currentRow();
  if((cr >= 0) && (cr < lwPages->count()))
  {
    leRic_3->clear();
    teMessage_3->clear();
    gascopdb->takeAt(lwPages->currentRow());
    writeTextFile(userDir + "/gascop.db", gascopdb);
    populatePagerMsgsEditor();
    if((cr >= 0) && (cr < lwPages->count()))
      lwPages->setCurrentRow(cr);
    else if(lwPages->count())
      lwPages->setCurrentRow(lwPages->count() - 1);
    lwPages->setFocus();
  }
}

// Change made to an item in the Pager IDs editor
void MainWin::pagerIDstextChanged(QString)
{
  pbUpdateEntry->setEnabled(true);
}

// Change made to an item in the Pager IDs editor
void MainWin::pagerIDsvalChanged(int)
{
  pbUpdateEntry->setEnabled(true);
}

// Change made to an item in the Pager IDs editor
void MainWin::pagerIDsChange()
{
  pbUpdateEntry->setEnabled(true);
}

//  This watchdog timer is called every 5 seconds and checks if the pages database modified time has changed.
void MainWin::watchdogTimeout()
{
  watchdog->stop();                                                   // Stop the timer
  if(dtSendTime->dateTime().toTime_t() < (timeToInt() + 50))
    dtSendTime->setDateTime(QDateTime::currentDateTime());            // Keep the date/time on pager entry correct if it is within 8 seconds
  checkForPagesToSend();
}

// Check for pages to send
void MainWin::checkForPagesToSend()
{
  uint timeNow = timeToInt();
  bool lm = (pagesDbLastModified != getLastModified(userDir + "/gascop.db")); // get a flag showing if the database has been modified
  int index = 0;
  if(lm)
    setFileLock("gascop.db");                                       // If the database has been modified since last use then open it again now
  while(index < dbPages.count())
  {
    if((dbPages.at(index).flag == 0) && (dbPages.at(index).sendTime <= timeNow) &&
                                        (dbPages.at(index).tx == txOrder[txRotate]) && (dbPages.at(index).baud == baudOrder[baudRotate]))
    {
      dbPages[index].flag = 1;                                      // Set the flag to 1 to show we are processing this page
      gascopdb->replace(index, "1" + gascopdb->at(index).mid(1));   // Set the flag to 1 directly in the database file
      txSendPage.funcBits = dbPages.at(index).funcBits;             // Get the data we will need to be able to create a pager message
      txSendPage.numeric = dbPages.at(index).numeric;
      txSendPage.ric = QSn(dbPages.at(index).ric);
      txSendPage.msg = dbPages.at(index).msg;
      txSendPage.ricPos = RICPOS(dbPages.at(index).ric);
      txSendPages.append(txSendPage);
    }
    index++;
  }
  currentBaudRate = baudOrder[baudRotate];
  currentTx = txOrder[txRotate];
  baudRotate++;
  if(baudRotate > 5)  // Go through all the baud rates
  {
    baudRotate = 0;   // Once all the baud rates are checked then check the next tx
    txRotate++;
    if(txRotate > 7)  //
      txRotate = 0;
  }
  if(txSendPages.count())                                           // If there are pages to send...
  {
    displayMessages("Sending " + QSn(txSendPages.count()) + " pages @ " + QSn(currentBaudRate) + " baud");
    writeTextFile(userDir + "/gascop.db", gascopdb);                // Write the updated db file back out
    QFile::remove(userDir + "/gascop.db.lck");                      // Remove the lock file
    txComplete = false;
    isPlayingWav = true;
    stackPages();                                                   // Go stack and send the pages
    return;
  }
  if(lm)
    QFile::remove(userDir + "/gascop.db.lck");                      // Remove the lock file if we opened it at the start of this function
  watchdog->start();
}

// Retruns the time a file was last modified as a unix time value
uint MainWin::getLastModified(QString file)
{
  QFileInfo dbInfo(file);
  return(dbInfo.lastModified().toTime_t());
}

// Supplied with just the filename (like... gascop.db) this will create a lock file, if there is a lock file it waits 3 seconds and deletes it
void MainWin::setFileLock(QString filename)
{
  uint t = timeToInt() + 2;                                 // Wait for up to 2 seconds for the lock to expire
  while(QFile::exists(userDir + "/" + filename + ".lck"))   // Loop while we wait
  {
    gotoSleep::msleep(200);                                 // This while loop can use a lot of cpu so we sleep for a good part of the time
    QCoreApplication::processEvents(QEventLoop::AllEvents); // Keep the GUI responsive
    if(timeToInt() >= t)                                    // Must be a stale lock to get into here: the 2 seconds is up
    {
      QFile::remove(userDir + "/" + filename + ".lck");     // Delete the lock
      break;                                                // Break out of the loop
    }
  }
  QFile::copy(":/files/" + filename + ".lck", userDir + "/" + filename + ".lck"); // write a new lock file (Remember it expires in 2 seconds!)
  if(filename == "gascop.db")
  {
    readTextFile(userDir + "/gascop.db", gascopdb);         // The file is read and MUST be written within the next 2 seconds
    pagesDbLastModified = getLastModified(userDir + "/gascop.db");
    getPagesElements();
    populatePagerMsgsEditor();
  }
  else
  {
    readTextFile(userDir + "/pagerid.db", pagerIDdb);      // The file is read and MUST be written withing 2 seconds
    getPagerIDElements();
  }
}

// takes the info in a dbPage and turns it into a line entry for the pages database
void MainWin::writeToPagesDatabase()
{
  int cr = lwPages->currentRow();
  QString dbEntry;
  QString spare = "0000";
  QString schedule = QSn(dbPage.schedule);
  while(schedule.length() < 3)
    schedule = "0" + schedule;
  QString howMany = QSn(dbPage.howMany);
  while(howMany.length() < 2)
    howMany = "0" + howMany;
  QString baud = QSn(dbPage.baud);
  while(baud.length() < 4)
    baud = "0" + baud;
  QString ric = QSn(dbPage.ric);
  while(ric.length() < 7)
    ric = "0" + ric;
  dbEntry = QSn(dbPage.flag) + QSn(dbPage.sendTime) + spare + schedule + howMany + QSn(dbPage.tx) + baud + QSn(dbPage.numeric) +
            QSn(dbPage.funcBits) + ric + dbPage.msg;
  watchdog->stop();
  while(watchdog->isActive())
  {
    gotoSleep::msleep(100);
    QCoreApplication::processEvents(QEventLoop::AllEvents);       // Keep the GUI responsive
  }
  setFileLock("gascop.db");                                       // We now have 2 seconds before we must give up the lock
  gascopdb->append(dbEntry);
  writeTextFile(userDir + "/gascop.db", gascopdb);
  QFile::remove(userDir + "/gascop.db.lck");                      // Remove the lock file
  populatePagerMsgsEditor();
  if((cr >= 0) && (cr < lwPages->count()))
    lwPages->setCurrentRow(cr);
  watchdog->start();
}

void MainWin::pagerIDsClearAll()
{
  int cr = lwPagerIDs->currentRow();
  leRic_2->clear();
  leFrequency->clear();
  leName_2->clear();
  sbTx_2->setValue(1);
  if(cr >= 0)
    lwPagerIDs->setCurrentRow(cr);
}

// takes the info in a dbPage and turns it into a line entry for the pages database
void MainWin::writeToPagerIDDatabase(int index)
{
  QString dbEntry;
  QString fx = dbPagerID.fx;
  while(fx.length() < 9)
    fx = fx + "0";
  QString baud = QSn(dbPagerID.baud);
  while(baud.length() < 4)
    baud = "0" + baud;
  QString ric = QSn(dbPagerID.ric);
  while(ric.length() < 7)
    ric = "0" + ric;
  QString msgLen = QSn(dbPagerID.msgLen);
  while(msgLen.length() < 3)
    msgLen = "0" + msgLen;
    //  15450893012001153.17500255BT EasyReach 77C   : This is a typical entry line in the database file
  dbEntry = ric + QSn(dbPagerID.funcBits) + QSn(dbPagerID.numeric) + baud + QSn(dbPagerID.tx) + fx + msgLen + dbPagerID.name;
  if(index < 0)
    pagerIDdb->append(dbEntry);
  else
    pagerIDdb->replace(index, dbEntry);
  writeTextFile(userDir + "/pagerid.db", pagerIDdb);
}

// takes pagerID info from the Editor and places it in the QVector structure
void MainWin::pagerIDsEditorToStruct()
{
  dbPagerID.ric = leRic_2->text().toInt();
  dbPagerID.funcBits = getFunctionBits2();
  dbPagerID.numeric = 0;
  if(tbNumeric_2->isChecked())
    dbPagerID.numeric = 1;
  dbPagerID.baud = getBaudRate2();
  dbPagerID.tx = sbTx_2->value();
  dbPagerID.fx = leFrequency->text();
  dbPagerID.msgLen = sbMaxMsgLen->value();
  dbPagerID.name = leName_2->text();
}

// Returns the elements of the entries in the pager ID database
void MainWin::getPagerIDElements()
{
  dbPagerIDs.clear();
  int index = 0;
  while(index < pagerIDdb->count())
  {
    dbPagerID.ric = pagerIDdb->at(index).left(7).toInt();
    dbPagerID.funcBits = pagerIDdb->at(index).mid(7, 1).toInt();
    dbPagerID.numeric = pagerIDdb->at(index).mid(8, 1).toInt();
    dbPagerID.baud = pagerIDdb->at(index).mid(9, 4).toInt();
    dbPagerID.tx = pagerIDdb->at(index).mid(13, 1).toInt();
    dbPagerID.fx = pagerIDdb->at(index).mid(14, 9);
    dbPagerID.msgLen = pagerIDdb->at(index).mid(23, 3).toInt();
    dbPagerID.name = pagerIDdb->at(index++).mid(26);
    dbPagerIDs.append(dbPagerID);
  }
}

// Place the pager ID info into the pager IDs list
void MainWin::populatePagerIDsEditor()
{
  lwPagerIDs->clear();
  readTextFile(userDir + "/pagerid.db", pagerIDdb);
  getPagerIDElements();
  int index = 0;
  while(index < dbPagerIDs.count())
  {
    dbPagerID = dbPagerIDs.at(index++);
    QString name = dbPagerID.name;
    while(name.length() < 20)
      name += " ";
    QString baud = QSn(dbPagerID.baud);
    while(baud.length() < 4)
      baud = " " + baud;
    QString fx = dbPagerID.fx;
    while(fx.length() < 9)
      fx = " " + fx;
    QString ric = QSn(dbPagerID.ric);
    while(ric.length() < 7)
      ric = " " + ric;
    QString msgLen = QSn(dbPagerID.msgLen);
    while(msgLen.length() < 3)
      msgLen = " " + msgLen;
    QString numeric = "Alpha  ";
    if(dbPagerID.numeric)
      numeric = "Numeric";
    QString line = name + "  " + ric + "  " + baud + "  " + QSn(dbPagerID.tx) + "  " + fx + "  " +
                                                QSn(dbPagerID.funcBits) + "  " + numeric + "  " + msgLen;
    lwPagerIDs->addItem(line);
  }
}

// takes pagerID info from the QVector and places it in the editor
void MainWin::pagerIDsListToEditor(int index)
{
  sspFuncBits2 = dbPagerIDs.at(index).funcBits;
  setFunctionBits2();
  sspBaud2 = dbPagerIDs.at(index).baud;
  setBaudRate2();
  if(dbPagerIDs.at(index).numeric == 1)
    tbNumeric_2->setChecked(true);
  else
    tbAlpha_2->setChecked(true);
  sbTx_2->setValue(dbPagerIDs.at(index).tx);
  leRic_2->setText(QSn(dbPagerIDs.at(index).ric));
  leName_2->setText(dbPagerIDs.at(index).name);
  leFrequency->setText(dbPagerIDs.at(index).fx);
  sbMaxMsgLen->setValue(dbPagerIDs.at(index).msgLen);
}

// return the current function bits value
int MainWin::getFunctionBits2()
{
  if(tbFuncAlpha_2->isChecked())
    return(3);
  else if(tbFuncTone2_2->isChecked())
    return(2);
  else if(tbFuncTone1_2->isChecked())
    return(1);
  else
    return(0);
}

// Return the current baud rate
int MainWin::getBaudRate2()
{
  if(tb512_2->isChecked())
    return(512);
  else if(tb1200_2->isChecked())
    return(1200);
  else
    return(2400);
}

// A new row is selected in the pagerIDs list so we get the info of that entry into the editor
void MainWin::pagerIDsRowChanged(int index)
{
  if(index >= 0)
    pagerIDsListToEditor(index);
}

// Return the elements from an entry in the gascop database plain text file
void MainWin::getPagesElements()
{
  dbPages.clear();
  int index = 0;
  while(index < gascopdb->count())
  {
    dbPage.flag = gascopdb->at(index).left(1).toInt();
    dbPage.sendTime = gascopdb->at(index).mid(1, 10).toUInt();
    dbPage.spare = gascopdb->at(index).mid(11, 4).toInt();
    dbPage.schedule = gascopdb->at(index).mid(15, 3).toInt();
    dbPage.howMany = gascopdb->at(index).mid(18, 2).toInt();
    dbPage.tx = gascopdb->at(index).mid(20, 1).toInt();
    dbPage.baud = gascopdb->at(index).mid(21, 4).toInt();
    dbPage.numeric = gascopdb->at(index).mid(25, 1).toInt();
    dbPage.funcBits = gascopdb->at(index).mid(26, 1).toInt();
    dbPage.ric = gascopdb->at(index).mid(27, 7).toInt();
    dbPage.msg = gascopdb->at(index++).mid(34);
    dbPages.append(dbPage);
  }
}

// place the pager messages into the pager messages list
void MainWin::populatePagerMsgsEditor()
{
  lwPages->clear();
  readTextFile(userDir + "/gascop.db", gascopdb);
  getPagesElements();
  int index = 0;
  while(index < dbPages.count())
  {
    dbPage = dbPages.at(index++);
    QString msg = dbPage.msg;
//    if(msg.length() > 39)
//      msg = msg.left(36) + "...";
    QString baud = QSn(dbPage.baud);
    while(baud.length() < 4)
      baud = " " + baud;
    QString schedule = QSn(dbPage.schedule);
    while(schedule.length() < 3)
      schedule = "0" + schedule;
    QString howMany = QSn(dbPage.howMany);
    while(howMany.length() < 2)
      howMany = "0" + howMany;
    QString ric = QSn(dbPage.ric);
    while(ric.length() < 7)
      ric = " " + ric;
    QString numeric = "Alpha  ";
    if(dbPage.numeric)
      numeric = "Numeric";
    QString line = QSn(dbPage.flag) + "  " + QSn(dbPage.sendTime) + "  " + ric + "  " + baud + "  " +
                    QSn(dbPagerID.tx) + "  " + schedule + "  " + howMany + "  " + QSn(dbPagerID.funcBits) + "  " + numeric + "  " + msg;
    lwPages->addItem(line);
  }
  if(lwPages->count() > 0)
    lwPages->setCurrentRow(0);
}

// takes pages info from the QVector and places it in the editor
void MainWin::pagesListToEditor(int index)
{
  if(index >= 0)
  {
    sspFuncBits3 = dbPages.at(index).funcBits;
    setFunctionBits3();
    sspBaud3 = dbPages.at(index).baud;
    setBaudRate3();
    if(dbPages.at(index).numeric == 1)
      tbNumeric_3->setChecked(true);
    else
      tbAlpha_3->setChecked(true);
    sbTx_3->setValue(dbPages.at(index).tx);
    leRic_3->setText(QSn(dbPages.at(index).ric));
    teMessage_3->setPlainText(dbPages.at(index).msg);
    dtSendTime_3->setDateTime(QDateTime::currentDateTime().fromTime_t(dbPages.at(index).sendTime));
    cbFlags_3->setCurrentIndex(dbPages.at(index).flag);
    sbPeriodQty_3->setValue(dbPages.at(index).schedule >> 4);
    cbPeriod_3->setCurrentIndex(dbPages.at(index).schedule & 0x0f);
    sbHowMany_3->setValue(dbPages.at(index).howMany);
  }
}

// A new row is selected in the pages list so we get the info of that entry into the editor
void MainWin::pagesRowChanged(int index)
{
  if(index >= 0)
    pagesListToEditor(index);
}

// Sets the baud rate buttons on the pagerIDs editor tab
void MainWin::setBaudRate2()
{
  if(sspBaud2 == 512)
    tb512_2->setChecked(true);
  else if(sspBaud2 == 1200)
    tb1200_2->setChecked(true);
  else
    tb2400_2->setChecked(true);
}

// Sets the function bits buttons on the pagerIDs editor tab
void MainWin::setFunctionBits2()
{
  if(sspFuncBits2 == 0)
    tbFuncNumeric_2->setChecked(true);
  else if(sspFuncBits2 == 1)
    tbFuncTone1_2->setChecked(true);
  else if(sspFuncBits2 == 2)
    tbFuncTone2_2->setChecked(true);
  else
    tbFuncAlpha_2->setChecked(true);
}

// Sets the baud rate buttons on the pages editor tab
void MainWin::setBaudRate3()
{
  if(sspBaud3 == 512)
    tb512_3->setChecked(true);
  else if(sspBaud3 == 1200)
    tb1200_3->setChecked(true);
  else
    tb2400_3->setChecked(true);
}

// Sets the function bits buttons on the pages editor tab
void MainWin::setFunctionBits3()
{
  if(sspFuncBits3 == 0)
    tbFuncNumeric_3->setChecked(true);
  else if(sspFuncBits3 == 1)
    tbFuncTone1_3->setChecked(true);
  else if(sspFuncBits3 == 2)
    tbFuncTone2_3->setChecked(true);
  else
    tbFuncAlpha_3->setChecked(true);
}

// PagerID button on the single page tab is clicked: show a list of pagers in the database
void MainWin::getPagerIDClicked()
{
  pagerid = new Pagerid(this);
  int index = 0;
  getPagerIDElements();
  while(index < dbPagerIDs.count())
    pagerid->lwPagerIDs->addItem(dbPagerIDs.at(index++).name);
  pagerid->lwPagerIDs->setCurrentRow(0);
  if(pagerid->exec())
  {   
    index = pagerid->lwPagerIDs->currentRow();
    if(index >= 0)
    {
      lePagerName->setText(dbPagerIDs.at(index).name);
      leRic->setText(QSn(dbPagerIDs.at(index).ric));
      sbTx->setValue(dbPagerIDs.at(index).tx);
      maxMsgLen = dbPagerIDs.at(index).msgLen;
      sspFuncBits = dbPagerIDs.at(index).funcBits;
      setFunctionBits();
      sspBaud = dbPagerIDs.at(index).baud;
      setBaudRate();
      if(dbPagerIDs.at(index).numeric)
        tbNumeric->setChecked(true);
      else
        tbAlpha->setChecked(true);
      teMessage->setFocus();
    }
 }
  delete pagerid;
  pagerid = NULL;
}

// Called when the user selects a different tab
void MainWin::currentTabChanged(int tabIndex)
{
  if((isAutoPagingActive == false) && (twMain->tabText(tabIndex) != "Pager Msgs"))
  {
    while(isPlayingWav)                                     // Need to wait just incase some pages are being transmitted
    {
      gotoSleep::msleep(200);
      QCoreApplication::processEvents(QEventLoop::AllEvents); // Keep the GUI responsive
    }
    watchdog->start();
    isAutoPagingActive = true;
  }
  if(twMain->tabText(tabIndex) == "Page Entry")            // The page entry tab is selected
  {
    tbGetPagerID->setFocus();
  }
  else if(twMain->tabText(tabIndex) == "Pager Msgs")
  {
    QMessageBox msgBox;
    msgBox.setText("Access to the paging messages database file requires disabling the automatic paging system.");
    msgBox.setInformativeText("If you select 'Ok' then automatic paging will be disabled.\n"
                              "If a transmission is in progress you will have to wait until it finishes.\n"
                              "Automatic paging will be enabled when you leave this tab.\n\n"
                              "If you select 'Cancel' then you will not be able to edit the database.");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setGeometry(x() + 80, y() + 100, msgBox.width(), msgBox.height());
    int ret = msgBox.exec();
    if(ret == QMessageBox::Ok)
    {
      isAutoPagingActive = false;
      watchdog->stop();
      while(!txComplete)                                     // Need to wait until the database is not being used
      {
        gotoSleep::msleep(200);
        QCoreApplication::processEvents(QEventLoop::AllEvents); // Keep the GUI responsive
      }
      watchdog->stop();
      lwPages->clear();
      leRic_3->clear();
      teMessage_3->clear();
      readTextFile(userDir + "/gascop.db", gascopdb);
      populatePagerMsgsEditor();
      pbDeleteEntry_3->setEnabled(true);
    }
    else
    {
      pbDeleteEntry_3->setEnabled(false);
    }
    lwPages->setFocus();
  }
  else if(twMain->tabText(tabIndex) == "Pager IDs")
  {
    lwPagerIDs->setFocus();
  }
}

// Cgange the message font colour when the max msg length for the pager is exceded
void MainWin::msgTextChanged()
{
  if((teMessage->document()->characterCount() > maxMsgLen) && (teMessage->textColor() != Qt::red))
    teMessage->setTextColor(Qt::red);
  else if((teMessage->document()->characterCount() < maxMsgLen) && (teMessage->textColor() != Qt::black))
    teMessage->setTextColor(Qt::black);
}

void MainWin::periodIndexChanged(QString text)
{
  if(text == "Minutes") sbPeriodQty->setMaximum(59);
  if(text == "Hours") sbPeriodQty->setMaximum(23);
  if(text == "Days") sbPeriodQty->setMaximum(6);
  if(text == "Weeks") sbPeriodQty->setMaximum(51);
  if(text == "Months") sbPeriodQty->setMaximum(11);
  if(text == "Years") sbPeriodQty->setMaximum(20);
}

// Load the log file into the plain text editor
void MainWin::logReload()
{
  QStringList lf;
  readTextFile(userDir + "/gascop.log", &lf);
  logFile->document()->setPlainText(lf.join("\n"));
}

void MainWin::loadPrevPages()
{
  pPages.clear();
  readTextFile(userDir + "/prev-pages.txt", &pPages);     // Read the previous pages file into the prev list widget
  lwPrevPages->clear();
  for(int x = 0; x < pPages.count(); x++)                 // Populate the previous pages list
  {
    QStringList ln = pPages.at(x).split("%|%");
    if(ln.count() == 3)
    {
      lwPrevPages->addItem(ln.at(0) + "-" + ln.at(1) + " " + ln.at(2));
    }
  }
  lwPrevPages->setCurrentRow(0);                          // Select the first row in the previous pages list
}

// Save the currently loaded log file
void MainWin::logSave()
{
  QStringList lf;
  lf = logFile->document()->toPlainText().split("\n", QString::KeepEmptyParts);
  writeTextFile(userDir + "/gascop.log", &lf);
}

// Writes various information and messages to the loaded log file and also writes it to the log file on disk
void MainWin::displayMessages(QString msg)
{
  QStringList sl;
  sl << intToTime(timeToInt()) + " : " + msg;
  logFile->appendPlainText(intToTime(timeToInt()) + " : " + msg);
  appendTextFile(userDir + "/gascop.log", &sl);
}

void MainWin::dataLevelChanged(int value)
{
  levelLabel->setText(QSn(value));
}

// Sent constant preamble as a tuning up signal
void MainWin::tuneupTransmit()
{
  tbTune->setEnabled(false);
  pbSend->setEnabled(false);
  tbTuneStop->setEnabled(true);
  for(int x = 0; x < 3700; x++)
  {
    msgcw.append(preambleCW);
  }
  singlePage = true;
  buildPocsagWaveData(&msgcw, getBaudRate());
}

void MainWin::tuneupStop()
{
  tbTuneStop->setEnabled(false);
  if(isPlayingWav)
  {
    isPlayingWav = false;
    Mix_HaltChannel(pocsagChannel);   // Halt the channel
  }
  Mix_FreeChunk(SC_sound);            // Free the chunck
  SC_sound = NULL;                    // Null the pointer
  txLengthTimeout();                  // Run the normal function for when a transmission is finished
}

// If an alpha msg is in the message window and numeric is selected this will clear the msg window unless the text is valid for numeric messages
void MainWin::numericClicked()
{
  QString txt = teMessage->toPlainText();
  for(int x = 0; x < txt.length(); x++)
  {
    if((txt[x] < '0') || (txt[x] > '9'))
    {
      if((txt[x] != 'U') && (txt[x] != '[') && (txt[x] != ']') && (txt[x] != '-'))
        teMessage->clear();
    }
  }
}

// Fast search for a RIC code
void MainWin::ricSearch()
{
  if((leStartRic->text() == "") || (leStartRic->text().toUInt() > 2097151))
  {
    QMessageBox::critical(0, "Input Data Error", "You must enter a start RIC smaller than 2097151");
    return;
  }
  txComplete = false;
  isRicSearching = true;
  isPlayingWav = true;
  pbStart->setEnabled(false);
  int startRIC;
  leStartRic->setText(QSn((leStartRic->text().toInt() / 8) * 8));
  startRIC = leStartRic->text().toInt();
  lwRicSearch->clear();
  msgcw.clear();
  msgcw.append(syncCW);
  while(startRIC < leStartRic->text().toInt() + sbAmtRics->value())
  {
    if(cbSendRic->isChecked()) // This section sends the RIC code (last 5 digits only) as a numeric message to the RIC being transmitted.
    {
      QList<quint32> ncw;
      for(int x = 0; x < 8; x++)
      {
        msgcw.append(getRicCode(startRIC, getFunctionBits()));
        ncw.clear();
        encodeNumericCW(QSn(startRIC + x).right(5), &ncw);  // Just encode the last 5 digits of the RIC as it fits in a single codeword
        msgcw.append(ncw.at(0));
      }
      msgcw.append(syncCW);
      startRIC += 8;
    }
    else  // This section just transmits RICs only to set a pager off, every position contains a RIC code hence this is fast
    {
      for(int x = 0; x < 8; x++)
      {
        msgcw.append(getRicCode(startRIC, getFunctionBits()));
        startRIC += 8;
        msgcw.append(getRicCode(startRIC, getFunctionBits()));
        startRIC -= 8;
      }
      msgcw.append(syncCW);
      startRIC += 16;
    }
  }
  for(int y = 0; y < 16; y++)
  {
    msgcw.append(idleCW);
  }
  searchInfo->setText("Sending RIC " + leStartRic->text() + " to " + QSn(startRIC - 1));
  displayMessages("# RIC SEARCH #  " + searchInfo->text() + "  # RIC SEARCH #");
  leStartRic->setText(QSn(startRIC));
  displayPocsagData(lwRicSearch);
  pbAbort->setEnabled(true);
  buildPocsagWaveData(&msgcw, getBaudRate());
}

// Abort the RIC search
void MainWin::ricSearchAbort()
{
  sbAmtTxs->setValue(1);
  if(isPlayingWav)
  {
    isPlayingWav = false;
    Mix_HaltChannel(pocsagChannel);   // Halt the channel
  }
  Mix_FreeChunk(SC_sound);            // Free the chunck
  SC_sound = NULL;                    // Null the pointer
  txLengthTimeout();                  // Run the normal function for when a transmission is finished
  pbAbort->setEnabled(false);         // Disbale the abort button
}

void MainWin::resetTimeClicked()
{
  dtSendTime->setDateTime(QDateTime::currentDateTime());
}

// Send pager messages one at a time by hand
void MainWin::sendSinglePage()
{
  if(dtSendTime->dateTime().toTime_t() > (timeToInt() + 30))  // If the page is for a time more than 30 seconds into the future send to the db
  {
    dbPage.baud = getBaudRate();
    dbPage.funcBits = getFunctionBits();
    dbPage.ric = leRic->text().toInt();
    dbPage.schedule = (sbPeriodQty->value() << 4) + cbPeriod->currentIndex();
    dbPage.howMany = sbHowMany->value();
    dbPage.tx = sbTx->value();
    dbPage.msg = teMessage->toPlainText();
    dbPage.numeric = tbNumeric->isChecked();
    dbPage.sendTime = dtSendTime->dateTime().toTime_t();
    dbPage.flag = 0;
    dbPage.spare = 0;
    writeToPagesDatabase();
  }
  else                                                        // If the time is either now or earlier then send it now
  {
    pbSend->setEnabled(false);                                // Disable the ability to send while sending current page
    txComplete = false;
    isPlayingWav = true;
    sspFuncBits = getFunctionBits();
    sspBaud = getBaudRate();
    while(leRic->text().length() < 7)
      leRic->setText("0" + leRic->text());
    msgToSend = leRic->text() + "  " + teMessage->toPlainText();  // This is the page info to display with the pocsag data output
    savePreviousPages(leRic->text(), teMessage->toPlainText());
    sendOnePage(leRic->text().toInt(), sspFuncBits, teMessage->toPlainText(), sspBaud, tbNumeric->isChecked());
  }
}

// sets the baud rate buttons
void MainWin::setBaudRate()
{
  if(sspBaud == 512)
    tb512->setChecked(true);
  else if(sspBaud == 1200)
    tb1200->setChecked(true);
  else
    tb2400->setChecked(true);
}

// Sets the function bits buttons
void MainWin::setFunctionBits()
{
  if(sspFuncBits == 0)
    tbFuncNumeric->setChecked(true);
  else if(sspFuncBits == 1)
    tbFuncTone1->setChecked(true);
  else if(sspFuncBits == 2)
    tbFuncTone2->setChecked(true);
  else
    tbFuncAlpha->setChecked(true);
}

// return the current function bits value
int MainWin::getFunctionBits()
{
  if(tbFuncAlpha->isChecked())
    return(3);
  else if(tbFuncTone2->isChecked())
    return(2);
  else if(tbFuncTone1->isChecked())
    return(1);
  else
    return(0);
}

// Return the current baud rate
int MainWin::getBaudRate()
{
  if(tb512->isChecked())
    return(512);
  else if(tb1200->isChecked())
    return(1200);
  else
    return(2400);
}

// User selects a prev page from the list by either double clicking or pressing <Enter>
void MainWin::prevPagesDoubleClicked(QListWidgetItem * item)
{
  if(item->text().left(1) == "0")
  {
    sspBaud = 512;
    tb512->setChecked(true);
  }
  else if(item->text().left(1) == "1")
  {
    sspBaud = 1200;
    tb1200->setChecked(true);
  }
  else
  {
    sspBaud = 2400;
    tb2400->setChecked(true);
  }
  sspFuncBits = item->text().mid(1, 1).toInt();
  if(sspFuncBits == 0)
    tbFuncNumeric->setChecked(true);
  else if(sspFuncBits == 1)
    tbFuncTone1->setChecked(true);
  else if(sspFuncBits == 2)
    tbFuncTone2->setChecked(true);
  else
    tbFuncAlpha->setChecked(true);
  if(item->text().mid(2, 1) == "0")
    tbAlpha->setChecked(true);
  else
    tbNumeric->setChecked(true);
  leRic->setText(item->text().mid(item->text().indexOf("-") + 1, item->text().indexOf(" ") - 4));  // Grab the RIC from the prev page
  teMessage->setPlainText(item->text().mid(item->text().indexOf(" ") + 1));  // Grab the message from the prev page
}

// Save the page message just sent to the previous pages list
void MainWin::savePreviousPages(QString ric, QString msg)
{
  QString fbb;
  if(sspBaud == 512)
    fbb = "0";
  else if(sspBaud == 1200)
    fbb = "1";
  else
    fbb = "2";
  fbb += QSn(sspFuncBits);
  if(tbNumeric->isChecked())
    fbb += "1";
  else
    fbb += "0";
  pPages << (fbb + "%|%" + ric + "%|%" + msg);
  pPages.removeDuplicates();
  int crow = lwPrevPages->currentRow();
  writeTextFile(userDir + "/prev-pages.txt", &pPages);
  loadPrevPages();
  lwPrevPages->setCurrentRow(crow);
}

// Given a RIC, function bits, a message and a baud rate this function calls the required functions to build the required
// codewords for a transmission.
void MainWin::sendOnePage(quint32 ric, quint8 funcBits, QString message, quint16 baud, bool numeric)
{
  msgcw.clear();
  quint32 rcw = getRicCode(ric, funcBits);
  int addrPosition = (ric % 8) * 2;
  for(int x = 0; x < addrPosition; x++)
    msgcw.append(idleCW);
  msgcw.append(rcw);
  if(message.length())
  {
    if(numeric)
      encodeNumericCW(message, &msgcw);
    else
      encodeAlphaCW(message, &msgcw);
  }
  while(msgcw.count() % 16)
  {
    msgcw.append(idleCW);
  }
  QString pagesText = "";
  QString rc = QSn(ric);
  while(rc.length() < 7)
    rc = "0" + rc;
  //pagesText = rc + " " + message.left(50);
  pagesText = rc + " " + message;
//  if(message.length() > 50)
//    pagesText += "...";
  displayMessages(pagesText);
  singlePage = true;
  formatPocsagData();
  displayPocsagData(lwPocsag);
  buildPocsagWaveData(&msgcw, baud);
}

// Add the sync and idle codewords to the data and ensure it conforms to the protocol
void MainWin::formatPocsagData()
{
  QList<quint32> fpd;                                   // This is the list we are formatting the data into
  int index = 1;                                        // Keeps track of how many codewords between syncs
  fpd.append(syncCW);                                   // Add the first starting sync codeword
  while(msgcw.count() > 0)                              // Add the rest of the data
  {
    fpd.append(msgcw.takeFirst());
    if(index % 16 == 0)                                 // Detect when sync codewords need to be inserted
    fpd.append(syncCW);
    index++;
  }
  if(fpd.at(fpd.count() - 1) == syncCW)
  fpd.removeLast();
  msgcw.clear();
  while(fpd.count())                                    // Place the formatted data back into the pocsag data list
    msgcw.append(fpd.takeFirst());
  if(msgcw.at(msgcw.count() - 1) != idleCW)             // If the last batch ends on a message or RIC code word then we need to add a batch of idle CWs
  {
    msgcw.append(syncCW);
    for(int x= 0; x < 16; x++)
      msgcw.append(idleCW);
  }
}

// Create a complete transmission of pages to be sent stacked as efficiently as possible into a transmission.
void MainWin::stackPages()
{
  QString pagesText = "";
  msgToSend = "";
  for(int x = 0; x < txSendPages.count(); x++)
  {
    QString theRic = txSendPages.at(x).ric;
    while(theRic.length() < 7)
      theRic = "0" + theRic;
//    if(txSendPages.at(x).msg.length() > 57)
//      pagesText = theRic + "  " + txSendPages.at(x).msg.left(57) + "...\n";
//    else
//      pagesText = theRic + "  " + txSendPages.at(x).msg.left(60) + "\n";
    pagesText = theRic + "  " + txSendPages.at(x).msg;
    displayMessages(pagesText.trimmed());                 // Now we have the pages we display the ric and message
    msgToSend += pagesText;                               // Used in the pocsag data view
  }
  bool gotOne = false;                                    // Gets set to true every time a page of the correct ric position is found
  int index;                                              // The index into the txSendPages vector
  int lookFor = 0;                                        // The ric position we are looking for to place in the transmission
  msgcw.clear();                                          // Clear the codeword list
  while(txSendPages.count())                              // While we still have pages to encode
  {
    gotOne = false;                                       // Reset the flag showing we don't have a page
    for(int x = 0; x < txSendPages.count(); x++)          // Iterate through all the pager messages
    {
      int rp = txSendPages.at(x).ricPos * 2;              // ric positions are stored as 0 to 7 but we are checking as 0 to 15
      if((rp == lookFor) || (rp+1 == lookFor))            // Test if the page has the correct ric position (note there are 2 positions per ric!)
      {
        gotOne = true;                                    // Found one so set the flag
        index =x;                                         // Keep a note of the index
        break;                                            // Get out of the for loop
      }
    }
    if(gotOne)                                            // If we got a page to encode at the current position...
    {
      quint32 rcw = getRicCode(txSendPages.at(index).ric.toInt(), txSendPages.at(index).funcBits); // Encode the ric
      msgcw.append(rcw);                                  // Place the ric into the codeword list
      if(txSendPages.at(index).msg.length())              // Only encode the message if there is one
      {
        if(txSendPages.at(index).numeric)                     // Test if this is a numeric page or an alpha page
          encodeNumericCW(txSendPages.at(index).msg, &msgcw); // Encode the message into numeric codewords
        else
          encodeAlphaCW(txSendPages.at(index).msg, &msgcw);   // Encode the message into alpha codewords
      }
      txSendPages.remove(index);                          // Remove the message from the vector now that we have it
      lookFor = (msgcw.count() % 16);                     // Determine what the next ric position point is
    }
    else                                                  // If there was no page at that ric position
    {
      if(!(lookFor % 2))                                  // If there is not a remainder then lookFor is even and we need to idle both cw's
      {
        msgcw.append(idleCW);                             // Put an idle cw in the first pos
        lookFor++;                                        // Move lookFor to the second pos
      }
      msgcw.append(idleCW);                               // Put an idle cw in the second pos
      lookFor++;                                          // Move on to the next ric position
      if(lookFor > 15)                                    // If we reach the last ric position...
        lookFor = 0;                                      // ...Go back to the first ric position
    }
  }
  while(msgcw.count() % 16)                               // While the last batch is not complete...
  {
    msgcw.append(idleCW);                                 // ...fill it with idle codewords
  }
  formatPocsagData();
  displayPocsagData(lwPocsag);
  buildPocsagWaveData(&msgcw, currentBaudRate);
}

// Create a pocsag data array of 32bit unsigned integers and a .wav sound file (if needed)
void MainWin::buildPocsagWaveData(QList<quint32> * cw, int baud)
{
  pd.clear();
  for(int i = 0; i < 18; i++)                             // Loop through 18 codewords of preamble
    pd.append(preambleCW);                                // Add the preamble to the buffer
  for(int i = 0; i < cw->count(); i++)                    // Add the message data to the vector
    pd.append(cw->at(i));
  int changePoint = 32;                                   // Default to 1200 baud
  if(baud == 512)
    changePoint = 75;                                     // How many data bits equals a wave file bit
  if(baud == 1200)
    changePoint = 32;                                     // 19 is close to 1200 baud
  if(baud == 2400)
    changePoint = 16;
  int posPtr = createWaveHeader(changePoint);             // Create the header part of the wave file
  if(posPtr == -1)
    return;
  posPtr = createWaveData(posPtr, changePoint);           // Create the data part of the wave file
  if(DEBUG)
    createPocsagWaveFile(posPtr);                         // This will write the pocsag wave data to a .wav file
  msgcw.clear();                                          // Posag is displayed and the wave file is created we can dump this data now
  double bits;                                            // 18 CW's preamble : n CW's of data : tb = ((18 + n) * 32)
  double timePerBit;                                      // One bit takes (1 / baud) = tpb
  double totalTxTime;                                     // Total transmission time is : tpb * tb
  bits = (double)pd.count() * 32;                         // Also this --->  ((((float)waveFileDataLength / (float)22050) * 1000) + 1000);
  timePerBit = 1.0 / 1200.0;                              // datalen / samplesPerSecond = time in seconds that the file will play for
  totalTxTime = ((bits * timePerBit) * 1000.0) + 200;
  int tt;
  tt = round(totalTxTime);
  txLength->setInterval(tt);
  playWaveData();
}

int MainWin::createWaveHeader(int changePoint)
{
  int waveFileSize = ((pd.count() * 4) * 16) * changePoint;
  if(pocsagBuffer != NULL)
    delete [] pocsagBuffer;
  pocsagBuffer = new (nothrow) Uint8[waveFileSize + 48];  // Making the array 1 integer(32 bit) bigger than needed (coz I can)
  if(pocsagBuffer == NULL)
  {
    QMessageBox::critical(0, "Memory Allocation Error", "Not enough memory available to generate the pocsag data!\n"
                              "Try closing some other applications to free up more memory.");
    tbTune->setEnabled(true);
    pbSend->setEnabled(true);
    return(-1);
  }
  quint32 wavHeader[11] = {0x52494646, 0x00000000, 0x57415645, 0x666D7420, 0x10000000, 0x01000100, 0x00960000, 0x002C0100,
                           0x02001000, 0x64617461, 0x00000000};
  wavHeader[1] = qToBigEndian(waveFileSize + 36);         // Set the size of the wave data into the header - Change it to big endian on route
  wavHeader[10] = qToBigEndian(waveFileSize);             // ditto
  int r = 0;                                              // Index to place all the bytes of the header and message bytes into the Uint8 buffer
  for(int i = 0; i < 11; i++)                             // Iterate through all 11 32 bit codewords of the header
  {
    quint32 andMask = 0xff000000;                         // Set a mask for the top 8 bits
    pocsagBuffer[r++] = ((wavHeader[i] & andMask) >> 24); // Add the wave header to the wave buffer
    andMask = 0x00ff0000;                                 // Set a mask for the next 8 bits
    pocsagBuffer[r++] = ((wavHeader[i] & andMask) >> 16);
    andMask = 0x0000ff00;                                 // Set a mask for the next 8 bits
    pocsagBuffer[r++] = ((wavHeader[i] & andMask) >> 8);
    andMask = 0x000000ff;                                 // Set a mask for the bottom 8 bits
    pocsagBuffer[r++] = (wavHeader[i] & andMask);
  }
  return(r);
}

int MainWin::createWaveData(int r, int changePoint)
{
  quint32 codewordBitMask = 0x80000000;                   // Set up a start point for the wave data bit mask
  quint8 bitval = 0x80;                                   // Set a bit mask for the buffer bytes
  int pdindex = 0;                                        // Set an index into the first 32 bit codeword vector
  quint8 volumeVal = pocsagLevel->value();                // Valid values are 0 = max volume or 255 = silent
  while(pdindex < pd.count())                             // While there is codewords to process
  {
    while(codewordBitMask > 0)                            // While the codeword mask is greater than zero
    {
      if(tbInverted->isChecked())                         // Check to see if we need to invert the data
      {
        if((pd.at(pdindex) & codewordBitMask) != 0)       // Test the bit in the codeword
          bitval = 0xff - volumeVal;                      // Set a byte value if the bit is a one
        else
          bitval = 0x00 + volumeVal;                      // Set a byte value of zero if the bit is zero
      }
      else
      {
        if((pd.at(pdindex) & codewordBitMask) == 0)       // Test the bit in the codeword
          bitval = 0xff - volumeVal;                      // Set a byte value if the bit is a one
        else
          bitval = 0x00 + volumeVal;                      // Set a byte value of zero if the bit is zero
      }
      for(int x = 0; x < changePoint; x++)  // This is setting the bytes of the wave file. Make it work at bit level for greater fine tuning accuracy.
      {
        pocsagBuffer[r++] = bitval;                       // When a byte is set we add it to the byte array buffer:
                                                        // Note r it is used continually from indexing the header above to indexing the message bytes.
        pocsagBuffer[r++] = bitval;
      }
      codewordBitMask >>= 1;                              // Keep shifting the bit mask for the codeword
    }
    codewordBitMask = 0x80000000;                         // Once the bit mask has been shifted doen to zero we must reset it for the next codeword.
    pdindex++;                                            // Increase the iterator pointing to each 23 bit codeword
  }
  return(r);
}

// This function is called whenever a transmission completes
void MainWin::txLengthTimeout()
{
  txLength->stop();                               // Stop this timer now we are here
  while(isPlayingWav)                             // This will never happen but if somehow we get here and it is still playing then wait till its done
  {
    gotoSleep::msleep(100);
    QCoreApplication::processEvents(QEventLoop::AllEvents); // Process events while we are waiting
  }
  if(rbSop->isChecked())
  {
    if(singlePage)
    {
      if(normInv[sbTx->value()-1] == 1)
        QProcess::startDetached("gpio -g write " + QSn(gpioPin[sbTx->value()-1]) + " 0"); // Dekey the transmitter
      else
        QProcess::startDetached("gpio -g write " + QSn(gpioPin[sbTx->value()-1]) + " 1");
    }
    else
    {
      if(normInv[currentTx-1] == 1)
        QProcess::startDetached("gpio -g write " + QSn(gpioPin[currentTx-1]) + " 0"); // Dekey the transmitter
      else
        QProcess::startDetached("gpio -g write " + QSn(gpioPin[currentTx-1]) + " 1");
    }
  }
  if(rbPic->isChecked())
  {

  }
  tbTune->setEnabled(true);                       // If it was a tune up enable the button, if it wasn't then this does nothing
  pbSend->setEnabled(true);                       // As remark above
  pbStart->setEnabled(true);                      // As remark above
  gbLevel->setEnabled(true);
  tbTuneStop->setEnabled(false);
  delete [] pocsagBuffer;                         // Free the pocsag buffer
  pocsagBuffer = NULL;                            // Null the pointer
  SDL_FreeWAV(&pocsagBuffer[0]);                  // Free the audio pocsag buffer
  Mix_CloseAudio();                               // Close the Mixer audio: !!!!! Important: without it you get high CPU usage after the tx !!!!!
  if((isRicSearching) && (sbAmtTxs->value() > 1)) // If we are searching for RICs and there are more transmissions required...
  {
    sbAmtTxs->setValue(sbAmtTxs->value() - 1);    // Bump down the transmission amount counter
    ricSearch();                                  // call the Ric search routine again
    return;                                       // Lets get out of here
  }
  isRicSearching = false;                         // Clear the flag now that RIC searching is finished
  if(singlePage)                                  // If just single paging then get out of here now
  {
    singlePage = false;
    pbSend->setFocus();
    return;
  }
  if(!testMode)                                   //  If we are not in test mode then delete the sent pages from the database
    deleteSentPages();                            // delete the pages just sent
  else
    resetSentPages();                             // reset the pages just sent when in test mode
  populatePagerMsgsEditor();                      // Update the pager messages editor.
  watchdog->start();                              // Start the message checking timer again.
  txComplete = true;
}

// Delete the pages just sent, they have the processing flag set to true
void MainWin::deleteSentPages()
{
  setFileLock("gascop.db");   // We now have 2 seconds before we must give up the lock
  setRepeatTime();
  int index = gascopdb->count() -1;
  while(index >= 0)
  {
    if(gascopdb->at(index).left(1) == "1")
      gascopdb->removeAt(index);
    index--;
  }
  writeTextFile(userDir + "/gascop.db", gascopdb);
  QFile::remove(userDir + "/gascop.db.lck");
}

// reset the processing flag to false on sent pages when in test mode
void MainWin::resetSentPages()
{
  setFileLock("gascop.db");   // We now have 2 seconds before we must give up the lock
  setRepeatTime();
  int index = 0;
  while(index < gascopdb->count())
  {
    if(gascopdb->at(index).left(1) == "1")
      gascopdb->replace(index, "0" + gascopdb->at(index).mid(1));
    index++;
  }
  writeTextFile(userDir + "/gascop.db", gascopdb);
  QFile::remove(userDir + "/gascop.db.lck");
}

void MainWin::setRepeatTime()
{
  int index = gascopdb->count() -1;
  while(index >= 0)
  {
    if((gascopdb->at(index).left(1) == "1") && (gascopdb->at(index).mid(18,2).toInt() > 0) && (gascopdb->at(index).mid(15,3).toInt() > 0))
    {
      int periodQty = gascopdb->at(index).mid(15,3).toInt() >> 4;
      int period = gascopdb->at(index).mid(15,3).toInt() & 0x0f;
      int hm = gascopdb->at(index).mid(18,2).toInt();                 // Decrement this and delete the page when it reaches zero
      hm--;
      if(hm >= 0)
      {
        uint tPeriod = 0;                                                // How many seconds to add to the unix time for the pager message
        if(period == 0) tPeriod = periodQty * 60;                     // Mins
        else if(period == 1) tPeriod = periodQty * 3600;              // Hours
        else if(period == 2) tPeriod = periodQty * 86400;             // Days
        else if(period == 3) tPeriod = periodQty * 604800;            // Weeks
        else if(period == 4)                                          // Months
        {
          QDateTime cdt = QDateTime::currentDateTime();
          cdt = cdt.addMonths(periodQty);
          tPeriod = cdt.toTime_t() - QDateTime::currentDateTime().toTime_t();
        }
        else if(period == 5)                                          // Years
        {
          QDateTime cdt = QDateTime::currentDateTime();
          cdt = cdt.addYears(periodQty);
          tPeriod = cdt.toTime_t() - QDateTime::currentDateTime().toTime_t();
        }
        uint newTime = gascopdb->at(index).mid(1,10).toUInt() + tPeriod;
        QString hmany = QSn(hm);
        if(hmany.length() < 2)
          hmany = "0" + hmany;
        QString newLine = "0" + QSn(newTime) + gascopdb->at(index).mid(11,7) + hmany + gascopdb->at(index).mid(20);
        gascopdb->replace(index, newLine);
      }
      else
        gascopdb->removeAt(index);
    }
    index--;
  }
}

// Write the pocsag data to a .wav file and save the file to disk
void MainWin::createPocsagWaveFile(int size)
{
  QByteArray buffer;                        // Fill a byte array for filing the wave file
  buffer.resize(size);
  buffer.clear();
  for(int x = 0; x < size; x++)
    buffer.append((uchar) pocsagBuffer[x]);
  writeBinFile(userDir + "/pocsag-out.wav", &buffer);   // temp for test only
}

// This is where the wave file created in memory is actually played using SDL
void MainWin::playWaveData()
{                                   // Play the pocsag data in memory to the soundcard using SDL.
  audio_rate = 38400;               // Frequency of audio playback
  audio_format = AUDIO_S16SYS;         // Format of the audio we're playing
  audio_channels = 1;               // 2 channels = stereo
  audio_buffers = 2048;             // Size of the audio buffers in memory
  if(SDL_Init(SDL_INIT_AUDIO) != 0) // Initialize SDL audio
  {
    displayMessages("Unable to initialize SDL: " + (QString)SDL_GetError());
    return;
  }
  if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0) // Initialize SDL_mixer with our chosen audio settings
  {
    displayMessages("Unable to initialize audio: " + (QString)Mix_GetError());
    return;
  }
  SC_sound = Mix_QuickLoad_WAV(&pocsagBuffer[0]); // Pointer to our sound, in memory
  if(SC_sound == NULL)
  {
    displayMessages("Unable to load POCSAG data: " + (QString)Mix_GetError());
    return;
  }
  if(rbSop->isChecked())
  {
    if(singlePage)
    {
      QProcess::startDetached("gpio -g write " + QSn(gpioPin[sbTx->value()-1]) + " " + QSn(normInv[sbTx->value()-1]));
      gotoSleep::msleep(keyDelay[sbTx->value()-1]);
    }
    else
    {
      QProcess::startDetached("gpio -g write " + QSn(gpioPin[currentTx-1]) + " " + QSn(normInv[currentTx-1]));
      gotoSleep::msleep(keyDelay[currentTx-1]);
    }
  }
  if(rbPic->isChecked())
  {
    if(singlePage)
      gotoSleep::msleep(keyDelay[sbTx->value()-1]);
    else
      gotoSleep::msleep(keyDelay[currentTx-1]);
  }
  pocsagChannel = Mix_PlayChannel(-1, SC_sound, 0); // Play our sound file, and capture the channel on which it is played
  Mix_ChannelFinished(channelDone);
  //void Mix_ChannelFinished(void (*channel_finished)(int channel));
  if(pocsagChannel == -1)
  {
    displayMessages("Unable to send POCSAG data: " + (QString)Mix_GetError());
    return;
  }
  else
  {
    gbLevel->setEnabled(false);
    txLength->start();
    //isPlayingWav = true;
  }
}

// Callback for the SDL Mix_Play_Channel, we release the sound chunk and NULL it.
void channelDone(int channel)
{
  if(isPlayingWav)
  {
    isPlayingWav = false;
    Mix_HaltChannel(channel);
  }
  if(SC_sound != NULL)
    Mix_FreeChunk(SC_sound);
  SC_sound = NULL;
}

// Test keying on the setup tab
void MainWin::keyTxClicked()
{
  if(rbSop->isChecked())
  {
    if(pbKeyTx->isChecked())
      QProcess::startDetached("gpio -g write " + QSn(gpioPin[sbTxTest->value()-1]) + " " + QSn(normInv[sbTxTest->value()-1]));
    else
    {
      if(normInv[sbTxTest->value()-1] == 1)
        QProcess::startDetached("gpio -g write " + QSn(gpioPin[sbTxTest->value()-1]) + " 0"); // Dekey the transmitter
      else
        QProcess::startDetached("gpio -g write " + QSn(gpioPin[sbTxTest->value()-1]) + " 1");
    }
  }
}

// Returns a RIC number from an address codeword
quint32 MainWin::addrCwToRic(quint32 cw)  // Changes an address codeword to a RIC
{
  if((cw & 0x80000000) > 0)       // Make sure this codeword is an address codeword
    return(0);                    // return zero to show this is not a address codeword
  else
    return((cw >> 13) * 8);       // Extract and return the RIC from the codeword
}

// Returns a pocsag codeword for a given RIC and its function bits
quint32 MainWin::getRicCode(quint32 ric, quint8 func)
{
  quint32 r1 = (ric >> 3) << 5;   // divide by 8
  r1 = (r1 + (func << 3));
  quint32 r2 = genChkBits(r1);
  return(buildCW(r1, r2));
}

// join the cw and checksum
quint32 MainWin::buildCW(quint32 cw, quint32 cb)
{
  cw = cw << 8;
  cb = (cb & 0x7FF);
  quint32 temp = cw + cb;
  quint32 tmp1 = temp;
  quint32 tmp2 = 0;
  for(int x = 1; x <= 32; x++)
  {
    if((tmp1 & 1) == 1)
      tmp2++;
    tmp1 = tmp1 >> 1;
  }
  if((tmp2 % 2) != 0)
    temp = temp + 1;
 return(temp);
}

// Generate the checkbits for a codeword
quint32 MainWin::genChkBits(quint32 cw)
{
  ushort chkBits = 0;
  for(int n = 1; n <= 21; n++)
  {
    int bit = ((cw >> 20) & 8);
    bit = bit >> 3;
    cw = cw << 1;
    if((1 & (bit ^ (chkBits >> 10))) == 1)
      chkBits = chkBits ^ 0x769;
    chkBits = chkBits << 1;
  }
  return(chkBits);
}

// Sets the even parity on the completed cw
quint32 MainWin::setParity(quint32 cw)
{
  int bit = 0;
  quint32 temp;
  temp = cw;
  for(int n = 1; n <= 32; n++)
  {
    if(temp & 0x00000001)
      bit += 1;
    temp >>= 1;
  }
  if(bit % 2)
    cw ^= 0x00000001;
  return(cw);
}

// Create the alpha/numeric codewords required the send msg.
void MainWin::encodeAlphaCW(QString msg, QList<quint32> * cwc)
{
  int i = 0;                      // The index of the character in the message that is being encoded
  quint8 n = 0;                   // This holds the current character being encoded as an ascii value.
  n = msg[i].toAscii() & 0x7F;    // Set n to equal the 7 bit ascii value of the first character in the message.
  quint8 msg_index = 0x01;        // This tracks the bits in each word of the message.
  quint32 cw_index = 0x40000000;  // This tracks the 20 bits of the words in the codeword.
  quint32 cwv = 0x80000000;       // Message codewords always have the top bit set.
  while(true)                     // Loop through all the characters in the message.
  {
    if(n & msg_index)             // n is the current character we are encoding into the codeword. If (n and msg_index) is true then...
      cwv |= cw_index;            // ...set the appropriate bit in the codeword.
    msg_index <<= 1;              // Move up to the next bit in the message character.
    cw_index >>= 1;               // Move down to the next bit in the codeword. (Remember the chars in the codeword are back to front.)
    if(msg_index == 128)          // If msg_index is 128 then the current character is complete.
    {
      msg_index = 0x01;           // Reset msg_index so it points to the lsb in the character.
      i++;                        // Incrementing i moves us onto the next character in the message.
      if(i == msg.length())        // Now the string is encoded we need to endcode EOT 0x04
        n = 0x04;
      else if(i >= msg.length() + 1) // Now the message complete with EOT is encoded we add 0x00 to the end of the current CW
        n = 0x00;
      else
        n = msg[i].toAscii() & 0x7F; // n equals the 7 bit ascii value of a character in the message.
    }
    if(cw_index == 0x400)         // cw_index reaches the end of the encoded message bits, now we generate the check bits and store the codeword.
    {
      cwv >>= 8;                  // Prepare codeword for generating the codeword.
      quint32 cksum = genChkBits(cwv); // Get the codeword complete with error checking code.
      cwv = buildCW(cwv, cksum);  // Join the checksum with the codeword.
      cwv = setParity(cwv);       // Set or clear the parity bit.
      cw_index = 0x40000000;      // Reset the codeword bit index ready for the next codeword
      cwc->append(cwv);           // Add the completed codeword to the QList.
      cwv = 0x80000000;           // Reset the working codeword to an empty message codeword.
      if(n == 0)                  // If we have completed a CW and we are adding 0x00 NULLs then we have finished
        break;
    }
  }
}

void MainWin::encodeNumericCW(QString msg, QList<quint32> * cwc)
{
  int i = 0;                      // The index of the character in the message that is being encoded
  quint8 n = 0;                   // This holds the current character being encoded as an ascii value.
  n = msg[i].toAscii() & 0x7F;    // Set n to equal the first bit of the first digit in the message.
  quint8 msg_index = 0x01;        // This tracks the four bits in each digit of the message.
  quint32 cw_index = 0x40000000;  // This tracks the 20 bits of the digits in the codeword.
  quint32 cwv = 0x80000000;       // Message codewords always have the top bit set, so we set it here.
  while(true)                     // Loop through all the characters in the message.
  {
    if(n == '\x41')               // 0xA Reserved (possibly used for address extension)
      n = '\x0a';
    else if(n == '\x55')          // 0xB Character U (urgency)
      n = '\x0b';
    else if(n == '\x20')          // 0xC " ", Space (blank)
      n = '\x0c';
    else if(n == '\x2d')          // 0xD "-", Hyphen (or dash)
      n = '\x0d';
    else if(n == '\x5d')          // 0xE "]", Left bracket
      n = '\x0e';
    else if(n == '\x5b')          // 0xF "[", Right bracket
      n = '\x0f';
    if(n & msg_index)             // n is the current character we are encoding into the codeword. If (n and msg_index) is true then...
      cwv |= cw_index;            // ...set the appropriate bit in the codeword.
    msg_index <<= 1;              // Move up to the next bit in the message character.
    cw_index >>= 1;               // Move down to the next bit in the codeword. (Remember the chars in the codeword are back to front.)
    if(msg_index == 0x10)         // If msg_index is 128 then the current character is complete.
    {
      msg_index = 0x01;           // Reset msg_index so it points to the lsb in the character.
      i++;                        // Incrementing i moves us onto the next character in the message.
      if(i >= msg.length())
        n = 0x0c;
      else
        n = msg[i].toAscii() & 0x7F; // n equals the 7 bit ascii value of a character in the message.
    }
    if(cw_index == 0x400)         // cw_index reaches the end of the encoded message bits, now we generate the check bits and store the codeword.
    {
      cwv >>= 8;                  // Prepare codeword for generating the codeword.
      quint32 cksum = genChkBits(cwv); // Get the codeword complete with error checking code.
      cwv = buildCW(cwv, cksum);  // Join the checksum with the codeword.
      cwv = setParity(cwv);       // Set or clear the parity bit.
      cw_index = 0x40000000;      // Reset the codeword bit index ready for the next codeword
      cwc->append(cwv);           // Add the completed codeword to the QList.
      cwv = 0x80000000;           // Reset the working codeword to an empty message codeword.
      if((i >= msg.length()) && (n == 0x0c))
        break;
    }
  }
}

// These two codewords are numeric A54DA1DA 960998CD
// Extract the numeric messaage from the message codewords.
QString MainWin::numericCWToStr(QList<quint32> * cw)
{
  QString message = "";
  int index_num = 0x01;               // points to the bit in the res char
  int index_cw = 0x40000000;          // points to the bit in the CW that we are testing
  int num_bit = 0;                    // is either zero or non-zero according to the bit we are testing in CW
  quint8 res = '\0';                  // This is the working result char(really a quint8) that is handed to result when a char is complete
  int x = 0;                          // This is the index for iterating through the codewords
  while(x < cw->count())              // Loop through each codeword of the message
  {
    num_bit = cw->at(x) & index_cw;   // num_bit is either 0 or not zero. This is used to set or clear a bit in the res char.
    if(num_bit > 0)                   // Test if we need to set a bit in res or just leave it zero
      res |= index_num;               // This sets the bit in res. index_num is the bit mask
    index_num <<= 1;                  // This moves index_char pointer on so we are pointing at the next MSB of res
    if(index_num == 0x10)             // If the pointer to the res bit points to the 4th bit then the result is finished for this bit...
    {                                 // ... we only have 4 bit BCD data.
      if(res == '\x0a')               // 0xA Reserved (possibly used for address extension)
        res = '\x41';
      else if(res == '\x0b')          // 0xB Character U (urgency)
        res = '\x55';
      else if(res == '\x0c')          // 0xC " ", Space (blank)
        res = '\x20';
      else if(res == '\x0d')          // 0xD "-", Hyphen (or dash)
        res = '\x2d';
      else if(res == '\x0e')          // 0xE "]", Left bracket
        res = '\x5d';
      else if(res == '\x0f')          // 0xF "[", Right bracket
        res = '\x5b';
      if(res < '\x0a')
        res |= 0x30;                  // Change the BCD to ASCII
      if(res == numeric_end_value)    // Test what the user says is an end of numeric data...
        break;                        // If the char is an end of numeric data then get out of here.
      message += res;                 // Place the quint8 value onto the end of the final message string.
      res = '\0';                     // Clear the working char
      index_num = 0x01;               // Set the char bit pointer to the first bit of the next res char
    }
    index_cw >>= 1;                   // bump the CW bit index down by 1 (divide by 2)
    if(index_cw < 0x800)              // If there are no more bits required from this CW
    {
      index_cw = 0x40000000;          // Reset the bit marker
      x++;                            // Move on to the next CW
    }
  }
  return(message.trimmed());          // Return the message with any trailing spaces removed
}

// Extract the alpha/num messaage from the message codewords. ####### Must check for invalid end of CW that could cause a hang ##############
QString MainWin::alphaCWToStr(QList<quint32> * cw)
{
  QString message = "";
  quint8 index_char = 0x01;           // points to the bit in the res char
  quint32 index_cw = 0x40000000;      // points to the bit in the CW that we are testing
  int char_bit = 0;                   // is either zero or non-zero according to the bit we are testing in CW
  quint8 res = '\0';                  // This is the working result char that is handed to result when a char is complete
  int x = 0;                          // This is the index for iterating through the codewords
  while(x < cw->count())              // Loop through each codeword of the message
  {
    char_bit = cw->at(x) & index_cw;  // char_bit is either 0 or not zero. This is used to set or clear a bit in the res char.
    if(char_bit > 0)                  // Test if we need to set a bit in res or just leave it zero
      res |= index_char;              // This sets the bit in res. index_char is the bit mask
    index_char <<= 1;                 // This moves index_char pointer on so we are pointing at the next MSB of res
    if(index_char == 0x80)            // If the pointer to the res bit points to the 8th bit then the result is finished ...
    {                                 // ... for this bit. we only have 7 bit data.
      message += res;                 // Place the quint8 value onto the end of the final message string.
      res = '\0';                     // Clear the working char
      index_char = 0x01;              // Set the char bit pointer to the first bit of the next res char
    }
    index_cw >>= 1;                   // bump the CW bit index down by 1 (divide by 2)
    if(index_cw < 0x800)              // If there are no more bits required from this CW
    {
      index_cw = 0x40000000;          // Reset the bit marker
      x++;                            // Move on to the next CW
    }
  }
  if(message.indexOf('\x04') <= 0)    // If there is no '\x04' then this cant be an alpha msg so must be numeric
    return("||##$*$##||");
  message = message.left(message.indexOf('\x04'));  // Remove the 0x04 and 0x00 from the end of the message
  return(message); // Return the complete message as a QString
}

// Shows POCSAG data in a list widget window
void MainWin::displayPocsagData(QListWidget * lw)
{
  if(!isRicSearching)
  {
    lw->clear();
    lw->addItem("             The Pager Messages Listed Here Are Encoded Below");
    lw->addItem("=============================================================================");
    lw->addItem(msgToSend);
    lw->addItem("=============================================================================");
  }
  int index = 0;
  while(index < msgcw.count())
  {
    lw->addItem(QSn(msgcw.at(index++), 16));
    QString line;
    for(int y = 0; y < 2; y++)
    {
      for(int x = 0; x < 8; x++)
      {
        line += QSn(msgcw.at(index++), 16) + " ";
      }
      lw->addItem(line);
      line = "";
    }
  }
  if(!isRicSearching)
  {
    lw->addItem("");
    lw->addItem("");
    lw->addItem("   The messages in order as extracted by 'decoding' the pocsag data above");
    lw->addItem("-----------------------------------------------------------------------------");
    lw->addItem("  RIC   pos     Message");
    lw->addItem("-----------------------------------------------------------------------------");
    QList<quint32> msgCodeWords;
    QString theRic;
    QString theMsg;
    int ricPos = 0;
    QString rp = "";
    index = 0;
    while(index < msgcw.count())
    {
      if((msgcw[index] == idleCW) || (msgcw[index] == syncCW))
      {
        if(msgcw[index] == syncCW)
          ricPos = 0;
        else
          ricPos++;
        index++;
        continue;
      }
      if(!(msgcw[index] & 0x80000000))                          // If this evaluates true then we have a RIC code
      {
        theRic = QSn((ricPos / 2) + addrCwToRic(msgcw[index]));  // Need to use the RIC position to get the RIC correct ###!!!!!!###
        rp = QSn(ricPos / 2);
        ricPos++;
        index++;
        while((msgcw[index] & 0x80000000))                      // If this evaluates true then we do have a message otherwise it is a RIC or idleCW
        {
          msgCodeWords.append(msgcw[index++]);
          ricPos++;
          if(msgcw[index] == syncCW)
          {
            ricPos = 0;
            index++;
          }
        }
        if(msgCodeWords.count())
        {
          theMsg = alphaCWToStr(&msgCodeWords); // We don't know if it is an aplha or numeric so decode it as an alpha and if there is not a '\x04'...
          if(theMsg == "||##$*$##||")           // ...on the end then it must be a numeric so the alphaCWToStr returns "||##$*$##||"
            theMsg = numericCWToStr(&msgCodeWords); // so now we decode it as a numeric CW
          msgCodeWords.clear();
        }
        while(theRic.length() < 7)
          theRic = "0" + theRic;
        lw->addItem(theRic + "  " + rp + "  " + theMsg);
        theMsg = "";
      }
    }
  }
}

// This event filter allows us to trap the keypress events for the pager text editor so we can limit the alowed keys for numeric pagers
bool MainWin::eventFilter(QObject * target, QEvent * event)
{
  if(target == teMessage)							// make sure the target is the message editor viewport
  {
    if(tbNumeric->isChecked())                                                  // Only if numeric is selected
    {
      if(event->type() == QEvent::KeyPress)					// make sure it is a keypress event
      {
        QKeyEvent * keyEvent = static_cast<QKeyEvent *>(event);
        if((keyEvent->key() == 0x5b) || (keyEvent->key() == 0x5d) || (keyEvent->key() == 0x55) || (keyEvent->key() == 0x41))
          return(false);
        if((keyEvent->key() == 0x01000012) || (keyEvent->key() == 0x01000013) || (keyEvent->key() == 0x20) || (keyEvent->key() == 0x2d))
          return(false);
        if((keyEvent->key() == 0x01000014) || (keyEvent->key() == 0x01000015) || (keyEvent->key() == 0x01000003) || (keyEvent->key() == 0x01000001))
          return(false);
        if((keyEvent->key() == 0x01000007) || (keyEvent->key() == 0x01000004) || (keyEvent->key() == 0x01000010))
          return(false);
        if((keyEvent->key() == 0x01000001) || (keyEvent->key() == 0x01000002))
          return(false);
        if((keyEvent->key() > 0x39) || (keyEvent->key() < 0x30))
          return(true);
      }
    }
  }
  if(target == leRic)
  {
    if(event->type() == QEvent::KeyPress)					// make sure it is a keypress event
    {
      QKeyEvent * keyEvent = static_cast<QKeyEvent *>(event);
      if((keyEvent->key() == 0x01000010) || (keyEvent->key() == 0x01000011) || (keyEvent->key() == 0x01000012))
        return(false);
      if((keyEvent->key() == 0x01000013) || (keyEvent->key() == 0x01000014) || (keyEvent->key() == 0x01000015))
        return(false);
      if((keyEvent->key() == 0x01000007) || (keyEvent->key() == 0x01000004) || (keyEvent->key() == 0x01000003))
        return(false);
      if((keyEvent->key() == 0x01000001) || (keyEvent->key() == 0x01000002))
        return(false);
      if((keyEvent->key() > 0x39) || (keyEvent->key() < 0x30))
        return(true);
    }
  }
  if(target == lwPrevPages)
  {
    if(event->type() == QEvent::KeyPress)				// make sure it is a keypress event
    {
      int key = static_cast<QKeyEvent*>(event)->key();
      //int kek = keyEvent->key();
      //lePagerName->setText(lePagerName->text() + " : " + QSn(kek));
      if(key == Qt::Key_Return)                             // So the user can just press enter on the prev pages list
      {
        prevPagesDoubleClicked(lwPrevPages->currentItem());
        return(true);
      }
      if(key == Qt::Key_Backspace)
      {
        teMessage->setPlainText(lwPrevPages->item(lwPrevPages->currentRow())->text().mid(12));
      }
      if(key == Qt::Key_Delete)
      {
        lwPrevPages->takeItem(lwPrevPages->currentRow());
        pPages.clear();
        for(int x = 0; x < lwPrevPages->count(); x++)
        {
          QString tmp = lwPrevPages->item(x)->text();
          tmp = tmp.left(3) + "%|%" + tmp.mid(4, 7) + "%|%" + tmp.mid(12);
          pPages.append(tmp);
        }
        pPages.removeDuplicates();
        writeTextFile(userDir + "/prev-pages.txt", &pPages);

      }
    }
  }
  if(target == tbGetPagerID)
  {
    if(event->type() == QEvent::KeyPress)					// make sure it is a keypress event
    {
      int key = static_cast<QKeyEvent*>(event)->key();
      if(key == Qt::Key_Return)                             // So the user can just press enter on the prev pages list
      {
        getPagerIDClicked();
        return(true);
      }
    }
  }
  if(target == pbSend)
  {
    if(event->type() == QEvent::KeyPress)					// make sure it is a keypress event
    {
      int key = static_cast<QKeyEvent*>(event)->key();
      if(key == Qt::Key_Return)                             // So the user can just press enter on the prev pages list
      {
        sendSinglePage();
        return(true);
      }
    }
  }
  return(false);
}

void MainWin::applySettings()
{
  if(leIdleCW->text().length() == 10)
  {
    bool ok;
    idleCW = leIdleCW->text().toInt(&ok,16);
  }
  else
    QMessageBox::critical(0, "ERROR", "The idle codeword must be 8 hex digits preceded with 0x.");
  if(leSyncCW->text().length() == 10)
  {
    bool ok;
    syncCW = leSyncCW->text().toInt(&ok,16);
  }
  else
    QMessageBox::critical(0, "ERROR", "The sync codeword must be 8 hex digits preceded with 0x.");
  txRotate = 0;
  QStringList txo = leTxOrder->text().split(",");
  if(txo.count() < 8)
    QMessageBox::critical(0, "ERROR", "You MUST enter 8 transmitter numbers separated by commas.");
  else
  {
    for(int x =0; x < txo.count(); x++)
      txOrder[x] = txo.at(x).toInt();
  }
  baudRotate = 0;
  QStringList bo = leBaudOrder->text().split(",");
  if(txo.count() < 8)
    QMessageBox::critical(0, "ERROR", "You MUST enter 6 baud rates separated by commas.");
  else
  {
    for(int x =0; x < bo.count(); x++)
      baudOrder[x] = bo.at(x).toInt();
  }
  QStringList ni = leNormInv->text().split(",");
  if(ni.count() < 8)
    QMessageBox::critical(0, "ERROR", "You MUST enter 8 0 and/or 1 separated by commas for the keying.");
  else
  {
    for(int x =0; x < ni.count(); x++)
      normInv[x] = ni.at(x).toInt();
  }
  QStringList kd = leKeyDelay->text().split(",");
  if(kd.count() < 8)
    QMessageBox::critical(0, "ERROR", "You MUST enter 8 values between 0 and 999 separated by commas for the key delay.");
  else
  {
    for(int x =0; x < kd.count(); x++)
      keyDelay[x] = kd.at(x).toInt();
  }
  if(rbSop->isChecked())
  {
    gpioPin[0] = 17;
    gpioPin[1] = 18;
    gpioPin[2] = 21;
    gpioPin[3] = 22;
    gpioPin[4] = 23;
    gpioPin[5] = 24;
    gpioPin[6] = 25;
    gpioPin[7] = 4;
    for(int x = 0; x < 8; x++)  // Set up the keying pins and de-key each transmitter
    {
      QProcess::startDetached("gpio export " + QSn(gpioPin[x]) + " out");
      QProcess::startDetached("gpio -g mode " + QSn(gpioPin[x]) + " out");
      QProcess::startDetached("gpio -g write " + QSn(gpioPin[x]) + " " + QSn(!normInv[x]));
    }
  }
  if(rbPic->isChecked())
  {

  }
  writeSettings();
}

//=========================================================================================================================================

void MainWin::writeSettings()  // Write the users settings to a config file.
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,"CLPC-Software", "gascop");
  settings.setValue("left", formleft);
  settings.setValue("top", formtop);
  settings.setValue("height", height());
  settings.setValue("width", width());
  settings.setValue("synccw", leSyncCW->text());
  settings.setValue("idlecw", leIdleCW->text());
  settings.setValue("baudorder", leBaudOrder->text());
  settings.setValue("txorder", leTxOrder->text());
  settings.setValue("numeric_end_value", numeric_end_value);
  settings.setValue("pocsaglevel", pocsagLevel->value());
  settings.setValue("tb2400", tb2400->isChecked());
  settings.setValue("tb1200", tb1200->isChecked());
  settings.setValue("tb512", tb512->isChecked());
  settings.setValue("tbFuncNumeric", tbFuncNumeric->isChecked());
  settings.setValue("rbt1", tbFuncTone1->isChecked());
  settings.setValue("rbt2", tbFuncTone2->isChecked());
  settings.setValue("tbFuncAlpha", tbFuncAlpha->isChecked());
  settings.setValue("funcbits", sspFuncBits);
  settings.setValue("baud", sspBaud);
  settings.setValue("tbnumeric", tbNumeric->isChecked());
  settings.setValue("tbinverted", tbInverted->isChecked());
  settings.setValue("lestartric", leStartRic->text());
  settings.setValue("sbamtrics", sbAmtRics->value());
  settings.setValue("norminv", leNormInv->text());
  settings.setValue("keydelay", leKeyDelay->text());
  settings.setValue("picinterface", rbPic->isChecked());
  settings.setValue("sopinterface", rbSop->isChecked());
}

void MainWin::readSettings() // Read the users settings from the config file.
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,"CLPC-Software", "gascop");
  move(settings.value("left", 100).toInt(), settings.value("top", 100).toInt());
  resize(settings.value("width", 550).toInt(), settings.value("height", 450).toInt()); // CHANGE FORM SIZE HERE
  bool ok;
  leSyncCW->setText(settings.value("synccw", "0x7cd215d8").toString());
  leIdleCW->setText(settings.value("idlecw", "0x7a89c197").toString());
  syncCW = leSyncCW->text().toInt(&ok,16);
  idleCW = leIdleCW->text().toInt(&ok,16);
  leBaudOrder->setText(settings.value("baudorder", "1200,1200,1200,512,1200,2400").toString());
  leTxOrder->setText(settings.value("txorder", "1,2,3,4,5,6,7,8").toString());
  numeric_end_value = settings.value("numeric_end_value", 0x0C).toInt();
  pocsagLevel->setValue(settings.value("pocsaglevel", "120").toInt());
  tb2400->setChecked(settings.value("tb2400", false).toBool());
  tb1200->setChecked(settings.value("tb1200", true).toBool());
  tb512->setChecked(settings.value("tb512", false).toBool());
  tbFuncNumeric->setChecked(settings.value("tbFuncNumeric", false).toBool());
  tbFuncTone1->setChecked(settings.value("rbt1", false).toBool());
  tbFuncTone2->setChecked(settings.value("rbt2", false).toBool());
  tbFuncAlpha->setChecked(settings.value("tbFuncAlpha", true).toBool());
  sspFuncBits = settings.value("funcbits", 3).toInt();
  sspBaud = settings.value("baud", 1200).toInt();
  tbNumeric->setChecked(settings.value("tbnumeric", false).toBool());
  tbInverted->setChecked(settings.value("tbinverted", false).toBool());
  leStartRic->setText(settings.value("lestartric", "1000000").toString());
  sbAmtRics->setValue(settings.value("sbamtrics", 512).toInt());
  leNormInv->setText(settings.value("norminv", "1,1,1,1,1,1,1,1").toString());
  leKeyDelay->setText(settings.value("keydelay", "100,100,100,100,100,100,100,100").toString());
  rbPic->setChecked(settings.value("picinterface", true).toBool());
  rbSop->setChecked(settings.value("sopinterface", false).toBool());
}

/*
The canonical WAVE format starts with the RIFF header:

 Offset  Length   Contents
 0     4 bytes  'RIFF'
 4     4 bytes  <file length - 8>
 8     4 bytes  'WAVE'
(The '8' in the second entry is the length of the first two entries. I.e., the second entry is the number of bytes that follow in the file.)

Next, the fmt chunk describes the sample format:

 12    4 bytes  'fmt '
 16    4 bytes  0x00000010   // Length of the fmt data (16 bytes)
 20    2 bytes  0x0001     // Format tag: 1 = PCM
 22    2 bytes  <channels>   // Channels: 1 = mono, 2 = stereo
 24    4 bytes  <sample rate>  // Samples per second: e.g., 44100
 28    4 bytes  <bytes/second> // sample rate * block align
 32    2 bytes  <block align>  // channels * bits/sample / 8
 34    2 bytes  <bits/sample>  // 8 or 16
Finally, the data chunk contains the sample data:

 36    4 bytes  'data'
 40    4 bytes  <length of the data block>
 44    bytes  <sample data>
The sample data must end on an even byte boundary.
All numeric data fields are in the Intel format of low-high byte ordering.
8-bit samples are stored as unsigned bytes, ranging from 0 to 255. 16-bit
samples are stored as 2's-complement signed integers, ranging from -32768 to 32767.

For multi-channel data, samples are interleaved between channels, like this:

sample 0 for channel 0
sample 0 for channel 1
sample 1 for channel 0
sample 1 for channel 1
...
For stereo audio, channel 0 is the left channel and channel 1 is the right.

// amt of codewords * 4 = bytes * 8 = bits * changePoint = total bits required for wav file.
// THE HEADER MUST BE 38400 IT IS THE ONLY VALUE THAT 512 1200 AND 2400 ALL DIVIDE INTO EQUALLY!!!!!!!!!!!!!
// R I F F datalen+36W A V E f m t 8bit data PCM mono22050sr 22050 blk_a bps d a t a datalen
//==========================16 bit signed header=========================
  quint32 wavHeader[11] = {0x52494646, 0x00000000, 0x57415645, 0x666D7420,
                           0x10000000, 0x01000100, 0x00960000, 0x002C0100,
                           0x02001000, 0x64617461, 0x00000000};


//===================== 8 bit unsigned header ===========================
//  quint32 wavHeader[11] = {0x52494646, 0x00000000, 0x57415645, 0x666D7420,
//                           0x10000000, 0x01000100, 0x00960000, 0x00960000,
//                           0x01000800, 0x64617461, 0x00000000};       // A Basic 8 bit wave header @ 22050

//==========================32 bit signed header=========================
// quint32 wavHeader[11] = {0x52494646, 0x00000000, 0x57415645, 0x666D7420,
//        0x10000000, 0x01000100, 0x22560000, 0x88580100,
//        0x04002000, 0x64617461, 0x00000000};

*/

//
