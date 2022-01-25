#ifndef SDDTHREAD_H
#define SDDTHREAD_H
#include <QThread>
#include <QString>
#include "animax.pb.h"
#include <google/protobuf/text_format.h>
#include <QVector>
#include <QMap>

typedef QVector<uint32_t> spectrumdata;
typedef QMap<std::string, QVector<uint32_t>> roidata;

class sddThread : public QThread
{
    Q_OBJECT

signals:
    void sendSpectrumDataToGUI(int, spectrumdata);
    void sendScanIndexDataToGUI(long, int, int);
    void sendLineBreakDataToGUI(roidata, long, int, int);
    void sendDeviceStatus(QString, QString);
    void sddReady();

public:
    // constructor
    // set name using initializer
    explicit sddThread(QString s, QString roijson, int scanX, int scanY);

    // overriding the QThread's run() method
    void run();
    bool stop = false;
    bool sddReadyState = false;
    bool dummybool = false;
private:
    QString ip;
    QVector<QVector<uint32_t>> ROIs;
    roidata ROImap;
    QString roijson;
    int scanX;
    int scanY;

    QVector<std::string> elements;

    QVector<uint32_t> ROI_P;
    QVector<uint32_t> ROI_Fe;

};

#endif // SDDTHREAD_H
