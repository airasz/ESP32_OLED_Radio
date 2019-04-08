

//=== bring another part ino file here ====//
#include "inputcontrol.h"
#include "localmp3.h"
#include "module.h"



int tmrinfo2=0;
bool networkinfo=true;
String prevt;
bool showclock=false;
char* prevtextclock;

//===========================function for dynamicinfo > call every 0.5 seconds=========
void dynamicinfo(){                    
    tmrinfo+=1;
    
    if(tmrinfo==1){
        digitalWrite(pinleddummyload, HIGH);
    }else{digitalWrite(pinleddummyload, LOW);}
    if(tmrinfo>10){
      if(networkinfo==true){
        oledshow(0,"Wlan0 : "+WiFi.SSID() );
        if(isplaying){oledshow(2,radioinfo);}
        
        networkinfo=false;
      }else{
        oledshow(0,"IP    : "+WiFi.localIP().toString());
        
        if(isplaying){oledshow(2,radioinfo2);}
        networkinfo=true;
      }
      if(isplaying){
      
      } else {
      // statement
      }

      // gettime() ;  

      // oledshow(3,timetxt);                                   // Yes, get the current time
      if((WiFi.localIP().toString())=="0.0.0.0"){
        oledshow(0, "ESP32 WebRadio");
        oledshow(3,"connection down!");
      }else{
        oledshow(3,"");
      }
      //displaytime ( timetxt ) ; 
      tmrinfo=0;
      if (showclock==0){showclock=1;}  


    }

    //   gettime() ;
    // if(showclock==1&&timetxt!=prevtextclock){
    //   oledshow(3,timetxt);
    //   prevtextclock=timetxt;
    // }

    if ( (tmrinfo %2)==0){
      if(radioinfo!=prevt){
        if((WiFi.localIP().toString())=="0.0.0.0"){
            oledshow(2,"Connection down !\nAttempt to reconnecting");
            prevt="Connection down !\nAttempt to reconnecting";
        }else{

          oledshow(2,radioinfo);
          prevt=radioinfo;
        }
      }
    }
    curvsvol = vs1053player->getVolume() ; 
    oledshow(1, "preset : " + String(ini_block.newpreset)+" | volume : " +  String(curvsvol));
    //dbgprint(String(tmrinfo).c_str());
}   

String prevtext;
String prevtext1;
String prevtext2;
String prevtext3;

//function for oledshow
void oledshow(int row, String text){

switch (row) {
                        case 0:
                          dsp_fillRect ( 0, 0,                             // clear sector 0,12,128,24 for new text
                          dsp_getwidth(), 8, BLACK ) ;    
                          //dsp_update();
                          tftset(row,text);
                          break;
                        case 1:
                        if(text!=prevtext1){
                            
                          dsp_fillRect ( 0, 8,                             // clear sector 0,12,128,24 for new text
                          dsp_getwidth(), 16-8, BLACK ) ;    
                          //dsp_update();
                        }
                          prevtext1=text;
                          tftset(row,text);
                          break;
                        case 2:
                          dsp_fillRect ( 0, 16,                             // clear sector 0,12,128,24 for new text
                            dsp_getwidth(), 48-16, BLACK ) ;    
                        //  dsp_update();
                          tftset(row,text);
                          break;
                        case 3:

                          dsp_fillRect ( 0, 48,
                            dsp_getwidth(), 56-48, BLACK ) ;                    // Paint red part
                          dsp_fillRect ( 0, 52,
                            dsp_getwidth(), 1, RED ) ;                    // Paint red part
                          if(text!=prevtext3)
                          dsp_fillRect ( 0, 56,                             // clear sector 0,12,128,24 for new text
                            dsp_getwidth(), dsp_getheight()-56, BLACK) ;    
                          //dsp_update();

                          tftset(row,text);
                          prevtext3=text;
                          break;
                        
                          // do something
                    }                 
    
}   

//**************************************************************************************************
//                                      T F T S E T                                                *
//**************************************************************************************************
// Request to display a segment on TFT.  Version for char* and String parameter.                   *
//**************************************************************************************************
void tftset ( uint16_t inx, const char *str )
{
  if ( inx < TFTSECS )                                  // Segment available on display
  {
    if ( str )                                          // String specified?
    {
      tftdata[inx].str = String ( str ) ;               // Yes, set string
    }
    tftdata[inx].update_req = true ;                    // and request flag
  }
}

void tftset ( uint16_t inx, String& str )
{
  tftdata[inx].str = str ;                              // Set string
  tftdata[inx].update_req = true ;                      // and request flag
}



//**************************************************************************************************
//                                      U T F 8 A S C I I                                          *
//**************************************************************************************************
// UTF8-Decoder: convert UTF8-string to extended ASCII.                                            *
// Convert a single Character from UTF8 to Extended ASCII.                                         *
// Return "0" if a byte has to be ignored.                                                         *
//**************************************************************************************************
byte utf8ascii ( byte ascii )
{
  static const byte lut_C3[] =
  { "AAAAAAACEEEEIIIIDNOOOOO#0UUUU###aaaaaaaceeeeiiiidnooooo##uuuuyyy" } ;
  static byte       c1 ;              // Last character buffer
  byte              res = 0 ;         // Result, default 0

  if ( ascii <= 0x7F )                // Standard ASCII-set 0..0x7F handling
  {
    c1 = 0 ;
    res = ascii ;                     // Return unmodified
  }
  else
  {
    switch ( c1 )                     // Conversion depending on first UTF8-character
    {
      case 0xC2: res = '~' ;
        break ;
      case 0xC3: res = lut_C3[ascii - 128] ;
        break ;
      case 0x82: if ( ascii == 0xAC )
        {
          res = 'E' ;                 // Special case Euro-symbol
        }
    }
    c1 = ascii ;                      // Remember actual character
  }
  return res ;                        // Otherwise: return zero, if character has to be ignored
}


//**************************************************************************************************
//                                      U T F 8 A S C I I                                          *
//**************************************************************************************************
// In Place conversion UTF8-string to Extended ASCII (ASCII is shorter!).                          *
//**************************************************************************************************
void utf8ascii ( char* s )
{
  int  i, k = 0 ;                     // Indexes for in en out string
  char c ;

  for ( i = 0 ; s[i] ; i++ )          // For every input character
  {
    c = utf8ascii ( s[i] ) ;          // Translate if necessary
    if ( c )                          // Good translation?
    {
      s[k++] = c ;                    // Yes, put in output string
    }
  }
  s[k] = 0 ;                          // Take care of delimeter
}


//**************************************************************************************************
//                                      U T F 8 A S C I I                                          *
//**************************************************************************************************
// Conversion UTF8-String to Extended ASCII String.                                                *
//**************************************************************************************************
String utf8ascii ( const char* s )
{
  int  i ;                            // Index for input string
  char c ;
  String res = "" ;                   // Result string

  for ( i = 0 ; s[i] ; i++ )          // For every input character
  {
    c = utf8ascii ( s[i] ) ;          // Translate if necessary
    if ( c )                          // Good translation?
    {
      res += String ( c ) ;           // Yes, put in output string
    }
  }
  return res ;
}

 
//**************************************************************************************************
//                                          D B G P R I N T                                        *
//**************************************************************************************************
// Send a line of info to serial output.  Works like vsprintf(), but checks the DEBUG flag.        *
// Print only if DEBUG flag is true.  Always returns the formatted string.                         *
//**************************************************************************************************
char* dbgprint ( const char* format, ... )
{
  static char sbuf[DEBUG_BUFFER_SIZE] ;                // For debug lines
  va_list varArgs ;                                    // For variable number of params

  va_start ( varArgs, format ) ;                       // Prepare parameters
  vsnprintf ( sbuf, sizeof(sbuf), format, varArgs ) ;  // Format the message
  va_end ( varArgs ) ;                                 // End of using parameters
  if ( DEBUG )                                         // DEBUG on?
  {
    Serial.print ( "D: " ) ;                           // Yes, print prefix
    Serial.println ( sbuf ) ;                          // and the info
  }
  return sbuf ;                                        // Return stored string
}

String strlog;
int logingline=0;
//int loggingline=0;
//**************************************************************************************************
//                                           T F T L O G                                           *
//**************************************************************************************************
// Log to TFT if enabled.                                                                          *
//**************************************************************************************************
void tftlog ( const char *str )
{
  if ( tft )                                           // TFT configured?
  {
      logingline+=1;
      // dsp_setRotation() ;                                // Use landscape format
      // dsp_erase() ;                                      // Clear screen
      if(logingline<3){
        dsp_setTextSize ( 1 ) ;                            // Small character font
        dsp_setTextColor ( WHITE ) ;                       // Info in white
        dsp_setCursor ( 0, 16 ) ; 
        // strlog=strlog+str;
        dsp_println ( str ) ;                              // Yes, show error on TFT
        dsp_update() ;                                     // To physical screen   
      }
      if(logingline==4){
        dsp_setTextSize ( 1 ) ;                            // Small character font
        dsp_setTextColor ( WHITE ) ;                       // Info in white
        dsp_setCursor ( 0, 16 ) ; 
        // strlog=strlog+str;
        dsp_print ( str ) ;                              // Yes, show error on TFT
        dsp_update() ;                                     // To physical screen
      }
      if(logingline>3){
        dsp_fillRect ( 0, 16,                             // clear sector 0,12,128,24 for new text
        dsp_getwidth(), 48-16, BLACK ) ;    
        logingline=0;  
      }
      //                    dsp_update();
                          
  }
}



//**************************************************************************************************
//                                     S P F T A S K                                               *
//**************************************************************************************************
// Handles display of text, time and volume on TFT.                                                *
// Handles ADC meassurements.                                                                      *
// This task runs on a low priority.                                                               *
//**************************************************************************************************
void spftask ( void * parameter )
{
  while ( true )
  {
    handle_spec() ;                                                 // Maybe some special funcs?
    vTaskDelay ( 100 / portTICK_PERIOD_MS ) ;                       // Pause for a short time
    adcval = ( 15 * adcval +                                        // Read ADC and do some filtering
               adc1_get_raw ( ADC1_CHANNEL_0 ) ) / 16 ;
  }
  //vTaskDelete ( NULL ) ;                                          // Will never arrive here
}

//**************************************************************************************************
//                                   H A N D L E _ S P E C                                         *
//**************************************************************************************************
// Handle special (non-stream data) functions for spftask.                                         *
//**************************************************************************************************
void handle_spec()
{
  // Do some special function if necessary
  if ( dsp_usesSPI() )                                        // Does display uses SPI?
  {
    claimSPI ( "hspec" ) ;                                    // Yes, claim SPI bus
  }
  if ( tft )                                                  // Need to update TFT?
  {
    handle_tft_txt() ;                                        // Yes, TFT refresh necessary
    dsp_update() ;                                            // Be sure to paint physical screen
  }
  if ( dsp_usesSPI() )                                        // Does display uses SPI?
  {
    releaseSPI() ;                                            // Yes, release SPI bus
  }
  claimSPI ( "hspec" ) ;                                      // Claim SPI bus
  if ( muteflag )                                             // Mute or not?
  {
    vs1053player->setVolume ( 0 ) ;                           // Mute
  }
  else
  {
    vs1053player->setVolume ( ini_block.reqvol ) ;            // Unmute
  }
  if ( reqtone )                                              // Request to change tone?
  {
    reqtone = false ;
    vs1053player->setTone ( ini_block.rtone ) ;               // Set SCI_BASS to requested value
  }
  if ( time_req )                                             // Time to refresh timetxt?
  {
    time_req = false ;                                        // Yes, clear request
    if ( NetworkFound  )                                      // Time available?
    {
      gettime() ;                                             // Yes, get the current time
      //displaytime ( timetxt ) ;                               // Write to TFT screen
      //displayvolume() ;                                       // Show volume on display
      //displaybattery() ;                                      // Show battery charge on display
      dynamicinfo();
    }
  }
  releaseSPI() ;                                              // Release SPI bus
  if ( mqtt_on )
  {
    if ( !mqttclient.connected() )                            // See if connected
    {
      mqttreconnect() ;                                       // No, reconnect
    }
    else
    {
      mqttpub.publishtopic() ;                                // Check if any publishing to do
    }
  }
}


//**************************************************************************************************
//                                H A N D L E _ T F T _ T X T                                      *
//**************************************************************************************************
// Check if tft refresh is requested.                                                              *
//**************************************************************************************************
bool handle_tft_txt()
{
  for ( uint16_t i = 0 ; i < TFTSECS ; i++ )              // Handle all sections
  {
    if ( tftdata[i].update_req )                          // Refresh requested?
    {
      displayinfo ( i ) ;                                 // Yes, do the refresh
      dsp_update() ;                                      // Updates to the screen
      tftdata[i].update_req = false ;                     // Reset request
      return true ;                                       // Just handle 1 request
    }
  }
  return false ;                                          // Not a single request
}


//**************************************************************************************************
//                                      D I S P L A Y T I M E                                      *
//**************************************************************************************************
// Show the time on the LCD at a fixed position in a specified color                               *
// To prevent flickering, only the changed part of the timestring is displayed.                    *
// An empty string will force a refresh on next call.                                              *
// A character on the screen is 8 pixels high and 6 pixels wide.                                   *
//**************************************************************************************************
void displaytime ( const char* str, uint16_t color )
{
  static char oldstr[9] = "........" ;             // For compare
  uint8_t     i ;                                  // Index in strings
  uint8_t     pos = dsp_getwidth() + TIMEPOS ;     // X-position of character

  if ( str[0] == '\0' )                            // Empty string?
  {
    for ( i = 0 ; i < 8 ; i++ )                    // Set oldstr to dots
    {
      oldstr[i] = '.' ;
    }
    return ;                                       // No actual display yet
  }
  if ( tft )                                       // TFT active?
  {
    dsp_setTextColor ( color ) ;                   // Set the requested color
    for ( i = 0 ; i < 8 ; i++ )                    // Compare old and new
    {
      if ( str[i] != oldstr[i] )                   // Difference?
      {
       // dsp_fillRect ( pos, 0, 6, 8, BLACK ) ;     // Clear the space for new character
        dsp_setCursor ( pos, 0 ) ;                 // Prepare to show the info
        dsp_print ( str[i] ) ;                     // Show the character
        oldstr[i] = str[i] ;                       // Remember for next compare
      }
      pos += 6 ;                                   // Next position
    }
  }
}


//**************************************************************************************************
//                                      D I S P L A Y I N F O                                      *
//**************************************************************************************************
// Show a string on the LCD at a specified y-position (0..2) in a specified color.                 *
// The parameter is the index in tftdata[].                                                        *
//**************************************************************************************************
void displayinfo ( uint16_t inx )
{
  uint16_t       width = dsp_getwidth() ;                  // Normal number of colums
  scrseg_struct* p = &tftdata[inx] ;
  uint16_t len ;                                           // Length of string, later buffer length

  if ( inx == 0 )                                          // Topline is shorter
  {
    width += TIMEPOS ;                                     // Leave space for time
  }
  if ( tft )                                               // TFT active?
  {
   // dsp_fillRect ( 0, p->y, width, p->height, BLACK ) ;    // Clear the space for new info
    if ( ( dsp_getheight() > 64 ) && ( p->y > 1 ) )        // Need and space for divider?
    {
      dsp_fillRect ( 0, p->y - 4, width, 1, GREEN ) ;      // Yes, show divider above text
    }
    uint16_t len = p->str.length() ;                       // Required length of buffer
    if ( len++ )                                           // Check string length, set buffer length
    {
      char buf [ len ] ;                                   // Need some buffer space
      p->str.toCharArray ( buf, len ) ;                    // Make a local copy of the string
      utf8ascii ( buf ) ;                                  // Convert possible UTF8
      dsp_setTextColor ( p->color ) ;                      // Set the requested color
      dsp_setCursor ( 0, p->y ) ;                          // Prepare to show the info
      dsp_println ( buf ) ;                                // Show the string
    }
  }
}




//**************************************************************************************************
//* Function that are called from spftask.                                                         *
//**************************************************************************************************

//**************************************************************************************************
//                                      D I S P L A Y B A T T E R Y                                *
//**************************************************************************************************
// Show the current battery charge level on the screen.                                            *
// It will overwrite the top divider.                                                              *
// No action if bat0/bat100 not defined in the preferences.                                        *
//**************************************************************************************************
void displaybattery()
{
  if ( tft )
  {
    if ( ini_block.bat0 < ini_block.bat100 )              // Levels set in preferences?
    {
      static uint16_t oldpos = 0 ;                        // Previous charge level
      uint16_t        ypos ;                              // Position on screen
      uint16_t        v ;                                 // Constarinted ADC value
      uint16_t        newpos ;                            // Current setting

      v = constrain ( adcval, ini_block.bat0,             // Prevent out of scale
                      ini_block.bat100 ) ;
      newpos = map ( v, ini_block.bat0,                   // Compute length of green bar
                     ini_block.bat100,
                     0, dsp_getwidth() ) ;
      if ( newpos != oldpos )                             // Value changed?
      {
        oldpos = newpos ;                                 // Remember for next compare
        ypos = tftdata[1].y - 5 ;                         // Just before 1st divider
        dsp_fillRect ( 0, ypos, newpos, 2, GREEN ) ;      // Paint green part
        dsp_fillRect ( newpos, ypos,
                      dsp_getwidth() - newpos,
                      2, RED ) ;                          // Paint red part
      }
    }
  }
}


//**************************************************************************************************
//                                      D I S P L A Y V O L U M E                                  *
//**************************************************************************************************
// Show the current volume as an indicator on the screen.                                          *
// The indicator is 2 pixels heigh.                                                                *
//**************************************************************************************************
void displayvolume()
{
  if ( tft )
  {
    static uint8_t oldvol = 0 ;                         // Previous volume
    uint8_t        newvol ;                             // Current setting
    uint8_t        pos ;                                // Positon of volume indicator

    newvol = vs1053player->getVolume() ;                // Get current volume setting
    if ( newvol != oldvol )                             // Volume changed?
    {
      oldvol = newvol ;                                 // Remember for next compare
      pos = map ( newvol, 0, 100, 0, dsp_getwidth() ) ; // Compute position on TFT
      //dsp_fillRect (x,y,w,h,color)
      // dsp_fillRect ( 0, dsp_getheight() - 1,
      //                pos, 1, RED ) ;                    // Paint red part
      // dsp_fillRect ( pos, dsp_getheight() - 1,
      //                dsp_getwidth() - pos, 1, GREEN ) ; // Paint green part
    }
      if(newvol<100){
          dsp_fillRect ( dsp_getwidth()-16, 0,
                         16, 16, RED ) ;                    // Paint red part
          dsp_fillRect ( dsp_getwidth()-15, 0,
                         16, 15, GREEN ) ;        
          dsp_setTextColor ( GREEN ) ;   
          dsp_setCursor ( dsp_getwidth()-12, 4 ) ;          
      }else{
         dsp_fillRect ( dsp_getwidth()-20, 0,
                         20, 16, RED ) ;                    // Paint red part
          dsp_fillRect ( dsp_getwidth()-19, 0,
                         20, 15, GREEN ) ;        
          dsp_setTextColor ( GREEN ) ;   
          dsp_setCursor ( dsp_getwidth()-12, 4 ) ;
      }

      dsp_print(voll.c_str());
      dsp_update() ;  
       dsp_setTextColor ( WHITE ) ;   
  }

}
