/*
 *  clpc library of functions
 *
 *  Copyright (C) 2009 Clive Cooper - <clive@clivecooper.co.uk>
 *
 *  http://clivecooper.co.uk/index.html
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 */

#include <QtGui>
#include <QFile>
#include <time.h>
#include "clpclib.h"

/*
  QTcpSocket * senderSkt = qobject_cast<QTcpSocket *>(sender());  // How to grab the sender from inside a slot, not the proper way to do things, so I don't use this but maybe one day I might
  QCoreApplication::processEvents(QEventLoop::AllEvents);         // How to sit in a loop for a short period without the GUI becoming frozen
*/

uchar reverseUchar(uchar rm)
{
  uchar res = 0x00;
  for(int x = 1; x < 256; x *= 2)
  {
    res = res << 1;
    if(rm & x)
      res += 1;
  }
  return(res);                           // Return the reverse byte
}

QChar reverseQChar(QChar rm)
{
  uchar res = 0x00;
  for(int x = 1; x < 256; x *= 2)
  {
    res = res << 1;
    if(rm.cell() & x)
      res += 1;
  }
  return(QChar(res));                           // Return the reverse byte
}

// use like this  p dbiStr(var)  in debugging
void dbiStr(QString * var)
{
  // You can get SIGSEGV      11       Core    Invalid memory reference here with no way of trying and catching
  qDebug() << *var;
}

void dbiStrList(QStringList var, int index)
{
  if(index)
    qDebug() << var[index];
  else
    qDebug() << var;
}

// Get UNIX time as a unsigned long integer (should work on all platforms)
uint timeToInt()
{
  return(QDateTime::currentDateTime().toTime_t());
}

// convert the unix time to a date and time string
QString intToTime(uint secs)
{
  QDateTime dt;
  return(dt.fromTime_t(secs).toString("yyyy/MM/dd hh:mm:ss"));
}

// Returns the unix time integer as a string that is padded to be 'len' chars long
QString timeToStr(quint8 len)
{
  return(padFront(QString::number(timeToInt()), "0", len));
}

// I know hardly worth the trouble creating a function for this
void makeDir(QString dir)
{
  QDir md;
  md.mkpath(dir);
}

// puts commas in a number returning it as a string, so 1234567890 becomes 1,234,567,890
QString commaNumber(quint64 num)
{
  QString st = QString::number(num);
  for(int x = st.length() - 3; x > 0; x -= 3)
    st.insert(x, ",");
  return(st);
}

// pad a string on the front with a char, so you can turn 1234 into 0000001234
QString padFront(QString str, QString padChar, int size)
{
  if(str.length() >= size)
    return(str.left(size));
  while(str.length() < size)
    str = padChar + str;
  return(str);
}

// Use this to create a directory in the users HOME directory
void createUserDirectory(QString dirName)
{
  if(dirName.length() > 0)
  {
    QDir md;
    if(md.exists(QDir::homePath() + "/" + dirName) == false)  // if false then this is the first time Mencofe has been run
    {
      md.mkpath(QDir::homePath() + "/" + dirName);
    }
    return;
  }
  return;
}

// Extracts the path from a full path/filename string
QString extractFilePath(QString fn)
{
  if(fn == "")
    return("");
  if(fn.indexOf("/") < 0)
    return(fn);
  while(fn.right(1) != "/")  // strip the filename down to just a path
  {
    fn = fn.left(fn.size() - 1);
  }
  return(fn);
}

// extracts and returns the filename only from a full path/filename
QString extractFileName(QString fn)
{
  if(fn == "")
    return("");
  if(fn.indexOf("/") < 0)
    return(fn);
  int t = fn.lastIndexOf("/");
  fn = fn.mid(t+1);
  return(fn);
}

// changes the file extension you must supply a full extension including the period i.e. .zip
QString changeFileExt(QString fn, QString newExt)
{
  if(fn == "")
    return("");
  if(fn.lastIndexOf(".") > 0)
  {
    fn = fn.left(fn.lastIndexOf("."));
  }
  fn = fn + newExt;
  return(fn);
}

// returns the file extension including the period
QString extractFileExt(QString fn)
{
  if(fn == "")
    return("");
  if(fn.lastIndexOf(".") <= 0)
    return("");
  fn = fn.mid(fn.lastIndexOf("."));
  return(fn);
}

// returns the file name without the extension or period
QString extractFilePart(QString fn)
{
  if(fn == "")
    return("");
  fn = fn.left(fn.lastIndexOf("."));
  return(fn);
}

// returns the name of the immediate parent of a path with filename
QString extractParentDir(QString fn)
{
  if(fn == "")
    return("");
  fn = extractFilePath(fn);
  if(fn == "/")
    return("");
  fn = fn.left(fn.size() - 1);
  fn = fn.mid(fn.lastIndexOf("/") + 1);
  return(fn);
}

// Reads a text file line by line into a string list supplied by the caller
bool readTextFile(QString fn, QStringList* sl)
{
  bool res;
  res = false;
  sl->clear();
  QFile file(fn);
  if(file.exists())
  {
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
//      QMessageBox::critical(0, "PROBLEM", "Cannot Open The Text File");
      return(res);
    }
    QTextStream lineIn(&file);
    while (!lineIn.atEnd())
      {
        QString line = lineIn.readLine();
        sl->append(line);
      }
    file.close();
    res = true;
  }
  return(res);
}

// Writes the contents of a string list to a file adding a \n at the end of each line
bool writeTextFile(QString fn, QStringList* sl)
{
  bool res;
  res = false;
  QFile file(fn);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
//    QMessageBox::critical(0, "PROBLEM", "Cannot Create The Text File");
    return(res);
  }
  QTextStream out(&file);
  for(int x = 0; x < sl->count(); x++)
  {
    QString remindLine = sl->at(x) + "\n";
    out << remindLine;
  }
  file.close();
  res = true;
  return(res);
}

// Append the contents of a string list to a file adding a \n at the end of each line
bool appendTextFile(QString fn, QStringList* sl)
{
  bool res;
  res = false;
  if(sl->count() > 0)
  {
    QFile file(fn);
    if(!file.open(QIODevice::Append | QIODevice::Text))
    {
//      QMessageBox::critical(0, "PROBLEM", "Cannot Create The Text File");
      return(res);
    }
    QTextStream out(&file);
    for(int x = 0; x < sl->count(); x++)
    {
      QString remindLine = sl->at(x) + "\n";
      out << remindLine;
    }
    file.close();
    res = true;
  }
  return(res);
}

// Reads a binary file into a bytearray supplied by the caller
bool readBinFile(QString fn, QByteArray * ds)
{
  QFile file(fn);
  if(file.exists())
  {
    if (!file.open(QIODevice::ReadOnly))
      return(false);
    *ds = file.readAll();
    file.close();
  }
  return(true);
}

// Writes a bytearray to a file
bool writeBinFile(QString fn, QByteArray * ds)
{
  QFile file(fn);
  if(!file.open(QIODevice::WriteOnly))
    return(false);
  file.write(*ds);
  file.close();
  return(true);
}

// Returns the file size for the given filename or -1 if the file does not exist
int getFileSize(QString fn)
{
  QFile file(fn);
  if(file.exists())
    return(file.size());
  else
    return(-1);
}
// This will return the filename given unless it exists and then it returns the filename appened with Copy#n
QString createSafeFileName(QString fn)
{
  int fnum = 1;
  QString fpath = extractFilePath(fn);
  QString fOnly = extractFileName(fn);
  fOnly = extractFilePart(fOnly);
  QString fext = extractFileExt(fn);
  while(QFile::exists(fn))
  {
    fn = fpath + fOnly + "-Copy#" + QString::number(fnum) + fext;
    fnum++;
  }
  return(fn);
}

// Create a file of 'size' bytes with each byte set to zero
QString createEmptyFile(QString fn, int size)
{
  QFile file(fn);
  if(!file.open(QIODevice::WriteOnly))
  {
//    QMessageBox::critical(0, "PROBLEM", "Cannot create the binary file");
    return("");
  }
  file.resize(size);
  file.close();
  return(fn);
}

// Dencode a byte array against a key of any length
QByteArray encodeBuffer(QByteArray din, int howMany, QByteArray key)
{
  int y = 0;
  for(int x = 0; x < howMany; x ++)
  {
    din[x] = din[x] ^ key[y];
    y++;
    if(y >= key.size())
      y = 0;
  }
  return(din);
}

// This Encrypts and Decrypts a string
QString dencrypt(QString dataIn, QString key)
{
  int x, y, t;
  char a, b;
  QByteArray u, v, m;
  u = dataIn.toUtf8();
  v = key.toUtf8();
  t = v.size() - 1;
  y = 0;
  for(x = 0; x < dataIn.size(); x++)
  {
    a = (u[x] & 0x0f) ^ (v[y] & 0x0f);
    b = (u[x] & 0xf0) + a;
    u[x] = b;
    y++;
    if (y > t)
      y = 0;
  }
  return QString(u);
}
