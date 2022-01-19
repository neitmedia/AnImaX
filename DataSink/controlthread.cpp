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

controlThread::controlThread(QString s): ip(s)
{
    qRegisterMetaType<imagepixeldata>( "imagepixeldata" );
    qRegisterMetaType<settingsdata>( "settingsdata" );
    qRegisterMetaType<metadata>("metadata");
    qRegisterMetaType<imagepreviewdata>( "imagepreviewdata" );
}

void controlThread::setCurrentSTXMPreview(imagepreviewdata stxmprev) {
    stxmpreview = stxmprev;
    newSTXMpreview = true;
}

void controlThread::setCurrentCCDImage(std::string ccdprev) {
    ccdpreview = ccdprev;
    newCCDpreview = true;
}

void controlThread::setCurrentROIs(roidata ROIs) {
    ROIdata = ROIs;
    newROIs = true;
}

void controlThread::run()
{
    std::cout<<"controller thread is running..."<<std::endl;
    zmq::context_t ctx(1);
    // Prepare subscriber
    zmq::socket_t subscriber(ctx, zmq::socket_type::sub);
    subscriber.connect("tcp://"+ip.toStdString());
    subscriber.set(zmq::sockopt::subscribe, "settings");
    subscriber.set(zmq::sockopt::subscribe, "metadata");
    subscriber.set(zmq::sockopt::subscribe, "scannote");
    subscriber.set(zmq::sockopt::subscribe, "scanstatus");

    zmq::socket_t gui(ctx, zmq::socket_type::pub);

    bool filecreated = false;
    while (!stop) {
        zmq::message_t env;
        (void)subscriber.recv(env, zmq::recv_flags::dontwait);
        if (env.size() > 0) {
            std::string env_str = std::string(static_cast<char*>(env.data()), env.size());
            std::cout << "Received envelope: "<<env_str<< std::endl;
            zmq::message_t msg;
            (void)subscriber.recv(msg);
            std::cout << "Received data" << std::endl;

            if (env_str == "settings") {
                animax::Measurement Measurement;
                Measurement.ParseFromArray(msg.data(), msg.size());

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

                std::cout<<"compression mode:"<<settings.file_compression<<std::endl;

                // network settings
                settings.ccdIP = Measurement.ccdip();
                settings.ccdPort = Measurement.ccdport();
                settings.sddIP = Measurement.sddip();
                settings.sddPort = Measurement.sddport();
                settings.datasinkIP = Measurement.datasinkip();
                settings.datasinkPort = Measurement.datasinkport();

                // ccd settings
                settings.binning_x = Measurement.binning_x();
                settings.binning_y = Measurement.binning_y();
                settings.ccdHeight = Measurement.ccdheight();
                settings.ccdWidth = Measurement.ccdwidth();
                settings.pixelcount = Measurement.pixelcount();
                settings.frametransfer_mode = Measurement.frametransfer_mode();
                settings.number_of_accumulations = Measurement.number_of_accumulations();
                settings.number_of_scans = Measurement.number_of_scans();
                settings.set_kinetic_cycle_time = Measurement.set_kinetic_cycle_time();
                settings.read_mode = Measurement.read_mode();
                settings.acquision_mode = Measurement.acquision_mode();
                settings.shutter_mode = Measurement.shutter_mode();
                settings.shutter_output_signal = Measurement.shutter_output_signal();
                settings.shutter_open_time = Measurement.shutter_open_time();
                settings.shutter_close_time = Measurement.shutter_close_time();
                settings.triggermode = Measurement.triggermode();
                settings.exposure_time = Measurement.exposure_time();
                settings.accumulation_time = Measurement.accumulation_time();
                settings.kinetic_time = Measurement.kinetic_time();
                settings.min_temp = Measurement.min_temp();
                settings.max_temp = Measurement.max_temp();
                settings.target_temp = Measurement.target_temp();
                settings.pre_amp_gain = Measurement.pre_amp_gain();
                settings.em_gain_mode = Measurement.em_gain_mode();
                settings.em_gain = Measurement.em_gain();

                // sdd settings
                settings.sebitcount = Measurement.sebitcount();
                settings.filter = Measurement.filter();
                settings.energyrange = Measurement.energyrange();
                settings.tempmode = Measurement.tempmode();
                settings.zeropeakperiod = Measurement.zeropeakperiod();
                settings.acquisitionmode = Measurement.acquisitionmode();
                settings.checktemperature = Measurement.checktemperature();
                settings.sdd1 = Measurement.sdd1();
                settings.sdd2 = Measurement.sdd2();
                settings.sdd3 = Measurement.sdd3();
                settings.sdd4 = Measurement.sdd4();

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

                if (!connected) {
                    // Prepare publisher
                    gui.bind("tcp://*:"+std::to_string(settings.datasinkPort));
                    connected = true;
                }

                // give out some debug info
                std::cout<<"width: "<<settings.scanWidth<<std::endl;
                std::cout<<"height: "<<settings.scanHeight<<std::endl;
                std::cout<<"ccd width: "<<settings.ccdWidth<<std::endl;
                std::cout<<"ccd height: "<<settings.ccdHeight<<std::endl;

                if (!filecreated) {
                    filecreated = true;
                    emit sendSettingsToGUI(settings);
                }

                if ((this->ccdReady) && (this->sddReady)) {
                    gui.send(zmq::str_buffer("statusdata"), zmq::send_flags::sndmore);
                    gui.send(zmq::str_buffer("ready"), zmq::send_flags::none);
                }
            } else if (env_str == "metadata") {
                if (waitForMetadata) {
                    animax::Metadata Metadata;
                    Metadata.ParseFromArray(msg.data(), msg.size());

                    metadata metadata;
                    metadata.acquisition_number = Metadata.acquisition_number();
                    metadata.acquisition_time = Metadata.acquisition_time();
                    metadata.set_energy = Metadata.set_energy();
                    metadata.beamline_energy = Metadata.beamline_energy();
                    metadata.ringcurrent = Metadata.ringcurrent();
                    metadata.horizontal_shutter = Metadata.horizontal_shutter();
                    metadata.vertical_shutter = Metadata.vertical_shutter();

                    emit sendMetadataToGUI(metadata);
                    waitForMetadata = false;
                }
            } else if (env_str == "scannote") {
                std::cout<<"received scan note!"<<std::endl;
                animax::scannote ScanNote;
                ScanNote.ParseFromArray(msg.data(), msg.size());
                emit sendScanNoteToGUI(ScanNote.text());

            } else if (env_str == "scanstatus") {
                std::cout<<"received scan status!"<<std::endl;
                animax::scanstatus scanstatus;
                scanstatus.ParseFromArray(msg.data(), msg.size());
                emit sendScanStatusToGUI(scanstatus.status());
            }

        }

        // check if stxm preview needs to be sent
        if (newSTXMpreview) {
            animax::preview Preview;
            Preview.set_type("stxm");
            Preview.set_previewdata(stxmpreview.constData(), stxmpreview.count()*sizeof(uint32_t));
            // serialize data and write into ZMQ request
            size_t size = Preview.ByteSizeLong();
            void *sendbuffer = malloc(size);
            Preview.SerializeToArray(sendbuffer, size);
            zmq::message_t request(size);
            memcpy ((void *) request.data (), sendbuffer, size);

            // send "preview" envelope with preview data content
            gui.send(zmq::str_buffer("previewdata"), zmq::send_flags::sndmore);
            gui.send(request, zmq::send_flags::none);

            newSTXMpreview = false;
            std::cout<<"sent new stxm preview data"<<std::endl;
        }

        // check if ccd preview needs to be sent
        if (newCCDpreview) {
            animax::preview Preview;
            Preview.set_type("ccd");
            Preview.set_previewdata(ccdpreview);

            // serialize data and write into ZMQ request
            size_t size = Preview.ByteSizeLong();
            void *sendbuffer = malloc(size);
            Preview.SerializeToArray(sendbuffer, size);
            zmq::message_t request(size);
            memcpy ((void *) request.data (), sendbuffer, size);

            // send "preview" envelope with preview data content
            gui.send(zmq::str_buffer("previewdata"), zmq::send_flags::sndmore);
            gui.send(request, zmq::send_flags::none);

            newCCDpreview = false;
            std::cout<<"sent new ccd preview data"<<std::endl;
        }

        // check if ccd preview needs to be sent
        if (newROIs) {
            auto const ROIkeys = ROIdata.keys();
            for (std::string e : ROIkeys) {
                    //std::cout<<"sending ROI data for "<<e<<":"<<std::endl;
                    animax::ROI roi;
                    roi.set_element(e);
                    roi.set_roidata(ROIdata[e].constData(), ROIdata[e].count()*sizeof(uint32_t));
                    // serialize data and write into ZMQ request
                    size_t size = roi.ByteSizeLong();
                    void *sendbuffer = malloc(size);
                    roi.SerializeToArray(sendbuffer, size);
                    zmq::message_t request(size);
                    memcpy ((void *) request.data (), sendbuffer, size);

                    // send "preview" envelope with preview data content
                    gui.send(zmq::str_buffer("roidata"), zmq::send_flags::sndmore);
                    gui.send(request, zmq::send_flags::none);
            }

            newROIs = false;

        }

        // check if a part of NEXAFS scan is ready
        // if so, tell GUI
        if (partScanFinished) {
            animax::scanstatus scanstatus;
            scanstatus.set_status("part finished");
            size_t size = scanstatus.ByteSizeLong();
            void *sendbuffer = malloc(size);
            scanstatus.SerializeToArray(sendbuffer, size);
            zmq::message_t request(size);
            memcpy ((void *) request.data (), sendbuffer, size);

            // send "scanstatus" envelope with status info
            gui.send(zmq::str_buffer("scanstatus"), zmq::send_flags::sndmore);
            gui.send(request, zmq::send_flags::none);

            partScanFinished = false;
            waitForMetadata = true;
        }

        // check if a whole scan is ready
        // if so, tell GUI
        if (wholeScanFinished) {
            animax::scanstatus scanstatus;
            scanstatus.set_status("whole scan finished");
            size_t size = scanstatus.ByteSizeLong();
            void *sendbuffer = malloc(size);
            scanstatus.SerializeToArray(sendbuffer, size);
            zmq::message_t request(size);
            memcpy ((void *) request.data (), sendbuffer, size);

            // send "scanstatus" envelope with status info
            gui.send(zmq::str_buffer("scanstatus"), zmq::send_flags::sndmore);
            gui.send(request, zmq::send_flags::none);

            wholeScanFinished = false;
        }
    }
}
