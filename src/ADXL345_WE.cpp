/********************************************************************
* This is a library for the ADXL345 accelerometer.
*
* You'll find an example which should enable you to use the library. 
*
* You are free to use it, change it or build on it. In case you like 
* it, it would be cool if you give it a star.
* 
* If you find bugs, please inform me!
* 
* Written by Wolfgang (Wolle) Ewald
* https://wolles-elektronikkiste.de/adxl345-teil-1 (German)
* https://wolles-elektronikkiste.de/en/adxl345-the-universal-accelerometer-part-1 (English)
*
*********************************************************************/

#include "ADXL345_WE.h"

/************  Constructors ************/

ADXL345_WE::ADXL345_WE(int addr){
    useSPI = false;
    _wire = &Wire;
    i2cAddress = addr;   
}

ADXL345_WE::ADXL345_WE(){
    useSPI = false;
    _wire = &Wire;
    i2cAddress = 0x53;   
}

ADXL345_WE::ADXL345_WE(TwoWire *w, int addr){
    useSPI = false;
    _wire = w;
    i2cAddress = addr; 
}

ADXL345_WE::ADXL345_WE(TwoWire *w){
    useSPI = false;
    _wire = w;
    i2cAddress = 0x53;
}

ADXL345_WE::ADXL345_WE(SPIClass *s, int cs, bool spi){
    useSPI = spi;
    _spi = s;
    csPin = cs;  
}

ADXL345_WE::ADXL345_WE(int cs, bool spi){
    useSPI = spi;
    _spi = &SPI;
    csPin = cs;
}

/************ Basic settings ************/
    
bool ADXL345_WE::init(){    
    if(useSPI){
        _spi->begin();
        _spi->setDataMode(SPI_MODE3);
        pinMode(csPin, OUTPUT);
        digitalWrite(csPin, HIGH);
    }
    writeRegister(ADXL345_POWER_CTL,0);
    writeRegister(ADXL345_POWER_CTL, 16);   
    setMeasureMode(true);
    corrFact.x = 1.0;
    corrFact.y = 1.0;
    corrFact.z = 1.0;
    offsetVal.x = 0.0;
    offsetVal.y = 0.0;
    offsetVal.z = 0.0;
    writeRegister(ADXL345_DATA_FORMAT,0);
    setFullRes(true);
    if(!((readRegister8(ADXL345_DATA_FORMAT)) & (1<<ADXL345_FULL_RES))){
        return false;
    }
    writeRegister(ADXL345_INT_ENABLE, 0);
    writeRegister(ADXL345_INT_MAP,0);
    writeRegister(ADXL345_TIME_INACT, 0);
    writeRegister(ADXL345_THRESH_INACT,0);
    writeRegister(ADXL345_ACT_INACT_CTL, 0);
    writeRegister(ADXL345_DUR,0);
    writeRegister(ADXL345_LATENT,0);
    writeRegister(ADXL345_THRESH_TAP,0);
    writeRegister(ADXL345_TAP_AXES,0);
    writeRegister(ADXL345_WINDOW, 0);
    readAndClearInterrupts();
    writeRegister(ADXL345_FIFO_CTL,0);
    writeRegister(ADXL345_FIFO_STATUS,0);
     
    return true;
}

void ADXL345_WE::setCorrFactors(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax){
    corrFact.x = UNITS_PER_G / (0.5 * (xMax - xMin));
    corrFact.y = UNITS_PER_G / (0.5 * (yMax - yMin));
    corrFact.z = UNITS_PER_G / (0.5 * (zMax - zMin));
    offsetVal.x = (xMax + xMin) * 0.5;
    offsetVal.y = (yMax + yMin) * 0.5;
    offsetVal.z = (zMax + zMin) * 0.5;
}

void ADXL345_WE::setDataRate(adxl345_dataRate rate){
    regVal |= readRegister8(ADXL345_BW_RATE);
    regVal &= 0xF0;
    regVal |= rate;
    writeRegister(ADXL345_BW_RATE, regVal);
}
    
adxl345_dataRate ADXL345_WE::getDataRate(){
    return (adxl345_dataRate)(readRegister8(ADXL345_BW_RATE) & 0x0F);
}

String ADXL345_WE::getDataRateAsString(){
    adxl345_dataRate dataRate = (adxl345_dataRate)(readRegister8(ADXL345_BW_RATE) & 0x0F);
    String returnString = "";
    
    switch(dataRate) {
        case ADXL345_DATA_RATE_3200: returnString = "3200 Hz"; break;
        case ADXL345_DATA_RATE_1600: returnString = "1600 Hz"; break;
        case ADXL345_DATA_RATE_800:  returnString = "800 Hz";  break;
        case ADXL345_DATA_RATE_400:  returnString = "400 Hz";  break;
        case ADXL345_DATA_RATE_200:  returnString = "200 Hz";  break;
        case ADXL345_DATA_RATE_100:  returnString = "100 Hz";  break;
        case ADXL345_DATA_RATE_50:   returnString = "50 Hz";   break;
        case ADXL345_DATA_RATE_25:   returnString = "25 Hz";   break;
        case ADXL345_DATA_RATE_12_5: returnString = "12.5 Hz"; break;
        case ADXL345_DATA_RATE_6_25: returnString = "6.25 Hz"; break;
        case ADXL345_DATA_RATE_3_13: returnString = "3.13 Hz"; break;
        case ADXL345_DATA_RATE_1_56: returnString = "1.56 Hz"; break;
        case ADXL345_DATA_RATE_0_78: returnString = "0.78 Hz"; break;
        case ADXL345_DATA_RATE_0_39: returnString = "0.39 Hz"; break;
        case ADXL345_DATA_RATE_0_20: returnString = "0.20 Hz"; break;
        case ADXL345_DATA_RATE_0_10: returnString = "0.10 Hz"; break;
    }
    
    return returnString;
}

uint8_t ADXL345_WE::getPowerCtlReg(){
    return readRegister8(ADXL345_POWER_CTL);
}

void ADXL345_WE::setRange(adxl345_range range){
    uint8_t regVal = readRegister8(ADXL345_DATA_FORMAT);
    if(adxl345_lowRes){
        switch(range){
            case ADXL345_RANGE_2G:  rangeFactor = 1.0;  break;
            case ADXL345_RANGE_4G:  rangeFactor = 2.0;  break;
            case ADXL345_RANGE_8G:  rangeFactor = 4.0;  break;
            case ADXL345_RANGE_16G: rangeFactor = 8.0;  break;  
        }
    }
    else{
        rangeFactor = 1.0;
    }
    regVal &= 0b11111100;
    regVal |= range;
    writeRegister(ADXL345_DATA_FORMAT, regVal);
}

adxl345_range ADXL345_WE::getRange(){
    regVal = readRegister8(ADXL345_DATA_FORMAT);
    regVal &= 0x03; 
    return adxl345_range(regVal);
}

void ADXL345_WE::setFullRes(boolean full){
    regVal = readRegister8(ADXL345_DATA_FORMAT);
    if(full){
        adxl345_lowRes = false;
        rangeFactor = 1.0;
        regVal |= (1<<ADXL345_FULL_RES);
    }
    else{
        adxl345_lowRes = true;
        regVal &= ~(1<<ADXL345_FULL_RES);
        setRange(getRange());
    }
    writeRegister(ADXL345_DATA_FORMAT, regVal);
}

String ADXL345_WE::getRangeAsString(){
    String rangeAsString = "";
    adxl345_range range = getRange();
    switch(range){
        case ADXL345_RANGE_2G:  rangeAsString = "2g";   break;
        case ADXL345_RANGE_4G:  rangeAsString = "4g";   break;
        case ADXL345_RANGE_8G:  rangeAsString = "8g";   break;
        case ADXL345_RANGE_16G: rangeAsString = "16g";  break;
        
    }
    return rangeAsString;
}

/************ x,y,z results ************/

xyzFloat ADXL345_WE::getRawValues(){
    rawVal.x = readRegister16(ADXL345_DATAX0);
    rawVal.y = readRegister16(ADXL345_DATAY0);
    rawVal.z = readRegister16(ADXL345_DATAZ0);
    return rawVal;
}

xyzFloat ADXL345_WE::getCorrectedRawValues(){
    uint64_t xyzDataReg = readRegister3x16(ADXL345_DATAX0);
    int16_t xRaw = (int16_t)((xyzDataReg >> 32) & 0xFFFF);
    int16_t yRaw = (int16_t)((xyzDataReg >> 16) & 0xFFFF);
    int16_t zRaw = (int16_t)(xyzDataReg & 0xFFFF);
    
    rawVal.x = xRaw - (offsetVal.x / rangeFactor);
    rawVal.y = yRaw - (offsetVal.y / rangeFactor);
    rawVal.z = zRaw - (offsetVal.z / rangeFactor);
    
    return rawVal;
}

xyzFloat ADXL345_WE::getGValues(){
    getCorrectedRawValues();
    gVal.x = rawVal.x * MILLI_G_PER_LSB * rangeFactor * corrFact.x / 1000.0;
    gVal.y = rawVal.y * MILLI_G_PER_LSB * rangeFactor * corrFact.y / 1000.0;
    gVal.z = rawVal.z * MILLI_G_PER_LSB * rangeFactor * corrFact.z / 1000.0;
    return gVal;
}

xyzFloat ADXL345_WE::getAngles(){
    getGValues();
    if(gVal.x > 1){
        gVal.x = 1;
    }
    else if(gVal.x < -1){
        gVal.x = -1;
    }
    angleVal.x = (asin(gVal.x)) * 57.296;
    
    if(gVal.y > 1){
        gVal.y = 1;
    }
    else if(gVal.y < -1){
        gVal.y = -1;
    }
    angleVal.y = (asin(gVal.y)) * 57.296;
    
    if(gVal.z > 1){
        gVal.z = 1;
    }
    else if(gVal.z < -1){
        gVal.z = -1;
    }
    angleVal.z = (asin(gVal.z)) * 57.296;
    
    return angleVal;
}

xyzFloat ADXL345_WE::getCorrAngles(){
    getAngles();
    xyzFloat corrAnglesVal;
    corrAnglesVal.x = angleVal.x - angleOffsetVal.x;
    corrAnglesVal.y = angleVal.y - angleOffsetVal.y;
    corrAnglesVal.z = angleVal.z - angleOffsetVal.z;
        
    return corrAnglesVal;
}

/************ Angles and Orientation ************/ 

void ADXL345_WE::measureAngleOffsets(){
    getAngles();
    angleOffsetVal.x = angleVal.x; 
    angleOffsetVal.y = angleVal.y;
    angleOffsetVal.z = angleVal.z;
}

adxl345_orientation ADXL345_WE::getOrientation(){
    adxl345_orientation orientation = FLAT;
    getAngles();
    if(abs(angleVal.x) < 45){      // |x| < 45
        if(abs(angleVal.y) < 45){      // |y| < 45
            if(angleVal.z > 0){          //  z  > 0
                orientation = FLAT;
            }
            else{                        //  z  < 0
                orientation = FLAT_1;
            }
        }
        else{                         // |y| > 45 
            if(angleVal.y > 0){         //  y  > 0
                orientation = XY;
            }
            else{                       //  y  < 0
                orientation = XY_1;   
            }
        }
    }
    else{                           // |x| >= 45
        if(angleVal.x > 0){           //  x  >  0
            orientation = YX;       
        }
        else{                       //  x  <  0
            orientation = YX_1;
        }
    }
    return orientation;
}

String ADXL345_WE::getOrientationAsString(){
    adxl345_orientation orientation = getOrientation();
    String orientationAsString = "";
    switch(orientation){
        case FLAT:      orientationAsString = "z up";   break;
        case FLAT_1:    orientationAsString = "z down"; break;
        case XY:        orientationAsString = "y up";   break;
        case XY_1:      orientationAsString = "y down"; break;
        case YX:        orientationAsString = "x up";   break;
        case YX_1:      orientationAsString = "x down"; break;
    }
    return orientationAsString;
}

float ADXL345_WE::getPitch(){
    getGValues();
    float pitch = (atan2(gVal.x, sqrt(gVal.x*gVal.y + gVal.z*gVal.z))*180.0)/M_PI;
    return pitch;
}
    
float ADXL345_WE::getRoll(){
    getGValues();
    float roll = (atan2(gVal.y, gVal.z)*180.0)/M_PI;
    return roll;
}

/************ Power, Sleep, Standby ************/ 

void ADXL345_WE::setMeasureMode(bool measure){
    regVal = readRegister8(ADXL345_POWER_CTL);
    if(measure){
        regVal |= (1<<ADXL345_MEASURE);
    }
    else{
        regVal &= ~(1<<ADXL345_MEASURE);
    }
    writeRegister(ADXL345_POWER_CTL, regVal);
}

void ADXL345_WE::setSleep(bool sleep, adxl345_wUpFreq freq){
    regVal = readRegister8(ADXL345_POWER_CTL);
    regVal &= 0b11111100;
    regVal |= freq;
    if(sleep){
        regVal |= (1<<ADXL345_SLEEP);
    }
    else{
        setMeasureMode(false);  // it is recommended to enter Stand Mode when clearing the Sleep Bit!
        regVal &= ~(1<<ADXL345_SLEEP);
        regVal &= ~(1<<ADXL345_MEASURE);
    }
    writeRegister(ADXL345_POWER_CTL, regVal);
    if(!sleep){
        setMeasureMode(true);
    }
}

void ADXL345_WE::setSleep(bool sleep){
    regVal = readRegister8(ADXL345_POWER_CTL);
    if(sleep){
        regVal |= (1<<ADXL345_SLEEP);
    }
    else{
        setMeasureMode(false);  // it is recommended to enter Stand Mode when clearing the Sleep Bit!
        regVal &= ~(1<<ADXL345_SLEEP);
        regVal &= ~(1<<ADXL345_MEASURE);
    }
    writeRegister(ADXL345_POWER_CTL, regVal);
    if(!sleep){
        setMeasureMode(true);
    }
}
    
void ADXL345_WE::setAutoSleep(bool autoSleep, adxl345_wUpFreq freq){
    if(autoSleep){
        setLinkBit(true);
    }
    regVal = readRegister8(ADXL345_POWER_CTL);
    regVal &= 0b11111100;
    regVal |= freq;
    if(autoSleep){
        regVal |= (1<<ADXL345_AUTO_SLEEP);      
    }
    else{
        regVal &= ~(1<<ADXL345_AUTO_SLEEP);
    }
    writeRegister(ADXL345_POWER_CTL, regVal);
}
        
void ADXL345_WE::setAutoSleep(bool autoSleep){
    if(autoSleep){
        setLinkBit(true);
        regVal = readRegister8(ADXL345_POWER_CTL);
        regVal |= (1<<ADXL345_AUTO_SLEEP);
        writeRegister(ADXL345_POWER_CTL, regVal);
    }
    else{
        regVal = readRegister8(ADXL345_POWER_CTL);
        regVal &= ~(1<<ADXL345_AUTO_SLEEP);
        writeRegister(ADXL345_POWER_CTL, regVal);
    }
        
}

bool ADXL345_WE::isAsleep(){
    return readRegister8(ADXL345_ACT_TAP_STATUS) & (1<<ADXL345_ASLEEP);
}

void ADXL345_WE::setLowPower(bool lowpwr){
    regVal = readRegister8(ADXL345_BW_RATE);
    if(lowpwr){
        regVal |= (1<<ADXL345_LOW_POWER);
    }
    else{
        regVal &= ~(1<<ADXL345_LOW_POWER);
    }
    writeRegister(ADXL345_BW_RATE, regVal);
}
            
/************ Interrupts ************/


void ADXL345_WE::setInterrupt(adxl345_int type, uint8_t pin){
    regVal = readRegister8(ADXL345_INT_ENABLE);
    regVal |= (1<<type);
    writeRegister(ADXL345_INT_ENABLE, regVal);
    regVal = readRegister8(ADXL345_INT_MAP);
    if(pin == INT_PIN_1){
        regVal &= ~(1<<type);
    }
    else {
        regVal |= (1<<type);
    }
    writeRegister(ADXL345_INT_MAP, regVal);
}

void ADXL345_WE::setInterruptPolarity(uint8_t pol){
    regVal = readRegister8(ADXL345_DATA_FORMAT);
    if(pol == ADXL345_ACT_HIGH){
        regVal &= ~(0b00100000);
    }
    else if(pol == ADXL345_ACT_LOW){
        regVal |= 0b00100000;
    }
};

void ADXL345_WE::deleteInterrupt(adxl345_int type){
    regVal = readRegister8(ADXL345_INT_ENABLE);
    regVal &= ~(1<<type);
    writeRegister(ADXL345_INT_ENABLE, regVal);  
}

uint8_t ADXL345_WE::readAndClearInterrupts(){
    regVal = readRegister8(ADXL345_INT_SOURCE);
    return regVal;
}

bool ADXL345_WE::checkInterrupt(uint8_t source, adxl345_int type){
    source &= (1<<type);
    return source;
}
void ADXL345_WE::setLinkBit(bool link){
    regVal = readRegister8(ADXL345_POWER_CTL);
    if(link){
        regVal |= (1<<ADXL345_LINK);
    }
    else{
        regVal &= ~(1<<ADXL345_LINK);
    }
    writeRegister(ADXL345_POWER_CTL, regVal);
}

void ADXL345_WE::setFreeFallThresholds(float ffg, float fft){
    regVal = (uint8_t)(round(ffg / 0.0625));
    if(regVal<1){
        regVal = 1;
    }
    writeRegister(ADXL345_THRESH_FF, regVal);
    regVal = (uint8_t)(round(fft / 5));
    if(regVal<1){
        regVal = 1;
    }
    writeRegister(ADXL345_TIME_FF, regVal);
}

void ADXL345_WE::setActivityParameters(adxl345_dcAcMode mode, adxl345_actTapSet axes, float threshold){
    regVal = (uint8_t)(round(threshold / 0.0625));
    if(regVal<1){
        regVal = 1;
    }
    
    writeRegister(ADXL345_THRESH_ACT, regVal);

    regVal = readRegister8(ADXL345_ACT_INACT_CTL);
    regVal &= 0x0F;
    regVal |= ((uint8_t)mode + uint8_t(axes))<<4;
    writeRegister(ADXL345_ACT_INACT_CTL, regVal);
}

void ADXL345_WE::setInactivityParameters(adxl345_dcAcMode mode, adxl345_actTapSet axes, float threshold, uint8_t inactTime){
    regVal = (uint8_t)(round(threshold / 0.0625));
    if(regVal<1){
        regVal = 1;
    }
    writeRegister(ADXL345_THRESH_INACT, regVal);

    regVal = readRegister8(ADXL345_ACT_INACT_CTL);
    regVal &= 0xF0;
    regVal |= (uint8_t)mode + uint8_t(axes);
    writeRegister(ADXL345_ACT_INACT_CTL, regVal);

    writeRegister(ADXL345_TIME_INACT, inactTime);
}

void ADXL345_WE::setGeneralTapParameters(adxl345_actTapSet axes, float threshold, float duration, float latent){
    regVal = readRegister8(ADXL345_TAP_AXES);
    regVal &= 0b11111000;
    regVal |= uint8_t(axes);
    writeRegister(ADXL345_TAP_AXES, regVal);
    
    regVal = (uint8_t)(round(threshold / 0.0625));
    if(regVal<1){
        regVal = 1;
    }
    writeRegister(ADXL345_THRESH_TAP,regVal);
    
    regVal = (uint8_t)(round(duration / 0.625));
    if(regVal<1){
        regVal = 1;
    }
    writeRegister(ADXL345_DUR, regVal);
    
    regVal = (uint8_t)(round(latent / 1.25));
    if(regVal<1){
        regVal = 1;
    }
    writeRegister(ADXL345_LATENT, regVal);      
}

void ADXL345_WE::setAdditionalDoubleTapParameters(bool suppress, float window){
    regVal = readRegister8(ADXL345_TAP_AXES);
    if(suppress){
        regVal |= (1<<ADXL345_SUPPRESS);
    }
    else{
        regVal &= ~(1<<ADXL345_SUPPRESS);
    }
    writeRegister(ADXL345_TAP_AXES, regVal);
    
    regVal = (uint8_t)(round(window / 1.25));
    writeRegister(ADXL345_WINDOW, regVal);
}

uint8_t ADXL345_WE::getActTapStatus(){
    return readRegister8(ADXL345_ACT_TAP_STATUS);
}

String ADXL345_WE::getActTapStatusAsString(){
    uint8_t mask = (readRegister8(ADXL345_ACT_INACT_CTL)) & 0b01110000;
    mask |= ((readRegister8(ADXL345_TAP_AXES)) & 0b00000111);
        
    String returnStr = "";
    regVal = readRegister8(ADXL345_ACT_TAP_STATUS); 
    regVal &= mask;
        
    if(regVal & (1<<ADXL345_TAP_Z)) { returnStr += "TAP-Z "; }
    if(regVal & (1<<ADXL345_TAP_Y)) { returnStr += "TAP-Y "; }
    if(regVal & (1<<ADXL345_TAP_X)) { returnStr += "TAP-X "; }
    if(regVal & (1<<ADXL345_ACT_Z)) { returnStr += "ACT-Z "; }
    if(regVal & (1<<ADXL345_ACT_Y)) { returnStr += "ACT-Y "; }
    if(regVal & (1<<ADXL345_ACT_X)) { returnStr += "ACT-X "; }
    
    return returnStr;
}

/************ FIFO ************/

void ADXL345_WE::setFifoParameters(adxl345_triggerInt intNumber, uint8_t samples){
    regVal = readRegister8(ADXL345_FIFO_CTL);
	regVal &= 0b11000000;
    regVal |= (samples-1);
    if(intNumber == ADXL345_TRIGGER_INT_2){
        regVal |= 0x20;
    }
    writeRegister(ADXL345_FIFO_CTL, regVal);
}

void ADXL345_WE::setFifoMode(adxl345_fifoMode mode){
    regVal = readRegister8(ADXL345_FIFO_CTL);
    regVal &= 0b00111111;
    regVal |= (mode<<6);
    writeRegister(ADXL345_FIFO_CTL,regVal);
}

uint8_t ADXL345_WE::getFifoStatus(){
    return readRegister8(ADXL345_FIFO_STATUS);
}

void ADXL345_WE::resetTrigger(){
    setFifoMode(ADXL345_BYPASS);
    setFifoMode(ADXL345_TRIGGER);
}


/************************************************ 
    private functions
*************************************************/

uint8_t ADXL345_WE::writeRegister(uint8_t reg, uint8_t val){
    if(!useSPI){
        _wire->beginTransmission(i2cAddress);
        _wire->write(reg);
        _wire->write(val);
  
        return _wire->endTransmission();
    }
    else{
        digitalWrite(csPin, LOW);
        _spi->transfer(reg); 
        _spi->transfer(val);
        digitalWrite(csPin, HIGH);
        return false; // to be amended
    }
}
  
uint8_t ADXL345_WE::readRegister8(uint8_t reg){
    uint8_t regValue = 0;
    if(!useSPI){    
        _wire->beginTransmission(i2cAddress);
        _wire->write(reg);
        _wire->endTransmission(false);
        _wire->requestFrom(i2cAddress,1);
        if(_wire->available()){
            regValue = _wire->read();
        }
    }
    else{
        reg |= 0x80;
        digitalWrite(csPin, LOW);
        _spi->transfer(reg); 
        regValue = _spi->transfer(0x00);
        digitalWrite(csPin, HIGH);
    }
    return regValue;
}


int16_t ADXL345_WE::readRegister16(uint8_t reg){
    uint8_t MSByte = 0, LSByte = 0;
    int16_t regValue = 0;
    if(!useSPI){
        _wire->beginTransmission(i2cAddress);
        _wire->write(reg);
        _wire->endTransmission(false);
        _wire->requestFrom(i2cAddress,2);
        if(_wire->available()){
            LSByte = _wire->read();
            MSByte = _wire->read();
        }
    }
    else{
        reg = reg | 0x80;
        reg = reg | 0x40;
        digitalWrite(csPin, LOW);
        _spi->transfer(reg); 
        LSByte = _spi->transfer(0x00);
        MSByte = _spi->transfer(0x00);
        digitalWrite(csPin, HIGH);
    }
    regValue = (MSByte<<8) + LSByte;    
    return regValue;
}

uint64_t ADXL345_WE::readRegister3x16(uint8_t reg){    
    uint8_t byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0, byte4 = 0, byte5 = 0;
    uint64_t regValue = 0;
    if(!useSPI){
        _wire->beginTransmission(i2cAddress);
        _wire->write(reg);
        _wire->endTransmission(false);
        _wire->requestFrom(i2cAddress,6);
        if(_wire->available()){
            byte0 = _wire->read();
            byte1 = _wire->read();
            byte2 = _wire->read();
            byte3 = _wire->read();
            byte4 = _wire->read();
            byte5 = _wire->read();
        }
    }
    else{
        reg = reg | 0x80;
        reg = reg | 0x40;
        digitalWrite(csPin, LOW);
        _spi->transfer(reg); 
        byte0 = _spi->transfer(0x00);
        byte1 = _spi->transfer(0x00);
        byte2 = _spi->transfer(0x00);
        byte3 = _spi->transfer(0x00);
        byte4 = _spi->transfer(0x00);
        byte5 = _spi->transfer(0x00);
        digitalWrite(csPin, HIGH);
    }
        
    regValue = ((uint64_t) byte1<<40) + ((uint64_t) byte0<<32) +((uint64_t) byte3<<24) + 
           + ((uint64_t) byte2<<16) + ((uint64_t) byte5<<8) +  (uint64_t)byte4;
    return regValue;
}



