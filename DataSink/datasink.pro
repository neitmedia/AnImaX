QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += c++11 console

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    animax.pb.cc \
    ccdthread.cpp \
    controlthread.cpp \
    hdf5nexus.cpp \
    main.cpp \
    mainwindow.cpp \
    sddthread.cpp

HEADERS += \
    animax.pb.h \
    ccdthread.h \
    controlthread.h \
    hdf5nexus.h \
    mainwindow.h \
    sddthread.h \
    structs.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += -L/usr/lib /usr/lib/libhdf5_hl_cpp.so /usr/lib/libhdf5_cpp.so /usr/lib/libhdf5_hl.so /usr/lib/libhdf5.so -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now -lsz -lz -ldl -lm -lzmq -lprotobuf -Wl,-rpath -Wl,/usr/lib
