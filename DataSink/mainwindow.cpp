#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include <QFileDevice>
#include <QImage>
#include <iostream>
#include <string>
#include <math.h>
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <random>
#include <structs.h>
#include <QDateTime>
#include <QSettings>

#include "H5Cpp.h"
using namespace H5;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qRegisterMetaType<imagepixeldata>( "imagepixeldata" );
    qRegisterMetaType<spectrumdata>( "spectrumdata" );
    qRegisterMetaType<settingsdata>( "settingsdata" );
    qRegisterMetaType<metadata>( "metadata" );
    qRegisterMetaType<std::string>();
    qRegisterMetaType<imagepreviewdata>( "imagepreviewdata" );
    qRegisterMetaType<roidata>( "roidata" );
    hdf5filename = "measurement_"+QString::number(QDateTime::currentMSecsSinceEpoch())+".h5";
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showEvent( QShowEvent* event ) {
    QWidget::showEvent( event );
    // get IP settings from ini file
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnImaX", "DataSink");

    settings.beginGroup("Network");

    QString guiIP = settings.value("guiIP").toString();
    QString guiPort = settings.value("guiPort").toString();

    settings.endGroup();

    std::cout<<"GUI IP: "<<guiIP.toStdString()<<" GUI PORT: "<<guiPort.toStdString()<<std::endl;

    if ((guiIP == "") || (guiPort == "")) {
        // write standard IP settings to ini file
        guiIP = "127.0.0.1";
        guiPort = "5555";
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnImaX", "DataSink");
        settings.beginGroup("Network");
        settings.setValue("guiIP", guiIP);
        settings.setValue("guiPort", guiPort);
        settings.endGroup();
    }

    // launch control thread
    controlth = new controlThread(guiIP+':'+guiPort);
    const bool connected = connect(controlth, SIGNAL(sendSettingsToGUI(settingsdata)),this,SLOT(getScanSettings(settingsdata)));
    const bool connected2 = connect(controlth, SIGNAL(sendMetadataToGUI(metadata)),this,SLOT(getMetadata(metadata)));

    controlth->start();

}

void MainWindow::getMetadata(metadata metadata) {
    currentmetadata = metadata;

    std::cout<<"gui got metadata - "<<metadata.aquisition_number;

    /* WRITE BEAMLINE PARAMETER TO FILE */
    hsize_t dimsext[2] = {1, 1}; // extend dimensions

    DataSet *dataset = new DataSet(nexusfile->file->openDataSet("/measurement/metadata/aquisition_number"));

    hsize_t offset[2];

    H5::ArrayType arrtype = dataset->getArrayType();

    // Select a hyperslab in extended portion of the dataset.
    DataSpace *filespace = new DataSpace(dataset->getSpace());
    hsize_t size[2];
    int n_dims = filespace->getSimpleExtentDims(size);

    offset[0] = size[0];
    offset[1] = 0;

    size[0]++;
    size[1] = 1;

    dataset->extend(size);

    // Select a hyperslab in extended portion of the dataset.
    DataSpace *filespacenew = new DataSpace(dataset->getSpace());

    filespacenew->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

    // Define memory space.
    DataSpace *memspacenew = new DataSpace(2, dimsext, NULL);

    int aquisition_number = metadata.aquisition_number;

    // Write data to the extended portion of the dataset.
    dataset->write(&aquisition_number, PredType::STD_I32LE, *memspacenew, *filespacenew);

    dataset->close();
    delete dataset;
    filespace->close();
    delete filespace;
    filespacenew->close();
    delete filespacenew;
    memspacenew->close();
    delete memspacenew;
}

void MainWindow::getScanSettings(settingsdata settings) {
    std::cout<<"gui knows settings:"<<std::endl;
    scansettings = settings;

    ccdX = scansettings.ccdWidth;
    ccdY = scansettings.ccdHeight;

    // allocate memory for transmission preview image
    stxmimage = (uint32_t*) malloc(ccdX*ccdY*sizeof(uint32_t));

    scanX = scansettings.scanWidth;
    scanY = scansettings.scanHeight;

    std::cout<<"ccd ip:port "<<scansettings.ccdIP+':'+std::to_string(scansettings.ccdPort)<<std::endl;
    std::cout<<"sdd ip:port "<<scansettings.sddIP+':'+std::to_string(scansettings.sddPort)<<std::endl;

    // create HDF5/NeXus file
    hdf5filename = "measurement_testmessung_1_"+QString::number(QDateTime::currentMSecsSinceEpoch())+".h5";
    nexusfile = new hdf5nexus();
    nexusfile->createDataFile(hdf5filename, scansettings);

    // start ccd thread
    ccd = new zmqThread(QString::fromStdString(scansettings.ccdIP+':'+std::to_string(scansettings.ccdPort)), scanX, scanY);
    connect(ccd, SIGNAL(ccdReady()),this,SLOT(ccdReady()));

    const bool connected = connect(ccd, SIGNAL(sendImageData(int, std::string)),this,SLOT(getImageData(int, std::string)));
    if (connected) {
        std::cout<<"getImageData connected"<<std::endl;
    } else {
        std::cout<<"getImageData not connected"<<std::endl;
    }

    const bool connected1 = connect(ccd, SIGNAL(sendCCDSettings(int, int)),this,SLOT(getCCDSettings(int, int)));
    if (connected1) {
        std::cout<<"getCCDSettings connected"<<std::endl;
    } else {
        std::cout<<"getCCDSettings not connected"<<std::endl;
    }

    sdd = new sddThread(QString::fromStdString(scansettings.sddIP+':'+std::to_string(scansettings.sddPort)), QString::fromStdString(scansettings.roidefinitions), scanX, scanY);
    connect(sdd, SIGNAL(sddReady()),this,SLOT(sddReady()));

    const bool connected2 = connect(sdd, SIGNAL(sendScanIndexDataToGUI(int, int, int)),this,SLOT(writeScanIndexData(int, int, int)));
    if (connected2) {
        std::cout<<"writeScanIndexData connected"<<std::endl;
    } else {
        std::cout<<"writeScanIndexData not connected"<<std::endl;
    }

    const bool connected3 = connect(sdd, SIGNAL(sendLineBreakDataToGUI(roidata, int, int, int)),this,SLOT(writeLineBreakData(roidata, int, int, int)));
    if (connected3) {
        std::cout<<"writeLineBreakData connected"<<std::endl;
    } else {
        std::cout<<"writeLineBreakData not connected"<<std::endl;
    }

    const bool connected4 = connect(sdd, SIGNAL(sendSpectrumDataToGUI(int, spectrumdata)),this,SLOT(showIncomingSpectrum(int, spectrumdata)));
    if (connected4) {
        std::cout<<"showIncomingSpectrum connected"<<std::endl;
    } else {
        std::cout<<"showIncomingSpectrum not connected"<<std::endl;
    }

    // start threads
    ccd->start();
    sdd->start();

    // tell control thread that it should listen for metadata
    controlth->waitForMetadata = true;

    /*
    // start ccd thread
    ccd = new zmqThread("127.0.0.1", scanX, scanY);
    connect(ccd, SIGNAL(ccdReady()),this,SLOT(ccdReady()));

    const bool connected = connect(ccd, SIGNAL(sendImageData(int, std::string)),this,SLOT(getImageData(int, std::string)));
    if (connected) {
        std::cout<<"getImageData connected"<<std::endl;
    } else {
        std::cout<<"getImageData not connected"<<std::endl;
    }

    const bool connected1 = connect(ccd, SIGNAL(sendCCDSettings(int, int)),this,SLOT(getCCDSettings(int, int)));
    if (connected1) {
        std::cout<<"getCCDSettings connected"<<std::endl;
    } else {
        std::cout<<"getCCDSettings not connected"<<std::endl;
    }
    */

    /*
    sdd = new sddThread("127.0.0.1", QString::fromStdString(settings.roidefinitions), scanX, scanY);
    connect(sdd, SIGNAL(sddReady()),this,SLOT(sddReady()));

    const bool connected2 = connect(sdd, SIGNAL(sendScanIndexDataToGUI(int, int, int)),this,SLOT(writeScanIndexData(int, int, int)));
    if (connected2) {
        std::cout<<"writeScanIndexData connected"<<std::endl;
    } else {
        std::cout<<"writeScanIndexData not connected"<<std::endl;
    }

    const bool connected3 = connect(sdd, SIGNAL(sendLineBreakDataToGUI(roidata, int, int, int)),this,SLOT(writeLineBreakData(roidata, int, int, int)));
    if (connected3) {
        std::cout<<"writeLineBreakData connected"<<std::endl;
    } else {
        std::cout<<"writeLineBreakData not connected"<<std::endl;
    }

    const bool connected4 = connect(sdd, SIGNAL(sendSpectrumDataToGUI(int, spectrumdata)),this,SLOT(showIncomingSpectrum(int, spectrumdata)));
    if (connected4) {
        std::cout<<"showIncomingSpectrum connected"<<std::endl;
    } else {
        std::cout<<"showIncomingSpectrum not connected"<<std::endl;
    }

    // start threads
    ccd->start();
    sdd->start();


    // tell control thread that it should listen for metadata
    controlth->waitForMetadata = true;
    */
}

void MainWindow::ccdReady() {
    std::cout<<"GUI knows that CCD is ready"<<std::endl;
    controlth->ccdReady = true;
}

void MainWindow::sddReady() {
    std::cout<<"GUI knows that SDD is ready"<<std::endl;
    controlth->sddReady = true;
}

void MainWindow::updateSTXMPreview(int scanX, int scanY, int pixnum) {
       u_int32_t max = 0;
       u_int32_t min = 0xFFFFFFFF;

       int count = 0;
       for (int j = 0; j < scanY; ++j) {
         for (int i = 0; i < scanX; ++i) {
             if (count <= pixnum) {
                 uint32_t dummy = stxmimage[i + (j * scanX)];
                 if (dummy > max) {
                     max = dummy;
                 }
                 if (dummy < min) {
                     min = dummy;
                 }
             }
             count++;
         }
       }

       double factor =(double)65535/(max-min);

       QImage image = QImage(scanX, scanY, QImage::Format_Grayscale16);
       count = 0;
       for (int j = 0; j < scanY; ++j) {
         quint16 *dst = (quint16*)(image.bits() + j * image.bytesPerLine());
         for (int i = 0; i < scanX; ++i) {
           dst[i] = floor((stxmimage[i + j * scanX]-min)*factor);
         count++;
         }
       }

       ui->minlabel->setText(QString::number(min));
       ui->maxlabel->setText(QString::number(max));
       ui->factor->setText(QString::number(factor));

       ui->imagepreview_2->setGeometry(230, 400, scanX*2, scanY*2);
       QPixmap scaledpixmp = QPixmap::fromImage(image).scaledToWidth(scanX*2);
       ui->imagepreview_2->setPixmap(scaledpixmp);
}

void MainWindow::getImageData(int cntx, std::string datax) {
     uint16_t max = 0;
     uint16_t min = 65535;

     uint16_t savedata[1][ccdX][ccdY];
     uint32_t ccdpixelcount = ccdX*ccdY;
     uint32_t ccddatabytecount = ccdpixelcount * 2;
     uint16_t imagex[ccdpixelcount];

     uint32_t sum = 0;
     int counterx = 0;
     int x = 0;
     int y = 0;
     for (uint32_t i = 0; i < ccddatabytecount; i=i+2) {
             uint16_t value = ((uint16_t) (uint8_t)datax[i+1] << 8) | (uint8_t)datax[i];

             if (value > max) {
                 max = value;
             }

             if (value < min) {
                 min = value;
             }

             imagex[counterx] = value;

             if (((counterx-1)%ccdX == 0) && (counterx > 0)) {
                 x = 0;
                 y++;
             } else {
                 if (counterx > 0) {
                    x++;
                 }
             }

             savedata[0][x][y] = value;

             sum = sum + value;

             counterx++;
     }

        double factor = 65535/(max-min);

        ui->label_3->setText("image #"+QString::number(cntx)+" received");

        if (ui->chbPreview->isChecked()) {
            if (((cntx+1)%scanX) == 0) {
                 QImage image = QImage(ccdX, ccdY, QImage::Format_Grayscale16);
                 for (int j = 0; j < ccdY; ++j)
                 {
                    quint16 *dst =  reinterpret_cast<quint16*>(image.bits() + j * image.bytesPerLine());
                    for (int i = 0; i < ccdX; ++i)
                    {
                         unsigned short pixelval =  static_cast<unsigned short>(imagex[i + j * ccdX]);
                         dst[i] = (pixelval-min)*factor;
                    }
                 }

                 QPixmap pixmp = QPixmap::fromImage(image);
                 QPixmap scaledpixmp = pixmp.scaledToHeight(300);

                 ui->imagepreview->setPixmap(scaledpixmp);
             }
        }

        stxmimage[cntx] = sum;
        stxmpreviewvec.append(sum);

        if (ui->chbSaveData->isChecked()) {
            // write data to file

            hsize_t size[3];
            size[1] = ccdX;
            size[2] = ccdY;

            hsize_t offset[3];
            offset[1] = 0;
            offset[2] = 0;

            long pixelvalue[1];
            pixelvalue[0] = 0;


            // WRITE DETECTOR DATA TO FILE
            size[0] = cntx+1;
            offset[0] = cntx;

            hsize_t dimsext[3] = {1, (hsize_t)ccdX, (hsize_t)ccdY}; // extend dimensions

            DataSet *dataset = new DataSet(nexusfile->file->openDataSet("/measurement/instruments/ccd/data"));

            dataset->extend(size);

            // Select a hyperslab in extended portion of the dataset.
            DataSpace *filespace = new DataSpace(dataset->getSpace());

            filespace->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

            // Define memory space.
            DataSpace *memspace = new DataSpace(3, dimsext, NULL);

            // Write data to the extended portion of the dataset.
            dataset->write(savedata, PredType::STD_U16LE, *memspace, *filespace);

            // PLOTTABLE SUM IMAGE

            hsize_t dimsextsumimage[2] = {1, 1}; // extend dimensions

            hsize_t sizesumimage[2];
            hsize_t offsetsumimage[2];

            if (scancounterx < scanX) {
                    scancounterx++;
                } else {
                    scancounterx = 1;
                    xmax = scanX;
                    scancountery++;
                }

            sizesumimage[0] = scancountery;
            if (xmax > 0) {
                sizesumimage[1] = xmax;
            } else {
                sizesumimage[1] = scancounterx;
            }
            offsetsumimage[0] = scancountery-1;
            offsetsumimage[1] = scancounterx-1;

            DataSet *datasetsumimage = new DataSet(nexusfile->file->openDataSet("/measurement/data/data"));

            datasetsumimage->extend(sizesumimage);

            // Select a hyperslab in extended portion of the dataset.
            DataSpace *filespacesumimage = new DataSpace(datasetsumimage->getSpace());

            filespacesumimage->selectHyperslab(H5S_SELECT_SET, dimsextsumimage, offsetsumimage);

            // Define memory space.
            DataSpace *memspacesumimage = new DataSpace(2, dimsextsumimage, NULL);

            uint64_t blubb[1];
            blubb[0] = stxmimage[cntx];

            // Write data to the extended portion of the dataset.
            datasetsumimage->write(blubb, PredType::STD_I64LE, *memspacesumimage, *filespacesumimage);

            dataset->close();
            delete dataset;
            filespace->close();
            delete filespace;
            memspace->close();
            delete memspace;
            datasetsumimage->close();
            delete datasetsumimage;
            filespacesumimage->close();
            delete filespacesumimage;
            memspacesumimage->close();
            delete memspacesumimage;
        }

        if (((cntx+1)%scanX) == 0) {
            if (ui->chbPreview->isChecked()) {
                updateSTXMPreview(scanX, scanY, cntx);
            }
            controlth->setCurrentSTXMPreview(stxmpreviewvec);
            controlth->setCurrentCCDImage(datax);
    }

    if (cntx == (scanX*scanY)-1) {
        ccdReceived = true;
        checkIfScanIsFinished();
    }
}

void MainWindow::showIncomingSpectrum(int cnt, spectrumdata blubb) {
    if (ui->chbSaveData->isChecked()) {
        addSDDDataChunk(cnt, blubb);
    }
    emit addSpecData(cnt, blubb);
}

void MainWindow::addSDDDataChunk(int32_t pxnum, spectrumdata specdata) {
    hsize_t size[2];
    size[1] = 4096;

    hsize_t offset[2];
    offset[0] = pxnum;
    offset[1] = 0;

    // END PIXEL VALUES
    int32_t data[4096];
    for (int i=0;i<4096;i++) {
            data[i] = specdata.at(i);
    }

    // WRITE DETECTOR DATA TO FILE
    size[0] = pxnum+1;
    offset[0] = pxnum;

    hsize_t dimsext[2] = {1, 4096}; // extend dimensions

    DataSet *dataset = new DataSet(nexusfile->file->openDataSet("/measurement/instruments/sdd/data"));

    dataset->extend(size);

    // Select a hyperslab in extended portion of the dataset.
    DataSpace *filespace = new DataSpace(dataset->getSpace());

    filespace->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

    // Define memory space.
    DataSpace *memspace = new DataSpace(2, dimsext, NULL);

    // Write data to the extended portion of the dataset.
    dataset->write(data, PredType::STD_I32LE, *memspace, *filespace);

    dataset->close();
    delete dataset;
    filespace->close();
    delete filespace;
    memspace->close();
    delete memspace;
}

void MainWindow::writeScanIndexData(int dataindex, int nopx, int stopx) {
    if (ui->chbSaveData->isChecked()) {
        // WRITE BEAMLINE PARAMETER TO FILE
        hsize_t dimsext[2] = {1, 3}; // extend dimensions

        DataSet *dataset = new DataSet(nexusfile->file->openDataSet("/measurement/instruments/sdd/log/scanindex"));

        hsize_t offset[2];

        H5::ArrayType arrtype = dataset->getArrayType();

        hsize_t size[2];

        DataSpace *filespace = new DataSpace(dataset->getSpace());
        int n_dims = filespace->getSimpleExtentDims(size);

        offset[0] = size[0];
        offset[1] = 0;

        size[0]++;
        size[1] = 3;

        dataset->extend(size);

        // Select a hyperslab in extended portion of the dataset.
        DataSpace *filespacenew = new DataSpace(dataset->getSpace());

        filespacenew->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

        // Define memory space.
        DataSpace *memspacenew = new DataSpace(2, dimsext, NULL);

        int scanindexdata[3] = {dataindex, nopx, stopx};

        // Write data to the extended portion of the dataset.
        dataset->write(scanindexdata, PredType::STD_I32LE, *memspacenew, *filespacenew);

        std::cout<<"wrote scan index data to file, size: <<"<<size[0]<<", offset: "<<offset[0]<<std::endl;

        dataset->close();
        delete dataset;
        filespace->close();
        delete filespace;
        filespacenew->close();
        delete filespacenew;
        memspacenew->close();
        delete memspacenew;
    }
}

void MainWindow::writeLineBreakData(roidata ROImap, int dataindex, int nopx, int stopx) {


    std::cout<<"roidata length: "<<ROImap["P"].length()<<","<<dataindex<<","<<nopx<<","<<stopx<<std::endl;

    controlth->setCurrentROIs(ROImap);

    if (ui->chbSaveData->isChecked()) {
        // WRITE BEAMLINE PARAMETER TO FILE
        hsize_t dimsext[2] = {1, 3}; // extend dimensions

        DataSet *dataset = new DataSet(nexusfile->file->openDataSet("/measurement/instruments/sdd/log/linebreaks"));

        hsize_t offset[2];

        H5::ArrayType arrtype = dataset->getArrayType();

        hsize_t size[2];

        DataSpace *filespace = new DataSpace(dataset->getSpace());
        int n_dims = filespace->getSimpleExtentDims(size);

        offset[0] = size[0];
        offset[1] = 0;

        size[0]++;
        size[1] = 3;

        dataset->extend(size);

        // Select a hyperslab in extended portion of the dataset.
        DataSpace *filespacenew = new DataSpace(dataset->getSpace());

        filespacenew->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

        // Define memory space.
        DataSpace *memspacenew = new DataSpace(2, dimsext, NULL);

        int scanindexdata[3] = {dataindex, nopx, stopx};

        // Write data to the extended portion of the dataset.
        dataset->write(scanindexdata, PredType::STD_I32LE, *memspacenew, *filespacenew);

        if ((nopx != 0) && (stopx == 0)) {
            stopx = scanY;
        }


        // write ROIs
        if ((nopx != 0) && (stopx != 0)) {
            auto const ROIkeys = ROImap.keys();
            for (std::string e : ROIkeys) {
                std::cout<<"writing ROI data for "<<e<<" to file..."<<std::endl;
                hsize_t dimsextroi[2] = {1, (hsize_t)scanX}; // extend dimensions
                DataSet *datasetroi = new DataSet(nexusfile->file->openDataSet("/measurement/instruments/sdd/roi/"+e));
                hsize_t sizeroi[2];
                hsize_t offsetroi[2];
                offsetroi[0] = stopx-1;
                offsetroi[1] = 0;

                sizeroi[0] = stopx;
                sizeroi[1] = (hsize_t)scanX;

                datasetroi->extend(sizeroi);
                // Select a hyperslab in extended portion of the dataset.
                DataSpace *filespacenewroi = new DataSpace(datasetroi->getSpace());

                filespacenewroi->selectHyperslab(H5S_SELECT_SET, dimsextroi, offsetroi);
                // Define memory space.
                DataSpace *memspacenewroi = new DataSpace(2, dimsextroi, NULL);

                QVector<uint32_t> roidata = ROImap[e];

                uint32_t writedata[scanX];

                std::cout<<"länge des datenarrays: "<<roidata.length()<<"stopx: "<<stopx<<std::endl;
                int pxcounter = 0;
                for (int i=scanX*(stopx-1); i<roidata.length(); i++) {
                    writedata[pxcounter] = roidata.at(i);
                    pxcounter++;
                }

                // Write data to the extended portion of the dataset.
                datasetroi->write(writedata, PredType::STD_I32LE, *memspacenewroi, *filespacenewroi);

                datasetroi->close();
                delete datasetroi;
                filespacenewroi->close();
                delete filespacenewroi;
                memspacenewroi->close();
                delete memspacenewroi;

            }
        }

        dataset->close();
        delete dataset;
        filespace->close();
        delete filespace;
        filespacenew->close();
        delete filespacenew;
        memspacenew->close();
        delete memspacenew;

    }

    if (nopx == scanX*scanY) {
        sddReceived = true;
        checkIfScanIsFinished();
    }
}

void MainWindow::getCCDSettings(int width, int height) {
    std::cout<<"real ccd width: "<<width;
    std::cout<<"real ccd height: "<<height;

    hsize_t fdim[] = {1};
    DataSpace fspace(1, fdim);

    DataSet* settingsgroupccdWidth = new DataSet(nexusfile->file->openDataSet("/measurement/settings/ccdWidth"));
    int ccdWidth = width;
    settingsgroupccdWidth->write(&ccdWidth, PredType::STD_I32LE, fspace);
    settingsgroupccdWidth->close();
    delete settingsgroupccdWidth;

    DataSet* settingsgroupccdHeight = new DataSet(nexusfile->file->openDataSet("/measurement/settings/ccdHeight"));
    int ccdHeight = height;
    settingsgroupccdHeight->write(&ccdHeight, PredType::STD_I32LE, fspace);
    settingsgroupccdWidth->close();
    delete settingsgroupccdHeight;

    fspace.close();
}

void MainWindow::checkIfScanIsFinished() {
    if ((sddReceived) && (ccdReceived)) {
        nexusfile->closeDataFile();
        sddReceived = false;
        ccdReceived = false;

        std::cout<<"closed file '"<<hdf5filename.toStdString()<<"'"<<std::endl;

        int currentScanNumber = currentmetadata.aquisition_number;

        ui->label_3->setText("scan "+QString::number(currentScanNumber)+" finished");

        if ((scansettings.scantype == "NEXAFS") && (currentmetadata.aquisition_number < scansettings.energycount)) {
            currentmetadata.aquisition_number++;
            hdf5filename = "measurement_testmessung_"+QString::number(currentmetadata.aquisition_number)+"_"+QString::number(QDateTime::currentMSecsSinceEpoch())+".h5";

            nexusfile = new hdf5nexus();
            nexusfile->createDataFile(hdf5filename, scansettings);
            ui->label_3->setText("created new datafile "+hdf5filename);
            std::cout<<"created datafile "<<hdf5filename.toStdString()<<std::endl;

            std::cout<<"scan part is ready"<<std::endl;

            // clear variables
            counterx = 0;
            countery = 1;
            scancounterx = 0;
            scancountery = 1;
            xmax = 0;
            stxmpreviewvec.clear();

            // tell control thread to send finish signal
            controlth->partScanFinished = true;
            controlth->waitForMetadata = true;
        } else {
            // stop communication threads
            controlth->stop = true;
            sdd->stop = true;
            ccd->stop = true;
        }
    }
}

void MainWindow::on_pushButton_12_clicked()
{
    scansettings.roidefinitions = "";
    scansettings.ccdHeight = 128;
    scansettings.ccdWidth = 128;
    scansettings.scanHeight = 40;
    scansettings.scanWidth = 30;
    scansettings.scantype = "XRF";
    scansettings.sddChannels = 4096;

    nexusfile = new hdf5nexus();
    nexusfile->createDataFile("testfile.h5", scansettings);
    nexusfile->closeDataFile();
}

