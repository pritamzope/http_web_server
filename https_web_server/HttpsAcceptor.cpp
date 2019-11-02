#include <atomic>
#include <thread>
#include <memory>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "HttpsService.hpp"
#include "HttpsAcceptor.hpp"

using namespace boost;

//initialize TCP endpoint with IPv4 and ssl context with sslv23 server
HttpsAcceptor::HttpsAcceptor(boost::asio::io_service& ios, unsigned short port):
        m_IOService(ios),
        m_Acceptor(m_IOService, 
            asio::ip::tcp::endpoint(asio::ip::address_v4::any(), port)),
        m_IsStopped(false),
        m_SSLContext(asio::ssl::context::sslv23_server)
{
  //setting up the ssl context.
  m_SSLContext.set_options(
      boost::asio::ssl::context::default_workarounds | 
      boost::asio::ssl::context::no_sslv2 | 
      boost::asio::ssl::context::single_dh_use);

  // set ssl certification file
  m_SSLContext.use_certificate_chain_file("HttpsWebServer.cert");
  // set ssl private key file
  m_SSLContext.use_private_key_file("HttpsWebServer.key", boost::asio::ssl::context::pem);
}


void HttpsAcceptor::Start()
{
  // listen for clients
  m_Acceptor.listen();
  // create new ssl stream with ioservice & above created context
  boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_stream(m_IOService, m_SSLContext);

  // accept client request
  m_Acceptor.accept(ssl_stream.lowest_layer());

  // create Https service and handle that request
  HttpsService service;
  service.HttpsHandleRequest(ssl_stream);

}

void HttpsAcceptor::Stop()
{
  m_IsStopped.store(true);
}




