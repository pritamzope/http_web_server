#include <atomic>
#include <thread>
#include <memory>
#include <iostream>
#include <boost/asio.hpp>

#include "HttpServer.hpp"

using namespace boost;

//create new work of io_service
HttpServer::HttpServer()
{
  m_Work.reset(new asio::io_service::work(m_IOService));
}

//start the http server
void HttpServer::Start(unsigned short port, unsigned int thread_pool_size)
{
  if(thread_pool_size <= 0)  return;

  //create and start HttpAcceptor for accepting connection requests
  m_Acceptor.reset(new HttpAcceptor(m_IOService, port));
  m_Acceptor->Start();

  std::cout<<"Server started at address: 127.0.0.1, port: 1234"<<std::endl;
  std::cout<<"Goto http://127.0.0.1:1234/"<<std::endl;

  //create specified number of threads and add them to the pool
  for(unsigned int i = 0; i < thread_pool_size; i++){
      std::unique_ptr<std::thread> 
             th(new std::thread([this]()
               {
                 //run the socket io service
                 m_IOService.run();
               })
             );

          m_ThreadPool.push_back(std::move(th));
    }
}

//stop the server.
void HttpServer::Stop()
{
  m_Acceptor->Stop();
  m_IOService.stop();
  for(auto& th : m_ThreadPool){
    th->join();
  }
}


