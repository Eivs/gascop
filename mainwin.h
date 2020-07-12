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
 
#ifndef MAINWIN_H
#define MAINWIN_H

#include "ui_mainwin.h"
#include <QProcess>
#include <QTimer>
#include <QBitArray>

#include "clpclib.h"
#include "pagerid.h"

//#include "SDL/SDL_getenv.h"
#include "SDL/SDL.h"
#include "SDL/SDL_audio.h"
#include "SDL/SDL_mixer.h"

#define NUM_SOUNDS 1

class GasCopServer;

class MainWin : public QMainWindow, public Ui::MainWin
{
Q_OBJECT
  public:
    MainWin( QWidget * parent = 0, Qt::WFlags f = 0 );

    bool startRunning;

  protected:
    void showEvent(QShowEvent *event);
    void closeEvent(QCloseEvent *event);
    void moveEvent(QMoveEvent *);
    bool eventFilter(QObject * target, QEvent * event);

  private slots:
    void txLengthTimeout();
    void sendSinglePage();
    void displayMessages(QString msg);
    void currentTabChanged(int tabIndex);
    void prevPagesDoubleClicked(QListWidgetItem * item);
    void tuneupTransmit();
    void tuneupStop();
    void ricSearch();
    void ricSearchAbort();
    void logSave();
    void logReload();
    void numericClicked();
    void dataLevelChanged(int value);
    void watchdogTimeout();
    void getPagerIDClicked();
    void msgTextChanged();
    void periodIndexChanged(QString);
    void pagerIDsRowChanged(int index);
    void pagesRowChanged(int index);
    void pagerIDsChange();
    void pagerIDstextChanged(QString);
    void pagerIDsvalChanged(int);
    void pagerIDsAddEntry();
    void pagerIDsUpdateEntry();
    void pagerIDsClearAll();
    void pagerIDsDeleteEntry();
    void pagesDeleteEntry();
    void resetTimeClicked();
    void applySettings();
    void keyTxClicked();

  private:
    Pagerid * pagerid;
    QList<quint32> msgcw;
    QTimer * txLength;
    QTimer * watchdog;
    QString userPath;
    int currentBaudRate;
    QString msgToSend;
    int sspFuncBits, sspBaud, sspFuncBits2, sspBaud2, sspFuncBits3, sspBaud3;
    QStringList pPages;             // Previous pages list
    QString userDir;                // Points to /home/<user>/.gascop
    QStringList * gascopdb;         // Main pages database list
    QStringList * pagerIDdb;        // Main Pager IDs database list
    uint pagesDbLastModified;
    bool isAutoPagingActive;
    bool txComplete;
    bool testMode;
    int currentTx;
    quint32 idleCW, syncCW, preambleCW;

    Uint8 * pocsagBuffer;
    bool isRicSearching;

    int audio_rate;                                                            // Frequency of audio playback
    Uint16 audio_format;                                                       // Format of the audio we're playing
    int audio_channels;                                                        // 2 channels = stereo
    int audio_buffers;                                                         // Size of the audio buffers in memory
    int waveFileDataLength;
    int pDB;
    int pocsagChannel;
    int maxMsgLen;
    int baudRotate;
    int baudOrder[6];
    int txOrder[8];
    int txRotate;
    int gpioPin[8];
    int keyDelay[8];
    int normInv[8];

    int formleft, formtop;
    quint8 numeric_end_value;   // This is the BCD value in the numeric message that defines the end of the message
    bool running;
    bool singlePage;

    QVector<quint32> pd;

    struct pg
    {
      int flag;                 // 1 digit
      uint sendTime;            // 10 digits
      int spare;                // 4 digits
      int schedule;             // 3 digits
      int howMany;              // 2 digits
      int tx;                   // 1 digit
      int baud;                 // 4 digits
      int numeric;              // 1 digit
      int funcBits;             // 1 digit
      int ric;                  // 7 digit
      QString msg;              // variable length string
    };
    QVector<pg> dbPages;
    pg dbPage;

    struct pid
    {
      QString name;
      int ric;
      int funcBits;
      int numeric;
      int baud;
      int tx;
      QString fx;
      int msgLen;
    };
    QVector<pid> dbPagerIDs;
    pid dbPagerID;

    struct sd
    {
      QString ric;
      QString msg;
      int funcBits;
      int numeric;
      int ricPos;
    };
    QVector<sd> txSendPages;
    sd txSendPage;

    void setRepeatTime();
    void pagerIDsEditorToStruct();
    uint getLastModified(QString file);
    void checkForPagesToSend();
    void playWaveData();
    void writeToPagesDatabase();
    void writeToPagerIDDatabase(int index);
    void pagesListToEditor(int index);
    void pagerIDsListToEditor(int index);
    void populatePagerIDsEditor();
    void populatePagerMsgsEditor();
    void setFileLock(QString filename);
    void getPagerIDElements();
    void getPagesElements();
    void loadPrevPages();
    void setFunctionBits();
    void setBaudRate();
    void setFunctionBits2();
    void setBaudRate2();
    void setFunctionBits3();
    void setBaudRate3();
    int getFunctionBits2();
    int getBaudRate2();
    int getFunctionBits();
    int getBaudRate();
    int createWaveHeader(int changePoint);
    int createWaveData(int r, int changePoint);
    void deleteSentPages();
    void resetSentPages();
    void switchWidgets(bool sspOn);
    void savePreviousPages(QString ric, QString msg);
    void displayPocsagData(QListWidget * lw);
    void openDB();
    void sendOnePage(quint32 ric, quint8 funcBits, QString message, quint16 baud, bool numeric);
    void formatPocsagData();
    void stackPages();
    void createPocsagWaveFile(int size);
    void buildPocsagWaveData(QList<quint32> * cw, int baud);
    void encodeAlphaCW(QString msg, QList<quint32> * cwc);
    void encodeNumericCW(QString msg, QList<quint32> * cwc);
    QString alphaCWToStr(QList<quint32> * cwc);
    QString numericCWToStr(QList<quint32> * cw);
    quint32 addrCwToRic(quint32 cw);
    quint32 setParity(quint32 cw);
    void build_amsg(char *dat1, int cwcount);
    quint32 getRicCode(quint32 ric, quint8 func);
    quint32 buildCW(quint32 cw, quint32 cb);
    quint32 genChkBits(quint32 cw);
    int getCW_A(QString msg);
    quint32 getCW_N(QString msg, quint32 index);
    void writeSettings();
    void readSettings();
};
#endif
