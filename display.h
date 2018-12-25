 

void tftset ( uint16_t inx, const char *str );
void tftset ( uint16_t inx, String& str );
byte utf8ascii ( byte ascii );
void utf8ascii ( char* s );
String utf8ascii ( const char* s );
char* dbgprint ( const char* format, ... );
void tftlog ( const char *str );
void spftask ( void * parameter );
void handle_spec();
bool handle_tft_txt();
void displayinfo ( uint16_t inx );
void displaytime ( const char* str, uint16_t color );
void displayvolume();
void displaybattery();
void oledshow(int row, String text);
void dynamicinfo();