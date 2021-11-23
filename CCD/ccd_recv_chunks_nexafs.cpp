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

	// define ZMQ context
	zmq::context_t ctx(1);
	
    // Prepare subscriber
    zmq::socket_t subscriber(ctx, zmq::socket_type::sub);
    
    // connect to GUI (port 5555)
    subscriber.connect("tcp://127.0.0.1:5555");
    
    // subscribe to "settings" and "metadata"
    subscriber.set(zmq::sockopt::subscribe, "settings");
    subscriber.set(zmq::sockopt::subscribe, "metadata");
    
    // Prepare publisher
    zmq::socket_t gui(ctx, zmq::socket_type::pub);
    
    // Publisher listens at port 5556
    gui.bind("tcp://*:5556");

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
        	uint32_t aquisition_time = Measurement.aquisition_time();
        	uint32_t energy_count = Measurement.energy_count();
        	
        	std::cout<<"width: "<<width<<std::endl;
        	std::cout<<"height: "<<height<<std::endl;
        	std::cout<<"aquisition_time: "<<width<<std::endl;
        	std::cout<<"width: "<<width<<std::endl;
            scantype = Measurement.scantype();
                        
        	/*                      IMPORTANT INFORMATION                      */
        	/* TO DO: set all ccd settings and prepare everything for the scan */
            
            // send real settings
            animax::ccdsettings ccdsettings;
            ccdsettings.set_width(width);
            ccdsettings.set_height(height);
            
            size_t ccdsettingssize = ccdsettings.ByteSizeLong(); 
        
            void *ccdsettingsbuffersend = malloc(ccdsettingssize);
            ccdsettings.SerializeToArray(ccdsettingsbuffersend, ccdsettingssize);
            zmq::message_t requestccdsettings(ccdsettingssize);
            memcpy ((void *) requestccdsettings.data (), ccdsettingsbuffersend, ccdsettingssize);
            
        	gui.send(zmq::str_buffer("ccdsettings"), zmq::send_flags::sndmore);
        	gui.send(requestccdsettings, zmq::send_flags::none);
            
            // send ready signal
            
            gui.send(zmq::str_buffer("statusdata"), zmq::send_flags::sndmore);
        	gui.send(zmq::str_buffer("ready"), zmq::send_flags::none);
        	
        	// give out some debug info
        	std::cout<<"sent status info"<<width<<std::endl;
        } else if (env_str == "metadata") {
        	// if the received envelope is the "metadata" envelope, it means that the GUI received "ready" messages from all peripherals and everything is ready
        	ready = true;
        }
    }
    
    // the code after this is executed when everything is ready for the scan
    std::cout<<"ready to send data!"<<std::endl;
	
	std::cout<<"start sending data"<<std::endl;
	
	uint64_t counter = 0;
    
    uint32_t pixelanzahl = scanX*scanY;
    
    uint32_t ccdpixelcount = ccdX*ccdY;

    uint32_t ccdpixelbytecount = ccdpixelcount*2;
    
    // iterate through the images
    while(counter<pixelanzahl) {
    
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
            	
        usleep(5000);
  
    }
    
    // give out debug info
    std::cout<<"finished sending data"<<std::endl;
	
    if (scantype == "NEXAFS") {
        
        ready = false;
        
        std::cout<<"waiting for scan to finish..."<<std::endl;
        
        while (!ready) {
            // declare ZMQ envelope message
            zmq::message_t env;
            // receive envelope
            (void)subscriber.recv(env);
            // read out envelope content
            std::string env_str = std::string(static_cast<char*>(env.data()), env.size());
            
            // if the received envelope is the "settings" envelope, decode protobuf and give out values for debugging purposes
            if (env_str == "metadata") {
                // if the received envelope is the "metadata" envelope, it means that the GUI received "ready" messages from all peripherals and everything is ready
                ready = true;
            }
        }
        
        sleep(5);
        
        std::cout<<"start sending data"<<std::endl;
	
        uint64_t counter = 0;
        
        uint32_t pixelanzahl = scanX*scanY;
        
        uint32_t ccdpixelcount = ccdX*ccdY;

        uint32_t ccdpixelbytecount = ccdpixelcount*2;
        
        // iterate through the images
        while(counter<pixelanzahl) {
        
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
                    
            usleep(5000);
    
        }
        
        // give out debug info
        std::cout<<"finished sending data"<<std::endl;
    }
}
