#include "zmq.hpp" // ZMQ
#include "animax.pb.h" // compiled "animax" protobuf
#include <string>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>

int main (int argc, char** argv)
{
	// declare variables
	FILE *fileptr;
	char *buffer;
	uint32_t filelen;
    std::string guiIP = argv[6];
    std::string guiPort = argv[7];
    bool connected = false;
    
    uint32_t energy_count;

	// define ZMQ context
	zmq::context_t ctx(1);
	
    // Prepare subscriber
    zmq::socket_t subscriber(ctx, zmq::socket_type::sub);
    
    std::string connectstring = "tcp://"+guiIP+":"+guiPort;
    
    // connect to GUI
    subscriber.connect(connectstring);
    std::cout<<"connected to "<<connectstring<<std::endl;
    
    // subscribe to "settings" and "metadata"
    subscriber.set(zmq::sockopt::subscribe, "settings");
    subscriber.set(zmq::sockopt::subscribe, "metadata");
    subscriber.set(zmq::sockopt::subscribe, "scanstatus");
    
    // Prepare publisher
    zmq::socket_t gui(ctx, zmq::socket_type::pub);

	// declare and initialize ready variable (true means GUI is connected and settings and metadata have been successfully received)
	bool ready = false;
    
    //fileptr = fopen("stxm.bin", "rb");  
    fileptr = fopen(argv[1], "rb");
	fseek(fileptr, 0, SEEK_END);          
	filelen = ftell(fileptr);             
	rewind(fileptr);  
    fclose(fileptr);
    
    uint32_t ccdX = (uint32_t) atoi(argv[2]); 
    uint32_t ccdY = (uint32_t) atoi(argv[3]);
    uint32_t scanX = (uint32_t) atoi(argv[4]);
    uint32_t scanY = (uint32_t) atoi(argv[5]);
    
    std::ifstream in(argv[1], std::ios::binary);
    
    std::string scantype = "";
    
	// while not everything is ready, wait for incoming packets / envelopes
    while (!ready) {
    	// declare ZMQ envelope message
        zmq::message_t env;
        // receive envelope
        (void)subscriber.recv(env);
        // read out envelope content
        std::string env_str = std::string(static_cast<char*>(env.data()), env.size());
        
        // if the received envelope is the "settings" envelope, decode protobuf and give out values for debugging purposes
        if (env_str == "settings") {
        	std::cout << "Received envelope: "<<env_str<< std::endl;
        	zmq::message_t msg;
        	(void)subscriber.recv(msg);
			std::cout << "Received data" << std::endl;
			animax::Measurement Measurement;
        	Measurement.ParseFromArray(msg.data(), msg.size());
        	uint32_t width = Measurement.width();
        	uint32_t height = Measurement.height();
        	uint32_t acquisition_time = Measurement.acquisition_time();
        	energy_count = Measurement.energy_count();
        	
        	std::cout<<"width: "<<width<<std::endl;
        	std::cout<<"height: "<<height<<std::endl;
        	std::cout<<"acquisition_time: "<<width<<std::endl;
        	std::cout<<"width: "<<width<<std::endl;
            scantype = Measurement.scantype();
            uint32_t ccdPort = Measurement.ccdport();
            
            if (!connected) {
                std::cout<<"sending data on tcp://*:"+std::to_string(ccdPort)<<std::endl;
                
                // Publisher listens at port that was set in GUI
                gui.bind("tcp://*:"+std::to_string(ccdPort));
                connected = true;
            }
        
        	/*                      IMPORTANT INFORMATION                      */
        	/* TO DO: set all ccd settings and prepare everything for the scan */
            
            // send real settings
            animax::ccdsettings ccdsettings;
            ccdsettings.set_set_kinetic_cycle_time(0.005312);
            ccdsettings.set_exposure_time(0.0012342);
            ccdsettings.set_accumulation_time(0.023352);
            ccdsettings.set_kinetic_time(0.035342);
            
            size_t ccdsettingssize = ccdsettings.ByteSizeLong(); 
        
            void *ccdsettingsbuffersend = malloc(ccdsettingssize);
            ccdsettings.SerializeToArray(ccdsettingsbuffersend, ccdsettingssize);
            zmq::message_t requestccdsettings(ccdsettingssize);
            memcpy ((void *) requestccdsettings.data (), ccdsettingsbuffersend, ccdsettingssize);
            
        	gui.send(zmq::str_buffer("ccdsettings"), zmq::send_flags::sndmore);
        	gui.send(requestccdsettings, zmq::send_flags::none);
            
            // send ready signal
            
            gui.send(zmq::str_buffer("statusdata"), zmq::send_flags::sndmore);
        	gui.send(zmq::str_buffer("connection ready"), zmq::send_flags::none);
        	
        	// give out some debug info
        	std::cout<<"sent status info"<<width<<std::endl;
        } else if (env_str == "metadata") {
        	// if the received envelope is the "metadata" envelope, it means that the GUI received "ready" messages from all peripherals and everything is ready
        	ready = true;
        }
    }
    
    bool stopscan = false;

    for (int scanc = 0; scanc < energy_count; scanc++) {
            // tell the GUI that detector is ready ("statusdata" envelope with content "detector ready")
            gui.send(zmq::str_buffer("statusdata"), zmq::send_flags::sndmore);
            gui.send(zmq::str_buffer("detector ready"), zmq::send_flags::none);
            
            // wait for reply from the GUI
            ready = false;
            while (!ready) {
                // declare ZMQ envelope message
                zmq::message_t env;
                // receive envelope
                (void)subscriber.recv(env);
                // read out envelope content
                std::string env_str = std::string(static_cast<char*>(env.data()), env.size());
                if (env_str == "metadata") {
                    zmq::message_t msg;
                    (void)subscriber.recv(msg);
                    // if the received envelope is the "metadata" envelope, it means that the GUI received "ready" messages from all peripherals and everything is ready
                    animax::Metadata Metadata;
                    Metadata.ParseFromArray(msg.data(), msg.size());
                    uint32_t acq_num = Metadata.acquisition_number();
                    std::cout<<"acquisition_number: "<<acq_num<<std::endl;
                    ready = true;
                }
            }
            
            std::cout<<"start sending data"<<std::endl;
        
            uint64_t counter = 0;
            
            uint32_t pixelanzahl = scanX*scanY;
            
            uint32_t ccdpixelcount = ccdX*ccdY;

            uint32_t ccdpixelbytecount = ccdpixelcount*2;
            
            // iterate through the images
            while(counter<pixelanzahl) {
                
                
                // check for scan status changes
                // declare ZMQ envelope message
                zmq::message_t recvenv;
                // receive envelope
                (void)subscriber.recv(recvenv, zmq::recv_flags::dontwait);
                // read out envelope content
                std::string env_str = std::string(static_cast<char*>(recvenv.data()), recvenv.size());
                
                if (env_str == "scanstatus") {
                    zmq::message_t msg;
                    (void)subscriber.recv(msg, zmq::recv_flags::dontwait);
                    // if the received envelope is the "scanstatus" envelope, it means that the GUI sent a scan status change request
                    animax::scanstatus scanstatus;
                    scanstatus.ParseFromArray(msg.data(), msg.size());
                    std::string scanstatusstr = scanstatus.status();
                    
                    if (scanstatusstr == "stop") {
                        stopscan = true;
                    }
                    
                    if (scanstatusstr == "pause") {
                        while (scanstatusstr != "resume") {
                            (void)subscriber.recv(recvenv);
                            // read out envelope content
                            env_str = std::string(static_cast<char*>(recvenv.data()), recvenv.size());
                            if (env_str == "scanstatus") {
                                (void)subscriber.recv(msg, zmq::recv_flags::dontwait);
                                // if the received envelope is the "scanstatus" envelope, it means that the GUI sent a scan status change request
                                animax::scanstatus scanstatus;
                                scanstatus.ParseFromArray(msg.data(), msg.size());
                                scanstatusstr = scanstatus.status();
                                if (scanstatusstr == "stop") {
                                    stopscan = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            
                zmq::message_t env1(3);
                memcpy(env1.data(), "ccd", 3);
                
                animax::ccd ccd;
                
                /*                                           IMPORTANT INFORMATION                                        */	
                /* TO DO: get the real data from the sdd and write this data into the pixeldata field of the ccd protobuf */
                /*                                                                                                        */
                
                ccd.set_cnt(counter);
                uint64_t offset = counter*ccdpixelbytecount;        

                buffer = (char *)malloc(ccdpixelbytecount * sizeof(uint8_t)); 
                in.seekg(offset, std::ios_base::beg); 
                in.read(buffer, ccdpixelbytecount);

                ccd.set_pixeldata(buffer, ccdpixelbytecount);
                
                counter++;
                
                // serialize data and write into ZMQ request
                size_t size = ccd.ByteSizeLong(); 
                
                void *buffersend = malloc(size);
                ccd.SerializeToArray(buffersend, size);
                zmq::message_t request(size);
                memcpy ((void *) request.data (), buffersend, size);
                
                
                // send "ccd" envelope with sdd data content
                gui.send(env1, zmq::send_flags::sndmore);
                gui.send(request, zmq::send_flags::none);
                std::cout << "sent ccd data #"<<counter<<std::endl;
                
                free(buffer);
                free(buffersend);
                        
                usleep(100);
                
                if (stopscan) {
                    break;
                }
        
            }
            // tell the GUI that measurement is finished ("statusdata" envelope with content "finished measurement")
            gui.send(zmq::str_buffer("statusdata"), zmq::send_flags::sndmore);
            gui.send(zmq::str_buffer("finished measurement"), zmq::send_flags::none);
        
            // give out debug info
            std::cout<<"finished sending data"<<std::endl;
            
            if (stopscan) {
                break;
            }
        }
}
