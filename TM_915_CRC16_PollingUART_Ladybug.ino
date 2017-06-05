/*
Send EPC to PC by Serial Port. Baud rate is 9600bps at PC side.
Send Command to module by software Serial Port.
Arduino port2 (D2) to TXD(Green wire) of TM-915
Arduino port3 (D3) to RXD(white wire) of TM-915
Arduino port4 (D4) to select  Single_EPC or Multi_EPC scan. Connect to ground is Single_EPC.
 
Created by Jotek, 28-Jul-2016

Augmented by Kris Winer 06/04/17
*/

//#include <SoftwareSerial.h>
// software serial #1: RX = digital port2 (D2), TX = digital port3 (D3).
//SoftwareSerial TM915(2,3); 
#define TM915 Serial1  // use hardware serial on Serial1 for Butterfly

#define led            13 
#define insw           4 

#define SendCommand    1
#define ReadTimeOut    2
#define ResponseEPC    3

#define Single_EPC     1
#define Multi_EPC      2
#define EPCTimerOut_1  100
#define EPCTimerOut_4  400

#define FALUE_STATUS   0
#define SUCCESS_STATUS 1

#define P_CCITT        0x1021
#define UHF_CRC16_CHECK_RESULT  0x1D0F

#define AtLeastEPC     4

char  rx_buf[200];
char  EPCframe[40];
int   rxlen;
char  cnt;
char  CommandType;
char  TM915Status;
long  StartTime;
long  EPCTimeOutTime;
uint8_t ii; // counting index

void setup()
{
  pinMode(led, OUTPUT); 
  pinMode(insw, INPUT_PULLUP);   
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  //while(!Serial); // wait for serial port to connect. Needed for Leonardo only
  //TM-915 baud rate 38400bps.
  Serial1.begin(38400);
  delay(2000);
  TM915Status = SendCommand;
  Serial.println("TM-915 EVM");
  delay(2000);

  // Check reader version                
  Serial1.print("\nV\r");   // Send version query
  getResponse();
  // Parse response
  Serial.print("Firmware Version = ");  
  for(ii = 2; ii < 6; ii++){Serial.print(rx_buf[ii]);}
  Serial.println(" ");
  Serial.print("Reader ID = "); 
  for(ii = 7; ii < 15; ii++){Serial.print(rx_buf[ii]);}
  Serial.println(" ");
  Serial.print("Hardware Version = "); 
  for(ii = 16; ii < 18; ii++){Serial.print(rx_buf[ii]);}
  Serial.println(" ");
  Serial.print("RF Band Regulation = "); 
  for(ii = 19; ii < 20; ii++){Serial.print(rx_buf[ii]);}
  Serial.println(" ");

  // Get RF Power
  Serial1.print("\nN0,00\r"); // Send RF power query
  getResponse();
  // Parse response
  Serial.print("RF Power level = ");  
  for(ii = 1; ii < 4; ii++){Serial.print(rx_buf[ii]);}
  Serial.println(" ");

  // Set RF Power
  Serial1.print("\nN1,00\r"); // Set RF power, reader resets after this change
  Serial.println("Reset RF Power level");  
  delay(1000);
  
  // Get RF Power
  Serial1.print("\nN0,00\r"); // Send RF power query
  getResponse();
  // Parse response
  Serial.print("RF Power level = ");  
  for(ii = 1; ii < 4; ii++){Serial.print(rx_buf[ii]);}
  Serial.println(" ");
 
  // Get RF regulation
  Serial1.print("\nN4,00\r"); // Send RF power query
  getResponse();
  // Parse response
  Serial.print("RF Regulation level = ");  
  if(rx_buf[3] == 49) Serial.println("US 902 - 928 MHz band");
  if(rx_buf[3] == 50) Serial.println("TW 922 - 928 MHz band");
  if(rx_buf[3] == 51) Serial.println("CAN 920 - 925 MHz band");
  if(rx_buf[3] == 52) Serial.println("CAN2 840 - 845 MHz band");
  if(rx_buf[3] == 53) Serial.println("EU 865 - 868 MHz band");
  Serial.println(" ");

  // Set RF regulation
  Serial1.print("\nN5,01\r"); // Set RF regulation for US 902 - 928 MHz
  Serial.println("Reset RF Regulation");  
  Serial.println(" ");
  delay(1000);

  // Get RF regulation
  Serial1.print("\nN4,00\r"); // Send RF power query
  getResponse();
  // Parse response
  Serial.print("RF Regulation level = ");  
  if(rx_buf[3] == 49) Serial.println("US 902 - 928 MHz band");
  if(rx_buf[3] == 50) Serial.println("TW 922 - 928 MHz band");
  if(rx_buf[3] == 51) Serial.println("CAN 920 - 925 MHz band");
  if(rx_buf[3] == 52) Serial.println("CAN2 840 - 845 MHz band");
  if(rx_buf[3] == 53) Serial.println("EU 865 - 868 MHz band");
  Serial.println(" ");

  TM915.print("\nQ\r");   //Send scan single-EPC command.
  delay(1000);

  // Read EPC data on tag
  Serial1.print("\nR1,2,6\r"); // Read EPC bank (1), read word addresses from 2 to 6
  getResponse();
  // Parse response
  Serial.print("EPC data = ");  
  for(ii = 0; ii < 20; ii++){Serial.print(rx_buf[ii]);}
  Serial.println(" ");

  // Read TID data on tag
  Serial1.print("\nR2,0,4\r"); // Read TID bank (2), read word addresses from 0 to 4
  getResponse();
  // Parse response
  Serial.print("TID data = ");  
  for(ii = 0; ii < 20; ii++){Serial.print(rx_buf[ii]);}
  Serial.println(" ");

 // Read USER data on tag
  Serial1.print("\nR3,0,8\r"); // Read USER bank (3), read word addresses from 0 to 8
  getResponse();
  // Parse response
  Serial.print("USER data = ");  
  for(ii = 0; ii < 36; ii++){Serial.print(rx_buf[ii]);}
  Serial.println(" ");

  // Write USER data on tag
  Serial1.print("\nW3,0,8,000000000000000000000000000000000000\r"); // Write zeros to first eight words of USER bank (3)  
  getResponse();
  // Parse response
  Serial.print("Write response = ");  
  for(ii = 0; ii < 6; ii++){Serial.print(rx_buf[ii]);}
  Serial.println(" ");
  
  // Read USER data on tag
  Serial1.print("\nR3,0,8\r"); // Read USER bank (3), read word addresses from 0 to 8
  getResponse();
  // Parse response
  Serial.print("USER data = ");  
  for(ii = 0; ii < 36; ii++){Serial.print(rx_buf[ii]);}
  Serial.println(" ");
  
  /* end of set up*/
}

void loop()
{
 
  switch(TM915Status)
        {
          case SendCommand:
               digitalWrite(led, HIGH);               
               if (digitalRead(insw)==HIGH)
                  {
                     CommandType=Multi_EPC;
                     EPCTimeOutTime = EPCTimerOut_4;
                     Serial.println("\r\nRead Multi-EPC");                    
                     TM915.print("\nU\r");   //Send scan multi-EPC command.
                  }
               else
                  {
                     CommandType=Single_EPC;
                     EPCTimeOutTime = EPCTimerOut_1;
                     Serial.println("\r\nRead Single-EPC");
                     TM915.print("\nQ\r");   //Send scan single-EPC command.
                  } 

               rxlen = 0;
               StartTime = millis();
               TM915Status = ReadTimeOut;
               digitalWrite(led, LOW); 
               break;
          case ReadTimeOut:
               do
               {  //Receive EPC data by polling mode.
                    //Due to polling speed not so fast then 38400bps, 
                    //so for Arduino UNO, can receive only one EPC data completely. 
                    while (TM915.available())     
                          {           
                              rx_buf[rxlen] = TM915.read();        
                              rxlen++;
                          }                       
                 }
                 while ((millis() - StartTime) < EPCTimeOutTime);
               TM915Status = ResponseEPC;
               break;
          case ResponseEPC:          
               //Print out received data length. 
               //Serial.print("("); 
               //Serial.print(rxlen); 
               //Serial.println(")"); 
               if (rxlen>AtLeastEPC)
                  {          
                       //Print out all received data.   
                       //for (cnt=0; cnt<rxlen; cnt++)  
                       //     Serial.print(rx_buf[cnt]);
                       for (cnt=0; cnt<rxlen; cnt++)
                       {    
                            if ((rx_buf[cnt] == 0x0A) && (rx_buf[cnt+1] == 'Q'))
                                {   //verify single EPC
                                    cnt += 2;
                                    if (check_EPC(&rx_buf[cnt], (rxlen-cnt)) == SUCCESS_STATUS)
                                       {  //print out EPC data  
                                          do {
                                               Serial.print(rx_buf[cnt]);
                                               cnt++;
                                               if (rx_buf[cnt] == '\r') break;                                           
                                             }while(cnt < rxlen);
                                       }
                                }
                            else if ((rx_buf[cnt] == 0x0A) && (rx_buf[cnt+1] == 'U'))    
                                    {  //verify multi EPC
                                       cnt += 2;
                                       if (check_EPC(&rx_buf[cnt], (rxlen-cnt)) == SUCCESS_STATUS)
                                          {  //print out EPC data  
                                             do {
                                                  Serial.print(rx_buf[cnt]);
                                                  cnt++;
                                                  if (rx_buf[cnt] == '\r') break;                                           
                                                }while(cnt < rxlen);
                                          }
                                   }
                       }
                  }  
               delay(50);               
               TM915Status = SendCommand;
               break;
        }//switch

}

void getResponse()
{
  StartTime = millis();
  EPCTimeOutTime = EPCTimerOut_4;
  rxlen = 0;
               do{ 
                    while (Serial1.available())     
                          {           
                              rx_buf[rxlen] = Serial1.read();        
                              rxlen++;
                          }                       
                   } while ((millis() - StartTime) < EPCTimeOutTime);

}


// Calculate CRC16-CCITT
word calcu_crc16_CCITT_FFFF(char *data, char len)
{
  char i, j;
  word crc = 0xffff;

  for( i=0; i<len; i++)
     {
	  crc ^= (data[i]<<8);
          for( j=0; j<8; j++)
               if (crc & 0x8000)
                   crc =  (crc << 1) ^ P_CCITT;
               else
        	   crc =  (crc << 1);
      }
  return crc;
}

byte check_EPC(char *pstr, char len)
{
        char  val; 
        char  Vdata;
        char  count;
	for (count=0; count<(len/2); count++)
	    {
		  val = 0;
		  Vdata = *pstr;
	          if ((Vdata<='9')&&(Vdata>='0'))
		       Vdata -= '0';
	          else if ((Vdata<='F')&&(Vdata>='A'))
	    	            Vdata = Vdata-'A'+10;
	               else if (Vdata == '\r')
                                break;
                            else Vdata = 0;
	          val = Vdata;
	          pstr++;
		  Vdata = *pstr;
	          if ((Vdata<='9')&&(Vdata>='0'))
		       Vdata -= '0';
	          else if ((Vdata<='F')&&(Vdata>='A'))
	    	            Vdata = Vdata-'A'+10;
	               else if (Vdata == '\r')
                                return FALUE_STATUS;
                            else Vdata = 0;
	          pstr++;
	          val<<=4;
	          val += Vdata;
	          EPCframe[count] = val;
	    }
	if (calcu_crc16_CCITT_FFFF(EPCframe, count) == UHF_CRC16_CHECK_RESULT)
            return SUCCESS_STATUS;
        else
	    return FALUE_STATUS;
}


