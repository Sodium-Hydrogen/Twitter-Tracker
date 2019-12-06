#include "json.hpp"
#include <cstring>
#include <vector>

#include <iostream>

using namespace std;

json::json(string val, bool read_file){
  if(!read_file){
    contents = new char[val.size()+1];
    strcpy(contents, val.c_str());
    size = val.size();
  }else{
    contents = nullptr;
    filename = val;
    file.open(filename, fstream::in | fstream::ate | fstream::binary);
    if(!file.is_open()){
      throw "File " + filename + " couldn't be opened";
    }
    size = file.tellg();
    file.close();
    file.open(this->filename, fstream::in | fstream::binary);
    open = file.is_open();
    if(!file.is_open()){
      throw "File " + filename + " couldn't be opened";
    }
  }
}
json::~json(){
  if(open){
    file.close();
    open = false;
  }
  contents = nullptr;
}
void json::parse_json(){
  if(contents == nullptr){
    contents = new char[size + 1];
    file.read(contents, size);
    contents[size] = '\0';
  }
  parsed = parse_json(contents, size);
}
json_element json::parse_json(char * json_input, int json_size){
  if(json_size < 1){
    json_element val();
    return val;
  }
  if(json_input[0] == '['){
    int start = 1;
    int end = find_matching(json_input, json_size);
    map<int, json_element> arr;
    while(start<end){
      int length = find_matching(&json_input[start], json_size-start)+1;
      if(length == 0){
        char * loc = strchr(&json_input[start], ',');
        char * braket = strchr(&json_input[start], ']');
        if(loc == nullptr || start + (loc - &json_input[start]) >= json_size){
          if(braket == nullptr || start + (braket - &json_input[start]) >= json_size){
            break;
          }
          length = braket - &json_input[start];
        }else{
          length = loc - &json_input[start];
        }
      }
      json_element res = parse_json(&json_input[start], length);
      char * loc = strchr(&json_input[start + length], ',');
      if(loc != nullptr){
        start = loc - json_input + 1;
      }else{
        start = end;
      }
      arr[arr.size()] = res;
    }
    json_element retVal(arr);
    return retVal;
  }else if(json_input[0] == '{'){
    int end = find_matching(json_input, json_size);
    if(end != -1){
      int start = 1;
      map<string, json_element> arr;
      while(json_input[start] == '"' && start <= json_size){
        int key_length = find_matching(&json_input[start], json_size - start)-1;
        char * seperate = strchr(&json_input[start+key_length], ':');
        char key[key_length];
        if(key_length < 0){
          cout << start << " " << key_length << " " << json_input[start] << " " << json_size << endl;
        }
        memcpy(key, &json_input[start+1], key_length);
        key[key_length] = '\0';
        start = seperate - json_input + 1;
        //cout << json_input[start] << " " << start << " " << json_size << endl;
        int value_length = find_matching(&json_input[start], json_size - start);
        json_element val;
        if(value_length == -1){
          char * comma = strchr(&json_input[start], ',');
            value_length = comma - &json_input[start];
          if(comma == nullptr){
            value_length = end;
          }
          val = parse_json(&json_input[start], value_length);
        }else{
          value_length++;
          val = parse_json(&json_input[start], value_length);
        }
        string skey = key;
        arr.emplace(skey, val);
        start += value_length + 1;
      }
      json_element retVal(arr);
      return retVal;
    }
    json_element val;
    return val;
  }else if(json_input[0] == '"'){
    int length = find_matching(json_input, json_size)-1;
    char values[length];
    memcpy(values, &json_input[1], length);
    values[length] = '\0';
    string output = "";
    for(int i = 0; i < length; i++ ){
      if(values[i] == '\\'){
        if(values[i+1] == 'u'){
          char substring[5];
          memcpy(substring, &values[i+2], 4);
          substring[4] = '\0';
          uint32_t utf_value = stol(substring, nullptr, 16);
          char test[4] = "\0\0\0";
          if(utf_value >= 0x0800 && utf_value < 0xd800){
            test[0] = 0xe0 + ((utf_value >> 12) & 0xf);
            test[1] = 0x80 + ((utf_value >> 6) & 0x3f);
            test[2] = 0x80 + (utf_value & 0x3f);
          }else if(utf_value < 0x0800){
            test[0] = 0xc0 + ((utf_value >> 6) & 0x1f);
            test[1] = 0x80 + (utf_value & 0x3f);
          }else if(utf_value >= 0xd800){
            i+=6;
            memcpy(test, "�", 3);
          }else{
            test[0] = '?';
          }
          output += test;
          i+=5;
        }else{
          i++;
          if(values[i] == 'n'){
            output.push_back('\n');
          }else if(values[i] == 'r'){
            output.push_back('\r');
          }else if(values[i] == 't'){
            output.push_back('\t');
          }else{
            output.push_back(values[i]);
          }
        }
      }else{
        output.push_back(values[i]);
      }
    }
    json_element retVal(output);
    return retVal;
  }else{
    char substring[json_size+1];
    memcpy(substring, json_input, json_size);
    substring[json_size] = '\0';
    if(strchr(substring, '.') != nullptr){
      return stod(substring);
    }else if(strstr(substring, "false") != nullptr || strstr(substring, "true") != nullptr){
      return (strstr(substring, "true") != nullptr);
    }else if(strstr(substring, "null") != nullptr){
      json_element * retVal = new json_element;
      return *retVal;
    }else{
      return stoll(substring);
    }
  }
}
int json::find_matching(char * json_input, int json_size){
  char target = json_input[0];
  vector<char> still_open = {target};

  for(int i = 1; i < json_size; i++){
    if(json_input[i-1] == '\\' || (still_open.back() == '"' && json_input[i] != '"')){
      continue;
    }
    if((json_input[i] == ']' && still_open.back() == '[') ||
    (json_input[i] == '}' && still_open.back() == '{') ||
    (json_input[i] == '"' && still_open.back() == '"')){
      still_open.pop_back();
      if(still_open.size() == 0){
        return i;
      }
      continue;
    }
    if(json_input[i] == '[' || json_input[i] == '{' || json_input[i] == '"'){
      still_open.push_back(json_input[i]);
    }
  }
  return -1;
}

string json::get_filename(){
  if(this->open){
    return this->filename;
  }
  return "";
}

bool json::write_json(string filename, json_element save_json){
  fstream saveFile;
  saveFile.open(filename, fstream::out | fstream::binary);
  if(!saveFile.is_open()){
    return false;
  }
  string json_text = save_json.to_string();
  const char * json_data = json_text.c_str();
  saveFile.write(json_data, json_text.size());
  saveFile.close();
  return true;
}

void json::save_parsed(){
  if(this->open){
    file.close();
    open = false;
    write_json(this->filename, this->parsed);
  }
}

string json_element::get_value(){
  string * output = new string;
  *output = "null";
  switch(this->key){
    case 'b': *output = ((this->bval)?"true":"false"); break;
    case 'd': *output = std::to_string(this->dval); break;
    case 's': *output = sval; break;
    case 'v': *output = "array"; break;
    case 'm': *output = "assosiative array"; break;
    case 'f':
      *output = std::to_string(this->fval);
      while(output->back() == '0'){
        output->pop_back();
      }
      if(output->back() == '.')
        output->pop_back();
      break;
  }
  return *output;
}

vector<string> json_element::get_keys(){
  vector<string> * keys = new vector<string>;
  if(this->key == 'm'){
    for(auto content: this->mval){
      keys->push_back(content.first);
    }
  }
  return *keys;
}

int json_element::size(){
  if(this->key == 'v'){
    return this->vval.size();
  }
  return -1;
}

void json_element::push_back(json_element child){
  if(this->key == 'N'){
    this->key = 'v';
    this->vval.clear();
  }
  if(this->key != 'v'){
    return;
  }
  this->vval[this->vval.size()] = child;
}

json_element json_element::pop(int index){
  json_element * child = new json_element;
  if(this->key == 'v'){
    json_element tmp;
    for(auto item: this->vval){
      if(!(item.first == index)){
        tmp.push_back(item.second);
      }else{
        *child = this->vval[index];
      }
    }
    *this = tmp;
  }
  return child;
}

json_element json_element::pop(string key){
  json_element * child = new json_element;
  if(this->key == 'm'){
    *child = this->mval[key];
    this->mval.erase(key);
  }
  return child;
}

void json_element::remove_value(json_element target){
  if(this->key == 'v'){
    json_element tmp;
    for(auto item: this->vval){
      if(!(item.second == target)){
        tmp.push_back(item.second);
      }
    }
    *this = tmp;
  }
}

string json_element::to_string(){
  string * output = new string;
  switch(this->key){
    case 's': *output = "\"" + sval + "\""; break;
    case 'v':
      *output = "[";
      for(auto& value: vval){
        string subItem = value.second.to_string();
        if(subItem != "null"){
          *output += subItem + ",";
        }
      }
      output->back() = ']';
      if(*output == "]"){
        *output = "null";
        *this = new json_element;
      }
      break;
    case 'm':
      *output = "{";
      for(auto& value: mval){
        string subItem = value.second.to_string();
        if(subItem != "null"){
          *output += "\"" + value.first + "\":" + subItem + ",";
        }
      }
      output->back() = '}';
      if(*output == "}"){
        *output = "null";
        *this = new json_element;
      }
      break;
    default: *output = this->get_value(); break;
  }
  return *output;
}

ostream & operator <<(ostream & os, json_element& elem){
  os << elem.to_string();
  return os;
}
bool json_element::isnull(){
  return this->key == 'N';
}
json_element::json_element(){
  bval = 0;
  dval = 0;
  fval = 0;
  sval = "";
  vval.clear();
  mval.clear();
  key = 'N';
}
bool json_element::isarray(){return key == 'v';}
bool json_element::isdict(){return key == 'm';}
json_element & json_element::operator [](string val){
  if(key == 'N'){
    key = 'm';
  }
  return mval[val];
}
json_element & json_element::operator [](int index){
  if(key == 'N'){
    key = 'v';
  }
  return vval[index];
}
bool json_element::operator ==(json_element&  val){
  return ( this->key == val.key && this->to_string() == val.to_string() ) ? true : false;
}
json_element::json_element(bool                       val):json_element(){bval=val;key='b';}
json_element::json_element(long long                  val):json_element(){dval=val;key='d';}
json_element::json_element(double                     val):json_element(){fval=val;key='f';}
json_element::json_element(string                     val):json_element(){sval=val;key='s';}
json_element::json_element(map<int, json_element>     val):json_element(){vval=val;key='v';}
json_element::json_element(map<string, json_element>  val):json_element(){mval=val;key='m';}
bool json_element::operator ==(bool&          val){return (this->key=='b'&&val==this->bval)?true:false;}
bool json_element::operator ==(long long&     val){return (this->key=='d'&&val==this->dval)?true:false;}
bool json_element::operator ==(double&        val){return (this->key=='f'&&val==this->fval)?true:false;}
bool json_element::operator ==(string&        val){return (this->key=='s'&&val==this->sval)?true:false;}
json_element & json_element::operator =(bool&                       val){*this = json_element(val);return *this;}
json_element & json_element::operator =(long long&                  val){*this = json_element(val);return *this;}
json_element & json_element::operator =(double&                     val){*this = json_element(val);return *this;}
json_element & json_element::operator =(string&                     val){*this = json_element(val);return *this;}
json_element & json_element::operator =(map<int, json_element>&     val){*this = json_element(val);return *this;}
json_element & json_element::operator =(map<string, json_element>&  val){*this = json_element(val);return *this;}
