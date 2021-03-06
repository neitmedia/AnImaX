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
	uint64_t filelen;
    uint64_t filelenhalf;
    std::string guiIP = argv[2];
    std::string guiPort = argv[3];
    bool connected = false;
    uint32_t energy_count;

	// read file (here: raw.rtdat)
	fileptr = fopen(argv[1], "rb");
	fseek(fileptr, 0, SEEK_END);          
	filelen = ftell(fileptr);             
	rewind(fileptr);  
    fclose(fileptr);

    std::ifstream in(argv[1], std::ios::binary);
	
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
    
    std::string scantype = "";

	// declare and initialize ready variable (true means GUI is connected and settings and metadata have been successfully received)
	bool ready = false;
	// while not everything is ready, wait for incoming packets / envelopes
    while (!ready) {
    	// declare ZMQ envelope message
        zmq::message_t env;
        // receive envelope
        (void)subscriber.recv(env);
        // read out envelope content
        std::string env_str = std::string(static_cast<char*>(env.data()), env.size());
        // print envelope content for debugging purposes
        std::cout << "received envelope:" << env_str << std::endl;
        // if the received envelope is the "settings" envelope, decode protobuf and give out values for debugging purposes
        if (env_str == "settings") {
        	std::cout << "Received envelope: " << env_str << std::endl;
        	zmq::message_t msg;
        	(void)subscriber.recv(msg);
			std::cout << "Received data" << std::endl;
			
			animax::Measurement Measurement;
        	Measurement.ParseFromArray(msg.data(), msg.size());
        	uint32_t width = Measurement.width();
        	uint32_t height = Measurement.height();
        	uint32_t acquisition_time = Measurement.acquisition_time();
        	energy_count = Measurement.energy_count();
            scantype = Measurement.scantype();
            uint32_t sddPort = Measurement.sddport();
            
            
            if (!connected) {
                std::cout<<"sending data on tcp://*:"+std::to_string(sddPort)<<std::endl;
                // Publisher listens at port that was set in GUI
                gui.bind("tcp://*:"+std::to_string(sddPort));
                connected = true;
            }
        	
        	std::cout << "width: " << width << std::endl;
        	std::cout << "height: " << height << std::endl;
        	std::cout << "acquisition_time: " << acquisition_time << std::endl;
        	std::cout << "energy_count: " << energy_count << std::endl;
        	
        	/*                      IMPORTANT INFORMATION                      */
        	/* TO DO: set all sdd settings and prepare everything for the scan */
        	/*                                                                 */
        	
        	// tell the GUI that connection is ready ("statusdata" envelope with content "connection ready")
        	gui.send(zmq::str_buffer("statusdata"), zmq::send_flags::sndmore);
        	gui.send(zmq::str_buffer("connection ready"), zmq::send_flags::none);
        	
        	// give out some debug info
        	std::cout<<"sent status info"<<width<<std::endl;
        } else if (env_str == "metadata") {
            zmq::message_t msg;
        	(void)subscriber.recv(msg);
            animax::Metadata Metadata;
        	Metadata.ParseFromArray(msg.data(), msg.size());
        	uint32_t acq_num = Metadata.acquisition_number();
            std::cout<<"acquisition_number: "<<acq_num<<std::endl;
            uint32_t beamline_energy = Metadata.beamline_energy();
            std::cout<<"beamline energy: "<<beamline_energy<<std::endl;
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
                uint32_t beamline_energy = Metadata.beamline_energy();
                std::cout<<"beamline energy: "<<beamline_energy<<std::endl;
                ready = true;
            }
        }
        
        // the code after this is executed when everything is ready for the scan
        
        std::cout<<"ready to send data!"<<std::endl;
        
        std::cout<<"start sending data"<<std::endl;
        
        uint64_t counter = 0;
        uint32_t readsize = 20000;
        uint64_t offset = 0;
        // iterate through the images
        while(readsize == 20000) {
            
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
            memcpy(env1.data(), "sdd", 3);
            
            animax::sdd sdd;
            
            /*                                           IMPORTANT INFORMATION                                        */	
            /* TO DO: get the real data from the sdd and write this data into the pixeldata field of the sdd protobuf */
            /*                                                                                                        */
            
            uint64_t offset = counter*readsize;
            
            if (offset+readsize > filelen) {
                readsize = filelen-offset;
            }

            buffer = (char *)malloc(readsize * sizeof(uint8_t)); 
            in.seekg(offset, std::ios_base::beg); 
            in.read(buffer, readsize);
            
            sdd.set_pixeldata(buffer, readsize);
            
            counter++;
            
            // serialize data and write into ZMQ request
            size_t size = sdd.ByteSizeLong(); 
            
            void *buffersend = malloc(size);
            sdd.SerializeToArray(buffersend, size);
            zmq::message_t request(size);
            memcpy ((void *) request.data (), buffersend, size);
            
            
            // send "sdd" envelope with sdd data content
            gui.send(env1, zmq::send_flags::sndmore);
            gui.send(request, zmq::send_flags::none);
            std::cout << "sent sdd data #"<<counter<<std::endl;
            
            //std::cout << "offset | filesize | readsize: "<<offset<<" | "<<filelen<<" | "<<readsize<<std::endl;
            
            free(buffer);
            free(buffersend);
            
            usleep(2000);
            
            if (stopscan) {
                break;
            }
        }
        
        // tell the GUI that measurement is finished ("statusdata" envelope with content "finished measurement")
        gui.send(zmq::str_buffer("statusdata"), zmq::send_flags::sndmore);
        gui.send(zmq::str_buffer("finished measurement"), zmq::send_flags::none);
        // give out debug info
        std::cout<<"finished sending data!"<<std::endl;
        
        if (stopscan) {
            break;
        }
        
    }

}
