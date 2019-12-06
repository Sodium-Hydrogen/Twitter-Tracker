
#ifndef CORE_H
#define CORE_H

#include "json.hpp"
#include "request.hpp"

//extern const string CORE_GET_KEY  = "/verify_user";
extern const string COREURL_BASEURL;
extern const string COREURL_VERIFY;
extern const string COREURL_USERINFO;
extern const string COREURL_GETFOLLOW;
extern const string COREURL_FOLLOWINFO;
extern const string COREURL_APPKEY;
extern const string COREURL_GETKEY;
extern json_element CORE_APPKEY;
extern json_element CORE_USER;
extern json_element CORE_FOLLOWERS;
extern string CORE_APPPATH;
static bool DOWNLOAD_IMGS = false;

bool verify_key(string);
bool verify_key(json_element);

bool save_key(string, string);

void get_update(bool = false);

json_element id_new_followers(json_element, string = "/follower_ids.json");

void update_follower_info(json_element, string = "/follower_info.json");


#endif
