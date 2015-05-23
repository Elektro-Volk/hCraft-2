/*
 * hCraft 2 - A revised custom Minecraft server.
 * Copyright (C) 2015 Jacob Zhitomirsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WIN32

#include "os/http.hpp"
#include <curl/curl.h>


namespace hc {
  
  static size_t
  _write_data (void *ptr, size_t size, size_t nmemb, std::string *str)
  {
    char *arr = (char *)ptr;
    for (size_t i = 0; i < (size * nmemb); ++i)
      str->push_back (arr[i]);
    
    return size * nmemb;
  }
  
  
  /* 
   * Sends an HTTPS GET request to the specified server for the given resource.
   * Returns the returned response's body as a string.
   * 
   * Throws `http_error' on failure.
   */
  std::string
  https_get (const char *server, const std::string& resource, int timeout_sec)
  {
    CURL *curl = curl_easy_init ();
    if (!curl)
      throw http_error ("could not create curl object");
    
    std::string str;
    
    std::string uri = "https://";
    uri.append (server);
    uri.append (resource);
    
    curl_easy_setopt (curl, CURLOPT_TIMEOUT, timeout_sec);
    curl_easy_setopt (curl, CURLOPT_URL, uri.c_str ());
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, &_write_data);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, &str);
    
    int res = curl_easy_perform (curl);
    if (res != CURLE_OK)
      {
        curl_easy_cleanup (curl);
        throw http_error ("GET request failed");
      }
    
    curl_easy_cleanup (curl);
    return str;
  }
}

#endif

