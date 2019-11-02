#include <iostream>
#include <thread>
#include <boost/asio.hpp>

#include "HttpsServer.hpp"

std::string RESOURCE_DIRECTORY_PATH;

int main(int argc, char** argv)
{
  unsigned short port = 1234;

  if(argc < 2){
    std::cout<<"./httpserver <resource-directory-path>"<<std::endl;
    return 0;
  }

  std::string path(argv[1]);
  if(path.at(path.length() - 1) == '/')
    path.pop_back();
  
  RESOURCE_DIRECTORY_PATH = std::move(path);

  try{
    HttpsServer https_server;

    https_server.Start(port);

    //keep alive server for 5 minutes, delete this and next line
    //if you want to keep it alive for infinite time
    std::this_thread::sleep_for(std::chrono::seconds(60 * 5));

    https_server.Stop();
  }catch(boost::system::system_error &e){
    std::cout<<"Error occured, Error code = "<<e.code() 
             <<" Message: "<<e.what();
  }

  return 0;
}


