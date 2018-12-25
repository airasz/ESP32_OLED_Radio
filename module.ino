

//=== bring onother part ino file here ====//
#include "inputcontrol.h"
#include "localmp3.h"
#include "display.h"

//**************************************************************************************************
//                                      N V S O P E N                                              *
//**************************************************************************************************
// Open Preferences with my-app namespace. Each application module, library, etc.                  *
// has to use namespace name to prevent key name collisions. We will open storage in               *
// RW-mode (second parameter has to be false).                                                     *
//**************************************************************************************************
void nvsopen()
{
  if ( ! nvshandle )                                         // Opened already?
  {
    nvserr = nvs_open ( NAME, NVS_READWRITE, &nvshandle ) ;  // No, open nvs
    if ( nvserr )
    {
      dbgprint ( "nvs_open failed!" ) ;
    }
  }
}


//**************************************************************************************************
//                                      N V S C L E A R                                            *
//**************************************************************************************************
// Clear all preferences.                                                                          *
//**************************************************************************************************
esp_err_t nvsclear()
{
  nvsopen() ;                                         // Be sure to open nvs
  return nvs_erase_all ( nvshandle ) ;                // Clear all keys
}


//**************************************************************************************************
//                                      N V S G E T S T R                                          *
//**************************************************************************************************
// Read a string from nvs.                                                                         *
//**************************************************************************************************
String nvsgetstr ( const char* key )
{
  static char   nvs_buf[NVSBUFSIZE] ;       // Buffer for contents
  size_t        len = NVSBUFSIZE ;          // Max length of the string, later real length

  nvsopen() ;                               // Be sure to open nvs
  nvs_buf[0] = '\0' ;                       // Return empty string on error
  nvserr = nvs_get_str ( nvshandle, key, nvs_buf, &len ) ;
  if ( nvserr )
  {
    dbgprint ( "nvs_get_str failed %X for key %s, keylen is %d, len is %d!",
               nvserr, key, strlen ( key), len ) ;
    dbgprint ( "Contents: %s", nvs_buf ) ;
  }
  return String ( nvs_buf ) ;
}



//**************************************************************************************************
//                                      N V S S E T S T R                                          *
//**************************************************************************************************
// Put a key/value pair in nvs.  Length is limited to allow easy read-back.                        *
// No writing if no change.                                                                        *
//**************************************************************************************************
esp_err_t nvssetstr ( const char* key, String val )
{
  String curcont ;                                         // Current contents
  bool   wflag = true  ;                                   // Assume update or new key

  //dbgprint ( "Setstring for %s: %s", key, val.c_str() ) ;
  if ( val.length() >= NVSBUFSIZE )                        // Limit length of string to store
  {
    dbgprint ( "nvssetstr length failed!" ) ;
    return ESP_ERR_NVS_NOT_ENOUGH_SPACE ;
  }
  if ( nvssearch ( key ) )                                 // Already in nvs?
  {
    curcont = nvsgetstr ( key ) ;                          // Read current value
    wflag = ( curcont != val ) ;                           // Value change?
  }
  if ( wflag )                                             // Update or new?
  {
    //dbgprint ( "nvssetstr update value" ) ;
    nvserr = nvs_set_str ( nvshandle, key, val.c_str() ) ; // Store key and value
    if ( nvserr )                                          // Check error
    {
      dbgprint ( "nvssetstr failed!" ) ;
    }
  }
  return nvserr ;
}


//**************************************************************************************************
//                                      N V S C H K E Y                                            *
//**************************************************************************************************
// Change a keyname in in nvs.                                                                     *
//**************************************************************************************************
void nvschkey ( const char* oldk, const char* newk )
{
  String curcont ;                                         // Current contents

  if ( nvssearch ( oldk ) )                                // Old key in nvs?
  {
    curcont = nvsgetstr ( oldk ) ;                         // Read current value
    nvs_erase_key ( nvshandle, oldk ) ;                    // Remove key
    nvssetstr ( newk, curcont ) ;                          // Insert new
  }
}
