#include "mag_acc_gyro.h"

#include <math.h>

template <typename T> struct vector {
  T x, y, z;
};

template <typename Ta, typename Tb, typename To> static void vector_cross(const vector<Ta> *a, const vector<Tb> *b, vector<To> *out);
template <typename Ta, typename Tb> static float vector_dot(const vector<Ta> *a, const vector<Tb> *b);
static void vector_normalize(vector<float> *a);


template <typename Ta, typename Tb, typename To> void vector_cross(const vector<Ta> *a, const vector<Tb> *b, vector<To> *out)
{
  out->x = (a->y * b->z) - (a->z * b->y);
  out->y = (a->z * b->x) - (a->x * b->z);
  out->z = (a->x * b->y) - (a->y * b->x);
}

template <typename Ta, typename Tb> float vector_dot(const vector<Ta> *a, const vector<Tb> *b)
{
  return (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
}

void vector_normalize(vector<float> *a)
{
  float mag = sqrt(vector_dot(a, a));
  a->x /= mag;
  a->y /= mag;
  a->z /= mag;
}

void vector_rotate(vector<float> *m, float xRot, float yRot, vector<float> *mRotated) {
  float cosBeta  = cos(xRot);
  float sinBeta  = sin(xRot);
  float sinYotta = sin(yRot);
  float cosYotta = sin(yRot);

/*
  mRotated->x =  cosBeta + (sinBeta * sinYotta) + (sinBeta * cosYotta);
  mRotated->y =  cosBeta + cosYotta             - sinYotta;
  mRotated->z = -sinBeta + (cosBeta * sinYotta) + (cosBeta * cosYotta);
*/
/*
*/
  mRotated->x =  (m->x * cosYotta) + (m->z * sinYotta);
  mRotated->y =  m->y;
  mRotated->z = (-m->x * sinYotta) + (m->z * cosYotta);

  mRotated->x =  m->x;
  mRotated->y = (m->y * cosBeta) - (m->z * sinBeta);
  mRotated->z = (m->y * sinBeta) + (m->z * cosBeta);
/*
*/
}

void magAccGyroTask(void* parameter) {
  MagAccGyroTaskParameter* magAccGyroTaskParameter = (MagAccGyroTaskParameter*)parameter;

  BBI2C bbi2c;
  
  uint8_t WHO_AM_I    = 0x0F;
  
  uint8_t CTRL_REG1   = 0x20;
  uint8_t CTRL_REG2   = 0x21;
  uint8_t CTRL_REG3   = 0x22;
  uint8_t CTRL_REG4   = 0x23;
  uint8_t CTRL_REG5   = 0x24;
  
  uint8_t STATUS_REG  = 0x27;
  uint8_t OUT_X_L     = 0x28;
  uint8_t OUT_X_H     = 0x29;
  uint8_t OUT_Y_L     = 0x2A;
  uint8_t OUT_Y_H     = 0x2B;
  uint8_t OUT_Z_L     = 0x2C;
  uint8_t OUT_Z_H     = 0x2D;
  uint8_t TEMP_OUT_L  = 0x2E;
  uint8_t TEMP_OUT_H  = 0x2F;
  uint8_t INT_CFG     = 0x30;
  uint8_t INT_SRC     = 0x31;
  uint8_t INT_THS_L   = 0x32;
  uint8_t INT_THS_H   = 0x33;
  
  memset(&bbi2c, 0, sizeof(bbi2c));
  bbi2c.bWire = 0; // use bit bang, not wire library
  bbi2c.iSDA = SDA_PIN;
  bbi2c.iSCL = SCL_PIN;

  I2CInit(&bbi2c, 100000L);
  delay(100); // allow devices to power up

  Serial.println("====================LIS3MDL===================");
  Serial.println(I2CDiscoverDevice(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS));
  delay(500);

//  Serial.println("==============================================");
//  Serial.println(I2CDiscoverDevice(&bbi2c, DS33_SA0_HIGH_ADDRESS));
  delay(500);
  
  uint8_t whoAmIBuffer[1];
  bzero(whoAmIBuffer, 1);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, WHO_AM_I, whoAmIBuffer, 1);
//  Serial.println("==============================================");
//  Serial.println(whoAmIBuffer[0] == 0x3D);
//  Serial.println(whoAmIBuffer[0]);
  delay(500);

  bzero(whoAmIBuffer, 1);
  I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_WHO_AM_I, whoAmIBuffer, 1);
//  Serial.println("==============================================");
//  Serial.println(whoAmIBuffer[0] == DS33_WHO_ID);
//  Serial.println(whoAmIBuffer[0]);
  delay(500);

  uint8_t initBuffer1[2] = {
    // 0x70 = 0b01110000
    // 0x7C = 0b01111100
    // OM = 11 (ultra-high-performance mode for X and Y); D0 = 111 (80Hz ODR) //DO = 100 (10 Hz ODR)
    CTRL_REG1,
    0x70
  };
  I2CWrite(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, initBuffer1, 2);
  bzero(initBuffer1, 2);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, CTRL_REG1, &(initBuffer1[0]), 1);
//  Serial.println("==============================================");
//  Serial.println(initBuffer1[0] == 0x70);
  delay(500);


  uint8_t initBuffer2[2] = {
    // 0x00 = 0b00000000
    // FS = 00 (+/- 4 gauss full scale)
    CTRL_REG2,
    0x00,
  };
  I2CWrite(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, initBuffer2, 2);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, CTRL_REG2, &(initBuffer2[0]), 1);
//  Serial.println("==============================================");
//  Serial.println(initBuffer2[0] == 0x00);
  delay(500);

  uint8_t initBuffer3[2] = {
    // 0x00 = 0b00000000
    // MD = 00 (continuous-conversion mode)
    CTRL_REG3,
    0x00
  };
  I2CWrite(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, initBuffer3, 2);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, CTRL_REG3, &(initBuffer3[0]), 1);
//  Serial.println("==============================================");
//  Serial.println(initBuffer3[0] == 0x00);
  delay(500);

  uint8_t initBuffer4[2] = {
    // 0x0C = 0b00001100
    // OMZ = 11 (ultra-high-performance mode for Z)
    CTRL_REG4,
    0x0C,
  };
  I2CWrite(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, initBuffer4, 2);
  I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, CTRL_REG4, &(initBuffer4[0]), 1);
//  Serial.println("==============================================");
//  Serial.println(initBuffer4[0] == 0x0C);
  delay(500);

  // !!!GYRO!!!
  uint8_t initBuffer5[2] = {
    LSM6_CTRL1_XL,
    0x80,
  };
  I2CWrite(&bbi2c, DS33_SA0_HIGH_ADDRESS, initBuffer5, 2);
  I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_CTRL1_XL, &(initBuffer5[0]), 1);
  delay(500);
  
  uint8_t initBuffer6[2] = {
    LSM6_CTRL2_G,
    0x80,
  };
  I2CWrite(&bbi2c, DS33_SA0_HIGH_ADDRESS, initBuffer6, 2);
  I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_CTRL2_G, &(initBuffer6[0]), 1);
  delay(500);

  uint8_t initBuffer7[2] = {
    LSM6_CTRL3_C,
    0x04,
  };
  I2CWrite(&bbi2c, DS33_SA0_HIGH_ADDRESS, initBuffer7, 2);
  I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_CTRL3_C, &(initBuffer7[0]), 1);
  delay(500);

  delay(100);

  vector<float> x;
  vector<float> y;
  vector<float> z;

  x.x = 1;
  x.y = 0;
  x.z = 0;
  y.x = 0;
  y.y = 1;
  y.z = 0;
  z.x = 0;
  z.y = 0;
  z.z = 1;

  float degrees_per_radian = 180 / 3.14159;
  float half_pi_in_radians = 3.14159 / 2;

  bzero(magAccGyroTaskParameter->magX, HISTORY_LENGTH);
  bzero(magAccGyroTaskParameter->magY, HISTORY_LENGTH);
  bzero(magAccGyroTaskParameter->magZ, HISTORY_LENGTH);

  bzero(magAccGyroTaskParameter->accX, HISTORY_LENGTH);
  bzero(magAccGyroTaskParameter->accY, HISTORY_LENGTH);
  bzero(magAccGyroTaskParameter->accZ, HISTORY_LENGTH);

  bzero(magAccGyroTaskParameter->gyroX, HISTORY_LENGTH);
  bzero(magAccGyroTaskParameter->gyroY, HISTORY_LENGTH);
  bzero(magAccGyroTaskParameter->gyroZ, HISTORY_LENGTH);

  Serial.println("magX\tmagY\tmagZ\tmagXMax\tmagXMin\tmagYMax\tmagYMin\tmagZMax\tmagZMin");
  
  while(true) {
  /*
    unsigned char ucMap[16];
  
    I2CScan(&bbi2c, ucMap);
    I2CRead(uint8_t u8Address, uint8_t *pu8Data, int iLength);
    I2CReadRegister(uint8_t iAddr, uint8_t u8Register, uint8_t *pData, int iLen);
    I2CWrite(uint8_t iAddr, uint8_t *pData, int iLen); 
  */
  
    uint8_t magXYZBytes[6];
    bzero(magXYZBytes, 6);
    I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, OUT_X_L | 0x80, magXYZBytes, 6);
//    I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, OUT_X_L, &(magXYZBytes[0]), 1);
//    I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, OUT_X_H, &(magXYZBytes[1]), 1);
//    I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, OUT_Y_L, &(magXYZBytes[2]), 1);
//    I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, OUT_Y_H, &(magXYZBytes[3]), 1);
//    I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, OUT_Z_L, &(magXYZBytes[4]), 1);
//    I2CReadRegister(&bbi2c, LIS3MDL_SA1_HIGH_ADDRESS, OUT_Z_H, &(magXYZBytes[5]), 1);
  
    uint8_t gyroXYZBytes[6];
    bzero(gyroXYZBytes, 6);
    I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_OUTX_L_G, &(gyroXYZBytes[0]), 1);
    I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_OUTX_H_G, &(gyroXYZBytes[1]), 1);
    I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_OUTY_L_G, &(gyroXYZBytes[2]), 1);
    I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_OUTY_H_G, &(gyroXYZBytes[3]), 1);
    I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_OUTZ_L_G, &(gyroXYZBytes[4]), 1);
    I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_OUTZ_H_G, &(gyroXYZBytes[5]), 1);
  
    uint8_t accXYZBytes[6];
    bzero(accXYZBytes, 6);
    I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_OUTX_L_XL, &(accXYZBytes[0]), 1);
    I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_OUTX_H_XL, &(accXYZBytes[1]), 1);
    I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_OUTY_L_XL, &(accXYZBytes[2]), 1);
    I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_OUTY_H_XL, &(accXYZBytes[3]), 1);
    I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_OUTZ_L_XL, &(accXYZBytes[4]), 1);
    I2CReadRegister(&bbi2c, DS33_SA0_HIGH_ADDRESS, LSM6_OUTZ_H_XL, &(accXYZBytes[5]), 1);
    
    int16_t mag[3];  
    int16_t gyro[3];
    int16_t acc[3];
  
    int16_t valueL = 0;
    int16_t valueH = 0;
    
    for(int i = 0; i < 3; i++) {
      valueL = magXYZBytes[(i * 2) + 0];
      valueH = magXYZBytes[(i * 2) + 1];
      mag[i] = (valueH << 8) + valueL;
  
      valueL = gyroXYZBytes[(i * 2) + 0];
      valueH = gyroXYZBytes[(i * 2) + 1];
      gyro[i] = (valueH << 8) + valueL;
  
      valueL = accXYZBytes[(i * 2) + 0];
      valueH = accXYZBytes[(i * 2) + 1];
      acc[i] = (valueH << 8) + valueL;
    }

    magAccGyroTaskParameter->magX[magAccGyroTaskParameter->historyIndex] = mag[0];
    magAccGyroTaskParameter->magY[magAccGyroTaskParameter->historyIndex] = mag[1];
    magAccGyroTaskParameter->magZ[magAccGyroTaskParameter->historyIndex] = mag[2];

    magAccGyroTaskParameter->accX[magAccGyroTaskParameter->historyIndex] = acc[0];
    magAccGyroTaskParameter->accY[magAccGyroTaskParameter->historyIndex] = acc[1];
    magAccGyroTaskParameter->accZ[magAccGyroTaskParameter->historyIndex] = acc[2];

    magAccGyroTaskParameter->gyroX[magAccGyroTaskParameter->historyIndex] = gyro[0];
    magAccGyroTaskParameter->gyroY[magAccGyroTaskParameter->historyIndex] = gyro[1];
    magAccGyroTaskParameter->gyroZ[magAccGyroTaskParameter->historyIndex] = gyro[2];

    magAccGyroTaskParameter->historyCount++;

    magAccGyroTaskParameter->historyIndex++;
    if(magAccGyroTaskParameter->historyIndex >= HISTORY_LENGTH) {
      magAccGyroTaskParameter->historyIndex = 0;
    }

    int magAcc[3];
    int accAcc[3];
    int gyroAcc[3];
    
    for(int i = 0; i < HISTORY_LENGTH; i++) {
      magAcc[0] += magAccGyroTaskParameter->magX[i];
      magAcc[1] += magAccGyroTaskParameter->magY[i];
      magAcc[2] += magAccGyroTaskParameter->magZ[i];

      accAcc[0] += magAccGyroTaskParameter->accX[i];
      accAcc[1] += magAccGyroTaskParameter->accY[i];
      accAcc[2] += magAccGyroTaskParameter->accZ[i];

      gyroAcc[0] += magAccGyroTaskParameter->gyroX[i];
      gyroAcc[1] += magAccGyroTaskParameter->gyroY[i];
      gyroAcc[2] += magAccGyroTaskParameter->gyroZ[i];
    }

    magAcc[0] /= HISTORY_LENGTH;
    magAcc[1] /= HISTORY_LENGTH;
    magAcc[2] /= HISTORY_LENGTH;

    accAcc[0] /= HISTORY_LENGTH;
    accAcc[1] /= HISTORY_LENGTH;
    accAcc[2] /= HISTORY_LENGTH;

    gyroAcc[0] /= HISTORY_LENGTH;
    gyroAcc[1] /= HISTORY_LENGTH;
    gyroAcc[2] /= HISTORY_LENGTH;

  if(magAccGyroTaskParameter->historyCount > HISTORY_LENGTH) {
    if(magAcc[0] > magAccGyroTaskParameter->magXMax) {
      magAccGyroTaskParameter->magXMax = magAcc[0];
      magAccGyroTaskParameter->magXRange = (magAccGyroTaskParameter->magXMax - magAccGyroTaskParameter->magXMin);
      magAccGyroTaskParameter->magXOffset = magAccGyroTaskParameter->magXMax - (magAccGyroTaskParameter->magXRange / 2);
    }
    if(magAcc[0] < magAccGyroTaskParameter->magXMin) {
      magAccGyroTaskParameter->magXMin = magAcc[0];
      magAccGyroTaskParameter->magXRange = (magAccGyroTaskParameter->magXMax - magAccGyroTaskParameter->magXMin);
      magAccGyroTaskParameter->magXOffset = magAccGyroTaskParameter->magXMax - (magAccGyroTaskParameter->magXRange / 2);
    }
    if(magAcc[1] > magAccGyroTaskParameter->magYMax) {
      magAccGyroTaskParameter->magYMax = magAcc[1];
      magAccGyroTaskParameter->magYRange = (magAccGyroTaskParameter->magYMax - magAccGyroTaskParameter->magYMin);
      magAccGyroTaskParameter->magYOffset = magAccGyroTaskParameter->magYMax - (magAccGyroTaskParameter->magYRange / 2);
    }
    if(magAcc[1] < magAccGyroTaskParameter->magYMin) {
      magAccGyroTaskParameter->magYMin = magAcc[1];
      magAccGyroTaskParameter->magYRange = (magAccGyroTaskParameter->magYMax - magAccGyroTaskParameter->magYMin);
      magAccGyroTaskParameter->magYOffset = magAccGyroTaskParameter->magYMax - (magAccGyroTaskParameter->magYRange / 2);
    }
    if(magAcc[2] > magAccGyroTaskParameter->magZMax) {
      magAccGyroTaskParameter->magZMax = magAcc[2];
      magAccGyroTaskParameter->magZRange = (magAccGyroTaskParameter->magZMax - magAccGyroTaskParameter->magZMin);
      magAccGyroTaskParameter->magZOffset = magAccGyroTaskParameter->magZMax - (magAccGyroTaskParameter->magZRange / 2);
    }
    if(magAcc[2] < magAccGyroTaskParameter->magZMin) {
      magAccGyroTaskParameter->magZMin = magAcc[2];
      magAccGyroTaskParameter->magZRange = (magAccGyroTaskParameter->magZMax - magAccGyroTaskParameter->magZMin);
      magAccGyroTaskParameter->magZOffset = magAccGyroTaskParameter->magZMax - (magAccGyroTaskParameter->magZRange / 2);
    }
  }

    char posBuffer[100];
//    sprintf(posBuffer, "mag % .5d:% .5d:% .5d gyro % .5d:% .5d:% .5d acc % .5d:% .5d:% .5d", mag[0], mag[1], mag[2], gyro[0], gyro[1], gyro[2], acc[0], acc[1], acc[2]);
//    Serial.println(posBuffer);
  
//    sprintf(posBuffer, "mag % .5d:% .5d:% .5d gyro % .5d:% .5d:% .5d acc % .5d:% .5d:% .5d", magAcc[0], magAcc[1], magAcc[2], gyroAcc[0], gyroAcc[1], gyroAcc[2], accAcc[0], accAcc[1], accAcc[2]);
//    sprintf(posBuffer, "%d\t%d\t%d\t%d\t%d\t%d", magAcc[0], magAcc[1], magAcc[2], accAcc[0], accAcc[1], accAcc[2]);

    vector<float> m;
    vector<float> a;

    float magnitude_x = 1.0f;
    float magnitude_y = 1.0f;
    float magnitude_z = 1.0f;

    a.x = accAcc[0];
    a.y = accAcc[1];
    a.z = accAcc[2];


//    float a_x_dot = vector_dot(&a, &x);
//    float a_y_dot = vector_dot(&a, &y);
//    float a_z_dot = vector_dot(&a, &z);
    
//    float magnitude_a = sqrt(vector_dot(&a, &a));
//    float a_x_angle = acos(a_x_dot / (magnitude_x * magnitude_a));
//    float a_y_angle = acos(a_y_dot / (magnitude_y * magnitude_a));
//    float a_z_angle = acos(a_z_dot / (magnitude_z * magnitude_a));


    float a_x_angle = 0.0; //asin(a.z / a.x);
    float a_y_angle = 0.0; //asin(a.z / a.y);
    float a_z_angle = 0.0; //asin(a.x / a.y);

    if(a.y != 0.0) {
      a_y_angle = atan2(a.z, a.y) - half_pi_in_radians;
      a_z_angle = atan2(a.x, a.y) - half_pi_in_radians;
    }
    if(a.x != 0.0) {
      a_x_angle = atan2(a.z, a.x) - half_pi_in_radians;
    }

    vector_normalize(&a);

//    Serial.print(" aRot x ");
//    Serial.print(a_x_angle);
//    Serial.print(" y ");
//    Serial.print(a_y_angle);
//    Serial.print(" z ");
//    Serial.print(a_z_angle);
//
//    magAcc[0] -= magAccGyroTaskParameter->magXOffset;
//    magAcc[1] -= magAccGyroTaskParameter->magYOffset;
//    magAcc[2] -= magAccGyroTaskParameter->magZOffset;

      magAccGyroTaskParameter->magXAdj = magAcc[0] - magAccGyroTaskParameter->magXOffset;
      magAccGyroTaskParameter->magYAdj = magAcc[1] - magAccGyroTaskParameter->magYOffset;
      magAccGyroTaskParameter->magZAdj = magAcc[2] - magAccGyroTaskParameter->magZOffset;

//
//    sprintf(posBuffer, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d", 
//      magAcc[0], 
//      magAcc[1], 
//      magAcc[2],
//      magAccGyroTaskParameter->magXMax,
//      magAccGyroTaskParameter->magXMin,
//      magAccGyroTaskParameter->magYMax,
//      magAccGyroTaskParameter->magYMin,
//      magAccGyroTaskParameter->magZMax,
//      magAccGyroTaskParameter->magZMin
//    );
//    Serial.print(posBuffer);

    m.x = 1000.0f * ((float)magAccGyroTaskParameter->magXAdj) / ((float)magAccGyroTaskParameter->magXRange);
    m.y = 1000.0f * ((float)magAccGyroTaskParameter->magYAdj) / ((float)magAccGyroTaskParameter->magYRange);
    m.z = 1000.0f * ((float)magAccGyroTaskParameter->magZAdj) / ((float)magAccGyroTaskParameter->magZRange);

//    sprintf(posBuffer, "%d\t%d\t%d", 
//      int(m.x), 
//      int(m.y), 
//      int(m.z)
//    );
//    Serial.print(posBuffer);
    
    float magnitude_m = sqrt(vector_dot(&m, &m));

    vector<float> mRotated;
    vector<float> mRotatedNegative;

    // normalise m & a
//    vector_normalize(&m);
    vector_rotate(&m, a_x_angle, a_y_angle, &mRotated);
    vector_rotate(&m, -a_x_angle, -a_y_angle, &mRotatedNegative);
    
//    sprintf(posBuffer, "%d\t%d\t%d", 
//      int(m.x * 1000.0), 
//      int(m.y * 1000.0), 
//      int(m.z * 1000.0)
//    );
//    Serial.print(posBuffer);

    // work out angles between a and the various axes
/*         
    float m_x_dot = vector_dot(&m, &x);
    float m_y_dot = vector_dot(&m, &y);
    float m_z_dot = vector_dot(&m, &z);
    int m_x_angle = int(acos(m_x_dot / (magnitude_x * magnitude_m)) * degrees_per_radian);
    int m_y_angle = int(acos(m_y_dot / (magnitude_y * magnitude_m)) * degrees_per_radian);
    int m_z_angle = int(acos(m_z_dot / (magnitude_z * magnitude_m)) * degrees_per_radian);
*/

    int az1 = int(10.0 * atan2(m.x, m.y) * degrees_per_radian);
    int az2 = int(10.0 * atan2(mRotated.x, mRotated.y) * degrees_per_radian);
    int az3 = int(10.0 * atan2(mRotatedNegative.x, mRotatedNegative.y) * degrees_per_radian);
//    sprintf(posBuffer, "%d\t%d\t%d\t%d\t%d\t%d", 
//      int(az1), 
//      int(az2), 
//      int(az3),
//      int(a_x_angle * 1000.0),
//      int(a_y_angle * 1000.0),
//      int(a_z_angle * 1000.0)
//    );
//    sprintf(posBuffer, "%d\t%d\t%d", 
//      int(a_x_angle * 1000.0),
//      int(a_y_angle * 1000.0),
//      int(a_z_angle * 1000.0)
//    );
    sprintf(posBuffer, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d", 
      int(a.x * 1000.0),
      int(a.y * 1000.0),
      int(a.z * 1000.0),
//      int(m.x * 1000.0),
//      int(m.y * 1000.0),
//      int(m.z * 1000.0),
      magAcc[0],
      magAcc[1],
      magAcc[2],
      magAccGyroTaskParameter->magXAdj,
      magAccGyroTaskParameter->magYAdj,
      magAccGyroTaskParameter->magZAdj,
      int(a_x_angle * 1000.0 * degrees_per_radian),
      int(a_y_angle * 1000.0 * degrees_per_radian),
      int(a_z_angle * 1000.0 * degrees_per_radian),
      az1,
      az2,
      az3
    );
    Serial.print(posBuffer);

//    sprintf(posBuffer, "%d", int(az));
//    Serial.print(posBuffer);
        
//    Serial.print(" mRot x ");
//    Serial.print(m_x_angle);
//    Serial.print(" y ");
//    Serial.print(m_y_angle);
//    Serial.print(" z ");
//    Serial.print(m_z_angle);

/*
      lis3mdl_range_t range = getRange();
  float scale = 1; // LSB per gauss
  if (range == LIS3MDL_RANGE_16_GAUSS)
    scale = 1711;
  if (range == LIS3MDL_RANGE_12_GAUSS)
    scale = 2281;
  if (range == LIS3MDL_RANGE_8_GAUSS)
    scale = 3421;
  if (range == LIS3MDL_RANGE_4_GAUSS)
    scale = 6842;

  event->magnetic.x = x_gauss * 100; // microTesla per gauss
  event->magnetic.y = y_gauss * 100; // microTesla per gauss
  event->magnetic.z = z_gauss * 100; // microTesla per gauss

*/

//    float a_m_dot = vector_dot(&a, &m);
//    int a_m_angle = int(acos(a_m_dot / (magnitude_m * magnitude_a)) * degrees_per_radian);

//    Serial.print(" am ");
//    Serial.print(a_m_angle);
    
    Serial.println("");
    // rotate m by - <these angles>

    delay(50);
  }
}
