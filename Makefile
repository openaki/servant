### CSCI 551  Fall 08
#  Final Proj pt2
#
#  Name: Byung-Yeob Kim & Akshat Gupta
#
#  Usage:
#     make - Builds servant
#
#     make clean - Cleans all files create during the building process
#
# port: 15896 - 15911

CXX=g++
CXXFLAGS= -g -Wall  -D_REENTRANT -I/home/scf-22/csci551b/openssl/include 
LINKFLAGS = -lcrypto -lresolv -lnsl -lsocket -lcrypto -L/home/scf-22/csci551b/openssl/lib

sv_node: servant.o bitvector.o indice.o util.o init.o msg.o neighbor_list.o sig_handlers.o error.o event.o globals.o dispatch.o lru.o
	$(CXX) $(CXXFLAGS) servant.o bitvector.o indice.o util.o init.o msg.o neighbor_list.o error.o event.o globals.o dispatch.o lru.o $(LINKFLAGS) -o sv_node

servant.o: servant.cpp includes.h constants.h startup.h init.h error.h msg.h neighbor_list.h constants.h
	$(CXX) $(CXXFLAGS) -c servant.cpp

bitvector.o: bitvector.cpp bitvector.h
	$(CXX) $(CXXFLAGS) -c bitvector.cpp

indice.o: indice.cpp indice.h bitvector.h constants.h
	$(CXX) $(CXXFLAGS) -c indice.cpp

sig_handlers.o: sig_handlers.cpp sig_handlers.h constants.h
	$(CXX) $(CXXFLAGS) -c sig_handlers.cpp

init.o: init.cpp init.h startup.h constants.h
	$(CXX) $(CXXFLAGS) -c init.cpp

msg.o: msg.cpp msg.h error.h constants.h
	$(CXX) $(CXXFLAGS) -c msg.cpp

neighbor_list.o: neighbor_list.cpp neighbor_list.h msg.h constants.h
	$(CXX) $(CXXFLAGS) -c neighbor_list.cpp

error.o: error.cpp error.h constants.h
	$(CXX) $(CXXFLAGS) -c error.cpp

event.o: event.cpp event.h error.h constants.h
	$(CXX) $(CXXFLAGS) -c event.cpp

util.o: util.cpp util.h msg.h constants.h
	$(CXX) $(CXXFLAGS) -c util.cpp

globals.o: globals.cpp globals.h
	$(CXX) $(CXXFLAGS) -c globals.cpp

dispatch.o: dispatch.cpp dispatch.h util.h error.h event.h globals.h 
	$(CXX) $(CXXFLAGS) -c dispatch.cpp

lru.o: lru.cpp lru.h
	$(CXX) $(CXXFLAGS) -c lru.cpp

clean:
	rm -f sv_node *.o

