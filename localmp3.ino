 


//=== bring onother part ino file here ====//
#include "inputcontrol.h"
#include "display.h"
#include "module.h"


//**************************************************************************************************
//                                  S E L E C T N E X T S D N O D E                                *
//**************************************************************************************************
// Select the next or previous mp3 file from SD.  If the last selected song was random, the next   *
// track is a random one too.  Otherwise the next/previous node is choosen.                        *
// If nodeID is "0" choose a random nodeID.                                                        *
// Delta is +1 or -1 for next or previous track.                                                   *
// The nodeID will be returned to the caller.                                                      *
//**************************************************************************************************
String selectnextSDnode ( String curnod, int16_t delta )
{
  int16_t        inx, inx2 ;                           // Position in nodelist

  if ( hostreq )                                       // Host request already set?
  {
    return "" ;                                        // Yes, no action
  }
  dbgprint ( "SD_currentnode is %s, "
             "curnod is %s, "
             "delta is %d",
             SD_currentnode.c_str(),
             curnod.c_str(),
             delta ) ;
  if ( SD_currentnode == "0" )                         // Random playing?
  {
    return SD_currentnode ;                            // Yes, return random nodeID
  }
  else
  {
    inx = SD_nodelist.indexOf ( curnod ) ;             // Get position of current nodeID in list
    if ( delta > 0 )                                   // Next track?
    {
      inx += curnod.length() + 1 ;                     // Get position of next nodeID in list
      if ( inx >= SD_nodelist.length() )               // End of list?
      {
        inx = 0 ;                                      // Yes, wrap around
      }
    }
    else
    {
      if ( inx == 0 )                                  // At the begin of the list?
      {
        inx = SD_nodelist.length()  ;                  // Yes, goto end of list
      }
      inx-- ;                                          // Index of delimeter of previous node ID
      while ( ( inx > 0 ) &&
              ( SD_nodelist[inx - 1] != '\n' ) )
      {
        inx-- ;
      }
    }
    inx2 = SD_nodelist.indexOf ( "\n", inx ) ;         // Find end of node ID
  }
  return SD_nodelist.substring ( inx, inx2 ) ;         // Return nodeID
}



//**************************************************************************************************
//                                      G E T S D F I L E N A M E                                  *
//**************************************************************************************************
// Translate the nodeID of a track to the full filename that can be used as a station.             *
// If nodeID is "0" choose a random nodeID.                                                        *
//**************************************************************************************************
String getSDfilename ( String nodeID )
{
  String          res ;                                    // Function result
  File            root, file ;                             // Handle to root and directory entry
  uint16_t        n, i ;                                   // Current seqnr and counter in directory
  int16_t         inx ;                                    // Position in nodeID
  const char*     p = "/" ;                                // Points to directory/file
  uint16_t        rndnum ;                                 // Random index in SD_nodelist
  int             nodeinx = 0 ;                            // Points to node ID in SD_nodecount
  int             nodeinx2 ;                               // Points to end of node ID in SD_nodecount

  SD_currentnode = nodeID ;                                // Save current node
  if ( nodeID == "0" )                                     // Empty parameter?
  {
    dbgprint ( "getSDfilename random choice" ) ;
    rndnum = random ( SD_nodecount ) ;                     // Yes, choose a random node
    for ( i = 0 ; i < rndnum ; i++ )                       // Find the node ID
    {
      // Search to begin of the random node by skipping lines
      nodeinx = SD_nodelist.indexOf ( "\n", nodeinx ) + 1 ;
    }
    nodeinx2 = SD_nodelist.indexOf ( "\n", nodeinx ) ;     // Find end of node ID
    nodeID = SD_nodelist.substring ( nodeinx, nodeinx2 ) ; // Get node ID
  }
  dbgprint ( "getSDfilename requested node ID is %s",      // Show requeste node ID
             nodeID.c_str() ) ;
  while ( ( n = nodeID.toInt() ) )                         // Next sequence in current level
  {
    inx = nodeID.indexOf ( "," ) ;                         // Find position of comma
    if ( inx >= 0 )
    {
      nodeID = nodeID.substring ( inx + 1 ) ;              // Remove sequence in this level from nodeID
    }
    claimSPI ( "sdopen" ) ;                                // Claim SPI bus
    root = SD.open ( p ) ;                                 // Open the directory (this level)
    releaseSPI() ;                                         // Release SPI bus
    for ( i = 1 ; i <=  n ; i++ )
    {
      claimSPI ( "sdopenxt" ) ;                            // Claim SPI bus
      file = root.openNextFile() ;                         // Get next directory entry
      releaseSPI() ;                                       // Release SPI bus
      delay ( 10 ) ;                                       // Allow playtask
    }
    p = file.name() ;                                      // Points to directory- or file name
  }
  res = String ( "localhost" ) + String ( p ) ;            // Format result
  return res ;                                             // Return full station spec
}


//**************************************************************************************************
//                                  H A N D L E _ I D 3                                            *
//**************************************************************************************************
// Check file on SD card for ID3 tags and use them to display some info.                           *
// Extended headers are not parsed.                                                                *
//**************************************************************************************************
void handle_ID3 ( String &path )
{
  char*  p ;                                                // Pointer to filename
  struct ID3head_t                                          // First part of ID3 info
  {
    char    fid[3] ;                                        // Should be filled with "ID3"
    uint8_t majV, minV ;                                    // Major and minor version
    uint8_t hflags ;                                        // Headerflags
    uint8_t ttagsize[4] ;                                   // Total tag size
  } ID3head ;
  uint8_t  exthsiz[4] ;                                     // Extended header size
  uint32_t stx ;                                            // Ext header size converted
  uint32_t sttg ;                                           // Total tagsize converted
  uint32_t stg ;                                            // Size of a single tag
  struct ID3tag_t                                           // Tag in ID3 info
  {
    char    tagid[4] ;                                      // Things like "TCON", "TYER", ...
    uint8_t tagsize[4] ;                                    // Size of the tag
    uint8_t tagflags[2] ;                                   // Tag flags
  } ID3tag ;
  uint8_t  tmpbuf[4] ;                                      // Scratch buffer
  uint8_t  tenc ;                                           // Text encoding
  String   albttl = String() ;                              // Album and title

  tftset ( 2, "Playing from local file" ) ;                 // Assume no ID3
  p = (char*)path.c_str() + 1 ;                             // Point to filename
  showstreamtitle ( p, true ) ;                             // Show the filename as title (middle part)
  mp3file = SD.open ( path ) ;                              // Open the file
  mp3file.read ( (uint8_t*)&ID3head, sizeof(ID3head) ) ;    // Read first part of ID3 info
  if ( strncmp ( ID3head.fid, "ID3", 3 ) == 0 )
  {
    sttg = ssconv ( ID3head.ttagsize ) ;                    // Convert tagsize
    dbgprint ( "Found ID3 info" ) ;
    if ( ID3head.hflags & 0x40 )                            // Extended header?
    {
      stx = ssconv ( exthsiz ) ;                            // Yes, get size of extended header
      while ( stx-- )
      {
        mp3file.read () ;                                   // Skip next byte of extended header
      }
    }
    while ( sttg > 10 )                                     // Now handle the tags
    {
      sttg -= mp3file.read ( (uint8_t*)&ID3tag,
                             sizeof(ID3tag) ) ;             // Read first part of a tag
      if ( ID3tag.tagid[0] == 0 )                           // Reached the end of the list?
      {
        break ;                                             // Yes, quit the loop
      }
      stg = ssconv ( ID3tag.tagsize ) ;                     // Convert size of tag
      if ( ID3tag.tagflags[1] & 0x08 )                      // Compressed?
      {
        sttg -= mp3file.read ( tmpbuf, 4 ) ;               // Yes, ignore 4 bytes
        stg -= 4 ;                                         // Reduce tag size
      }
      if ( ID3tag.tagflags[1] & 0x044 )                     // Encrypted or grouped?
      {
        sttg -= mp3file.read ( tmpbuf, 1 ) ;               // Yes, ignore 1 byte
        stg-- ;                                            // Reduce tagsize by 1
      }
      if ( stg > ( sizeof(metalinebf) + 2 ) )               // Room for tag?
      {
        break ;                                             // No, skip this and further tags
      }
      sttg -= mp3file.read ( (uint8_t*)metalinebf,
                             stg ) ;                        // Read tag contents
      metalinebf[stg] = '\0' ;                              // Add delimeter
      tenc = metalinebf[0] ;                                // First byte is encoding type
      if ( tenc == '\0' )                                   // Debug all tags with encoding 0
      {
        dbgprint ( "ID3 %s = %s", ID3tag.tagid,
                   metalinebf + 1 ) ;
      }
      if ( ( strncmp ( ID3tag.tagid, "TALB", 4 ) == 0 ) ||  // Album title
           ( strncmp ( ID3tag.tagid, "TPE1", 4 ) == 0 ) )   // or artist?
      {
        albttl += String ( metalinebf + 1 ) ;               // Yes, add to string
        albttl += String ( "\n" ) ;                         // Add newline
      }
      if ( strncmp ( ID3tag.tagid, "TIT2", 4 ) == 0 )       // Songtitle?
      {
        tftset ( 2, metalinebf + 1 ) ;                      // Yes, show title
      }
    }
    tftset ( 1, albttl ) ;                                  // Show album and title
  }
  mp3file.close() ;                                         // Close the file
  mp3file = SD.open ( path ) ;                              // And open the file again
}
