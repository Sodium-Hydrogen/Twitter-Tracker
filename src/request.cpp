#include <iostream>
#include <fstream>
#include "request.hpp"

using namespace std;

bool requests::REQUESTS_STARTED = false;

void requests::global_start(){
  if(!requests::REQUESTS_STARTED){
    curl_global_init(CURL_GLOBAL_DEFAULT);
    requests::REQUESTS_STARTED = true;
  }
}

void requests::global_cleanup(){
  if(requests::REQUESTS_STARTED){
    curl_global_cleanup();
  }
}

requests::requests(){
  requests::global_start();
  status_code = 0;
  text = "";
  curl = curl_easy_init();
#ifdef SKIP_PEER_VERIFICATION
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
}
requests::~requests(){
  if(curl){
    curl_easy_cleanup(curl);
  }
}

size_t requests::callback(char * buffer, size_t size, size_t num, string * data){
  *data += buffer;
  return size * num;
}

size_t requests::write_callback(char * buffer, size_t size, size_t num, fstream * data){
  data->write(buffer, num);
  return size * num;
}

bool requests::get_page(string url, string post_data){
  const char *c_url = url.c_str();
  CURLcode res;
  text = "";
  status_code = 0;
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, c_url);
    curl_easy_setopt(curl, CURLOPT_POST, false);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &text);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &callback);
    if(post_data != ""){
      const char * cpost = post_data.c_str();
      curl_easy_setopt(curl, CURLOPT_POST, true);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_data.size());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, cpost);
    }
    res = curl_easy_perform(curl);
    if(res != CURLE_OK){
      text = "";
      return false;
    }else{
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
      return true;
    }
  }
  return false;
}

bool requests::save_file(string filepath, string url){
  text = "not saved";
  status_code = 0;
  if(curl) {
    fstream file;
    file.open(filepath, fstream::out | fstream::binary);
    if(!file.is_open()){
      return "unable to open file";
    }
    curl_easy_setopt(curl, CURLOPT_POST, false);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback);
    const char *c_url = url.c_str();
    CURLcode res;
    curl_easy_setopt(curl, CURLOPT_URL, c_url);
    res = curl_easy_perform(curl);
    file.close();
    if(res != CURLE_OK){
      return false;
    }else{
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
      return true;
    }
  }
  return false;
}
