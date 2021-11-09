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
    
    // connect to GUI (port 5555)
    subscriber.connect("tcp://127.0.0.1:5555");
    
    // subscribe to "settings" and "metadata"
    subscriber.set(zmq::sockopt::subscribe, "settings");
    subscriber.set(zmq::sockopt::subscribe, "metadata");
    
    // Prepare publisher
    zmq::socket_t gui(ctx, zmq::socket_type::pub);
    
    // Publisher listens at port 5557
    gui.bind("tcp://*:5557");

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
        	uint32_t aquisition_time = Measurement.aquisition_time();
        	uint32_t energy_count = Measurement.energy_count();
        	
        	std::cout << "width: " << width << std::endl;
        	std::cout << "height: " << height << std::endl;
        	std::cout << "aquisition_time: " << aquisition_time << std::endl;
        	std::cout << "energy_count: " << energy_count << std::endl;
        	
        	/*                      IMPORTANT INFORMATION                      */
        	/* TO DO: set all sdd settings and prepare everything for the scan */
        	/*                                                                 */
        	
        	// tell the GUI that everything is ready for the scan ("statusdata" envelope with content "ready")
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
    uint32_t readsize = 20000;
    uint64_t offset = 0;
    // iterate through the images
    while(readsize == 20000) {
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
    	
        usleep(4500);
    }

	// give out debug info
   	std::cout<<"finished sending data!"<<std::endl;
}
