#ifndef GSheet_h
#define GSheet_h

#include <Arduino.h>

class GSheet
{
private:
    String _GAS_ID;
    String url;
    const char* _ssid;
    const char* _pass;
    String _param;

public:
   GSheet(String GAS_ID);
   void connectWiFi(const char* ssid, const char* pass);

   void sendData(String colC);
   void sendData(String colC, String colD);
   void sendData(String colC, String colD, String colE);
   void sendData(String colC, String colD, String colE, String colF);
   void sendData(String colC, String colD, String colE, String colF, String colG);
   void clearData();

};
#endif