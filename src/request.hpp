

#ifndef REQUEST_H
#define REQUEST_H

#include <curl/curl.h>
#include <string>

using namespace std;

class requests{
  public:
    static bool REQUESTS_STARTED;
    static void global_start();
    static void global_cleanup();
    requests();
    ~requests();
    bool get_page(string, string = "");
    bool save_file(string, string);
    long status_code;
    string text;
  private:
    static size_t callback(char *, size_t, size_t, string *);
    static size_t write_callback(char *, size_t, size_t, fstream *);
    CURL * curl;
};


#endif
