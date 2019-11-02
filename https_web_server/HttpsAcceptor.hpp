#ifndef HTTPACCEPTOR_HPP
#define HTTPACCEPTOR_HPP

#include <string>
#include <memory>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

//a HTTPS coonection acceptor
class HttpsAcceptor
{
  public:
    HttpsAcceptor(boost::asio::io_service&, unsigned short);
    void Start();
    void Stop();

  private:
    void AcceptConnection();
    std::string get_password(std::size_t, boost::asio::ssl::context::password_purpose) const;

  private:
    boost::asio::io_service& m_IOService;
    boost::asio::ip::tcp::acceptor m_Acceptor;
    std::atomic<bool> m_IsStopped;
    boost::asio::ssl::context m_SSLContext;
};


#endif
