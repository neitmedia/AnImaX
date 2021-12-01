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
                settings.scanWidth = Measurement.width();
                settings.scanHeight = Measurement.height();
                settings.ccdHeight = Measurement.ccdheight();
                settings.ccdWidth = Measurement.ccdwidth();
                settings.sddChannels = 4096;
                settings.roidefinitions = Measurement.roidefinitions();
                settings.scantype = Measurement.scantype();
                settings.ccdIP = Measurement.ccdip();
                settings.ccdPort = Measurement.ccdport();
                settings.sddIP = Measurement.sddip();
                settings.sddPort = Measurement.sddport();
                settings.datasinkIP = Measurement.datasinkip();
                settings.datasinkPort = Measurement.datasinkport();
                settings.energycount = Measurement.energy_count();

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
                    /*
                    // create HDF5/NeXus file
                    hdf5filename = "measurement_"+QString::number(QDateTime::currentMSecsSinceEpoch())+".h5";
                    nexusfile = new hdf5nexus();
                    nexusfile->createDataFile(hdf5filename, settings);
                    filecreated = true;
                    */

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
                    metadata.beamline_energy = Metadata.beamline_energy();

                    emit sendMetadataToGUI(metadata);
                    waitForMetadata = false;
                }
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
    }
}
