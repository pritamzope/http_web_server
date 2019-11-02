#include <atomic>
#include <thread>
#include <memory>
#include <iostream>
#include <boost/asio.hpp>

#include "HttpService.hpp"
#include "HttpAcceptor.hpp"

using namespace boost;

//initialize TCP endpoint with IPv4
HttpAcceptor::HttpAcceptor(asio::io_service& ios, unsigned short port_num):
        m_IOService(ios),
        m_Acceptor(m_IOService, 
            asio::ip::tcp::endpoint(asio::ip::address_v4::any(), port_num)),
        m_IsStopped(false)
{}


void HttpAcceptor::Start()
{
  m_Acceptor.listen();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  AcceptConnection();
}

void HttpAcceptor::Stop()
{
  m_IsStopped.store(true);
}

//accept the client connection request
void HttpAcceptor::AcceptConnection()
{
  std::shared_ptr<asio::ip::tcp::socket> sock(new asio::ip::tcp::socket(m_IOService));

  m_Acceptor.async_accept(*sock.get(),
      [this, sock](const boost::system::error_code& ec)
      {
        if(ec == 0){
          (new HttpService(sock))->HttpHandleRequest();
        }else{
          std::cout<<"Error occured, Error code = "<<ec.value()
                   <<" Message: "<<ec.message();
        }

        // accept next request if not stopped yet
        if(!m_IsStopped.load()){
          AcceptConnection();
        }else{
          //stop accepting incoming connection requests
          m_Acceptor.close();
        }
      });
}



