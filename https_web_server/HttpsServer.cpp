#include <atomic>
#include <thread>
#include <memory>
#include <iostream>
#include <boost/asio.hpp>

#include "HttpsServer.hpp"

using namespace boost;

HttpsServer::HttpsServer()
{
  m_Stop = false;
}

//start the https server by starting accepting connections
void HttpsServer::Start(unsigned short port)
{
  m_Thread.reset(new std::thread([this, port]() {
     StartAcceptor(port);
  }));

  std::cout<<"Server started at address: 127.0.0.1, port: 1234"<<std::endl;
  std::cout<<"Goto https://127.0.0.1:1234/"<<std::endl;
}

void HttpsServer::StartAcceptor(unsigned short port)
{
  HttpsAcceptor acceptor(m_IOService, port);
  // continue thread m_Thread until m_Stop becomes true
  while(!m_Stop.load()) {
    acceptor.Start();
  }
}

//stop the server.
void HttpsServer::Stop()
{
  m_Stop.store(true);
  m_Thread->join();
}


