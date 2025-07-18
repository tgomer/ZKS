#ifndef ZKSCONFIG_H
#define ZKSCONFIG_H

// ZKSConfig implements a class used for handling
// the definitions and states
// definitions are stored in the esp32 preferences
// memory
// developed by Tobias Gomer


#include <Preferences.h>
#include <ESP32Time.h>

extern ESP32Time rtc;

class ZKSConfig{
  private:
    Preferences preferences;
    int pin;
    int timeout; 
    float sensfactor;
    String serialno;
    String devicename;
    String address;  // IP Address
    int limit1;
    int limit2;
    int servicelimit;
    int pullingminkg;
    int pullingminsec;
    int wireless;

    String slimit1;
    String slimit2;
    int pulls;
    int nlimit1;
    int nlimit2;
    int servicecount;

    double forceFactor;

    bool configmode;

  public: ZKSConfig(){
    // Factory defaults
  }

  public: bool getConfigMode(){
    return configmode;
  }

  public: void begin(){
    preferences.begin("zks");
    pin = preferences.getInt("pin", CFG_PIN);
    timeout = preferences.getInt("timeout", CFG_TIMEOUT); 
    sensfactor = preferences.getFloat("sensfactor", CFG_SENSFACTOR);
    serialno = preferences.getString("serialno", CFG_SERIALNO);
    devicename = preferences.getString("devicename", CFG_DEVICENAME);
    limit1 = preferences.getInt("limit1", CFG_LIMIT1);
    limit2 = preferences.getInt("limit2", CFG_LIMIT2);
    servicelimit = preferences.getInt("servicelimit", CFG_SERCICELIMIT);
    pullingminkg = preferences.getInt("pullingminkg", CFG_PULLINGMINKG);
    pullingminsec = preferences.getInt("pullingminsec", CFG_PULLINGMINSEC);
    wireless = preferences.getInt("wireless", 1);
    address = preferences.getString("address", "0.0.0.0");
    
    slimit1 = preferences.getString("slimit1", "");
    slimit2 = preferences.getString("slimit2", "");
    nlimit1 = preferences.getInt("nlimit1", 0);
    nlimit2 = preferences.getInt("nlimit2", 0);
    pulls  = preferences.getInt("pulls", 0);
    servicecount = preferences.getInt("servicecount", 0);
    forceFactor = 2148.0 / sensfactor;
    preferences.end();
    configmode = false;
  }

  public: double getForcefactor(){
    return forceFactor;
  }

  public: bool getConfigmode(){
    return configmode;
  }

  public: String getDevicename(){
    return this->devicename;
  }

  public: int getPullingMinKg(){
    return this->pullingminkg;
  }

  public: int getPullingMinSec(){
    return this->pullingminsec;
  }

  public: String getTime(){
    return rtc.getTime("%Y-%m-%d %H:%M:%S");
  }

  public: String setPullmin(String val){
    int kp=0,seconds=0;
    int entries = sscanf(val.c_str(), "#CFGPULLINGMIN %d %d", &kp, &seconds);
    if (entries < 0)
      return String(pullingminkg) + String(" ")  + String(pullingminsec);
    else if (entries < 2)
      return ("FAIL");
    if (!configmode)
      return("PIN MISSING");
    pullingminkg = kp;
    pullingminsec = seconds;
    preferences.begin("zks", false);
    preferences.putInt("pullingminkg", kp);
    preferences.putInt("pullingminsec", seconds);
    preferences.end();
    return ("OK");
  }

  public: String setCounters(String val){
    if (!configmode)
      return("PIN MISSING");
    int pulls=0, service=0, limit1, limit2;
    int entries = sscanf(val.c_str(), "#SETCOUNTERS %d %d %d %d", &pulls, &service, &limit1, &limit2);
    if (entries < 4)
      return ("FAIL");
    this->pulls = pulls;
    this->servicecount = service;
    this->nlimit1 = limit1;
    this->nlimit2 = limit2;
    preferences.begin("zks", false);
    preferences.putInt("pulls", pulls);
    preferences.putInt("servicecount", service);
    preferences.putInt("nlimit1", limit1);
    preferences.putInt("nlimit2", limit2);
    preferences.end();
    return ("OK");
  }

  public: String setTime(String time){
    int year=0,month=0,day=0,hours=0,minutes=0,seconds=0;
    int entries = sscanf(time.c_str(), "#TIME %d-%d-%d %d:%d:%d", &year, &month, &day, &hours, &minutes, &seconds);
    if (entries < 0)
      return getTime();
    else if (entries < 6)
      return ("FAIL");
    rtc.setTime(seconds, minutes, hours, day, month, year);
    return ("OK");
  }

  public: int getLimit1(){
    return this->limit1;
  }

  public: int getLimit2(){
    return this->limit2;
  }

  public: String setLimit1(String limit){
    int nlimit;
    int entries = sscanf(limit.c_str(), "#CFGLIMIT1 %d", &nlimit);
    if (entries < 0)
      return String(getLimit1());
    else if (entries < 1)
      return ("FAIL");
    if (!configmode)
      return("PIN MISSING");
    preferences.begin("zks", false);
    preferences.putInt("limit1", nlimit);
    preferences.end();
    limit1 = nlimit;    
    return ("OK");
  }

  public: String setLimit2(String limit){
    int nlimit;
    int entries = sscanf(limit.c_str(), "#CFGLIMIT2 %d", &nlimit);
    if (entries < 0)
      return String(getLimit2());
    else if (entries < 1)
      return ("FAIL");
    if (!configmode)
      return("PIN MISSING");
    preferences.begin("zks", false);
    preferences.putInt("limit2", nlimit);
    preferences.end();
    limit2 = nlimit;    
    return ("OK");
  }

  public: String setService(String val){
    int nlimit;
    int entries = sscanf(val.c_str(), "#CFGSERVICE %d", &nlimit);
    if (entries < 0)
      return String(getService());
    else if (entries < 1)
      return ("FAIL");
    if (!configmode)
      return("PIN MISSING");
    preferences.begin("zks", false);
    preferences.putInt("servicelimit", nlimit);
    preferences.end();
    servicelimit = nlimit;    
    return ("OK");
  }

  public: int getService(){
    return this->servicelimit;
  }

  public: int setDevicename(String devicename){
    this->devicename = devicename;
    preferences.begin("zks", false);
    preferences.putString("devicename", devicename);
    preferences.end();    
    return 0;
  }

  public: float getSensfactor(){
    return this->sensfactor;
  }

  private: int setSensfactor(float sensfactor){
    this->sensfactor = sensfactor;
    preferences.begin("zks", false);
    preferences.putFloat("sensfactor", sensfactor);
    preferences.end();
    return 0;
  }

  private: int setWireless(int nWireless){
    this->wireless = nWireless;
    preferences.begin("zks", false);
    preferences.putInt("wireless", nWireless);
    preferences.end();
    return 0;
  }

  public: int getWireless(){
    return this->wireless;
  }

  public: int getTimeout(){
    return timeout;
  }

  private: int setTimeout(int timeout){
    if (timeout < 1 && timeout > 3000000)
      return 1;
    if (timeout != this->timeout){
      preferences.begin("zks", false);
      preferences.putInt("timeout", timeout);
      preferences.end();
      }    
    this->timeout = timeout;
    return 0;
  }

  public: String getAddress(){
    return this->address;
  }

  private: int setAddress(String address){
    this->address = address;
    preferences.begin("zks", false);
    preferences.putString("address", address);
    preferences.end();
    return 0;
  }

  public: String getSerialno(){
    return this->serialno;
  }

  private: int setSerialno(String serialno){
    this->serialno = serialno;
    preferences.begin("zks", false);
    preferences.putString("serialno", serialno);
    preferences.end();
    return 0;
  }

  public: String enterPin(String val){
    if (val == "#CFGPIN 929555"){
      configmode = true;
      return("OK");
      }
    else{
      configmode = false;
      return("FAIL");
    }
  }

  public: String configTimeout(String val){
    int timeout;        
    if(sscanf(val.c_str(), "#CFGTIMEOUT %i", &timeout) == -1)
      return(String(getTimeout()));
    if (!configmode)
      return("PIN MISSING");
    if (setTimeout(timeout) != 0)
      return("FAIL");            
    return("OK");
  }

  public: String configDevice(String val){
    char buffer[32];
    int num =  sscanf(val.c_str(), "#CFGDEVICE %32s", buffer);
    if (num == -1)
      return(getDevicename() );
    if (!configmode)
      return("PIN MISSING");
    if (setDevicename(buffer) != 0)
        return("FAIL");            
    return("OK");
  }

  public: String configSerialno(String val){
    char buffer[32];
    if(sscanf(val.c_str(), "#CFGSERIALNO %32s", buffer) == -1)
      return(getSerialno());
    if (!configmode)
      return("PIN MISSING");
    if (setSerialno(buffer) != 0)
        return("FAIL");            
    return("OK");
  }

  public: String configAddress(String val){
    char buffer[32];
    if( sscanf(val.c_str(), "#CFGADDRESS %32s", buffer) == -1)
      return(getAddress());
    if (!configmode)
      return("PIN MISSING");
    if (setAddress(buffer) != 0)
      return("FAIL");            
    return("OK");
   }

  public: String clearService(){
    if (!configmode)
      return("PIN MISSING");
    servicecount = 0;
    preferences.begin("zks", false);
    preferences.putInt("servicecount", 0);
    preferences.end();
    return("OK");
  } 

  public: String clearCounters(String command){
    if (!configmode)
      return("PIN MISSING");
    preferences.begin("zks", false);
    servicecount = 0;
    preferences.putInt("servicecount", 0);
    slimit1 = "";
    preferences.putString("slimit1", "");
    slimit1 = "";
    preferences.putString("slimit2", "");
    nlimit1 = 0;
    preferences.putInt("nlimit1", 0);
    nlimit2 = 0;
    preferences.putInt("nlimit2", 0);
    if (command != "#CLEARCOUNTERS P"){
      pulls  = 0;
      preferences.putInt("pulls", 0);
    }
    preferences.end();
    return("OK");
  } 

  public: String clearData(){
    if (!configmode)
      return("PIN MISSING");
    slimit1 = "";
    slimit2 = "";
//    pulls = 0;
    preferences.begin("zks", false);
    preferences.putString("slimit1", "");
    preferences.putString("slimit2", "");
    nlimit1 = 0;
    preferences.putInt("nlimit1", 0);
    nlimit2 = 0;
    preferences.putInt("nlimit2", 0);
//    preferences.putInt("pulls", 0);
    preferences.end();
    return("OK");
  } 

  public: String configSensfactor(String val){
    float fVal;
    if (sscanf(val.c_str(), "#CFGSENSFACTOR %f", &fVal) == -1)
      return(String(getSensfactor(),5));
    if (!configmode)
      return("PIN MISSING");
    if (setSensfactor(fVal) != 0)
      return("FAIL");            
    return("OK");
  } 

  public: String configWireless(String val){
    int nVal;
    int num = sscanf(val.c_str(), "#CFGWIRELESS %d", &nVal);
    if (num != 1)
      return (String(getWireless())); 
    if (!configmode)
      return("PIN MISSING");
    setWireless(nVal);
    return("OK");
  } 

  public: String resetToFactorySettings(){
    if (!configmode)
      return("PIN MISSING");
    resetFactorySettings();
    return("OK");
  }

  public: String whoami(int akkulevel){
    String warnStatus("");
    if(nlimit1 || nlimit2)
      warnStatus = " WARNING";
    if (servicecount > servicelimit)
      warnStatus = warnStatus + " SERVICE";
    return(getDevicename() + " HW" + HWVersion + " SW" + SWVersion  + warnStatus +" BAT" + String(akkulevel));
  }

  public: String getSWVersion(){
    return String(SWVersion);
  }

  public: int incrementPulls(void){
    pulls++;
    servicecount++;
    preferences.begin("zks", false);
    preferences.putInt("pulls", pulls);
    preferences.putInt("servicecount", servicecount);
    preferences.end();
    return pulls;
  }

  public: int getPulls(void){
    return pulls;
  }

  public: int getServiceLimit(void){
    return servicelimit;
  }

  public: void limit1Topped(float force, bool increment){
    String status = String(pulls) + String("=") + String(force, 1) + String(" ") + getTime();
    if (increment){
      if (slimit1.length())
        slimit1 = slimit1 + String("\n");
      slimit1 = slimit1 + status;
      nlimit1++;
    }
    else{
      int lastString = slimit1.lastIndexOf("\n");
      if (lastString != -1)
        slimit1.remove(lastString);
      else
        slimit1 = "";
      if (slimit1.length())
        slimit1 = slimit1 + String("\n");
      slimit1 = slimit1 + status;
    }
    preferences.begin("zks", false);
    preferences.putString("slimit1", slimit1);
    preferences.putInt("nlimit1", nlimit1);
    preferences.end();
  }

  public: void removeLimit2(){
      int lastString = slimit2.lastIndexOf("\n");
      if (lastString != -1)
        slimit2.remove(lastString);
      else
        slimit2 = "";
      nlimit2--;
      preferences.begin("zks", false);
      preferences.putString("slimit2", slimit2);
      preferences.putInt("nlimit2", nlimit2);
      preferences.end();
  }   

  public: void limit2Topped(float force, bool increment){
    String status = String(pulls) + String("=") + String(force,1) + String(" ") + getTime();
    if (increment){
      if (slimit2.length())
        slimit2 = slimit2 + String("\n");
      slimit2 = slimit2 + status;
      nlimit2++;
    }
    else{
      int lastString = slimit2.lastIndexOf("\n");
      if (lastString != -1)
        slimit2.remove(lastString);
      else 
        slimit2 = "";
      if (slimit2.length())
        slimit2 = slimit2 + String("\n");
      slimit2 = slimit2 + status;
    }
    preferences.begin("zks", false);
    preferences.putString("slimit2", slimit2);
    preferences.putInt("nlimit2", nlimit2);
    preferences.end();
  }

  public: int getServiceCount(){
    return servicecount;
  }

  public: String getLimit1Pulls(){
    return slimit1;
  }

  public: String getLimit2Pulls(){
    return slimit2;
  }

  public: int getLimit1PullsCount(){
    return nlimit1;
  }

  public: int getLimit2PullsCount(){
    return nlimit2;
  }

  private: void resetFactorySettings(){
    preferences.begin("zks", false);
    preferences.clear();
    preferences.end();
  }
};
#endif