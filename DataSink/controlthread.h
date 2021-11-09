#ifndef CONTROLTHREAD_H
#define CONTROLTHREAD_H
#include <QThread>
#include <QString>
#include <google/protobuf/text_format.h>
#include <QVector>
#include <animax.pb.h>
#include <structs.h>
#include <QDateTime>
#include <zmqthread.h>
#include <sddthread.h>

typedef QVector<QVector<uint32_t>> imagepixeldata;
typedef QVector<uint32_t> imagepreviewdata;

class controlThread : public QThread
{
    Q_OBJECT

signals:
    //void sendBeamlineDataToGUI(float, float, float);
    void sendSettingsToGUI(settingsdata);
    void sendMetadataToGUI(metadata);

public:
    // constructor
    // set name using initializer
    explicit controlThread();

    // overriding the QThread's run() method
    void run();
    void setCurrentSTXMPreview(imagepreviewdata);
    void setCurrentCCDImage(std::string);
    void setCurrentROIs(roidata);
    bool stop = false;
    bool ccdReady = false;
    bool sddReady = false;

private:
    QString hdf5filename;
    zmqThread* ccd;
    sddThread* sdd;
    imagepreviewdata stxmpreview;
    roidata ROIdata;
    bool newSTXMpreview = false;
    std::string ccdpreview;
    bool newCCDpreview = false;
    bool newROIs = false;

private slots:
};

#endif // CONTROLTHREAD_H
