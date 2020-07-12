TEMPLATE += 		app
QT += 					gui \
								core \
  							network

CONFIG += 			qt

LIBS += 				-lSDL \
								-lSDL_mixer

RESOURCES +=   	mainwin.qrc

FORMS += 				mainwin.ui \
                pagerid.ui

HEADERS += 			mainwin.h \
								clpclib.h \
                pagerid.h

SOURCES += 			mainwin.cpp \
								clpclib.cpp \
								main.cpp \
                pagerid.cpp







