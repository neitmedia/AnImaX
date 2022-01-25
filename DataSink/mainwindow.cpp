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
#include <QListWidgetItem>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QMessageBox>

#include "H5Cpp.h"
using namespace H5;

// constructor of MainWindow
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // register Metatypes to be able to use them with signals/slots
    qRegisterMetaType<imagepixeldata>( "imagepixeldata" );
    qRegisterMetaType<spectrumdata>( "spectrumdata" );
    qRegisterMetaType<settingsdata>( "settingsdata" );
    qRegisterMetaType<metadata>( "metadata" );
    qRegisterMetaType<std::string>();
    qRegisterMetaType<imagepreviewdata>( "imagepreviewdata" );
    qRegisterMetaType<roidata>( "roidata" );

    // create filename of first file
    hdf5filename = "measurement_"+QString::number(QDateTime::currentMSecsSinceEpoch())+".h5";
}

// destructor of Mainwindow
MainWindow::~MainWindow()
{
    delete ui;
}

// overriding MainWindow::showEvent, because datasink shall read settings from .ini-file when it starts
void MainWindow::showEvent( QShowEvent* event ) {

    // do everything you would do in case of default MainWindow::showEvent
    QWidget::showEvent( event );

    // if application has not been initialized (initialized = false), read out network settings from .ini-file.
    if (!initialized) {
        // declare QSettings variable with properties (ini format, create / look for file in user directory in the folder AnImaX/DataSink)
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnImaX", "DataSink");

        // open group Network in ini-file
        settings.beginGroup("Network");

        // read out parameters guiIP and guiPort from .ini-file
        QString guiIP = settings.value("guiIP").toString();
        QString guiPort = settings.value("guiPort").toString();

        // close group
        settings.endGroup();

        // add log items
        addLogItem("read network settings from file "+settings.fileName());
        addLogItem("GUI - "+guiIP+":"+guiPort);

        // if guiIP or guiPort is empty
        if ((guiIP == "") || (guiPort == "")) {
            // write standard IP settings to ini file
            guiIP = "127.0.0.1";
            guiPort = "5555";

            // declare new settings object with given properties
            QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnImaX", "DataSink");

            // create group "Network"
            settings.beginGroup("Network");

            // create settings value "guiIP" with given value
            settings.setValue("guiIP", guiIP);

            // create settings value "guiPort" with given value
            settings.setValue("guiPort", guiPort);

            // close "Network" group
            settings.endGroup();
        }

        // set log path and filename with current date in format "yyyy-MM-dd-hh-mm-ss"
        QString logpath = QDir::currentPath();
        qint64 qiTimestamp=QDateTime::currentMSecsSinceEpoch();
        QDateTime dt;
        dt.setTime_t(qiTimestamp/1000);
        QString datestring = dt.toString("yyyy-MM-dd-hh-mm-ss");
        QString logfile = "measurement_"+datestring+".log";
        QString logfilepath = logpath+"/"+logfile;

        // show current log filename in GUI
        ui->txtLogFileName->setText(logfilepath);

        // launch control thread
        controlth = new controlThread(guiIP+':'+guiPort);

        /* BEGIN CONNECTING SIGNALS AND SLOTS */

        // connect sendSettingsToGUI signal of control thread to getScanSettings slot of datasink GUI thread
        const bool connected = connect(controlth, SIGNAL(sendSettingsToGUI(settingsdata)),this,SLOT(getScanSettings(settingsdata)));

        // connect sendMetadataToGUI signal of control thread to getMetadata slot of datasink GUI thread
        const bool connected2 = connect(controlth, SIGNAL(sendMetadataToGUI(metadata)),this,SLOT(getMetadata(metadata)));

        // connect sendScanNoteToGUI signal of control thread to getScanNote slot of datasink GUI thread
        const bool connected3 = connect(controlth, SIGNAL(sendScanNoteToGUI(std::string)),this,SLOT(getScanNote(std::string)));

        // connect sendScanStatusToGUI signal of control thread to getScanStatus slot of datasink GUI thread
        const bool connected4 = connect(controlth, SIGNAL(sendScanStatusToGUI(std::string)),this,SLOT(getScanStatus(std::string)));

        /* END CONNECTING SIGNALS AND SLOTS */

        // if all slots are connected, start thread
        if (connected && connected2 && connected3 && connected4) {

            // start control thread
            controlth->start();

            // add log item
            addLogItem("launched control thread");

            // set initialized = true, so log file is only read once and control thread only started once
            // at the start of the application, not every time the Window is showed after minimizing
            initialized = true;
        } else {

            // give out error message
            std::cout<<"error connecting signals and slots"<<std::endl;
        }
    }
}


// function that manages to write log entries to log file
// and add log entrys to log view in the user interface
void MainWindow::addLogItem(QString logtext) {
    // get current timestamp
    qint64 qiTimestamp=QDateTime::currentMSecsSinceEpoch();
    QDateTime dt;
    dt.setTime_t(qiTimestamp/1000);
    QString datestring = dt.toString("yyyy-MM-dd hh:mm:ss");

    // add log item to ListWidget in user interface
    new QListWidgetItem(datestring+": "+logtext, ui->lstStatusLog);

    // scroll to bottom in ListWidget so the newest entry is always visible and readable
    ui->lstStatusLog->scrollToBottom();

    // if "save to file"-checkbox is checked, save log entry to file
    if (ui->chbSaveLogToFile->isChecked()) {

        // read out filename from user interface
        QString filename = ui->txtLogFileName->text();

        // declare file object
        QFile file(filename);

        // open file and append log entry to file
        if (file.open(QIODevice::ReadWrite | QIODevice::Append)) {
            QTextStream stream(&file);
            stream << datestring+": "+logtext << Qt::endl;
        }
    }
}

// function that is invoked every time new metadata is received by the controlthread
// and this metadata has been sent to datasinks' GUI thread
void MainWindow::getMetadata(metadata metadata) {

    // update global metadata variable
    currentmetadata = metadata;

    // if "save data to file" checkbox in user interface is checked
    if (ui->chbSaveData->isChecked()) {

        // write metadata to file (see hdf5nexus.cpp)
        nexusfile->writeMetadata(metadata);

        // if acquisition_number is > 1, do not forget to add calculated ccd timing settings from ccd driver to file
        if (metadata.acquisition_number > 1) {
            // write ccd settings (see hdf5nexus.cpp)
            nexusfile->writeCCDSettings(ccdsettingsdata);
        }
    }

    // add log item
    addLogItem("received metadata");
}

// function that is invoked if scan settings are received by the controlthread
// and these settings have been sent to datasinks' GUI thread
void MainWindow::getScanSettings(settingsdata settings) {

    // set global scansettings variable
    scansettings = settings;

    // set global ccd dimension variables
    ccdX = scansettings.ccd_width;
    ccdY = scansettings.ccd_height;

    // set global scan dimension variables
    scanX = scansettings.scanWidth;
    scanY = scansettings.scanHeight;

    // update compression checkbox in user interface according to compression settings
    if (scansettings.file_compression) {
        ui->chbCompressionEnabled->setChecked(true);
    } else {
        ui->chbCompressionEnabled->setChecked(false);
    }

    // give out some debug info about the compression settings
    std::cout<<"file compression: "<<scansettings.file_compression<<std::endl;

    // update compression level combo box in user interface according to compression settings
    ui->cmbCompressionLevel->setCurrentIndex(scansettings.file_compression_level);

    // allocate memory for STXM transmission preview image (4 byte (32 bit) per scan pixel)
    stxmimage = (uint32_t*) malloc(scanX*scanY*sizeof(uint32_t));

    // make sure stxmimage contains only zeros
    for (int i=0;i<scanX*scanY;i++) {
        stxmimage[i] = 0;
    }

    // give out some debug info
    std::cout<<"ccd ip:port "<<scansettings.ccdIP+':'+std::to_string(scansettings.ccdPort)<<std::endl;
    std::cout<<"sdd ip:port "<<scansettings.sddIP+':'+std::to_string(scansettings.sddPort)<<std::endl;

    // create HDF5/NeXus file
    // check if provided path exists
    QFileInfo my_dir(QString::fromStdString(scansettings.save_path));

    // if provided path exists and is writable, write file to this folder
    if ((my_dir.exists()) && (my_dir.isWritable())) {
        addLogItem("provided folder "+QString::fromStdString(scansettings.save_path)+" exists and is writetable, create file...");
        hdf5filename = QString::fromStdString(scansettings.save_path)+"/measurement_"+QString::fromStdString(scansettings.save_file)+"_1_"+QString::number(QDateTime::currentMSecsSinceEpoch())+".h5";
    // if provided path does not exist or is not writable, write file to application folder
    } else {
        addLogItem("provided folder "+QString::fromStdString(scansettings.save_path)+" does not exist or is not writetable, create file in application directory...");
        hdf5filename = "measurement_"+QString::fromStdString(scansettings.save_file)+"_1_"+QString::number(QDateTime::currentMSecsSinceEpoch())+".h5";
    }

    // create HDF5/NeXus file
    nexusfile = new hdf5nexus();
    nexusfile->createDataFile(hdf5filename, scansettings);

    // add log entry
    addLogItem("created file "+hdf5filename);

    // add file to scan file list
    scanFiles.append(hdf5filename);

    // show filename in GUI
    ui->lblFilename->setText("current file: "+hdf5filename);

    /* prepare ROI preview labels, for this deserialize json data first */
    // declare QJsonParseError
    QJsonParseError jsonError;

    // write roidefinitions to roijson string
    QString roijson = QString::fromStdString(scansettings.roidefinitions);

    // deserialize json data into QJsonDocument object document
    QJsonDocument document = QJsonDocument::fromJson(roijson.toLocal8Bit(), &jsonError);

    // declare jsonObject
    QJsonObject jsonObj = document.object();

    // iterate through all keys of jsonObj to add all keys to combobox in user interface
    foreach(QString key, jsonObj.keys()) {
        // add roi item to combo box in user interface
        ui->cmbROISelect->addItem(key);
    }

    // start ccd thread
    ccd = new ccdThread(QString::fromStdString(scansettings.ccdIP+':'+std::to_string(scansettings.ccdPort)), scanX, scanY);

    // connect ccdReady-signal of ccd thread to ccdReady slot of datasinks' GUI thread
    connect(ccd, SIGNAL(ccdReady()),this,SLOT(ccdReady()));

    // connect sendImageData-signal of ccd thread to getImageData slot of datasinks' GUI thread
    const bool connected = connect(ccd, SIGNAL(sendImageData(int, std::string)),this,SLOT(getImageData(int, std::string)));

    // give out some debug info about the connection state of the signal/slot
    if (connected) {
        std::cout<<"getImageData connected"<<std::endl;
    } else {
        std::cout<<"getImageData not connected"<<std::endl;
    }

    // connect sendCCDSettings-signal of ccd thread to getCCDSettings slot of datasinks' GUI thread
    const bool connected1 = connect(ccd, SIGNAL(sendCCDSettings(ccdsettings)),this,SLOT(getCCDSettings(ccdsettings)));

    // give out some debug info about the connection state of the signal/slot
    if (connected1) {
        std::cout<<"getCCDSettings connected"<<std::endl;
    } else {
        std::cout<<"getCCDSettings not connected"<<std::endl;
    }

    sdd = new sddThread(QString::fromStdString(scansettings.sddIP+':'+std::to_string(scansettings.sddPort)), QString::fromStdString(scansettings.roidefinitions), scanX, scanY);
    connect(sdd, SIGNAL(sddReady()),this,SLOT(sddReady()));

    // connect sendScanIndexDataToGUI-signal of ccd thread to writeScanIndexData slot of datasinks' GUI thread
    const bool connected2 = connect(sdd, SIGNAL(sendScanIndexDataToGUI(long, int, int)),this,SLOT(writeScanIndexData(long, int, int)));

    // give out some debug info about the connection state of the signal/slot
    if (connected2) {
        std::cout<<"writeScanIndexData connected"<<std::endl;
    } else {
        std::cout<<"writeScanIndexData not connected"<<std::endl;
    }

    // connect sendLineBreakDataToGUI-signal of ccd thread to writeLineBreakData slot of datasinks' GUI thread
    const bool connected3 = connect(sdd, SIGNAL(sendLineBreakDataToGUI(roidata, long, int, int)),this,SLOT(writeLineBreakData(roidata, long, int, int)));

    // give out some debug info about the connection state of the signal/slot
    if (connected3) {
        std::cout<<"writeLineBreakData connected"<<std::endl;
    } else {
        std::cout<<"writeLineBreakData not connected"<<std::endl;
    }

    // connect sendSpectrumDataToGUI-signal of ccd thread to showIncomingSpectrum slot of datasinks' GUI thread
    const bool connected4 = connect(sdd, SIGNAL(sendSpectrumDataToGUI(int, spectrumdata)),this,SLOT(showIncomingSpectrum(int, spectrumdata)));

    // give out some debug info about the connection state of the signal/slot
    if (connected4) {
        std::cout<<"showIncomingSpectrum connected"<<std::endl;
    } else {
        std::cout<<"showIncomingSpectrum not connected"<<std::endl;
    }

    // start ccd thread
    ccd->start();

    // start sdd thread
    sdd->start();

    // tell control thread that it should watch out for metadata
    controlth->waitForMetadata = true;

    // add log item
    addLogItem("received scan settings");
}

// function that is invoked if cdd ready signal was received by the ccd thread
// and this signal has been sent to datasinks' GUI thread
void MainWindow::ccdReady() {

    // give out some debug info
    std::cout<<"GUI knows that CCD is ready"<<std::endl;

    // set controlthreads' ccdReady variable = true
    controlth->ccdReady = true;

    // add log item
    addLogItem("ccd is ready");
}

// function that is invoked if sdd ready signal was received by the sdd thread
// and this signal has been sent to datasinks' GUI thread
void MainWindow::sddReady() {

    // give out some debug info
    std::cout<<"GUI knows that SDD is ready"<<std::endl;

    // set controlthreads' sddReady variable = true
    controlth->sddReady = true;

    // add log item
    addLogItem("sdd is ready");
}

// function that is invoked if transmission image preview shall be updated
// this function is invoked by getImageData slot of datasinks' GUI thread
// if new ccd image data is received and previews are enabled in the user interface
void MainWindow::updateSTXMPreview(int scanX, int scanY, int pixnum) {

       // declare and set variables for maximum and minimum pixel value
       u_int32_t max = 0;
       u_int32_t min = 0xFFFFFFFF;

       // declare and initialize counter variable
       int count = 0;

       // iterate through the whole transmission image
       for (int j = 0; j < scanY; ++j) {
         for (int i = 0; i < scanX; ++i) {
             // calculate maximum and minimum pixel values of transmission image
             // only calculate maximum and minimum for already received pixels
             if (count <= pixnum) {

                 // declare variable and write current pixel value into it
                 uint32_t pxval = stxmimage[i + (j * scanX)];

                 // if current pixel value is > max (maximum value), take value as new maximum value
                 if (pxval > max) {
                     max = pxval;
                 }

                 // is current pixel value is < min (minimum value), take value as new minimum value
                 if (pxval < min) {
                     min = pxval;
                 }

             }

             // increment counter variable
             count++;
         }
       }

       // calculate factor to "shrink" 32 bit values to 16 bit value
       // for this use the difference between the maximum and the minimum pixel value
       // to make use of the maximal possible dynamic range
       double factor =(double)65535/(max-min);

       // declare QImage object
       QImage image = QImage(scanX, scanY, QImage::Format_Grayscale16);

       // iterate through whole image
       for (int j = 0; j < scanY; ++j) {

         // declare pointer to memory position where QImage image data line starts
         quint16 *dst = (quint16*)(image.bits() + j * image.bytesPerLine());

         // write all pixel values of the current line to the memory
         for (int i = 0; i < scanX; ++i) {
           // write pixel value that was reduced by the absolute value of the minimum value and scaled by factor
           // to the belonging memory position, use floor() to make sure only integers are written to the memory
           dst[i] = floor((stxmimage[i + j * scanX]-min)*factor);
         }
       }

       // declare QPixmap scaledpixmp
       QPixmap scaledpixmp;

       // scale image according to the pixel ratio to make sure that always the full preview image is visible in the user interface
       // if image width > image height
       if (scanX>=scanY) {
         // set fixed width
         scaledpixmp = QPixmap::fromImage(image).scaledToWidth(ui->imagepreview_2->width());
       }

       // if image width < image height
       if (scanX<scanY) {
         // set fixed height
         scaledpixmp = QPixmap::fromImage(image).scaledToHeight(ui->imagepreview_2->height());
       }

       // show image in user interface
       ui->imagepreview_2->setPixmap(scaledpixmp);
}

// function that is invoked if new image data has been received by the ccd thread
// and has been send to the datasinks' GUI thread
void MainWindow::getImageData(int cntx, std::string datax) {

     // declare and set variables for maximum and minimum 16 bit pixel value
     uint16_t max = 0;
     uint16_t min = 65535;

     // declare three dimensional array for the image data that will be written to the data file
     uint16_t savedata[ccdX][ccdY][1];

     // declare and set variable that contains the pixel count of the ccd image
     uint32_t ccdpixelcount = ccdX*ccdY;

     // declare and set variable that contains count of bytes necessary to save a ccd image
     uint32_t ccddatabytecount = ccdpixelcount * 2;

     // declare a one dimensional array that contains the ccd image data
     uint16_t imagex[ccdpixelcount];

     // declare and initialize a sum variable
     uint32_t sum = 0;

     // declare and initialize counter variables
     int counterx = 0;
     int x = 0;
     int y = 0;

     // iterate through all data bytes
     for (uint32_t i = 0; i < ccddatabytecount; i=i+2) {
             // the pixel value always consists of two bytes
             // calculate pixel value from two raw data bytes
             uint16_t value = ((uint16_t) (uint8_t)datax[i+1] << 8) | (uint8_t)datax[i];

             // if current pixel value > current maximum value,
             // take current value as new maximum value
             if (value > max) {
                 max = value;
             }

             // if current pixel value < current minimum value,
             // take current value as new minimum value
             if (value < min) {
                 min = value;
             }

             // write current pixel value to imagex pixel value array
             imagex[counterx] = value;

             // calculate current x and y value in the final ccd image
             // if counterx%ccdX == 0, a new line begins, except for the first pixel, where counterx equals zero
             if (((counterx-1)%ccdX == 0) && (counterx > 0)) {
                 x = 0;
                 y++;
             // if counterx%ccdX != 0, no new line begins but the row has to be incremented
             } else {
                 if (counterx > 0) {
                    x++;
                 }
             }

             // write pixel value to the corresponding pixel of the savedata array that will be written to the datafile
             savedata[x][y][0] = value;

             // add current pixel value to sum
             sum = sum + value;

             // increment x counter
             counterx++;
     }

        // calculate factor to "shrink" 32 bit values to 16 bit value
        // for this use the difference between the maximum and the minimum pixel value
        // to make use of the maximal possible dynamic range
        double factor = 65535/(max-min);

        // if "show preview" checkbox in user interface is checked, show preview image in user interface
        if (ui->chbPreview->isChecked()) {

            // update image only when a new line is finished
            if (((cntx+1)%scanX) == 0) {

                 // show current image count in user interface
                 ui->lblStatus->setText("image #"+QString::number(cntx)+" received");

                 // declare QImage object
                 QImage image = QImage(ccdX, ccdY, QImage::Format_Grayscale16);

                 // iterate through whole ccd image
                 for (int j = 0; j < ccdY; ++j)
                 {
                    // declare pointer to memory position where QImage image data line starts
                    quint16 *dst =  reinterpret_cast<quint16*>(image.bits() + j * image.bytesPerLine());

                    // write all pixel values of the current line to the memory
                    for (int i = 0; i < ccdX; ++i)
                    {
                         // write pixel value that was reduced by the absolute value of the minimum value and scaled by factor
                         // to the belonging memory position, use floor() to make sure only integers are written to the memory
                         unsigned short pixelval = static_cast<unsigned short>(imagex[i + j * ccdX]);
                         dst[i] = floor((pixelval-min)*factor);
                    }
                 }

                 // declare QPixmap pixmp and write calculated image data into it
                 QPixmap pixmp = QPixmap::fromImage(image);

                 // show ccd preview image in user interface
                 ui->imagepreview->setPixmap(pixmp);
             }
        }

        // write current sum of all ccd image pixels to global transmission image array stxmimage
        stxmimage[cntx] = sum;

        // append current sum of all ccd image pixels to global transmission image vector stxmpreviewvec
        stxmpreviewvec.append(sum);

        // if "save to file" checkbox is checked
        if (ui->chbSaveData->isChecked()) {

            // calculate current column from current image count and scan width
            col = cntx%scanX;

            // declare and define x_pos und y_pos, calculate pixel position
            // from current col and row and step sizes defined in scansettings
            float x_pos = col*scansettings.x_step_size;
            float y_pos = row*scansettings.y_step_size;

            /* START WRITING SCAN POSITION */

            // write scan position to file
            nexusfile->appendValueTo1DDataSet("/measurement/transmission/sample_x", cntx, x_pos);
            nexusfile->appendValueTo1DDataSet("/measurement/fluorescence/sample_x", cntx, x_pos);

            nexusfile->appendValueTo1DDataSet("/measurement/transmission/sample_y", cntx, y_pos);
            nexusfile->appendValueTo1DDataSet("/measurement/fluorescence/sample_y", cntx, y_pos);

            /* END WRITING SCAN POSITION */

            // if row is full, increment row counter
            if (col == scanX-1) {
                row++;
            }

            /* START WRITING CCD IMAGE DATA */

            // declare size array
            hsize_t size[3];

            // set the first element of the size array to the width of the ccd image
            size[0] = ccdX;
            // set the second element of the size array to the height of the ccd image
            size[1] = ccdY;


            // declare offset variable
            hsize_t offset[3];

            // set the first two elements of the offset array = 0
            offset[0] = 0;
            offset[1] = 0;

            // the third component of the size array has to be the current image counter + 1
            // because the data set in the hdf5 data file is extended dynamically
            // e.g. if two images (cntx=2) already have been written to the file, the next image written is the third (cntx+1=3) image
            size[2] = cntx+1;

            // the third component of the offset has to be to the current image counter
            // because the new image has to be appended to the existing dataset
            // e.g. if two images (cntx=2) already have been written to the file,
            // the next image has to be written at position 2 (first image: position 0, second image: position 1)
            offset[2] = cntx;

            // declare three dimensional array dimsext
            // the first element is set to ccdX, the second element is set to ccdY, the third element is set to 1
            // so the new added data will have the dimensions of a (ccdX x ccdY x 1) matrix => ccd image
            hsize_t dimsext[3] = {(hsize_t)ccdX, (hsize_t)ccdY, 1};

            // open ccd data dataset
            DataSet *dataset = new DataSet(nexusfile->file->openDataSet("/measurement/instruments/ccd/data"));

            // extend dimensions by (0x0x1) => see size array
            dataset->extend(size);

            // get the current dataspace of the dataset
            DataSpace *filespace = new DataSpace(dataset->getSpace());

            // Select a hyperslab in extended portion of the dataset.
            filespace->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

            // Define memory space.
            DataSpace *memspace = new DataSpace(3, dimsext, NULL);

            // Write data to the extended portion of the dataset.
            dataset->write(savedata, PredType::STD_U16LE, *memspace, *filespace);

            // close all created objects and
            // free the memory by deleting the objects
            dataset->close();
            delete dataset;
            filespace->close();
            delete filespace;
            memspace->close();
            delete memspace;

            /* END WRITING CCD IMAGE DATA */

            /* START WRITING PLOTTABLE SUM IMAGE DATA / TRANSMISSION IMAGE */

            // set first two element of dimsextsumimage = 1,
            // because data of dimensions 1x1 will be added to the dataset
            hsize_t dimsextsumimage[2] = {1, 1}; // extend dimensions

            // declare size array
            hsize_t sizesumimage[2];

            // declare offset array
            hsize_t offsetsumimage[2];

            // calculate counter values
            // if current x value is smaller than the width of the scan
            if (scancounterx < scanX) {
                    // increment x value
                    scancounterx++;
                // if current x value is not smaller than the width of the scan
                } else {
                    // set current x value = 1
                    scancounterx = 1;
                    // set xmax = width of the scan
                    xmax = scanX;
                    // increment y pixel counter / line count
                    scancountery++;
                }

            // set first element of the size array = y pixel counter  / line count
            sizesumimage[0] = scancountery;

            // if the first line is full, the dataset already has the full width
            if (xmax > 0) {
                // set second element of the array = the full width
                sizesumimage[1] = xmax;
            // if first line is not full
            } else {
                // set second element = current width of the transmission image
                sizesumimage[1] = scancounterx;
            }

            // scancounter values are both one count smaller than the sizes
            offsetsumimage[0] = scancountery-1;
            offsetsumimage[1] = scancounterx-1;

            // open data set
            DataSet *datasetsumimage = new DataSet(nexusfile->file->openDataSet("/measurement/transmission/data"));

            // extend the size of the data file
            datasetsumimage->extend(sizesumimage);

            // get the file space of the dataset
            DataSpace *filespacesumimage = new DataSpace(datasetsumimage->getSpace());

            // Select a hyperslab in extended portion of the dataset.
            filespacesumimage->selectHyperslab(H5S_SELECT_SET, dimsextsumimage, offsetsumimage);

            // Define memory space.
            DataSpace *memspacesumimage = new DataSpace(2, dimsextsumimage, NULL);

            // declare a 32 bit integer array with one element for writing into the dataset
            uint32_t writeelement[1];

            // write the pixel value into the array
            writeelement[0] = stxmimage[cntx];

            // Write data to the extended portion of the dataset.
            datasetsumimage->write(writeelement, PredType::STD_I32LE, *memspacesumimage, *filespacesumimage);

            // close all created objects and
            // free the memory by deleting the objects
            datasetsumimage->close();
            delete datasetsumimage;
            filespacesumimage->close();
            delete filespacesumimage;
            memspacesumimage->close();
            delete memspacesumimage;

            /* STOP WRITING PLOTTABLE SUM IMAGE DATA / TRANSMISSION IMAGE */

        }

        // if line is full...
        if (((cntx+1)%scanX) == 0) {
            // ... and checkbox "show preview" in user interface is checked ...
            if (ui->chbPreview->isChecked()) {
                // ... update the transmission image preview in the user interface
                updateSTXMPreview(scanX, scanY, cntx);
            }

            // send the new generated preview images to the control thread
            controlth->setCurrentSTXMPreview(stxmpreviewvec);
            controlth->setCurrentCCDImage(datax);
    }

    // if scan is finished
    if (cntx == (scanX*scanY)-1) {

        // set global ccdReceived variable = true
        ccdReceived = true;

        // add log item
        addLogItem("fully received ccd data");

        // check if scan is finished
        checkIfScanIsFinished();
    }
}

// function that is invoked if new spectrum data has been received by the sdd thread
// and has been send to the datasinks' GUI thread
void MainWindow::showIncomingSpectrum(int cnt, spectrumdata specdata) {

    // if "save to file" checkbox in user interface is checked
    if (ui->chbSaveData->isChecked()) {

        // write spectrum data to file
        nexusfile->writeSDDData(cnt, specdata);
    }
}

// function that is invoked if new scan index log data has been received by the sdd thread
// and has been send to the datasinks' GUI thread
void MainWindow::writeScanIndexData(long dataindex, int nopx, int stopx) {

    // if "save to file" checkbox in user interface is checked
    if (ui->chbSaveData->isChecked()) {

        // write scan index log data to file
        nexusfile->writeScanIndexData(dataindex, nopx, stopx);
    }
}

// function that is invoked if new line break log data has been received by the sdd thread
// and has been send to the datasinks' GUI thread
void MainWindow::writeLineBreakData(roidata ROImap, long dataindex, int nopx, int stopx) {
    // if function is invoked it also means that a new line is full
    // send the current ROI data to the control thread
    controlth->setCurrentROIs(ROImap);

    // if "save to file" checkbox in user interface is checked
    if (ui->chbSaveData->isChecked()) {
        // write line break log data and ROIs to data file
        nexusfile->writeLineBreakDataAndROIs(ROImap, dataindex, nopx, stopx, scanX, scanY);
    }

    // if fluorescence data has been completely received
    if (nopx == scanX*scanY) {
        // set sddReceived = true
        sddReceived = true;
        // check if scan is finished
        checkIfScanIsFinished();
        // add log item
        addLogItem("fully received sdd data");
    }

    // if preview checkbox is checked, show ROI preview
    if (ui->chbPreview->isChecked()) {

        // declare and set variables for maximum and minimum pixel values
        u_int32_t max = 0;
        u_int32_t min = 0xFFFFFFFF;

        // declare and initialize counter variable
        unsigned long count = 0;

        // declare and define variable that stores the pixel count of the scan
        unsigned long pixelcount = scanX*scanY;

        // declare and define a std::string and write the ROI element into it that is current selected in the GUI
        std::string ROIelement = ui->cmbROISelect->currentText().toStdString();

        // declare an array for the preview image
        uint32_t previewimage[pixelcount];

        // declare and define an QVector of uint32_t that stores the ROI data
        QVector<uint32_t> roidata = ROImap[ROIelement];

        // iterate through the whole ROI image
        for (unsigned long i = 0; i < pixelcount; i++) {
            // if new data is available
            if ((i+3) < (unsigned long) roidata.length()) {

                // get the pixel value from the QVector
                uint32_t pixelvalue = roidata.at(i);

                // calculate min and max values
                if (pixelvalue > max) {
                    max = pixelvalue;
                }
                if (pixelvalue < min) {
                    min = pixelvalue;
                }

                // set the corresponding array item to the pixel value
                previewimage[count] = pixelvalue;
            // if new new data is available
            } else {
                // set the corresponding array item to zero
                previewimage[count] = 0;
            }
            // increment counter
            count++;
        }

        // calculate factor to "shrink" 32 bit values to 16 bit value
        // for this use the difference between the maximum and the minimum pixel value
        // to make use of the maximal possible dynamic range
        double factor = (double)65535/(max-min);

        // declare QImage object
        QImage image = QImage(scanX, scanY, QImage::Format_Grayscale16);

        // iterate through whole ROI image
        for (int j = 0; j < scanY; ++j) {

          // declare pointer to memory position where QImage image data line starts
          quint16 *dst = (quint16*)(image.bits() + j * image.bytesPerLine());

          // write all ROI pixel values of the current line to the memory
          for (int i = 0; i < scanX; ++i) {

            // write pixel value that was reduced by the absolute value of the minimum value and scaled by factor
            // to the belonging memory position, use floor() to make sure only integers are written to the memory
            dst[i] = floor((previewimage[i + j * scanX]-min)*factor);
          }

        }

        // declare QPixmap
        QPixmap scaledpixmp;

        // scale image according to the pixel ratio to make sure that always the full preview image is visible in the user interface
        // if image width >= image height
        if (scanX>=scanY) {
          // set fixed width
          scaledpixmp = QPixmap::fromImage(image).scaledToWidth(ui->imagepreview->width());
        }

        // if image width < image height
        if (scanX<scanY) {
          // set fixed height
          scaledpixmp = QPixmap::fromImage(image).scaledToHeight(ui->imagepreview->height());
        }

        // show ROI preview in user interface
        ui->lblROIPreview->setPixmap(scaledpixmp);
    }
}

// function that is invoked if new ccd settings data (calculcated by the ccd driver) has been received by the sdd thread
// and has been send to the datasinks' GUI thread
void MainWindow::getCCDSettings(ccdsettings newccdsettingsdata) {

    // update ccdsettingsdata
    ccdsettingsdata = newccdsettingsdata;

    // write settings to file
    nexusfile->writeCCDSettings(ccdsettingsdata);

}

// function that is invoked if either all ccd or all sdd data has been received by the datasinks' GUI thread
void MainWindow::checkIfScanIsFinished() {
    // if sdd and ccd data transmission is completed
    if ((sddReceived) && (ccdReceived)) {

        // write final time stamp to data file
        nexusfile->writeEndTimeStamp();

        // close data file
        nexusfile->closeDataFile();

        // add log item
        addLogItem("closed file "+hdf5filename);

        // set sddReceived and ccdReceived = false, to the next scan can be started
        sddReceived = false;
        ccdReceived = false;

        // give some debug info
        std::cout<<"closed file '"<<hdf5filename.toStdString()<<"'"<<std::endl;

        // declare a variable that stores the current currentScanNumber from the global currentmetadata struct
        int currentScanNumber = currentmetadata.acquisition_number;

        // show status in user interface
        ui->lblStatus->setText("scan "+QString::number(currentScanNumber)+" finished");

        // add log item
        addLogItem("scan "+QString::number(currentScanNumber)+" finished");

        // if scan is a NEXAFS scan and not all energies are completed
        if ((scansettings.scantype == "NEXAFS") && (currentmetadata.acquisition_number < scansettings.energycount)) {

            // clear stxm transmission preview
            for (int i=0;i<=scanX*scanY;i++) {
                stxmimage[i] = 0;
            }

            // clear col and row
            col = 0;
            row = 0;

            // incremement acquisition number
            currentmetadata.acquisition_number++;

            // create a QFile Object
            QFileInfo my_dir(QString::fromStdString(scansettings.save_path));

            // if provided path exists and is writable, write file to this folder
            if ((my_dir.exists()) && (my_dir.isWritable())) {
                // add log item
                addLogItem("provided folder "+QString::fromStdString(scansettings.save_path)+" exists and is writetable, create file...");
                // update current filename
                hdf5filename = QString::fromStdString(scansettings.save_path)+"/measurement_"+QString::fromStdString(scansettings.save_file)+"_"+QString::number(currentmetadata.acquisition_number)+"_"+QString::number(QDateTime::currentMSecsSinceEpoch())+".h5";
            // if provided path does not exist or is not writable, write file to application folder
            } else {
                // add log item
                addLogItem("provided folder "+QString::fromStdString(scansettings.save_path)+" does not exist or is not writetable, create file in application directory...");
                // update current filename
                hdf5filename = "measurement_"+QString::fromStdString(scansettings.save_file)+"_"+QString::number(currentmetadata.acquisition_number)+"_"+QString::number(QDateTime::currentMSecsSinceEpoch())+".h5";
            }

            // create new nexusfile object
            nexusfile = new hdf5nexus();

            // create new data file
            nexusfile->createDataFile(hdf5filename, scansettings);

            // add file to scan file list
            scanFiles.append(hdf5filename);

            // add log item
            addLogItem("created file "+hdf5filename);

            // update filename in user interface
            ui->lblFilename->setText("current file: "+hdf5filename);

            // give out some debug info
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

            // tell GUI that whole scan is finished
            controlth->wholeScanFinished = true;
            sdd->stop = true;
            ccd->stop = true;
            std::cout<<"whole scan finished"<<std::endl;

            // give info message
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setText("Scan finished!");
            msgBox.exec();
        }
    }
}

// this function is invoked if user clicked the close option in the main menu
void MainWindow::on_actionClose_triggered()
{
    qApp->closeAllWindows();
}

// this function is invoked if user clicked the "select" button (log file)
void MainWindow::on_cmdSelectLogFile_clicked()
{
    // open a QFileDialog and write selected filename to logFileName
    QString logFileName = QFileDialog::getSaveFileName(this, tr("Save log file"), "/", tr("Log Files (*.log)"));

    // update log file name in user interface
    ui->txtLogFileName->setText(logFileName);
}

// this function is invoked if gui thread receives a new scan note
void MainWindow::getScanNote(std::string scannote) {

    // add log item
    addLogItem("new scan note: "+QString::fromStdString(scannote));

    // increment note counter
    notecounter++;

    // iterate through all files that were written during the finished scan
    foreach (QString fname, scanFiles)
    {
        // open data file
        nexusfile->openDataFile(fname);

        // write scan note to data file
        nexusfile->writeScanNote(scannote, notecounter);

        // close data file
        nexusfile->closeDataFile();

        // add log item
        addLogItem("added scan note to file '"+fname+"'");
    }

}

// this function is invoked if gui thread receives a new scan status change request
void MainWindow::getScanStatus(std::string scanstatus) {
    // if user wants to stop the scan
    if (scanstatus == "stop") {
        // stop communication threads
        sdd->stop = true;
        ccd->stop = true;
        controlth->stop = true;

        // add log item
        addLogItem("stopping scan...");

        // Start Timer with timeout of 5 seconds to make sure everything is written to file before file is closed
        QTimer *timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, &MainWindow::saveAndClose);
        timer->start(5000);
    }

    // if user wants to pause the scan
    if (scanstatus == "pause") {
        // add log item
        addLogItem("scan paused");
    }
}

// this function is invoked if timer timeout elapses
void MainWindow::saveAndClose() {

    // write end time stamp
    nexusfile->writeEndTimeStamp();

    // close data file
    nexusfile->closeDataFile();

    // add log item
    addLogItem("closed datafile and stopped scan");
}
