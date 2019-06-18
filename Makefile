CXX=g++
CXXFLAGS=-std=c++11 -lboost_system -lboost_filesystem -lpthread
OBJFILES=HttpService.o HttpAcceptor.o HttpServer.o main.o

httpserver : ${OBJFILES}
	${CXX} ${OBJFILES} -o $@ ${CXXFLAGS}
	
HttpService.o : HttpService.cpp
	${CXX} -c  HttpService.cpp -o $@ ${CXXFLAGS}

HttpAcceptor.o : HttpAcceptor.cpp
	${CXX} -c HttpAcceptor.cpp -o $@ ${CXXFLAGS}

HttpServer.o : HttpServer.cpp
	${CXX} -c HttpServer.cpp -o $@ ${CXXFLAGS}

main.o : main.cpp
	${CXX} -c main.cpp -o $@ ${CXXFLAGS}

cleanobj:
	rm *.o

clean:
	rm *.o
	rm httpserver
	
