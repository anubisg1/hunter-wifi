#include <Updater.h>

#include "LittleFS.h"
#include <ota.h>
#define U_PART U_FS


size_t content_len;

void handleUpdate(AsyncWebServerRequest *request) {
  String html = "<form method='POST' action='/doUpdate' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
  request->send(200, "text/html", html);
}

void handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {

  // if filename includes .json, upload to FS
  if (filename.indexOf(".json") > -1)
  {
    if(!index){
      Serial.printf("UploadStart: %s\n" , filename.c_str());
      // open the file on first call and store the file handle in the request object
      if (LittleFS.begin()) {
        request->_tempFile = LittleFS.open("/"+filename, "w");
      }
    }
    if(len) {
      // stream the incoming chunk to the opened file
      request->_tempFile.write(data,len);
    }
    if(final){
      Serial.printf("UploadEnd: %s, size: %d\n" ,filename.c_str(), index+len);
      // close the file handle as the upload is now done
      request->_tempFile.close();
      request->send(200, "text/html", "<head><meta http-equiv='refresh' content='10;URL=/'/></head><body>" + filename + " upload complete! Please wait while the device reboots</body>");
    }

  } else {
    if (!index){
      Serial.println("Update");
      content_len = request->contentLength();

      // if filename includes spiffs, update the spiffs partition
      int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
      Update.runAsync(true);
      if (!Update.begin(content_len, cmd)) {
        Update.printError(Serial);
      }
      request->send(200, "text/html", "<head><meta http-equiv='refresh' content='10;URL=/'/></head><body>Upload complete! Please wait while the device reboots</body>");
    }

    if (Update.write(data, len) != len) {
      Update.printError(Serial);
    } else {
      Serial.printf("Progress: %d%%\n", (Update.progress()*100)/Update.size());
    }

    if (final) {
      if (!Update.end(true)){
        Update.printError(Serial);
      } else {
        Serial.println("Update complete");
        Serial.flush();
        ESP.restart();
      }
    }
  }
}

void printProgress(size_t prg, size_t sz) {
  Serial.printf("Progress: %d%%\n", (prg*100)/content_len);
}
