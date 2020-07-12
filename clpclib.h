#ifndef CLPCLIB_H
#define CLPCLIB_H
#include <QString>
#include <QFile>
#include <QDateTime>

//<FUNCTIONS>
  uchar reverseUchar(uchar rm);                         // Reverse the bits in a uchar i.e. (0xEC decimal 236) will become (0x37 decimal 55)
  QChar reverseQChar(QChar rm);                         // Reverse the bits in a QChar i.e. (0xEC decimal 236) will become (0x37 decimal 55)
  void dbiStr(QString * var);
  void dbiStrList(QStringList var, int index);
  bool readTextFile(QString fn, QStringList* sl);       // Reads a text file line by line into a string list supplied by the caller
  bool writeTextFile(QString fn, QStringList* sl);      // Writes the contents of a string list to a file adding a \n at the end of each line
  bool appendTextFile(QString fn, QStringList* sl);     // Append the contents of a string list to a file adding a \n at the end of each line
  bool writeBinFile(QString fn, QByteArray * ds);       // Writes a bytearray to a file
  bool readBinFile(QString fn, QByteArray * ds);        // Reads a binary file into a bytearray supplied by the caller
  QString dencrypt(QString dataIn, QString key);        // This Encrypts and Decrypts a string
  uint timeToInt();                                     // Get UNIX time as a long integer
  QString intToTime(uint secs);
  QString timeToStr(quint8 len);                        // Returns the unix time as a string that is padded to be 'len' chars long
  void makeDir(QString dir);
  QString commaNumber(quint64 num);
  QString extractFilePath(QString fn);                  // Extracts the path from a full path/filename string
  QString extractFileName(QString fn);                  // extracts and returns the filename only from a full path/filename
  QString changeFileExt(QString fn, QString newExt);    // changes the file extension you must supply a full extension including the period i.e. .zip
  QString extractFileExt(QString fn);                   // returns the file extension including the period
  QString extractFilePart(QString fn);                  // returns the file name without the extension or period
  QString extractParentDir(QString fn);                 // returns the name of the immediate parent of a path with filename
  void createUserDirectory(QString dirName);            // Use this to create a directory in the users HOME directory
  QString createEmptyFile(QString fn, int size);        // Create a file of 'size' bytes with each byte set to zero
  QString createSafeFileName(QString fn);               // This will return the filename given unless it exists and then it returns the filename appened with Copy#n
  QByteArray encodeBuffer(QByteArray din, int howMany, QByteArray key); // Dencode a byte array against a key of any length
  int getFileSize(QString fn);                          // Returns the file size for the given filename or -1 if the file does not exist
  QString padFront(QString str, QString padChar, int size); // Pad the front of a string out with 'padChar' until it is size long. Note: truncates if larger than size!!!!
//</FUNCTIONS>
#endif // CLPCLIB_H
