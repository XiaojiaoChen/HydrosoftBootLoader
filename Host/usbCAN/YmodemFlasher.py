from usbCAN import USB_CAN

import numpy as np
from enum import Enum
import time
import os

USER_FLASH_SIZE = 0x0803F800 - 0x08004000

COM_OK       = 0
COM_ERROR    = 1
COM_ABORT    = 2
COM_TIMEOUT  = 3
COM_DATA     = 4
COM_LIMIT    = 5

MASTER_BROADCAST=0x07FF
MASTER_P2P_MASK =0x0400


PACKET_HEADER_SIZE=      3
PACKET_DATA_INDEX  =     3
PACKET_START_INDEX   =   0
PACKET_NUMBER_INDEX  =   1
PACKET_CNUMBER_INDEX =   2
PACKET_TRAILER_SIZE  =   2
PACKET_OVERHEAD_SIZE =   (PACKET_HEADER_SIZE + PACKET_TRAILER_SIZE)
PACKET_SIZE      =      128
PACKET_1K_SIZE    =     1024

# #/-------- Packet  Format in Ymodem------------------------------------------\
#  * | 0      |  1    |  2     |  3      | ... | n+3     | n+4  | n+5  | 
#  * |------------------------------------------------------------------------|
#  * | start | number | !num   | data[0] | ... | data[n] | crc0 | crc1 |
#  * \------------------------------------------------------------------------/


FILE_NAME_LENGTH=        64
FILE_SIZE_LENGTH=        16
SOH             =        0x01 # start of 128-byte data packet */
STX             =        0x02 # start of 1024-byte data packet */
EOT             =        0x04 # end of transmission */
ACK             =        0x06 # acknowledge */
NAK             =        0x15 # negative acknowledge */
CA              =        0x18 #two of these in succession aborts transfer */
CRC16           =        0x43 # 'C' == 0x43, request 16-bit CRC */
NEGATIVE_BYTE   =        0xFF
ABORT1          =        0x41 # 'A' == 0x41, abort by user */
ABORT2          =        0x61 # 'a' == 0x61, abort by uNAK_TIMEOUT     =        _t)0x100000)
DOWNLOAD_TIMEOUT=        5000 #One second retry delay */
MAX_ERRORS      =        5

#Choose one CAN channel (0 or 1) on the device as the flash channel
class YMODEM_FLASHER:
  def __init__(self,chn=0):
    self.CAN_ID_BROADCAST  = MASTER_BROADCAST
    self.CAN_ID_P2P_MASK = MASTER_P2P_MASK

    self.canbus=USB_CAN(CAN_ID=self.CAN_ID_BROADCAST)
    self.chn=chn
    self.packetBuffer=bytearray(PACKET_1K_SIZE + PACKET_OVERHEAD_SIZE)
    self.a_rx_ctrl=bytearray(1)
  
  def open(self,filterType=0,rxID=0):
    self.canbus.open(self.chn,filterType,rxID)
    self.canbus.start_keyboard()
    self.canbus.start_receiving(self.chn)

  def GoToApp(self):
    id=self.canbus.CAN_ID
    self.canbus.setCANID(self.CAN_ID_BROADCAST)
    holdChar = b'g'
    self.canbus.transmit(holdChar,1,self.chn)
    print("[INFO]Sent Command: jump to app")
    self.canbus.setCANID(id)

  def getInput(self):
    return self.canbus.getInput()

  def HoldAll(self):
    #set Tx ID to host ID,return char 'h' for holding on all CAN Node
    id=self.canbus.CAN_ID
    self.canbus.setCANID(self.CAN_ID_BROADCAST)
    holdChar = b'h'
    print("[INFO]Sent Command: Hold all")
    self.canbus.transmit(holdChar,1,self.chn)
    self.canbus.setCANID(id)



  def waitFor(self,waitingByte):
    status = COM_ERROR
      #Wait for Ack or 'C' */
    if (self.canbus.receive(self.a_rx_ctrl, 1) == COM_OK):
      if (self.a_rx_ctrl[0] == waitingByte):
        # if(waitingByte==ACK):
        #   print("[INFO]get ACK")
        status = COM_OK
      elif (self.a_rx_ctrl[0] == CA):
        print("[INFO]Get CA")
        if ((self.canbus.receive(self.a_rx_ctrl, 1) == COM_OK) and (self.a_rx_ctrl[0] == CA)):
          time.sleep( 2 )
          status = COM_ABORT
      elif (self.a_rx_ctrl[0] == CRC16):
        print("[INFO]Get C")
      else:
        print("[INFO]invalid code {}".format(self.a_rx_ctrl[0]))
        status = COM_ERROR
    else:
      status = COM_TIMEOUT

    return status


  def Flash(self,targetCANID,filePath):

    p_file_name=os.path.basename(filePath)
    p_file_content = open(filePath, 'rb').read()
    file_size=len(p_file_content)

    #set Tx ID to P2P ID,
    id=self.CAN_ID_P2P_MASK | targetCANID
    self.canbus.setCANID(self.CAN_ID_P2P_MASK | targetCANID)
    print("[INFO]Set CAN Tx ID to 0x{:x}".format(id))

    #send flash request
    print("[INFO]Sending Flash Request")
    self.canbus.clearRxBuffer()
    requestChar = b'f'
    self.canbus.transmit(requestChar,1,self.chn)
    result = COM_ERROR
    error=0
    while(result!=COM_OK):
      result = self.waitFor(CRC16)
      if(result==COM_ABORT):
        return COM_ABORT
      error+=1
      if(error>5):
        return COM_ERROR


    #Send Init Packet
    print("[INFO]Sending Init Packet")
    PrepareIntialPacket(self.packetBuffer, p_file_name, file_size)
    self.canbus.clearRxBuffer()
    result = COM_ERROR
    error=0
    while(result != COM_OK ):
      if(self.canbus.transmit(self.packetBuffer, PACKET_SIZE + PACKET_OVERHEAD_SIZE,self.chn)==COM_OK):
        print("[INFO]Erasing flash ...")
        result = self.waitFor(ACK)
        if(result==COM_OK):
          self.waitFor(CRC16)
        elif(result==COM_ABORT):
          return COM_ABORT
        error+=1
        if(error>5):
          return COM_ERROR

    #Send Data Packets
    pkt_size=0
    blk_number = 1
    p_file_ind = 0
    size = file_size
    packetNum=0
    packetTotal = size//1024+1
    result=COM_OK
    while ((size) and (result == COM_OK )):
      
      #Prepare next data packet */
      PreparePacket(p_file_content[p_file_ind:], self.packetBuffer, blk_number, size)
      packetNum +=1
      curT0=time.time_ns()//1000000

      #Resend packet if 'ACK' is not received*/
      result=COM_ERROR
      while (result != COM_OK ):
        curT1=time.time_ns()//1000000

        #Determine packet size */
        if (size >= PACKET_1K_SIZE):
          pkt_size = PACKET_1K_SIZE
        else:
          pkt_size = PACKET_SIZE
        
        #before sending every packet, clear the rx buffer
        self.canbus.clearRxBuffer()
        # print("[INFO]Sending Data Packet ({}/{}".format(packetNum,packetTotal))
        #Send data packet
        self.canbus.transmit(self.packetBuffer, pkt_size + PACKET_OVERHEAD_SIZE,self.chn)
        curT2 = time.time_ns()//1000000

        #Wait for ACK
        result = self.waitFor(ACK)

        #if get ACK, update next packet info
        if(result==COM_OK):
          curT3 = time.time_ns()//1000000
          print("[INFO]Tx: {} ms; ACK {} ms; Sent Data Packet ({}/{})".format(curT2-curT1,curT3-curT2,packetNum,packetTotal))
          if (size > pkt_size):
            p_file_ind += pkt_size
            size -= pkt_size
            if (blk_number == (USER_FLASH_SIZE / PACKET_1K_SIZE)):
              result = COM_LIMIT #boundary error */
            else:
              blk_number+=1
          else:
            p_file_ind += pkt_size
            size = 0
        elif(result==COM_ABORT):
          return COM_ABORT
      
    #Sending End Of Transmission char */
    result=COM_ERROR
    while (result != COM_OK ):
      print("[INFO]Sending EOT")
      self.canbus.transmit([EOT],1,self.chn)
      result = self.waitFor(ACK)

    #Empty packet to close session */
    PrepareIntialPacket(self.packetBuffer,'',0)
    result=COM_ERROR
    while (result != COM_OK ):
      print("[INFO]Sending Closing packet")
      self.canbus.transmit(self.packetBuffer, PACKET_SIZE + PACKET_OVERHEAD_SIZE,self.chn)
      result =COM_ERROR
      error=0
      while(result!=COM_OK):
        result = self.waitFor(ACK)
        error+=1
        if(error>2):
          return result

    return result #file transmitted successfully */

  def close(self):
    self.canbus.close()        



def UpdateCRC16(crc_in, byte):
  crc = crc_in
  inbyte = byte | 0x00000100
  condition=1
  while(condition):
    crc <<= 1
    inbyte <<= 1
    if(inbyte & 0x100):
      crc +=1
    if(crc & 0x10000):
      crc ^= 0x1021
    condition=((inbyte & 0x10000)==0)
  return (crc & 0xffff)


def Cal_CRC16(p_data, size):
  crc = 0
  for i in range(size):
    crc = UpdateCRC16(crc, p_data[i])
  crc = UpdateCRC16(crc, 0)
  crc = UpdateCRC16(crc, 0)
  return crc&0xffff

def CalcChecksum(p_data, size):
  sum = 0
  for i in range(size):
    sum += p_data[i]
  return (sum & 0xff)

#p_data is a bytearray
#p_file_name is a string

def PrepareIntialPacket(p_data, p_file_name, length):
  p_data[PACKET_START_INDEX] = SOH
  p_data[PACKET_NUMBER_INDEX] = 0x00
  p_data[PACKET_CNUMBER_INDEX] = 0xff

  #Filename writte
  fileNameLen=len(p_file_name)
  p_data[PACKET_DATA_INDEX:PACKET_DATA_INDEX+fileNameLen] = p_file_name.encode('utf-8')
  p_data[PACKET_DATA_INDEX+fileNameLen] = 0x00

  #file size written
  lenInd=PACKET_DATA_INDEX+fileNameLen + 1
  astring = str(length).encode('utf-8')
  p_data[lenInd:lenInd+len(astring)]=astring

  padInd=lenInd+len(astring)
  for i in range(padInd,PACKET_SIZE + PACKET_DATA_INDEX):
    p_data[i]=0x00

  #calculate CRC
  temp_crc = Cal_CRC16(p_data[PACKET_DATA_INDEX:], PACKET_SIZE)
  p_data[PACKET_DATA_INDEX+PACKET_SIZE]= (temp_crc >> 8)
  p_data[PACKET_DATA_INDEX+PACKET_SIZE+1]= (temp_crc & 0xFF)

def PreparePacket(p_source, p_packet,pkt_nr,size_blk):

  packet_size = PACKET_1K_SIZE if  size_blk >= PACKET_1K_SIZE else PACKET_SIZE
  size = size_blk if size_blk < packet_size  else packet_size
  
  #packet size indicator
  if (packet_size == PACKET_1K_SIZE):
    p_packet[PACKET_START_INDEX] = STX
  else:
    p_packet[PACKET_START_INDEX] = SOH
  
  #packet number
  p_packet[PACKET_NUMBER_INDEX] = pkt_nr
  p_packet[PACKET_CNUMBER_INDEX] = 255-pkt_nr  #(~pkt_nr)

  #packet data, fill empty space
  p_packet[PACKET_DATA_INDEX : PACKET_DATA_INDEX + size] = p_source[:size]
  if ( size  <= packet_size):
    for i in range(size + PACKET_DATA_INDEX ,packet_size + PACKET_DATA_INDEX):
        p_packet[i] = 0x1A #EOF (0x1A) or 0x00 */
  
  #calculate CRC
  temp_crc = Cal_CRC16(p_packet[PACKET_DATA_INDEX:], packet_size)
  p_packet[PACKET_DATA_INDEX+packet_size]= (temp_crc >> 8)
  p_packet[PACKET_DATA_INDEX+packet_size+1]= (temp_crc & 0xFF)