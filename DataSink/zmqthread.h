#ifndef ZMQTHREAD_H
#define ZMQTHREAD_H
#include <QThread>
#include <QString>
#include "animax.pb.h"
#include <google/protobuf/text_format.h>
#include <QVector>

typedef QVector<QVector<uint32_t>> imagepixeldata;

Q_DECLARE_METATYPE(std::string)

class zmqThread : public QThread
{
    Q_OBJECT



signals:
    void sendImageData(int, std::string);
    void sendCCDSettings(int, int);
    void ccdReady();

public:
    // constructor
    // set name using initializer
    explicit zmqThread(QString s, int scanX, int scanY);

    // overriding the QThread's run() method
    void run();
    bool stop = false;
    bool ccdReadyState = false;
private:
    QString ip;
    int scanX;
    int scanY;

};

#endif // ZMQTHREAD_H
