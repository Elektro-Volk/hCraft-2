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

#ifdef WIN32

#include "os/http.hpp"
#include "os/windows/stdafx.hpp"
#include <winhttp.h>


namespace hc {
  
  static std::wstring
  _to_unicode (const std::string& str)
  {
    std::wstring ret;
    for (int i = 0; i < str.length (); ++i)
      ret.push_back (str[i]);

    return ret;
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
    HINTERNET session = WinHttpOpen (L"hCraft server",
      WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME,
      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session)
      throw http_error ("https_get: could not create session");
    
    HINTERNET hconn = WinHttpConnect (session, _to_unicode (server).c_str (),
      INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hconn)
      {
        WinHttpCloseHandle (session);
        throw http_error ("https_get: could not connect to server");
      }
    
    HINTERNET hreq = WinHttpOpenRequest (hconn, L"GET",
      _to_unicode (resource).c_str (), NULL,
      WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hreq)
      {
        WinHttpCloseHandle (hconn);
        WinHttpCloseHandle (session);
        throw http_error ("https_get: could not open request");
      }
    
    WinHttpSetTimeouts (hreq, timeout_sec, timeout_sec, timeout_sec, timeout_sec);
    
    BOOL res = WinHttpSendRequest (hreq, WINHTTP_NO_ADDITIONAL_HEADERS,
      0, WINHTTP_NO_REQUEST_DATA, 0, WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH, NULL);
    if (!res)
      {
        WinHttpCloseHandle (hreq);
        WinHttpCloseHandle (hconn);
        WinHttpCloseHandle (session);
        throw http_error ("https_get: could not send request");
      }
    
    res = WinHttpReceiveResponse (hreq, NULL);
    if (!res)
      {
        WinHttpCloseHandle (hreq);
        WinHttpCloseHandle (hconn);
        WinHttpCloseHandle (session);
        throw http_error ("https_get: did not receive a response");
      }
    
    std::string ret;
    DWORD ds = 0;
    do
      {
        ds = 0;
        if (!WinHttpQueryDataAvailable (hreq, &ds))
          {
            WinHttpCloseHandle (hreq);
            WinHttpCloseHandle (hconn);
            WinHttpCloseHandle (session);
            throw http_error ("https_get: WinHttpQueryDataAvailable failed");
          }
        
        DWORD dw = 0;
        char *buf = new char [ds + 1];
        ZeroMemory (buf, ds + 1);
        if (!WinHttpReadData (hreq, (LPVOID)buf, ds, &dw))
          {
            WinHttpCloseHandle (hreq);
            WinHttpCloseHandle (hconn);
            WinHttpCloseHandle (session);
            throw http_error ("https_get: could not read data");
          }
        
        ret.append (buf);
        delete[] buf;
      }
    while (ds > 0);
    
    WinHttpCloseHandle (hreq);
    WinHttpCloseHandle (hconn);
    WinHttpCloseHandle (session);
    return ret;
  }
}

#endif

