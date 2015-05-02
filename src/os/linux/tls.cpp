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

#include "os/tls.hpp"


namespace hc {
  
  /* 
   * Allocates a thread-local variable and sets |key| to point to its unique
   * identifier.
   * Returns zero on success.
   */
  int
  tls_alloc (tls_key_t *key, void (*fin) (void *ptr))
  {
    return pthread_key_create (key, nullptr);
  }
  
  /* 
   * Releases the thread-local variable.
   */
  int
  tls_free (tls_key_t key)
  {
    return pthread_key_delete (key);
  }
  
  /* 
   * Stores a value in the thread local-storage variable identified by the
   * specified key.
   * Returns zero on success.
   */
  int
  tls_set (tls_key_t key, void *ptr)
  {
    return pthread_setspecific (key, ptr);
  }
  
  /* 
   * Retrieves the value from a local-storage variable identified by the
   * specified key.
   */
  void*
  tls_get (tls_key_t key)
  {
    return pthread_getspecific (key);
  }
}

#endif

