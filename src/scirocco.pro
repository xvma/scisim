QT += sql core gui widgets

HEADERS       = mainwindow.h \
    invdatabase.h \ 
    winder.h \
    spooler.h \
    doffer.h \
    anim.h \
    sleever.h \
    man.h \
    locator.h \
    logger.h \
    supervisor.h
SOURCES       = mainwindow.cpp \
                main.cpp \
    invdatabase.cpp \ 
    winder.cpp \
    spooler.cpp \
    doffer.cpp \
    anim.cpp \
    sleever.cpp \
    man.cpp \
    locator.cpp \
    logger.cpp \
    supervisor.cpp

# install
#target.path = $$[QT_INSTALL_EXAMPLES]/widgets/mainwindows/menus
#INSTALLS += target

RESOURCES += \
    scirocco.qrc
