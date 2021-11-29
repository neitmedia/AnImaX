# SDD publisher
small program that connects and interacts with DataSink and GUI and publishes binary sdd raw data from binary file
supports XRF and NEXAFS scans
dependencies: zmq, protobuf
compilation: g++ -m64 sdd_recv_chunks.cpp animax.pb.cc -lprotobuf -lzmq -o sdd_recv_chunks
command line syntax: ./sdd_recv_chunks <filename> <GUI IP> <GUI PORT>
