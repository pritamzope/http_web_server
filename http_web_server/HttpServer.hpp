#ifndef HTTPSERVER_HPP
#define HTTPSERVER_HPP

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <boost/asio.hpp>

#include "HttpAcceptor.hpp"

class HttpServer
{
  public:
    HttpServer();
    void Start(unsigned short, unsigned int);
    void Stop();

private:
    boost::asio::io_service m_IOService;
    std::unique_ptr<boost::asio::io_service::work> m_Work;
    std::unique_ptr<HttpAcceptor> m_Acceptor;
    std::vector<std::unique_ptr<std::thread>> m_ThreadPool;
};



#endif
