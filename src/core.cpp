#include "core.hpp"
#include <iostream>
#include <ctime>
//#include <cstring>

using namespace std;

const string COREURL_BASEURL    = "https://mikej.tech/twitter.php";
const string COREURL_VERIFY     = "/validate";
const string COREURL_USERINFO   = "/get_user_info";
const string COREURL_GETFOLLOW  = "/get_followers";
const string COREURL_FOLLOWINFO = "/get_follower_info";
const string COREURL_APPKEY     = "?application_key=";
const string COREURL_GETKEY     = "?verify_user";
string CORE_APPPATH = "";
json_element CORE_APPKEY;
json_element CORE_USER;
json_element CORE_FOLLOWERS;
//bool DOWNLOAD_IMGS = false;

#ifdef CORE_MAIN
int main(int argc, char ** argv){

  //requests req;
  //
  //req.get_page("https://mikej.tech/twitter.php?application_key");
  //
  //json res(req.text);
  //res.parse_json();
  //cout << res.parsed["message"] << endl;

  //string filename = "src/json/example.json";
  //
  //try{
  //  json file(filename, true);
  //  file.parse_json();
  //  cout << file.parsed << endl;
  //}catch(string e){
  //  cerr << e;
  //}

  string application_key = "oCbTWYFJOy61rva07AZl92vZwjccHyjLbL5hB2spL46UrRp7dW";
  cout << boolalpha << verify_key(application_key);
  requests::global_cleanup();

}
#endif

bool verify_key(string key){
  requests req;
  req.get_page(COREURL_BASEURL + COREURL_VERIFY + COREURL_APPKEY + key);
  return req.status_code == 200;
}
bool verify_key(json_element key){
  return verify_key(key.get_value());
}

bool save_key(string application_key, string path){
  json_element key;
  bool valid_key = verify_key(application_key);
  if(valid_key){
    key["application_key"] = application_key;
    key["time_created"] = (long long)time(NULL);
    CORE_APPKEY = key;
    return json::write_json(path + "/application_key.json", key);
  }else{
    return false;
  }
}

void get_update(bool save_pics){
  DOWNLOAD_IMGS = save_pics;
  requests req;
  req.get_page(COREURL_BASEURL + COREURL_USERINFO + COREURL_APPKEY + CORE_APPKEY["application_key"].get_value());
  if(req.status_code == 200){
    json res(req.text);
    res.parse_json();
    CORE_USER = res.parsed[0];
  }
  if(DOWNLOAD_IMGS){
    string img_url = CORE_USER["profile_image_url"].get_value();
    string img_path = CORE_APPPATH + "/profile";
    img_path += img_url.substr(img_url.rfind("."));
    CORE_USER["saved_image_path"] = img_path;
    req.save_file(img_path, img_url);
  }
  req.get_page(COREURL_BASEURL + COREURL_GETFOLLOW + COREURL_APPKEY + CORE_APPKEY["application_key"].get_value());
  if(req.status_code == 200){
    json follower_ids(req.text);
    follower_ids.parse_json();
    id_new_followers(follower_ids.parsed);
  }
  try{
    json followers(CORE_APPPATH + "/follower_ids.json", json::READ_FILE);
    json follower_ids(CORE_APPPATH + "/follower_info.json", json::READ_FILE);
    followers.parse_json();
    follower_ids.parse_json();
    CORE_FOLLOWERS["ids"] = followers.parsed;
    CORE_FOLLOWERS["info"] = follower_ids.parsed;
  }catch(string e){}
}

json_element id_new_followers(json_element res, string filename){
  try{
    json ids(CORE_APPPATH + filename, json::READ_FILE);
    ids.parse_json();
    json_element already_following;
    if(ids.parsed.isdict()){
      vector<string> keys = ids.parsed.get_keys();
      for(string key: keys){
        if(ids.parsed[key]["added"].isarray()){
          for(int i = 0; i < ids.parsed[key]["added"].size(); i++){
            already_following.push_back(ids.parsed[key]["added"][i]);
          }
        }
        if(ids.parsed[key]["removed"].isarray()){
          for(int i = 0; i < ids.parsed[key]["removed"].size(); i++){
            already_following.remove_value(ids.parsed[key]["removed"][i]);
          }
        }
      }
      json_element no_match;
      for(int i = 0; i < already_following.size(); i++){
        bool found = false;
        for(int n = 0; n < res["ids"].size(); n++){
          if(already_following[i] == res["ids"][n]){
            found = true;
            res["ids"].pop(n);
            break;
          }
        }
        if(!found){
          no_match.push_back(already_following[i]);
        }
      }
      json_element * to_return = new json_element;
      (*to_return)["added"] = res["ids"];
      (*to_return)["removed"] = no_match;
      ids.parsed[to_string(time(NULL))] = *to_return;
      ids.save_parsed();
      update_follower_info(*to_return);
      return *to_return;
    }
  }catch(string e){
    json_element * save = new json_element;
    string key = to_string(time(NULL));
    (*save)[key]["added"] = res["ids"];
    json::write_json(CORE_APPPATH + filename, *save);
    update_follower_info((*save)[key]);
    return (*save)[key];
  }
  return false;
}

void update_follower_info(json_element follower_changes, string filename){
  if(follower_changes.to_string() == "null"){
    return;
  }
  json_element old_info;
  try{
    json follower_info(CORE_APPPATH + filename, json::READ_FILE);
    follower_info.parse_json();
    old_info = follower_info.parsed;
  }catch(string e){
  }
  for(int i = 0; i < follower_changes["removed"].size(); i++){
    follower_changes["added"].push_back(follower_changes["removed"][i]);
  }
  string post_data = follower_changes["added"].to_string();
  if(post_data == "null"){
    return;
  }else{
    post_data = post_data.substr(1, post_data.size()-2);
  }
  post_data = "user_fields=id,name,screen_name,profile_image_url&user_ids=" + post_data;
  requests req;
  req.get_page(COREURL_BASEURL + COREURL_FOLLOWINFO + COREURL_APPKEY + CORE_APPKEY["application_key"].get_value(), post_data);
  if(req.status_code == 200){
    json res(req.text);
    res.parse_json();
    for(int i = 0; i < res.parsed.size(); i++){
      string id = res.parsed[i]["id"].get_value();
      res.parsed[i].pop("id");
      old_info[id] = res.parsed[i];
      old_info[id]["account_exists"] = true;
      for(int n = 0; n < follower_changes["added"].size(); n++){
        if(follower_changes["added"][n].get_value() == id){
          follower_changes["added"].pop(n);
        }
      }
      if(DOWNLOAD_IMGS){
        string img_url = old_info[id]["profile_image_url"].get_value();
        string img_path = CORE_APPPATH + "/imgs/" + id;
        img_path += img_url.substr(img_url.rfind("."));
        req.save_file(img_path, img_url);
        old_info[id]["saved_image_path"] = img_path;
      }
    }
    for(int i = 0; i < follower_changes["added"].size(); i++){
      old_info[follower_changes["added"][i].get_value()]["account_exists"] = false;
    }
  }
  json::write_json(CORE_APPPATH + filename, old_info);
}
