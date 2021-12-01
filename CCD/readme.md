# CCD publisher
small program that connects and interacts with DataSink and GUI and publishes binary ccd image data from binary file\
supports XRF and NEXAFS scans\
dependencies: zmq, protobuf\
compilation: g++ -m64 ccd_recv_chunks_nexafs.cpp animax.pb.cc -lprotobuf -lzmq -o ccd_recv_chunks_nexafs\
command line syntax: ./ccd_recv_chunks_nexafs filename ccdWidth ccdHeight scanWidth scanHeight GUI_IP GUI_PORT
