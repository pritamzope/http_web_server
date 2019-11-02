#ifndef HTTPSERVER_HPP
#define HTTPSERVER_HPP

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <boost/asio.hpp>

#include "HttpsAcceptor.hpp"

class HttpsServer
{
  public:
    HttpsServer();
    void Start(unsigned short);
    void Stop();

private:

  void StartAcceptor(unsigned short); 

  std::unique_ptr<std::thread> m_Thread;
	std::atomic<bool> m_Stop;
	boost::asio::io_service m_IOService;
};



#endif
