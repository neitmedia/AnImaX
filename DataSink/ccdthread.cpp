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

// constructor of ccdThread object
ccdThread::ccdThread(QString s, int scanX, int scanY) : ip(s), scanX(scanX), scanY(scanY)
{
    // register Metatypes to be able to use them as parameters
    qRegisterMetaType<std::string>();
    qRegisterMetaType<ccdsettings>();
}

// run function is started when thread is launched
void ccdThread::run()
{
        // give debug info
        std::cout<<"started ccd thread"<<std::endl;

        // declare zmq context and subscriber object
        zmq::context_t context(1);
        zmq::socket_t subscriber(context, ZMQ_SUB);

        // connect to ccd endpoint
        subscriber.connect("tcp://"+ip.toStdString());

        // subscribe to messages with envelope "statusdata":
        // "statusdata" messages are published by detector software
        // to give info about their status
        subscriber.set(zmq::sockopt::subscribe, "statusdata");

        // subscribe to messages with "ccd" envelope that contain the measurement data
        subscriber.set(zmq::sockopt::subscribe, "ccd");

        // subscribe to messages with "ccdsettings" envelope that contains the
        // timings calculated by the ccd driver that are actually set
        subscriber.set(zmq::sockopt::subscribe, "ccdsettings");

        // give debug info
        std::cout<<"waiting for ready signal from ccd..."<<std::endl;

        /* START INITIALIZATION LOOP */

        // wait for ready signal from ccd and sdd
        while (!ccdReadyState) {

            // declare message envelope object
            zmq::message_t env;

            // receive incoming message, wait until message is received
            subscriber.recv(env);

            // convert message envelope data to string
            std::string env_str = std::string(static_cast<char*>(env.data()), env.size());

            // give debug info
            std::cout << "Received envelope '" << env_str << "'" << std::endl;

            // if message envelope is "statusdata"
            if (env_str == "statusdata") {

                // declare message variable
                zmq::message_t msg;

                // receive message content
                subscriber.recv(msg);

                // convert message data to string
                std::string msg_str = std::string(static_cast<char*>(msg.data()), msg.size());

                // give out message content for debugging purposes
                std::cout << "Received:" << msg_str << std::endl;

                // if ccd is ready, message contains string "connection ready"
                if (msg_str == "connection ready") {

                    // give out debug info
                    std::cout<<"ccd is ready"<<std::endl;

                    // set global ccdReadyState = true, so initialization loop is leaved
                    ccdReadyState = true;

                    // send ccd status to mainwindow.cpp / datasink GUI
                    emit ccdReady();

                }

            // if message envelope is "ccdsettings"
            } else if (env_str == "ccdsettings") {

                // give out debug info
                std::cout << "Received real ccd settings" << std::endl;

                // declare message obj
                zmq::message_t msg;

                // receive message content
                subscriber.recv(msg);

                // declare protobuf/ccdsettings object
                animax::ccdsettings ccdsettingsbuf;

                // parse / deserialize message data into animax::ccdsettings object
                ccdsettingsbuf.ParseFromArray(msg.data(), msg.size());

                // declare variable of type ccdsettings (see structs.h)
                ccdsettings ccdsettings;

                // write data from animax::ccdsettings object to ccdsettings variable
                ccdsettings.set_kinetic_cycle_time = ccdsettingsbuf.set_kinetic_cycle_time();
                ccdsettings.exposure_time = ccdsettingsbuf.exposure_time();
                ccdsettings.accumulation_time = ccdsettingsbuf.accumulation_time();
                ccdsettings.kinetic_time = ccdsettingsbuf.kinetic_time();

                // send ccdsettings data to mainwindow.cpp / GUI
                emit sendCCDSettings(ccdsettings);
            }
        }

        /* END INITIALIZATION LOOP */

        /* START MAIN LOOP */

        // if ccd is ready, wait for incoming envelopes with image data, to do so enter main loop
        while (!stop) {
            try {

                // declare message envelope object
                zmq::message_t env;

                // wait for new message envelope and receive them
                subscriber.recv(env);

                // cast message envelope to string
                std::string env_str = std::string(static_cast<char*>(env.data()), env.size());

                // if envelope content is "ccd"
                if (env_str == "ccd") {

                    // declare message object
                    zmq::message_t msg;

                    // receive message content
                    subscriber.recv(msg);

                    // declare animax::ccd object ccd
                    animax::ccd ccd;

                    // parse/deserialize message data to animax::ccd object
                    ccd.ParseFromArray(msg.data(), msg.size());

                    // declare and define pxcnt variable (current scan pixel number)
                    int pxcnt = ccd.cnt();

                    // send scan pixel counter and ccd image data to mainwindow.cpp / GUI
                    emit sendImageData(pxcnt, ccd.pixeldata());

                    // if all pixels of a scan have been received
                    if (pxcnt == (scanX*scanY)-1) {
                        // give out debug info
                        std::cout << "ccd scan received" << std::endl;
                    }
                }

            // catch errors
            }  catch (zmq::error_t &err)
            {
                printf("%s", err.what());
            }
        }

        /* END MAIN LOOP */
}
