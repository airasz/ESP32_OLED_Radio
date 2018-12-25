// Default preferences in raw data format for PROGMEM
//
#define defaultprefs_version 180609
const char defprefs_txt[] PROGMEM = R"=====(
# Example configuration
# Programmable input pins:
#gpio_00 = uppreset = 1
#gpio_12 = upvolume = 2
#gpio_13 = downvolume = 2
#gpio_14 = stop
#gpio_17 = resume
#gpio_34 = station = live2.radiorodja.com:8000
#
#touch_04 = uppreset = 1
#touch_05 = downpreset = 1
# MQTT settings
mqttbroker = none
mqttport = 1883
mqttuser = none
mqttpasswd = none
mqqprefix = none
# Enter your WiFi network specs here:
wifi_00 = rumah/GIGIBOLONG
wifi_01 = al_ghuroba/air46664
#
volume = 72
toneha = 0
tonehf = 0
tonela = 0
tonelf = 0
#
preset = 6
# Some preset examples
preset_00 = live2.radiorodja.com:8000      			#  0 - Radio Rodja Low
preset_01 = live.radiomuslim.com/;stream/1			#  1 - Radio Muslim Jogja
preset_02 = live.insanifm.com:8989					#  2 - Radio insani FM Purbalingga
preset_03 = streaming.an-nashihah.com:8022 			#  3 - Radio an-nashihah makasar
preset_04 = live.suaraaliman.com:80					#  4 - Radio suara al iman surabaya
preset_05 = 119.82.232.93:80              			#  5 - idza'atul khair ponorogo
preset_06 = live.radiosunnah.net/;stream.mp3 		#  6 - Radio kita cirebon
preset_07 = suaraquran.com:8030            			#  7 - Radio suara quran solo
preset_08 = audio.rodja.tv:1010                   	#  8 - audio RodjaTV
preset_09 = 185.47.52:8000                       	#  9 - Hang FM Batam
preset_10 = live.bassfm.id                          # 10 - BASS FM Salatiga
preset_11 = assunnahfm.com:9010/;stream.nsv         # 11 - Radio Assunah Lombok
preset_12 = live.radiorodja.com/;stream.mp3         # 12 - Radio Rodja HQ
preset_13 = 91.121.221.202:8006/stream              # 13 - Quran 1
preset_14 = 176.31.140.226:8005                     # 14 - Quran 2
preset_15 = http://s4.voscast.com:8172         		# 15 - Quran 3
#
# Clock offset and daylight saving time
clk_server = pool.ntp.org                            # Time server to be used
clk_offset = 1                                       # Offset with respect to UTC in hours
clk_dst = 1                                          # Offset during daylight saving time (hours)
# Some IR codes
ir_22DD = upvolume = 2
ir_12ED = downvolume = 2
ir_02FD = uppreset = 1                      #right
ir_32CD = downpreset = 1
# GPIO pinnings
pin_ir = 35                                          # GPIO Pin number for IR receiver VS1838B
pin_enc_clk = -1#25                                     # GPIO Pin number for rotary encoder "CLK"
pin_enc_dt = -1#26                                      # GPIO Pin number for rotary encoder "DT"
pin_enc_sw = -1#27                                      # GPIO Pin number for rotary encoder "SW"
#
pin_tft_cs = -1#15                                      # GPIO Pin number for TFT "CS"
pin_tft_dc = -1#2                                       # GPIO Pin number for TFT "DC"
#
pin_sd_cs = -1#21                                       # GPIO Pin number for SD card "CS"
#
bat0=2318											# value battery empty
bat100=2916											# value battery full
#
pin_vs_cs = 5                                        # GPIO Pin number for VS1053 "CS"
pin_vs_dcs = 16                                      # GPIO Pin number for VS1053 "DCS"
pin_vs_dreq = 4                                      # GPIO Pin number for VS1053 "DREQ"
pin_tft_scl = 22
pin_tft_sda = 21
)=====" ;

