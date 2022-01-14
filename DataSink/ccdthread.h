#ifndef CCDTHREAD_H
#define CCDTHREAD_H
#include <QThread>
#include <QString>
#include "animax.pb.h"
#include <google/protobuf/text_format.h>
#include <QVector>
#include <structs.h>

typedef QVector<QVector<uint32_t>> imagepixeldata;

Q_DECLARE_METATYPE(std::string);
Q_DECLARE_METATYPE(ccdsettings);

class ccdThread : public QThread
{
    Q_OBJECT

signals:
    void sendImageData(int, std::string);
    void sendCCDSettings(ccdsettings);
    void ccdReady();

public:
    // constructor
    // set name using initializer
    explicit ccdThread(QString s, int scanX, int scanY);

    // overriding the QThread's run() method
    void run();
    bool stop = false;
    bool ccdReadyState = false;
private:
    QString ip;
    int scanX;
    int scanY;

};

#endif // CCDTHREAD_H
