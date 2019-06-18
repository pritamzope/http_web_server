#ifndef HTTPACCEPTOR_HPP
#define HTTPACCEPTOR_HPP

#include <string>
#include <memory>
#include <atomic>
#include <boost/asio.hpp>

//a HTTP coonection acceptor
class HttpAcceptor
{
  public:
    HttpAcceptor(boost::asio::io_service&, unsigned short);
    void Start();
    void Stop();

  private:
    void AcceptConnection();

  private:
    boost::asio::io_service& m_IOService;
    boost::asio::ip::tcp::acceptor m_Acceptor;
    std::atomic<bool> m_IsStopped;
};


#endif
