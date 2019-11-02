#include <fstream>
#include <atomic>
#include <thread>
#include <memory>
#include <chrono>
#include <ctime>
#include <iostream>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "HttpService.hpp"

using namespace boost;

/*
HTTP status codes
for more status codes https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
*/
std::unordered_map<unsigned int, std::string> HttpStatusTable =
{
  {101, "Switching Protocols"},
  {201, "Created"},
  {202, "Accepted"},
  {200, "200 OK" },
  {400, "Bad Request"},
  {401, "Unauthorized"},
  {404, "404 Not Found" },
  {408, "Request Timeout"},
  {413, "413 Request Entity Too Large" },
  {500, "500 Internal Server Error" },
  {501, "501 Not Implemented" },
  {502, "Bad Gateway"},
  {503, "Service Unavailable"},
  {505, "505 HTTP Version Not Supported" }
};


HttpRequestParser::HttpRequestParser(std::string& request):
    m_HttpRequest(request)
{}

//HTTP request parser, parses the request made by client
//stores it into HttpRequest structure and return it
std::shared_ptr<HttpRequest> HttpRequestParser::GetHttpRequest()
{
  if(m_HttpRequest.empty())  return nullptr;

  std::string request_method, resource, http_version;
  std::istringstream request_line_stream(m_HttpRequest);
  //extract request method, GET, POST, .....
  request_line_stream >> request_method;
  //extract requested resource
  request_line_stream >> resource;
  //extract HTTP version
  request_line_stream >> http_version;

  std::shared_ptr<HttpRequest> request(new HttpRequest);

  request->resource = std::move(resource);
  request->status = 0;

  if(request_method.compare("GET") == 0){
    request->method = HttpMethods::GET;
  }else if(request_method.compare("HEAD") == 0){
    request->method = HttpMethods::HEAD;
  }else if(request_method.compare("POST") == 0){
    request->method = HttpMethods::POST;
  }else if(request_method.compare("PUT") == 0){
    request->method = HttpMethods::PUT;
  }else if(request_method.compare("DELETE") == 0){
    request->method = HttpMethods::DELETE;
  }else if(request_method.compare("CONNECT") == 0){
    request->method = HttpMethods::CONNECT;
  }else if(request_method.compare("OPTIONS") == 0){
    request->method = HttpMethods::OPTIONS;
  }else if(request_method.compare("TRACE") == 0){
    request->method = HttpMethods::TRACE;
  }else{
    request->status = 400;
  }

  if(http_version.compare("HTTP/1.1") == 0){
    request->http_version = "1.1";
  }else{
    request->status = 505;
  }

  request->request = std::move(m_HttpRequest);

  return request;

}


extern std::string RESOURCE_DIRECTORY_PATH;


HttpService::HttpService(std::shared_ptr<asio::ip::tcp::socket> sock):
  m_Socket(sock),
  m_Request(4096),
  m_IsResponseSent(false)
{
}

//Handle each HTTP request made by client
void HttpService::HttpHandleRequest()
{
  //read the request from client
  asio::async_read_until(*m_Socket.get(),
        m_Request, '\r',
        [this](const boost::system::error_code& ec, 
               std::size_t bytes_transfered)
         {
           //get string from read stream
           std::string request_line;
           std::istream request_stream(&m_Request);
           std::getline(request_stream, request_line, '\0');

           //parse the string and get HTTP request
           HttpRequestParser parser(request_line);
           std::shared_ptr<HttpRequest> http_request = parser.GetHttpRequest();

           std::cout<<"[ Handling Client: "<<GetIp()<<" ]"<<std::endl;
           std::cout<<"Request: {"<<std::endl;
           std::cout<<http_request->request;
           std::cout<<"}"<<std::endl;

           std::istringstream istrstream(http_request->request);
           while(std::getline(istrstream, m_ScriptData)){}
             std::cout<<"ScriptData: "<<m_ScriptData<<std::endl;

           if(http_request->status == 0){
             m_RequestedResource = http_request->resource;
             //handle each method
             switch(http_request->method){
               case HttpMethods::GET :
                 ProcessGetRequest();
                 break;
               case HttpMethods::POST :
                 ProcessPostRequest();
                 break;
               case HttpMethods::HEAD :
                 ProcessHeadRequest();
                 break;
               case HttpMethods::DELETE :
                 ProcessDeleteRequest();
                 break;
               case HttpMethods::OPTIONS :
                 ProcessOptionsRequest();
                 break;
               default: break;
             }
          }else{
            m_ResponseStatusCode = http_request->status;
            if(!m_IsResponseSent)
              SendResponse();
              return;
         }

        });
}

//returns ip address of connected endpoint
std::string HttpService::GetIp()
{
  asio::ip::tcp::endpoint ep = m_Socket->remote_endpoint();
  asio::ip::address addr = ep.address();
  return std::move(addr.to_string());
}

//process the client GET request with requested resource
void HttpService::ProcessGetRequest()
{
  //if resource is empty then status code = 400
  if(m_RequestedResource.empty()){
    m_ResponseStatusCode = 400;
    SendResponse();
    return;
  }

  //if resource contains PHP script, then send it to POST request
  //for preprocessing php script
  if(boost::contains(m_RequestedResource, ".php") || 
     boost::contains(m_RequestedResource, ".cgi") || 
     boost::contains(m_RequestedResource, ".py") || 
     boost::contains(m_RequestedResource, ".pl")){
         ProcessPostRequest();
         return;
  }

  //if resource is null or / then set resource to index.html file
  if(m_RequestedResource.compare("/") == 0){
    m_RequestedResource = std::string("/index.html");
    std::ofstream outfile(RESOURCE_DIRECTORY_PATH + m_RequestedResource, std::ios::out);
    outfile<<m_DefaultIndexPage;
    outfile.close();
  }

  //get full resource file path
  std::string resource_file_path = RESOURCE_DIRECTORY_PATH + m_RequestedResource;

  //check if file exists
  //otherwise set status code to 404
  //and send response to client
  if(!boost::filesystem::exists(resource_file_path)){
    m_ResponseStatusCode = 404;
    SendResponse();
    return;
  }

  //open a requested file stream
  std::ifstream resource_fstream(resource_file_path, std::ifstream::binary);
  //if already open then status code to 500
  if (!resource_fstream.is_open()){
    m_ResponseStatusCode = 500;
    return;
  }

  //find out file size
  resource_fstream.seekg(0, std::ifstream::end);
  m_ResourceSizeInBytes = static_cast<std::size_t>(resource_fstream.tellg());

  //read file into buffer
  m_ResourceBuffer.reset(new char[m_ResourceSizeInBytes]);
  resource_fstream.seekg(std::ifstream::beg);
  resource_fstream.read(m_ResourceBuffer.get(), m_ResourceSizeInBytes);

  //send response with file
  SendResponse();

}

std::string HttpService::GetCGIProgram(std::string resource_file)
{
  std::ifstream in(resource_file, std::ios::in);
  char data[4096];
  while(in.getline(data, 4096)){
    std::string str(data);
    std::size_t find = str.find("#!");
    if(find != std::string::npos){
      std::string program = str.substr(find + 2, str.length() - 2);
      program.erase(remove_if(program.begin(), program.end(), isspace), program.end());
      in.close();
      return std::move(program);
    }
  }
  in.close();
  return "";
}

void HttpService::ProcessPostRequest()
{
  if(m_RequestedResource.empty()){
    m_ResponseStatusCode = 400;
    return;
  }

  std::string resource_file_path = RESOURCE_DIRECTORY_PATH + m_RequestedResource;

  if(!boost::filesystem::exists(resource_file_path)){
    m_ResponseStatusCode = 404;
    SendResponse();
    return;
  }

  //if resource contains .php substring
  if(boost::contains(m_RequestedResource, ".php")){
    //php command to execute
    std::string cmd = "php -f " + resource_file_path;

    //attach data to be passed to the script
    if(!m_ScriptData.empty())
      cmd = cmd + " \"" + m_ScriptData + "\"";
		
    //a temporary file where output of an PHP interpretor will be stores
    std::string php_output_file = RESOURCE_DIRECTORY_PATH + std::string("/tempfile");

    std::cout<<"Executing command: "<<cmd<<std::endl;

    //run php and get output
    ExecuteProgram(cmd, php_output_file);

    //copy output file contents from buffer to vector
    //for sending as a response to the client
    std::vector<asio::const_buffer> response_buffers;
    if(m_ResourceSizeInBytes > 0){
      response_buffers.push_back(asio::buffer(m_ResourceBuffer.get(), m_ResourceSizeInBytes));
    }

    //remove phpoutputfile
    std::remove(php_output_file.c_str());

    SendResponse();

  }else if(boost::contains(m_RequestedResource, ".cgi") || 
          boost::contains(m_RequestedResource, ".py") || 
          boost::contains(m_RequestedResource, ".pl")){

           std::string program = GetCGIProgram(resource_file_path);

           //if program name is empty and requested source contain .py,
           //then set program to python3 by default
           if(program.empty() && boost::contains(m_RequestedResource, ".py")){
             program = "/usr/bin/python3";
           }

           if(program.empty() && boost::contains(m_RequestedResource, ".pl")){
             program = "/usr/bin/perl";
           }

           //cgi program command to execute on server
           std::string cmd = program + " " + resource_file_path;

           //attach data to be passed to the script
           if(!m_ScriptData.empty())
             cmd = cmd + " \"" + m_ScriptData + "\"";

           std::cout<<"Executing command: "<<cmd<<std::endl;
           //a temporary file where output of an executed program will be stored
           std::string cgi_output_file = RESOURCE_DIRECTORY_PATH + std::string("/cgi_tempfile");

           ExecuteProgram(cmd, cgi_output_file);

           //copy output file contents from buffer to vector
           //for sending as a response to the client
           std::vector<asio::const_buffer> response_buffers;
           if(m_ResourceSizeInBytes > 0){
             response_buffers.push_back(asio::buffer(m_ResourceBuffer.get(), m_ResourceSizeInBytes));
           }

           //remove outputfile
           std::remove(cgi_output_file.c_str());

           SendResponse();

    }
}

void HttpService::ExecuteProgram(std::string command, std::string outputfile)
{
  boost::process::system(command, boost::process::std_out > outputfile);

  //set requested resouce to outputfile for response to client
  m_RequestedResource = outputfile;
		
  //read outputfile contents
  std::ifstream resource_fstream(outputfile, std::ifstream::binary);

  if(!resource_fstream.is_open()){
    m_ResponseStatusCode = 500;
    return;
  }

  m_ResponseStatusCode = 200;

  //find out file size
  resource_fstream.seekg(0, std::ifstream::end);
  m_ResourceSizeInBytes = static_cast<std::size_t>(resource_fstream.tellg());

  m_ResourceBuffer.reset(new char[m_ResourceSizeInBytes]);

  //read output file into resource buffer
  resource_fstream.seekg(std::ifstream::beg);
  resource_fstream.read(m_ResourceBuffer.get(), m_ResourceSizeInBytes);
}

//process head request by sending only headers not file
void HttpService::ProcessHeadRequest()
{
  if(m_RequestedResource.empty()){
    m_ResponseStatusCode = 400;
    SendResponse();
    return;
  }

  std::string resource_file_path = RESOURCE_DIRECTORY_PATH + m_RequestedResource;

  if(!boost::filesystem::exists(resource_file_path)){
    m_ResponseStatusCode = 404;
    SendResponse();
    return;
  }

  std::ifstream resource_fstream(resource_file_path, std::ifstream::binary);

  if(!resource_fstream.is_open()){
    m_ResponseStatusCode = 500;
    return;
  }

  resource_fstream.seekg(0, std::ifstream::end);
  m_ResourceSizeInBytes = static_cast<std::size_t>(resource_fstream.tellg());

  SendResponse();

}

//delete the requested resource
void HttpService::ProcessDeleteRequest()
{
  if(m_RequestedResource.empty()){
    m_ResponseStatusCode = 400;
    SendResponse();
    return;
  }

  std::string resource_file_path = RESOURCE_DIRECTORY_PATH + m_RequestedResource;

  if(!boost::filesystem::exists(resource_file_path)){
    m_ResponseStatusCode = 404;
    SendResponse();
    return;
  }

  std::remove(resource_file_path.c_str());

  SendResponse();

}

void HttpService::ProcessOptionsRequest()
{
  m_ServerOptions = "GET, POST, HEAD, DELETE, OPTIONS";
  SendResponse();
}

std::string HttpService::GetResponseStatus()
{
  std::string response_status;

  auto end = std::chrono::system_clock::now();
  std::time_t end_time = std::chrono::system_clock::to_time_t(end);
  std::string timestr(std::ctime(&end_time));

  m_Socket->shutdown(asio::ip::tcp::socket::shutdown_receive);

  auto status_line = HttpStatusTable[m_ResponseStatusCode];

  response_status = std::string("HTTP/1.1 ") + status_line + "\n";
  if(m_ResourceSizeInBytes > 0){
    response_status += std::string("Content-Length: ") +
                       std::to_string(m_ResourceSizeInBytes) + "\n";
  }
  if(!m_ContentType.empty()){
    response_status += m_ContentType + "\n";
  }else{
    response_status += std::string("Content-Type: text/html") + "\n";
  }
  if(!m_ServerOptions.empty()){
    response_status += std::string("Allow: ") + std::move(m_ServerOptions) + "\n";
  }
  response_status += std::string("Server: TinyHttpWebServer/0.0.1") + "\n";
  response_status += std::string("AcceptRanges: bytes") + "\n";
  response_status += std::string("Connection: Closed") + "\n";
  response_status += std::string("Date: ") + timestr + "\n";

  return std::move(response_status);
}

void HttpService::SendResponse()
{
  std::vector<asio::const_buffer> response_buffers;

  m_IsResponseSent = true;

  std::string response_status = GetResponseStatus();
  response_buffers.push_back(asio::buffer(std::move(response_status)));

  if(m_ResourceSizeInBytes > 0){
    response_buffers.push_back(asio::buffer(m_ResourceBuffer.get(), m_ResourceSizeInBytes));
  }

  //send response to client with data
  asio::async_write(*m_Socket.get(),
           response_buffers,
           [this](const boost::system::error_code& ec,
                 std::size_t bytes_transferred)
           {
             if(ec != 0){
               std::cout<<"Error occured, Error code = "<<ec.value()
                    <<" Message: "<<ec.message();
             }
             Finish();
           });
}

void HttpService::Finish()
{
  delete this;
}


