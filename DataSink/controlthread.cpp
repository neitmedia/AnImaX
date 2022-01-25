#include "controlthread.h"
#include <QDebug>
#include <stdio.h>
#include <QThread>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <unistd.h>

#include <zmq.hpp>

// constructor of controlThread
controlThread::controlThread(QString s): ip(s)
{
    // register metatypes to make them usable as function parameters
    qRegisterMetaType<imagepixeldata>( "imagepixeldata" );
    qRegisterMetaType<settingsdata>( "settingsdata" );
    qRegisterMetaType<metadata>("metadata");
    qRegisterMetaType<imagepreviewdata>( "imagepreviewdata" );
}

// function that is triggered by mainwindow.cpp / data sink GUI thread to tell the control thread that new transmission preview data is available for publishing
void controlThread::setCurrentSTXMPreview(imagepreviewdata stxmprev) {

    // write preview data to global stxmpreview variable
    stxmpreview = stxmprev;

    // set newSTXMpreview = true, so main loop in thread knows that new transmission preview data is ready to be published
    newSTXMpreview = true;

}

// function that is triggered by mainwindow.cpp / data sink GUI thread to tell the control thread that new ccd image preview data is available for publishing
void controlThread::setCurrentCCDImage(std::string ccdprev) {

    // write preview data to global ccdpreview variable
    ccdpreview = ccdprev;

    // set newCCDpreview = true, so main loop in thread knows that new ccd image preview data is ready to be published
    newCCDpreview = true;

}

// function that is triggered by mainwindow.cpp / data sink GUI thread to tell the control thread that there new ROI preview data is available for publishing
void controlThread::setCurrentROIs(roidata ROIs) {

    // write preview data to global ROIdata variable of type roidata (QMap<std::string, QVector<uint32_t>>)
    ROIdata = ROIs;

    // set newROIs = true, so main loop in thread knows that new ROI preview data is ready to be published
    newROIs = true;
}

// run function is started when thread is launched
void controlThread::run()
{
    // give out some debug info
    std::cout<<"controller thread is running..."<<std::endl;

    // declare zmq context variable
    zmq::context_t ctx(1);

    // declare zmq subscriber socket object
    zmq::socket_t subscriber(ctx, zmq::socket_type::sub);

    // connect subscriber socket to given IP
    subscriber.connect("tcp://"+ip.toStdString());

    // subscribe to messages with envelope "settings" that contain all settings that are relevant to the scan
    // message with "settings" envelope is sent only at the start of a scan
    subscriber.set(zmq::sockopt::subscribe, "settings");

    // subscribe to messages with envelope "metadata" that contain metadata
    // message with "metadata" envelope is sent at the start of every scan part (at the start of every new energy scan)
    subscriber.set(zmq::sockopt::subscribe, "metadata");

    // subscribe to messages with envelope "metadata" that contain post-scan notes
    subscriber.set(zmq::sockopt::subscribe, "scannote");

    // subscribe to messages with envelope "scanstatus" that contain user requested status changes
    subscriber.set(zmq::sockopt::subscribe, "scanstatus");

    // declare zmq publisher socket object GUI
    zmq::socket_t gui(ctx, zmq::socket_type::pub);

    // declare and initalize variable "filecreated"
    bool filecreated = false;

    /* BEGIN MAIN LOOP */

    while (!stop) {

        // declare message envelope variable
        zmq::message_t env;

        // receive message envelope if data is available
        // !!IMPORTANT!! -> do not wait if no data is available
        (void)subscriber.recv(env, zmq::recv_flags::dontwait);

        // if data has been received, process it
        if (env.size() > 0) {

            // cast envelope data to string
            std::string env_str = std::string(static_cast<char*>(env.data()), env.size());

            // give out some debug info
            std::cout << "Received envelope: "<<env_str<< std::endl;

            // declare message object
            zmq::message_t msg;

            // receive message content
            (void)subscriber.recv(msg);

            // give out some debug info
            std::cout << "Received data" << std::endl;

            // if received message has the envelope "settings"
            if (env_str == "settings") {

                // declare animax:Measurement object Measurement
                animax::Measurement Measurement;

                // parse / deserialize message data into Measurement protobuf object Measurement
                Measurement.ParseFromArray(msg.data(), msg.size());

                /* START GETTING SETTINGS FROM PROTOBUF */

                // get settings from protobuf and write them into settingsdata variable "settings"
                settingsdata settings;

                // general scan settings
                settings.scanWidth = Measurement.width();
                settings.scanHeight = Measurement.height();
                settings.x_step_size = Measurement.x_step_size();
                settings.y_step_size = Measurement.y_step_size();
                settings.scantitle = Measurement.scantitle();
                settings.energycount = Measurement.energy_count();
                settings.roidefinitions = Measurement.roidefinitions();
                settings.scantype = Measurement.scantype();
                settings.save_path = Measurement.save_path();
                settings.save_file = Measurement.save_file();
                settings.file_compression = Measurement.file_compression();
                settings.file_compression_level = Measurement.file_compression_level();

                // give out some debug info
                std::cout<<"compression mode:"<<settings.file_compression<<std::endl;

                // network settings
                settings.ccdIP = Measurement.ccdip();
                settings.ccdPort = Measurement.ccdport();
                settings.sddIP = Measurement.sddip();
                settings.sddPort = Measurement.sddport();
                settings.datasinkIP = Measurement.datasinkip();
                settings.datasinkPort = Measurement.datasinkport();

                // ccd settings
                settings.ccd_binning_x = Measurement.ccd_binning_x();
                settings.ccd_binning_y = Measurement.ccd_binning_y();
                settings.ccd_height = Measurement.ccd_height();
                settings.ccd_width = Measurement.ccd_width();
                settings.ccd_pixelcount = Measurement.ccd_pixelcount();
                settings.ccd_frametransfer_mode = Measurement.ccd_frametransfer_mode();
                settings.ccd_number_of_accumulations = Measurement.ccd_number_of_accumulations();
                settings.ccd_number_of_scans = Measurement.ccd_number_of_scans();
                settings.ccd_set_kinetic_cycle_time = Measurement.ccd_set_kinetic_cycle_time();
                settings.ccd_read_mode = Measurement.ccd_read_mode();
                settings.ccd_acquisition_mode = Measurement.ccd_acquisition_mode();
                settings.ccd_shutter_mode = Measurement.ccd_shutter_mode();
                settings.ccd_shutter_output_signal = Measurement.ccd_shutter_output_signal();
                settings.ccd_shutter_open_time = Measurement.ccd_shutter_open_time();
                settings.ccd_shutter_close_time = Measurement.ccd_shutter_close_time();
                settings.ccd_triggermode = Measurement.ccd_triggermode();
                settings.ccd_exposure_time = Measurement.ccd_exposure_time();
                settings.ccd_accumulation_time = Measurement.ccd_accumulation_time();
                settings.ccd_kinetic_time = Measurement.ccd_kinetic_time();
                settings.ccd_min_temp = Measurement.ccd_min_temp();
                settings.ccd_max_temp = Measurement.ccd_max_temp();
                settings.ccd_target_temp = Measurement.ccd_target_temp();
                settings.ccd_pre_amp_gain = Measurement.ccd_pre_amp_gain();
                settings.ccd_em_gain_mode = Measurement.ccd_em_gain_mode();
                settings.ccd_em_gain = Measurement.ccd_em_gain();

                // sdd settings
                settings.sdd_sebitcount = Measurement.sdd_sebitcount();
                settings.sdd_filter = Measurement.sdd_filter();
                settings.sdd_energyrange = Measurement.sdd_energyrange();
                settings.sdd_tempmode = Measurement.sdd_tempmode();
                settings.sdd_zeropeakperiod = Measurement.sdd_zeropeakperiod();
                settings.sdd_acquisitionmode = Measurement.sdd_acquisitionmode();
                settings.sdd_checktemperature = Measurement.sdd_checktemperature();
                settings.sdd_sdd1 = Measurement.sdd_sdd1();
                settings.sdd_sdd2 = Measurement.sdd_sdd2();
                settings.sdd_sdd3 = Measurement.sdd_sdd3();
                settings.sdd_sdd4 = Measurement.sdd_sdd4();

                // sample settings
                settings.sample_name = Measurement.sample_name();
                settings.sample_type = Measurement.sample_type();
                settings.sample_width = Measurement.sample_width();
                settings.sample_height = Measurement.sample_height();
                settings.sample_rotation_angle = Measurement.sample_rotation_angle();
                settings.sample_note = Measurement.sample_note();

                // source settings
                settings.source_name = Measurement.source_name();
                settings.source_probe = Measurement.source_probe();
                settings.source_type = Measurement.source_type();

                // additional settings
                settings.notes = Measurement.notes();
                settings.userdata = Measurement.userdata();

                /* END GETTING SETTINGS FROM PROTOBUF */

                // if connected variable is false it means that publisher is not connected
                if (!connected) {
                    // start publisher, bind to *:PORT
                    gui.bind("tcp://*:"+std::to_string(settings.datasinkPort));
                    // set connected variable = true so thread does not try to bind a second time
                    connected = true;
                }

                // give out some debug info
                std::cout<<"width: "<<settings.scanWidth<<std::endl;
                std::cout<<"height: "<<settings.scanHeight<<std::endl;
                std::cout<<"ccd width: "<<settings.ccd_width<<std::endl;
                std::cout<<"ccd height: "<<settings.ccd_height<<std::endl;

                // if filecreated = false it means that file has not been created by now (datasink GUI thread did not get scan settings)
                if (!filecreated) {
                    // set filecreated = true, so only one data file per scan is created
                    filecreated = true;
                    // send settings to datasink GUI thread
                    emit sendSettingsToGUI(settings);
                }

                // if ccd and sdd are ready, datasink is also ready
                if ((this->ccdReady) && (this->sddReady)) {
                    // send "statusdata" envelope
                    gui.send(zmq::str_buffer("statusdata"), zmq::send_flags::sndmore);
                    // send ready signal
                    gui.send(zmq::str_buffer("ready"), zmq::send_flags::none);
                }

            // if received message has the envelope "metadata"
            } else if (env_str == "metadata") {
                // if waitForMetadata is true, thread watches for new metadata messages
                // [important at the beginning of a scan and at the beginning of every energy scan]
                if (waitForMetadata) {

                    // declare animax::Metadata object Metadata
                    animax::Metadata Metadata;

                    // parse / deserialize data from message content
                    Metadata.ParseFromArray(msg.data(), msg.size());

                    // declare metadata struct of type metadata (see structs.h)
                    metadata metadata;

                    // write values from protobuf to metadata struct
                    metadata.acquisition_number = Metadata.acquisition_number();
                    metadata.acquisition_time = Metadata.acquisition_time();
                    metadata.set_energy = Metadata.set_energy();
                    metadata.beamline_energy = Metadata.beamline_energy();
                    metadata.ringcurrent = Metadata.ringcurrent();
                    metadata.horizontal_shutter = Metadata.horizontal_shutter();
                    metadata.vertical_shutter = Metadata.vertical_shutter();

                    // send metadata to datasink GUI thread
                    emit sendMetadataToGUI(metadata);

                    // for now, controlthread should not check for new metadata messages, so set waitForMetadata = false
                    waitForMetadata = false;
                }

            // if received message has the envelope "scannote", new post scan note has to be written to the files
            } else if (env_str == "scannote") {

                // give out some debug info
                std::cout<<"received scan note!"<<std::endl;

                // declare animax::scannote object ScanNote
                animax::scannote ScanNote;

                // parse/deserialize data from message to protobuf object ScanNote
                ScanNote.ParseFromArray(msg.data(), msg.size());

                // send note data to datasink GUI thread
                emit sendScanNoteToGUI(ScanNote.text());

            // if received message has the envelope "scannote", new post scan note has to be written to the files
            } else if (env_str == "scanstatus") {

                // give out some debug info
                std::cout<<"received scan status!"<<std::endl;

                // declare animax::scanstatus object scanstatus
                animax::scanstatus scanstatus;

                // parse / deserialize data from message into scanstatus
                scanstatus.ParseFromArray(msg.data(), msg.size());

                // send scan status to datasink GUI thread
                emit sendScanStatusToGUI(scanstatus.status());
            }

        }

        // check if new transmission image preview data is available for sending
        if (newSTXMpreview) {

            // declare animax::preview object Preview
            animax::preview Preview;

            // set Preview type to "stxm"
            Preview.set_type("stxm");

            // write preview data into animax::preview object Preview
            Preview.set_previewdata(stxmpreview.constData(), stxmpreview.count()*sizeof(uint32_t));

            // declare and define size variable with length of data that should be sent
            size_t size = Preview.ByteSizeLong();

            // allocate memory for the send buffer
            void *sendbuffer = malloc(size);

            // serialize preview data into protocol buffer
            Preview.SerializeToArray(sendbuffer, size);

            // declare zmq message object request
            zmq::message_t request(size);

            // write data from sendbuffer into request object
            memcpy ((void *) request.data (), sendbuffer, size);

            // send "preview" envelope
            gui.send(zmq::str_buffer("previewdata"), zmq::send_flags::sndmore);

            // send preview data
            gui.send(request, zmq::send_flags::none);

            // free previously allocated memory
            free(sendbuffer);

            // set newSTXMpreview = false, because newest preview data has now been sent
            newSTXMpreview = false;

            // give out some debug info
            std::cout<<"sent new stxm preview data"<<std::endl;
        }

        // check if new ccd image preview data is available for sending
        if (newCCDpreview) {

            // declare animax::preview object Preview
            animax::preview Preview;

            // set preview type to "ccd"
            Preview.set_type("ccd");

            // write ccd image preview data to animax::preview object Preview
            Preview.set_previewdata(ccdpreview);

            // declare and define size variable with length of data that should be sent
            size_t size = Preview.ByteSizeLong();

            // allocate necessary memory
            void *sendbuffer = malloc(size);

            // serialize data
            Preview.SerializeToArray(sendbuffer, size);

            // declare zmq request message
            zmq::message_t request(size);

            // write data into request
            memcpy ((void *) request.data (), sendbuffer, size);

            // send "preview" envelope
            gui.send(zmq::str_buffer("previewdata"), zmq::send_flags::sndmore);

            // send preview data
            gui.send(request, zmq::send_flags::none);

            // free previously allocated memory
            free(sendbuffer);

            // set newCCDpreview = false, because newest preview data has now been sent
            newCCDpreview = false;

            // give out some debug info
            std::cout<<"sent new ccd preview data"<<std::endl;
        }

        // check if new ROI preview data is available for sending
        if (newROIs) {

            // get keys from ROIdata
            auto const ROIkeys = ROIdata.keys();

            // iterate through ROIs
            for (std::string e : ROIkeys) {

                    // declare animax::ROI object roi
                    animax::ROI roi;

                    // write element info to roi object
                    roi.set_element(e);

                    // write ROI preview data into roi object
                    roi.set_roidata(ROIdata[e].constData(), ROIdata[e].count()*sizeof(uint32_t));

                    // declare and define size variable with length of data that should be sent
                    size_t size = roi.ByteSizeLong();

                    // allocate necessary memory
                    void *sendbuffer = malloc(size);

                    // serialize data
                    roi.SerializeToArray(sendbuffer, size);

                    // declare zmq request message
                    zmq::message_t request(size);

                    // write data into request
                    memcpy ((void *) request.data (), sendbuffer, size);

                    // send "roidata" envelope
                    gui.send(zmq::str_buffer("roidata"), zmq::send_flags::sndmore);

                    // send data
                    gui.send(request, zmq::send_flags::none);

                    // free previously allocated memory
                    free(sendbuffer);
            }

            // set newROIs = false, because newest ROI preview data has now been sent
            newROIs = false;

        }

        // check if a part of NEXAFS scan is ready (partScanFinished was set true by datasink GUI thread)
        // if so, publish status message "part finished"
        if (partScanFinished) {

            // declare animax::scanstatus object scanstatus
            animax::scanstatus scanstatus;

            // set scanstatus.status = "part finished"
            scanstatus.set_status("part finished");

            // get size of message
            size_t size = scanstatus.ByteSizeLong();

            // allocate necessary memory
            void *sendbuffer = malloc(size);

            // serialize data
            scanstatus.SerializeToArray(sendbuffer, size);

            // declare zmq request message
            zmq::message_t request(size);

            // write data into request
            memcpy ((void *) request.data (), sendbuffer, size);

            // send "scanstatus" envelope
            gui.send(zmq::str_buffer("scanstatus"), zmq::send_flags::sndmore);

            // send data
            gui.send(request, zmq::send_flags::none);

            // free previously allocated memory
            free(sendbuffer);

            // set partScanFinished = false, so "part finished" is sent out only one time
            partScanFinished = false;

            // set waitForMetadata = true, so controlthread looks out for new metadata
            waitForMetadata = true;
        }

        // check if a whole scan is ready (wholeScanFinished was set true by datasink GUI thread)
        // if so, publish status message "whole scan finished"
        if (wholeScanFinished) {

            // declare animax::scanstatus object scanstatus
            animax::scanstatus scanstatus;

            // set status to "whole scan finished"
            scanstatus.set_status("whole scan finished");

            // get message size
            size_t size = scanstatus.ByteSizeLong();

            // allocated necessary memory
            void *sendbuffer = malloc(size);

            // serialize data
            scanstatus.SerializeToArray(sendbuffer, size);

            // declare zmq request message
            zmq::message_t request(size);

            // write data into request
            memcpy ((void *) request.data (), sendbuffer, size);

            // send "scanstatus" envelope
            gui.send(zmq::str_buffer("scanstatus"), zmq::send_flags::sndmore);

            // send status data
            gui.send(request, zmq::send_flags::none);

            // free previously allocated memory
            free(sendbuffer);

            // set wholeScanFinished = false so status message is sent only one time
            wholeScanFinished = false;
        }
    }

    /* END MAIN LOOP */
}
