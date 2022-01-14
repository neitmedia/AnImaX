#include "ccdthread.h"
#include <QDebug>
#include <stdio.h>
#include <QThread>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <chrono>

#include <zmq.hpp>

using namespace std::chrono;

ccdThread::ccdThread(QString s, int scanX, int scanY) : ip(s), scanX(scanX), scanY(scanY)
{
    qRegisterMetaType<std::string>();
    qRegisterMetaType<ccdsettings>();
}

// We overrides the QThread's run() method here
// run() will be called when a thread starts
// the code will be shared by all threads

void ccdThread::run()
{
        std::cout<<"started ccd thread"<<std::endl;
        zmq::context_t context(1);
        zmq::socket_t subscriber(context, ZMQ_SUB);
        subscriber.connect("tcp://"+ip.toStdString());
        subscriber.set(zmq::sockopt::subscribe, "statusdata");
        subscriber.set(zmq::sockopt::subscribe, "ccd");
        subscriber.set(zmq::sockopt::subscribe, "ccdsettings");
        std::cout<<"waiting for ready signal from ccd..."<<std::endl;

        // wait for ready signal from ccd and sdd
        while (!ccdReadyState) {
            zmq::message_t env;
            subscriber.recv(env);
            std::string env_str = std::string(static_cast<char*>(env.data()), env.size());
            std::cout << "Received envelope '" << env_str << "'" << std::endl;

            if (env_str == "statusdata") {
                zmq::message_t msg;
                subscriber.recv(msg);
                std::string msg_str = std::string(static_cast<char*>(msg.data()), msg.size());

                std::cout << "Received:" << msg_str << std::endl;

                // if ccd is ready, set public variable "ccdReady" give controlthread the permission to get this info
                if (msg_str == "connection ready") {
                    std::cout<<"ccd is ready"<<std::endl;
                    ccdReadyState = true;
                    emit ccdReady();
                }
            } else if (env_str == "ccdsettings") {
                std::cout << "Received real ccd settings" << std::endl;
                zmq::message_t msg;
                subscriber.recv(msg);

                animax::ccdsettings ccdsettingsbuf;
                ccdsettingsbuf.ParseFromArray(msg.data(), msg.size());

                ccdsettings ccdsettings;
                ccdsettings.binning_x = ccdsettingsbuf.binning_x();
                ccdsettings.binning_y = ccdsettingsbuf.binning_y();
                ccdsettings.ccdWidth = ccdsettingsbuf.ccdwidth();
                ccdsettings.ccdHeight = ccdsettingsbuf.ccdheight();
                ccdsettings.pixelcount = ccdsettingsbuf.pixelcount();
                ccdsettings.frametransfer_mode = ccdsettingsbuf.frametransfer_mode();
                ccdsettings.number_of_accumulations = ccdsettingsbuf.number_of_accumulations();
                ccdsettings.number_of_scans = ccdsettingsbuf.number_of_scans();
                ccdsettings.set_kinetic_cycle_time = ccdsettingsbuf.set_kinetic_cycle_time();
                ccdsettings.read_mode = ccdsettingsbuf.read_mode();
                ccdsettings.acquision_mode = ccdsettingsbuf.acquision_mode();
                ccdsettings.shutter_mode = ccdsettingsbuf.shutter_mode();
                ccdsettings.shutter_output_signal = ccdsettingsbuf.shutter_output_signal();
                ccdsettings.shutter_open_time = ccdsettingsbuf.shutter_open_time();
                ccdsettings.shutter_close_time = ccdsettingsbuf.shutter_close_time();
                ccdsettings.triggermode = ccdsettingsbuf.triggermode();
                ccdsettings.set_integration_time = ccdsettingsbuf.set_integration_time();
                ccdsettings.exposure_time = ccdsettingsbuf.exposure_time();
                ccdsettings.accumulation_time = ccdsettingsbuf.accumulation_time();
                ccdsettings.kinetic_time = ccdsettingsbuf.kinetic_time();
                ccdsettings.min_temp = ccdsettingsbuf.min_temp();
                ccdsettings.max_temp = ccdsettingsbuf.max_temp();
                ccdsettings.target_temp = ccdsettingsbuf.target_temp();
                ccdsettings.pre_amp_gain = ccdsettingsbuf.pre_amp_gain();
                ccdsettings.em_gain_mode = ccdsettingsbuf.em_gain_mode();
                ccdsettings.em_gain = ccdsettingsbuf.em_gain();

                emit sendCCDSettings(ccdsettings);
            }
        }

        // if ccd is ready, wait for incoming envelopes with image data
        int bla = 0;
        auto start = high_resolution_clock::now();

        while (!stop) {
            try {
                zmq::message_t env;
                subscriber.recv(env);
                std::string env_str = std::string(static_cast<char*>(env.data()), env.size());
                if (bla == 0) {
                    start = high_resolution_clock::now();
                }

                if (env_str == "ccd") {
                    zmq::message_t msg;
                    subscriber.recv(msg);
                    bla++;

                    animax::ccd ccd;
                    ccd.ParseFromArray(msg.data(), msg.size());
                    std::string text_str;
                    int bla = ccd.cnt();

                    emit sendImageData(bla, ccd.pixeldata());
                    std::cout<<"received image #"<<bla<<std::endl;

                    if (bla == (scanX*scanY)-1) {
                        std::cout << "ccd scan received" << std::endl;
                        auto stop = high_resolution_clock::now();
                        auto duration = duration_cast<microseconds>(stop - start);

                        // To get the value of duration use the count()
                        // member function on the duration object
                        std::cout << duration.count() << std::endl;
                        std::cout << "clear ccd variables..." << std::endl;
                        std::cout << "cleared ccd variables..." << std::endl;
                    }


                }

            }  catch (zmq::error_t &err)
            {
                printf("%s", err.what());
            }
        }
}
