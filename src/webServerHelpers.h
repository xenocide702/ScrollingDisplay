
#include <WebServer.h>
#include <SPIFFS.h>

extern WebServer server;
File fsUploadFile;


String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  Serial.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    Serial.printf("Streaming %s\r\n",path.c_str());
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}
void ICACHE_FLASH_ATTR handleNotFound() {
  if(!handleFileRead(server.uri())){

    String message = "Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMeth: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArgs: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
  }
}

void handleFileList() {
  bool SPIFFSfs = server.hasArg("SPIFFSFS");
  if(!server.hasArg("dir")) {
    server.send(400,"text/plain","BAD ARGS");
    return;
  }
  String path = server.arg("dir");
  File dir;
  if(SPIFFSfs){
    if(path != "/" && !SPIFFS.exists((char *)path.c_str())) {
      server.send(400,"text/plain","BAD PATH");
      return;
    }
    dir = SPIFFS.open((char *)path.c_str());
  }
  else{
    if(path != "/" && !SPIFFS.exists((char *)path.c_str())) {
      server.send(400,"text/plain","BAD PATH");
      return;
    }
    dir = SPIFFS.open((char *)path.c_str());
  }
  path = String();
  if(!dir.isDirectory()){
    dir.close();
    server.send(200,"text/plain","NOT DIR");
    return;
  }
  dir.rewindDirectory();

  String output = "[";
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    if (!entry)
    break;

    if (cnt > 0)
      output += ',';

    output += "{\"type\":\"";
    output += (entry.isDirectory()) ? "dir" : "file";
    output += "\",\"name\":\"";
    // Ignore '/' prefix
    output += entry.name()+1;
    output += "\",\"size\":\"";
    output += entry.size();
    output += "\",\"date\":\"";
    output += entry.getLastWrite();
    output += "\"}";
    entry.close();
  }
  output += "]";
  server.send(200, "text/json", output);
  dir.close();
}
void ICACHE_FLASH_ATTR handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    //DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    if(server.hasArg("SPIFFSFS")){
      Serial.printf("Uploading %s to SPIFFS\r\n", filename.c_str());
      fsUploadFile = SPIFFS.open(filename, "w");
    }
    else{
      Serial.printf("Uploading %s to SPIFFS\r\n", filename.c_str());
      fsUploadFile=SPIFFS.open(filename,"w");
    }
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    //DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}
void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BARGS");
  String path = server.arg(0);
  Serial.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BPATH");

  File file;
  if(server.hasArg("SPIFFSFS")){
    if(SPIFFS.exists(path)){return server.send(500, "text/plain", "EXISTS");}
    file = SPIFFS.open(path, "w");
  }
  else{
    if(SPIFFS.exists(path)){return server.send(500, "text/plain", "EXISTS");}
    file = SPIFFS.open(path, "w");
  }

  if(file){file.close();}
  else{return server.send(500, "text/plain", "FAILED");}
  server.send(200, "text/plain", "");
  path = String();
}
void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BARGS");
  String path = server.arg(0);
  Serial.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BPATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "NotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}
void light(){
    pinMode(14,OUTPUT);
    if(server.hasArg("on")){
        digitalWrite(14,HIGH);
        server.send(200,"text/plain","Light on");
    }
    else{
        digitalWrite(14,LOW);
        server.send(200,"text/plain","Light on");
    }
}
void initHttpServer(){
  server.on("/edit", HTTP_PUT, handleFileCreate);
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);
  server.on("/light",HTTP_GET,light);
  
  server.on("/list",handleFileList);
  server.serveStatic("/",SPIFFS,"/");
  server.onNotFound(handleNotFound);

  server.begin();
}
