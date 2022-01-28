#include "gui.h"
#include "ui_gui.h"
#include "math.h"
#include <QMessageBox>

// constructor
GUI::GUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GUI)
{
    ui->setupUi(this);

    // register "settingsdata" and std::string as metatype because we want to use variables of "settingsdata" and std::string type as arguments
    qRegisterMetaType<settingsdata>( "settingsdata" );
    qRegisterMetaType<std::string>();
}

// destructor
GUI::~GUI()
{
    delete ui;
}

// if menu-entry "Exit" is clicked
void GUI::on_actionExit_triggered()
{
    // close application
    qApp->quit();
}


void GUI::on_cmdStartScan_clicked()
{
    /* StartScan function does:
       1) publish settings (settings protobuf)
       2) wait for data sink, ccd, sdd ready signals
       3) wait for real ccd settings calculcated and set by the CCD driver
       4) get parameters from beamline etc.
       5) publish parameters (metadata protobuf)
       6) start scan
    */

    // disable start scan button, note input field and note save button
    ui->cmdStartScan->setDisabled(true);
    ui->txtScanNote->setDisabled(true);
    ui->cmdSaveScanNote->setDisabled(true);

    // get settings from UI and write them in a variable of type "settingsdata" (see structs.h)
    settingsdata settings;

    settings.scanHeight = ui->spbScanHeight->value();
    settings.scanWidth = ui->spbScanWidth->value();
    settings.x_step_size = ui->dsbXStepSize->value();
    settings.y_step_size = ui->dsbYStepSize->value();
    settings.scantitle = ui->txtScanTitle->text().toStdString();

    settings.save_path = ui->txtFilePath->text().toStdString();
    settings.save_file = ui->txtFileName->text().toStdString();
    settings.file_compression = ui->chbEnableCompression->isChecked();
    settings.file_compression_level = ui->cmbCompressionLevel->currentIndex();

    // write scan width and height into global variables
    scanX = settings.scanWidth;
    scanY = settings.scanHeight;

    settings.energycount = ui->lstEnergies->count();

    // allocate energy for settings
    settings.energies = (float*) malloc(ui->lstEnergies->count()*sizeof(float));

    // iterate through energy list in user interface
    for(int i = 0; i < ui->lstEnergies->count(); ++i)
    {
        // add every energy to energylist in settings struct
        QListWidgetItem* item = ui->lstEnergies->item(i);
        settings.energies[i] = item->text().toFloat();
    }

    // get roidefinitions json data from user iterface
    settings.roidefinitions = ui->txtROIdefinitions->toPlainText().toStdString();

    // if NEXAFS checkbox is checked
    if (ui->rdbNEXAFS->isChecked()) {
        // set scantype to NEXAFS
        settings.scantype = "NEXAFS";
    // if XRF checkbox is checked
    } else if (ui->rdbXRF->isChecked()) {
        // set scantype to XRF and energycount to 1 (XRF => only one energy)
        settings.scantype = "XRF";
        settings.energycount = 1;
    }

    // read out network settings from user interface
    settings.datasinkIP = ui->txtDataSinkIP->text().toStdString();
    settings.datasinkPort = ui->spbDataSinkPort->value();

    settings.ccdIP = ui->txtCCDIP->text().toStdString();
    settings.ccdPort = ui->spbCDDPort->value();

    settings.sddIP = ui->txtSDDIP->text().toStdString();
    settings.sddPort = ui->spbSDDPort->value();

    settings.guiPort = ui->spbGUIPort->value();

    // ... ccd settings ...
    settings.ccd_binning_x = ui->spbCCDBinningX->value();
    settings.ccd_binning_y = ui->spbCCDBinningY->value();
    settings.ccd_height = ui->spbCCDHeight->value();
    settings.ccd_width = ui->spbCCDWidth->value();
    settings.ccd_pixelcount = ui->txtPixelCount->text().toInt();
    settings.ccd_frametransfer_mode = ui->spbCCDFTMode->value();
    settings.ccd_number_of_accumulations = ui->spbNumberOfAccumulations->value();
    settings.ccd_number_of_scans = ui->spbNumberOfScans->value();
    settings.ccd_set_kinetic_cycle_time = ui->dsbKineticCycleTime->value();
    settings.ccd_read_mode = ui->spbReadMode->value();
    settings.ccd_acquisition_mode = ui->spbAcquisitionMode->value();
    settings.ccd_shutter_mode = ui->spbShutterMode->value();
    settings.ccd_shutter_output_signal = ui->spbShutterOutputSignal->value();
    settings.ccd_shutter_open_time = ui->dsbShutterOpenTime->value();
    settings.ccd_shutter_close_time = ui->dsbShutterCloseTime->value();
    settings.ccd_triggermode = ui->spbTriggerMode->value();
    settings.ccd_exposure_time = ui->dsbExposureTime->value();
    settings.ccd_accumulation_time = ui->dsbAccumulationTime->value();
    settings.ccd_kinetic_time = ui->dsbKineticTime->value();
    settings.ccd_min_temp = ui->spbMinTemp->value();
    settings.ccd_max_temp = ui->spbMaxTemp->value();
    settings.ccd_target_temp = ui->spbTargetTemp->value();
    settings.ccd_pre_amp_gain = ui->spbPreAmpGain->value();
    settings.ccd_em_gain_mode = ui->spbEMGainMode->value();
    settings.ccd_em_gain = ui->spbEMGain->value();

    // write ccd width and height into global variables
    ccdX = settings.ccd_width;
    ccdY = settings.ccd_height;

    // ... sdd settings ...
    settings.sdd_sebitcount = ui->spbSebitcount->value();
    settings.sdd_filter = ui->cmbFilter->currentIndex();
    settings.sdd_energyrange = ui->cmbEnergyrange->currentIndex();
    settings.sdd_tempmode = ui->cmbTempmode->currentIndex();
    settings.sdd_zeropeakperiod = ui->spbZeroPeakPeriod->value();
    settings.sdd_checktemperature = ui->cmbCheckTemperature->currentIndex();

    // translate combo box indexes to sdd acquisionmodes
    if (ui->cmbAcquisionMode->currentIndex() == 0) {
        settings.sdd_acquisitionmode = 0;
    }

    if (ui->cmbAcquisionMode->currentIndex() == 1) {
        settings.sdd_acquisitionmode = 4;
    }

    settings.sdd_sdd1 = ui->chbSDD1->isChecked();
    settings.sdd_sdd2 = ui->chbSDD2->isChecked();
    settings.sdd_sdd3 = ui->chbSDD3->isChecked();
    settings.sdd_sdd4 = ui->chbSSD4->isChecked();

    // ... sample settings ...
    settings.sample_name = ui->txtSampleName->text().toStdString();
    settings.sample_type = ui->txtSampleType->text().toStdString();
    settings.sample_note = ui->txtSampleNote->toPlainText().toStdString();
    settings.sample_width = ui->dsbSampleWidth->value();
    settings.sample_height = ui->dsbSampleHeight->value();
    settings.sample_rotation_angle = ui->dsbSampleRotationAngle->value();

    // ... source settings ...
    settings.source_name = ui->txtSourceName->text().toStdString();
    settings.source_probe = ui->cmbSourceProbe->currentText().toStdString();
    settings.source_type = ui->cmbSourceType->currentText().toStdString();

    // ... additional settings ...
    settings.notes = ui->txtScanNote->toPlainText().toStdString();
    settings.userdata = ui->txtUserInfo->toPlainText().toStdString();

    // create scan thread object
    Scan = new scan(settings);
    // connect thread-signal "sendDeviceStatusToGUI" to GUI-slot "showDeviceStatus" because we want to show current device status in GUI
    QObject::connect(Scan,SIGNAL(sendDeviceStatusToGUI(QString, QString)),this,SLOT(showDeviceStatus(QString, QString)));
    QObject::connect(Scan,SIGNAL(sendPreviewDataToGUI(std::string, std::string)),this,SLOT(showPreview(std::string, std::string)));
    QObject::connect(Scan,SIGNAL(sendROIDataToGUI(std::string, std::string)),this,SLOT(showROI(std::string, std::string)));
    QObject::connect(Scan,SIGNAL(sendScanFinished()),this,SLOT(on_ScanFinished()));

    // start scan thread
    Scan->start();
}

// this function is invoked every time a the scan thread receives a "ready"-signal from CCD/SDD/datasink-endpoint
void GUI::showDeviceStatus(QString device, QString status) {
    if (device == "ccd") {
        if ((status == "connection ready") || (status == "detector ready")) {
            // set CCD checkbox = true
            ui->chbCCD->setChecked(true);
        } else {
            // set CCD checkbox = false
            ui->chbCCD->setChecked(false);
        }
    }
    if (device == "sdd") {
        if ((status == "connection ready") || (status == "detector ready")) {
            ui->chbSDD->setChecked(true);
        } else {
            ui->chbSDD->setChecked(false);
        }
    }
    if (device == "datasink") {
        if (status == "ready") {
            ui->chbDataSink->setChecked(true);
        } else {
            ui->chbDataSink->setChecked(false);
        }
    }
}

// this function is invoked every time new ROI data has been received
void GUI::showROI(std::string element, std::string previewdata) {

        // declare and initialize variables for the maximum and minimum pixel values
        u_int32_t max = 0;
        u_int32_t min = 0xFFFFFFFF;

        // declare and define counter variables
        unsigned long count = 0;

        unsigned long pixelcount = scanX*scanY;
        unsigned long bytecount = pixelcount*4;

        // declare an array for the image data of the preview image
        uint32_t previewimage[pixelcount];

        // iterate through all pixels
        for (unsigned long i = 0; i < bytecount; i=i+4) {
            if ((i+3) < previewdata.length()) {

                // create the 32 bit value from 4 bytes
                uint32_t pixelvalue = ((uint32_t)(previewdata[i+3]<<24)) | (uint8_t)previewdata[i+2]<<16 | (uint8_t)previewdata[i+1]<<8 | (uint8_t)previewdata[i];

                // calculate maximum and minimum pixel values
                if (pixelvalue > max) {
                    max = pixelvalue;
                }
                if (pixelvalue < min) {
                    min = pixelvalue;
                }

                // set preview image pixel value
                previewimage[count] = pixelvalue;
            } else {
                // if pixel not received by now, set pixel value to zero
                previewimage[count] = 0;
            }
            // increment counter variable
            count++;
        }

        // calculate factor to "shrink" 32 bit values to 16 bit value
        // for this use the difference between the maximum and the minimum pixel value
        // to make use of the maximal possible dynamic range
        double factor =(double)65535/(max-min);

        // create an QImage object
        QImage image = QImage(scanX, scanY, QImage::Format_Grayscale16);

        // initalize counter variable
        count = 0;

        // iterate though the complete image data
        for (int j = 0; j < scanY; ++j) {

          // declare pointer to memory position where QImage image data line starts
          quint16 *dst = (quint16*)(image.bits() + j * image.bytesPerLine());

          // write all pixel values of the current line to the memory
          for (int i = 0; i < scanX; ++i) {

            // write pixel value that was reduced by the absolute value of the minimum value and scaled by factor
            // to the belonging memory position, use floor() to make sure only integers are written to the memory
            dst[i] = floor((previewimage[i + j * scanX]-min)*factor);

            // increment counter
            count++;

          }
        }

        // create new pixmap object for the scaled image data
        QPixmap scaledpixmp;

        // scale image according to the pixel ratio to make sure that always the full preview image is visible in the user interface
        // if image width >= image height
        if (scanX>=scanY) {
          // set fixed width
          scaledpixmp = QPixmap::fromImage(image).scaledToWidth(ui->ROIPPreview->width());
        }

        // if image width < image height
        if (scanX<scanY) {
          // set fixed height
          scaledpixmp = QPixmap::fromImage(image).scaledToHeight(ui->ROIPPreview->height());
        }

        // set the pixmap of the corresponding preview label in the user interface
        if (element == "P") {
            ui->ROIPPreview->setPixmap(scaledpixmp);
        } else if (element == "O") {
            ui->ROIOPreview->setPixmap(scaledpixmp);
        } else if (element == "C") {
            ui->ROICPreview->setPixmap(scaledpixmp);
        } else if (element == "Zero") {
            ui->ROIZeroPreview->setPixmap(scaledpixmp);
        } else if (element == "Fe") {
            ui->ROIFePreview->setPixmap(scaledpixmp);
        }
}

void GUI::showPreview(std::string previewtype, std::string previewdata) {
    // if a transmission preview image has been received
    if (previewtype == "stxm") {

        // declare and initialize variables for the maximum and minimum pixel values
        u_int32_t max = 0;
        u_int32_t min = 0xFFFFFFFF;

        // declare and define counter variables
        unsigned long count = 0;

        unsigned long pixelcount = scanX*scanY;
        unsigned long bytecount = pixelcount*4;

        // declare an array for the image data of the preview image
        uint32_t previewimage[pixelcount];

        // iterate though the complete image data
        for (unsigned long i = 0; i < bytecount; i=i+4) {
            if ((i+3) < previewdata.length()) {
                // create the 32 bit value from 4 bytes
                uint32_t pixelvalue = ((uint32_t)(previewdata[i+3]<<24)) | (uint8_t)previewdata[i+2]<<16 | (uint8_t)previewdata[i+1]<<8 | (uint8_t)previewdata[i];

                // calculate maximum and minimum pixel values
                if (pixelvalue > max) {
                    max = pixelvalue;
                }
                if (pixelvalue < min) {
                    min = pixelvalue;
                }

                // set preview image pixel value
                previewimage[count] = pixelvalue;
            } else {
                // if pixel not received by now, set pixel value to zero
                previewimage[count] = 0;
            }
            // increment counter variable
            count++;
        }

        // calculate factor to "shrink" 32 bit values to 16 bit value
        // for this use the difference between the maximum and the minimum pixel value
        // to make use of the maximal possible dynamic range
        double factor =(double)65535/(max-min);

        // create an QImage object
        QImage image = QImage(scanX, scanY, QImage::Format_Grayscale16);

        // declare and initialize a counter variable
        count = 0;

        // iterate though the complete image data
        for (int j = 0; j < scanY; ++j) {

          // declare pointer to memory position where QImage image data line starts
          quint16 *dst = (quint16*)(image.bits() + j * image.bytesPerLine());

          // write all pixel values of the current line to the memory
          for (int i = 0; i < scanX; ++i) {

            // write pixel value that was reduced by the absolute value of the minimum value and scaled by factor
            // to the belonging memory position, use floor() to make sure only integers are written to the memory
            dst[i] = floor((previewimage[i + j * scanX]-min)*factor);

            // increment counter
            count++;
          }
        }

        // create another QPixmap for the scaled data
        QPixmap scaledpixmp;

        // scale image according to the pixel ratio to make sure that always the full preview image is visible in the user interface
        // if image width >= image height
        if (scanX>=scanY) {
          // set fixed width
          scaledpixmp = QPixmap::fromImage(image).scaledToWidth(ui->stxmPreview->width());
        }

        // if image width < image height
        if (scanX<scanY) {
          // set fixed height
          scaledpixmp = QPixmap::fromImage(image).scaledToHeight(ui->stxmPreview->height());
        }

        // show the preview image in the user interface
        ui->stxmPreview->setPixmap(scaledpixmp);
    }

    // if a ccd preview image has been received
    if (previewtype == "ccd") {

        // declare and initialize maximum und minimum values for 16 bit image data
        uint16_t max = 0;
        uint16_t min = 65535;

        // declare and define counter variables
        uint32_t ccdpixelcount = ccdX*ccdY;
        uint32_t ccddatabytecount = ccdpixelcount * 2;
        uint32_t imagex[ccdpixelcount];

        int counterx = 0;

        // iterate through the complete image data
        for (int i = 0; i < ccddatabytecount; i=i+2) {

                // build a 16 bit pixel value from two bytes
                uint16_t value = ((uint16_t) (uint8_t)previewdata[i+1] << 8) | (uint8_t)previewdata[i];

                // calculate maximum and minimum pixel values
                if (value > max) {
                    max = value;
                }

                if (value < min) {
                    min = value;
                }

                // write pixel value to image data array
                imagex[counterx] = value;

                // increment the counter
                counterx++;
        }

        // declare and define factor as 1 as long as maximum value equals minimum value
        double factor = 1;

        // if maximum and minimum are noch the same, calculate factor
        if (max != min) {
          factor = abs(65535/(max-min));
        }

                // create an QImage object
                QImage image = QImage(ccdX, ccdY, QImage::Format_Grayscale16);

                // iterate though the complete image data
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
                        dst[i] = (pixelval-min)*factor;
                   }
                }

        // create QPixmap object from image data
        QPixmap pixmp = QPixmap::fromImage(image);

        // show preview image in user interface
        ui->ccdPreview->setPixmap(pixmp);
    }

}

// this function is invoked if user clicks on the delete energy (-) button in the user interface
void GUI::on_cmdDeleteEnergy_clicked()
{
    // delete selected energy entries
    QList<QListWidgetItem*> items = ui->lstEnergies->selectedItems();

    foreach(QListWidgetItem * item, items)
    {
        delete ui->lstEnergies->takeItem(ui->lstEnergies->row(item));
    }
}

// this function is invoked if user clicks on the add energy (+) button in the user interface
void GUI::on_cmdAddEnergy_clicked()
{
    // add energy value from spinBox to energy list
    ui->lstEnergies->addItem(QString::number(ui->spbNewEnergy->value()));
}

// this function is invoked if the scan is finished
void GUI::on_ScanFinished()
{
    // enable scan note textbox and scan note save button
    ui->txtScanNote->setDisabled(false);
    ui->cmdSaveScanNote->setDisabled(false);

    // give info message
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText("Scan finished!");
    msgBox.exec();
}

// this function is invoked if the save scan note button is clicked
void GUI::on_cmdSaveScanNote_clicked()
{
    // get scan note from GUI and write it into scanthreads' scannote variable
    Scan->scannote = ui->txtScanNote->toPlainText().toStdString();
}

// this function is invoked if the stop button is clicked
void GUI::on_cmdStopScan_clicked()
{
    // set scanthreads' stopscan variable = true
    Scan->stopscan = true;
}

// this function is invoked if the pause/resume button is clicked
void GUI::on_cmdPauseScan_clicked()
{
    // if pause button has been clicked
    if (ui->cmdPauseScan->text() == "Pause") {
        // set scanthreads' pausescan variable = true
        Scan->pausescan = true;
        // change pause button text to "Resume"
        ui->cmdPauseScan->setText("Resume");
    } else {
        // set scanthreads' resumescan variable = true
        Scan->resumescan = true;
        // change resume button text to "Pause"
        ui->cmdPauseScan->setText("Pause");
    }
}

// this function is invoked if the user changes the CCD width in the user interface
void GUI::on_spbCCDWidth_valueChanged(int arg1)
{
    // calculate pixel count value and show it in user interface
    ui->txtPixelCount->setText(QString::number(ui->spbCCDHeight->value()*ui->spbCCDWidth->value()));
}

// this function is invoked if the user changes the CCD height in the user interface
void GUI::on_spbCCDHeight_valueChanged(int arg1)
{
    // calculate pixel count value and show it in user interface
    ui->txtPixelCount->setText(QString::number(ui->spbCCDHeight->value()*ui->spbCCDWidth->value()));
}

// this function is invoked if the user changes current shown tab page in the user interface
void GUI::on_tabWidget_currentChanged(int index)
{
    // calculate number of scans value and show it in user interface
    ui->spbNumberOfScans->setValue(ui->spbScanWidth->value()*ui->spbScanHeight->value());

    // calculate pixel count and show it in user interface
    ui->txtPixelCount->setText(QString::number(ui->spbCCDHeight->value()*ui->spbCCDWidth->value()));
}
