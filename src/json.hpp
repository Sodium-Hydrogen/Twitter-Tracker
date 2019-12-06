
#ifndef JSON_H
#define JSON_H

#include <fstream>
#include <map>
#include <vector>

using namespace std;


class json_element{
  public:
    json_element();
    json_element(bool);
    json_element(long long);
    json_element(double);
    json_element(string);
    json_element(map<int, json_element>);
    json_element(map<string, json_element>);

    bool isarray();
    bool isdict();
    bool isnull();
    string to_string();
    string get_value();
    vector<string> get_keys();
    int size();
    void push_back(json_element);
    json_element pop(int);
    json_element pop(string);
    void remove_value(json_element);

    json_element & operator [](string);
    json_element & operator [](int);

    json_element& operator =(bool&);
    json_element& operator =(long long&);
    json_element& operator =(double&);
    json_element& operator =(string&);
    json_element& operator =(map<int, json_element>&);
    json_element& operator =(map<string, json_element>&);

    bool operator ==(bool&);
    bool operator ==(long long&);
    bool operator ==(double&);
    bool operator ==(string&);
    bool operator ==(json_element&);

    friend ostream & operator <<(ostream &, json_element&);
  private:
    char key;
    bool bval;
    double fval;
    string sval;
    long long dval;
    map<int, json_element> vval;
    map<string, json_element> mval;
};

class json{

  public:
    static const bool READ_FILE = true;
    static bool write_json(string, json_element);
    json(string, bool = false);
    ~json();
    void parse_json();
    json_element parsed;
    string get_filename();
    void save_parsed();

  private:
    json_element parse_json(char *, int);
    int find_matching(char *, int);
    bool open;
    char * contents;
    string filename;
    fstream file;
    int size;
};




#endif
