CXX=g++
CXXFLAGS=-std=c++11 -g -lboost_system -lboost_filesystem -lcrypto -lssl -lpthread -I /usr/include/openssl/
OBJFILES=HttpsService.o HttpsAcceptor.o HttpsServer.o main.o

httpsserver : ${OBJFILES}
	${CXX} ${OBJFILES} -o $@ ${CXXFLAGS}
	
HttpsService.o : HttpsService.cpp
	${CXX} -c  HttpsService.cpp -o $@ ${CXXFLAGS}

HttpsAcceptor.o : HttpsAcceptor.cpp
	${CXX} -c HttpsAcceptor.cpp -o $@ ${CXXFLAGS}

HttpsServer.o : HttpsServer.cpp
	${CXX} -c HttpsServer.cpp -o $@ ${CXXFLAGS}

main.o : main.cpp
	${CXX} -c main.cpp -o $@ ${CXXFLAGS}

cleanobj:
	rm *.o

clean:
	rm *.o
	rm httpsserver
	
