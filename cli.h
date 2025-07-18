#ifndef ZKSCLI_H
#define ZKSCLI_H

// cli.h is part of the ZKS project
// defines command line interface methods which might 
// be accessed using blue tooth or the serial line
// developed by Tobias Gomer 

extern ZKSConfig cConfig;
extern BluetoothSerial SerialBT;
extern ADC cAdc;

extern volatile int updatePixels;
extern volatile bool do_it;
extern volatile bool check_akkulevel;

extern double startVoltage;   // The voltage at startup time considered to be zero
extern double currentForce;
extern double maxForcePerPull;
extern int akku;
extern unsigned long pullstart;
extern int currentPull;
extern bool limit1topped;
extern bool limit2topped;
extern unsigned long lastaction;

int getAkkuLevel(float voltage){
  if (voltage > 3.47)
    return 9;
  if (voltage > 3.44)
    return 8;
  if (voltage > 3.42)
    return 7;
  if (voltage > 3.4)
    return 6;
  if (voltage > 3.3)
    return 5;
  if (voltage > 3.25)
    return 4;
  if (voltage > 3.2)
    return 3;
  if (voltage > 3.15)
    return 2;
  if (voltage > 3.1)
    return 1;
  return 0;
//  return (round(30*(voltage - 3.3)));
}

void shutdown(){
  pinMode(SHUTDOWNPIN, OUTPUT);
  digitalWrite(SHUTDOWNPIN, LOW);
}

String getSampleTest(){
  float voltage = (cAdc.getValue(1, true));
  int value, offset;
  value = cAdc.getIntValue(1, false);
  offset = cAdc.getIntValue(2, false);

  float fvalue = cAdc.getValue(1, false);
  float foffset = cAdc.getValue(2, false);
  float fvalueoffset = cAdc.getValue(1, true);
  
  float akkuvoltage = cAdc.getValue(0) * POWERSUPPLYDIVIDER;
  if (check_akkulevel){
    check_akkulevel = false;
    akku = getAkkuLevel(akkuvoltage);
  }
  char buffer[128];
  sprintf(buffer, "i 1899 %4.6f %i %i %i %1.6f %1.6f %1.6f %1.6f %1.6f", 
    voltage, akku, value, offset, fvalue, foffset, fvalueoffset, cConfig.getForcefactor(), akkuvoltage);
  return(buffer);
}

String getHurz(String command){
  extern unsigned long fullloadedsince;
  String fullloadedSinceOut = String(" Fulloaded since:") + String((millis() - fullloadedsince) / 1000);
  String retVal =  String("Pulls:") + String(cConfig.getPulls()) + String(" since ") + String(rtc.getEpoch() - pullstart);
  if (currentPull)
    retVal += " Pull Valid";
  if(limit1topped)
    retVal += " limittopped";
  if(limit2topped)
    retVal += " limit2topped";
  retVal += " Charge:";
  retVal += digitalRead(CHARGEPIN); 
  retVal += " Standby:";
  retVal += digitalRead(STANDBYPIN);
  if (fullloadedsince)
    retVal += fullloadedSinceOut;
  if (command == "HURZ 0")
    shutdown();
  if (command == "HURZ 1")
    retVal += " Build date: " + String( __DATE__ ) + String(" ") + String( __TIME__);
  if (command == "HURZ 2")
    retVal = getSampleTest();
  return retVal;
}

void checkPullStatus(){
  if (!currentPull){
    // Start a valid pull using the pullmin settings
    if (currentForce > cConfig.getPullingMinKg()){
      if (!pullstart)
        pullstart = millis();
      else {
        if (millis() - pullstart > 1000 * cConfig.getPullingMinSec()){
          currentPull = cConfig.incrementPulls();
        }
      }
    }
  }
  else {
    if (currentForce > maxForcePerPull)
      maxForcePerPull = currentForce;
    if (!limit1topped){
      if (maxForcePerPull > (double) cConfig.getLimit1()){
        cConfig.limit1Topped(maxForcePerPull, true);
        limit1topped = true;
        cConfig.removeLimit2();
      }
    }
    if (!limit2topped && !limit1topped){
      if (maxForcePerPull > (double) cConfig.getLimit2()){
        cConfig.limit2Topped(maxForcePerPull, true);
        limit2topped = true;
      }
    }
    if (currentForce < cConfig.getPullingMinKg()/3.0) {
      // Pull ended
      currentPull = 0;
      pullstart = 0;
      if ( limit1topped )
        cConfig.limit1Topped(maxForcePerPull, false); 
      else if ( limit2topped )
        cConfig.limit2Topped(maxForcePerPull, false); 
      maxForcePerPull = 0.0;
      limit1topped = false;
      limit2topped = false;
    }
  }
}

double getForce(){
  //double voltage = (cAdc.getValue(1, true)) - startVoltage;
  //double force = voltage * cConfig.getSensfactor();
  return (((cAdc.getValue(1, true)) - startVoltage) * cConfig.getForcefactor());
}

String getSample(){
  char buffer[32];
  sprintf(buffer, "l 1899 %1.1f %i", currentForce, akku);
  return(buffer);
}

String getStatus(){
  float akku = cAdc.getValue(0) * POWERSUPPLYDIVIDER;
  return("l 1899 V" + cConfig.getSWVersion() + " 1 " + // 5.0V cAdc.getValue(1) + 
    // "5V "+   // Eingangsspannung 
    // "3.6V "     +  // Ladereglerspannung String(cAdc.getValue(2), 5)+"V " + 
    akku + "V " +
    getAkkuLevel(akku) + " " + String(cConfig.getSensfactor(), 5));
}

bool cli_execute(String command, Stream & device){
  String response;
  lastaction = millis();
  if (command == ""){
    device.println(getSample());
    device.println("e");
    return true;
  }
  else
    if (command=="?"){
      device.println(getStatus());
      device.println("e");
      return true;
    }
  else
    if (command=="#WHOAMI"){
      device.println(cConfig.whoami(akku));
      return true;
    }
  else
    if (command=="#CONFIG"){
      device.print("DEVICENAME=");
      device.println(cConfig.getDevicename());
      device.print("DEVICESN=");
      device.println(cConfig.getSerialno());
      device.print("SENSFACTOR=");
      device.println(cConfig.getSensfactor(),5);
      if( cConfig.getWireless() & 1)
        device.println("BTENABLED=1");
      else
        device.println("BTENABLED=0");
      if( cConfig.getWireless() & 2)
        device.println("WIFITCPENABLED=1");
      else
        device.println("WIFITCPENABLED=0");
      device.println("DEVICEIP=" + cConfig.getAddress());
      device.print("POWEROFF=");
      device.println(cConfig.getTimeout());
      device.print("LIMIT1=");
      device.println(cConfig.getLimit1());
      device.print("LIMIT2=");
      device.println(cConfig.getLimit2());
      device.print("SERVICE=");
      device.println(cConfig.getService());
      device.print("PULLING_MIN=");
      device.print(cConfig.getPullingMinKg());
      device.print(" ");
      device.println(cConfig.getPullingMinSec());
      // device.println("SERIALNUMBER=" + cConfig.getSerialno());
      return true;
    }
  else
    if (command.startsWith("#CFGPIN")){
      response = cConfig.enterPin(command);
      device.println(response);
      if (response == "OK")
        return true;
      else 
        return false;
    }
  else
    if (command.startsWith("#CFGDEVICE")){
      device.println(cConfig.configDevice(command));
      return true;
    }
  else
    if (command.startsWith("#CFGSERIALNO")){
      device.println(cConfig.configSerialno(command));
      return true;
    }
  else
    if (command.startsWith("#CFGSENSFACTOR")){
      device.println(cConfig.configSensfactor(command));      
      return true;
    }
  else
    if (command.startsWith("#CFGWIRELESS")){
      device.println(cConfig.configWireless(command));
      return true;
    }
  else
    if (command.startsWith("#CFGTIMEOUT")){
      device.println(cConfig.configTimeout(command));
      return true;
    }
  else
    if (command.startsWith("#CFGLIMIT1")){
      response = cConfig.setLimit1(command);
      device.println(response);
      if (response == "FAIL")
        return false;
      return true;
    }
  else
    if (command.startsWith("#CFGLIMIT2")){
      response = cConfig.setLimit2(command);
      device.println(response);
      if (response == "FAIL")
        return false;
      return true;
    }
  else
    if (command.startsWith("#CFGADDRESS")){
      device.println(cConfig.configAddress(command));
      return true;
    }
  else 
    if (command.startsWith("#CFGPULLINGMIN")){
      response = cConfig.setPullmin(command);
      device.println(response);
      if (response == "FAIL")
        return false;
      return true;
    }
  else
    if (command.startsWith("#CFGRESET")){
      device.println(cConfig.resetToFactorySettings());
      ESP.restart();
    }
  else
    if (command.startsWith("#CFGSERVICE")){
      response = cConfig.setService(command);
      device.println(response);
      if (response == "FAIL")
        return false;
      return true;
    }
  else
    if (command.startsWith("#CLEARDATA")){
      device.println(cConfig.clearData());
      return true;
      }
  else
    if (command.startsWith("#CLEARSERVICE")){
      device.println(cConfig.clearService());
      return true;
      }
  else
    if (command.startsWith("#CLEARCOUNTERS")){
      device.println(cConfig.clearCounters(command));
      return true;
      }
  else
    if (command.startsWith("#LISTDATA")){
      device.println("LIMIT1="+ String(cConfig.getLimit1PullsCount()));
      String limit = cConfig.getLimit1Pulls(); 
      if (limit != ""){
        limit.replace("\n", "\r\n");
        device.println(limit);
      }
      device.println("LIMIT2="+ String(cConfig.getLimit2PullsCount()));
      limit = cConfig.getLimit2Pulls();
      if (limit != ""){
        limit.replace("\n", "\r\n");
        device.println(limit);
      }
      device.println("PULLS=" + String(cConfig.getPulls()));
      device.println("PULLS_SINCE_SERVICE=" + String(cConfig.getServiceCount()));
      return true;
      }
  else
    if (command.startsWith("#SETCOUNTERS")){
      device.println(cConfig.setCounters(command));
      return true;
      }
  else
    if (command.startsWith("#STATUS")){
      device.print(String(cConfig.getPulls()) + " ");
      device.print(String(cConfig.getLimit1PullsCount()) + " ");
      device.print(String(cConfig.getLimit2PullsCount()) + " ");
//      if (cConfig.getServiceCount() > cConfig.getServiceLimit())
        device.println(String(cConfig.getServiceLimit() - cConfig.getServiceCount()));
//      else
//        device.println(String(cConfig.getServiceCount()));
      return true;
      }
  else 
    if (command.startsWith("#TIME")){
      response = cConfig.setTime(command);
      device.println(response);
      if (response == "FAIL")
        return false;
      return true;
    }
  else 
    if (command.startsWith("#POWER 0")){
        shutdown();
    }
  else 
    if (command.startsWith("HURZ")){
      response = getHurz(command);
      device.println(response);
      return true;
    }
  return false;
}

#endif