#ifndef SCAN_H
#define SCAN_H
#include <QThread>
#include <QString>
#include "animax.pb.h"
#include <google/protobuf/text_format.h>
#include <QVector>
#include <structs.h>

//typedef QVector<QVector<uint32_t>> imagepixeldata;

Q_DECLARE_METATYPE(std::string)

class scan : public QThread
{
    Q_OBJECT

signals:
   void sendDeviceStatusToGUI(QString, QString);
   void sendPreviewDataToGUI(std::string, std::string);
   void sendROIDataToGUI(std::string, std::string);
   void sendScanFinished();

public:
    // constructor
    // set name using initializer
    explicit scan(settingsdata s);

    // overriding the QThread's run() method
    void run();
    bool stop = false;
    settingsdata settings;
    int acquisition_number = 1;
    std::string scannote = "";
    bool stopscan = false;
    bool pausescan = false;
    bool resumescan = false;

private:
    //QString ip;
};

#endif // SCAN_H
