void nvsopen(); 
esp_err_t nvsclear();
String nvsgetstr ( const char* key );
esp_err_t nvssetstr ( const char* key, String val );
void nvschkey ( const char* oldk, const char* newk );