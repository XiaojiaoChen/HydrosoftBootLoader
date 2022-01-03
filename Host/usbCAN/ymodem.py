from usbCAN import USB_CAN

import numpy as np
from enum import Enum
import time
import os

canbus = USB_CAN() 


USER_FLASH_SIZE = 0x0803F800 - 0x08004000

COM_OK       = 0
COM_ERROR    = 1
COM_ABORT    = 2
COM_TIMEOUT  = 3
COM_DATA     = 4
COM_LIMIT    = 5


PACKET_HEADER_SIZE=      3
PACKET_DATA_INDEX  =     4
PACKET_START_INDEX   =   1
PACKET_NUMBER_INDEX  =   2
PACKET_CNUMBER_INDEX =   3
PACKET_TRAILER_SIZE  =   2
PACKET_OVERHEAD_SIZE =   (PACKET_HEADER_SIZE + PACKET_TRAILER_SIZE - 1)
PACKET_SIZE      =      128
PACKET_1K_SIZE    =     1024

# #/-------- Packet in IAP memory ------------------------------------------\
#  * | 0      |  1    |  2     |  3   |  4      | ... | n+4     | n+5  | n+6  | 
#  * |------------------------------------------------------------------------|
#  * | unused | start | number | !num | data[0] | ... | data[n] | crc0 | crc1 |
#  * \------------------------------------------------------------------------/
#  * the first byte is left unused for memory alignment reasons 


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

PACKET_1K_SIZE + PACKET_DATA_INDEX + PACKET_TRAILER_SIZE

aPacketData=bytearray(PACKET_1K_SIZE + PACKET_DATA_INDEX + PACKET_TRAILER_SIZE)

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


def PreparePacket(p_source, p_packet,pkt_nr,size_blk):
  #Make first three packet */
  packet_size = PACKET_1K_SIZE if  size_blk >= PACKET_1K_SIZE else PACKET_SIZE
  size = size_blk if size_blk < packet_size  else packet_size
  if (packet_size == PACKET_1K_SIZE):
    p_packet[PACKET_START_INDEX] = STX
  else:
    p_packet[PACKET_START_INDEX] = SOH
  
  p_packet[PACKET_NUMBER_INDEX] = pkt_nr
  p_packet[PACKET_CNUMBER_INDEX] = 255-pkt_nr  #(~pkt_nr)

  #Filename packet has valid data */
  p_packet[PACKET_DATA_INDEX : PACKET_DATA_INDEX + size] = p_source[:size]
 
  if ( size  <= packet_size):
    for i in range(size + PACKET_DATA_INDEX ,packet_size + PACKET_DATA_INDEX):
        p_packet[i] = 0x1A #EOF (0x1A) or 0x00 */


def UpdateCRC16(crc_in, byte):
  crc = crc_in
  inbyte = byte | 0x100
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




def Ymodem_Transmit(filePath):

  
  p_file_name=os.path.basename(filePath)
  p_buf = open(filePath, 'rb').read()
  file_size=len(p_buf)

  errors = 0
  ack_recpt = 0
  size = 0
  pkt_size=0
  p_buf_int=0
  result = COM_OK
  blk_number = 1
  a_rx_ctrl=bytearray(2)
  i=0  
  temp_crc=0




  tx_ctrl=bytearray(2)
  bchar='1'
  tx_ctrl[:1]=bchar.encode('utf-8')

  canbus.clearRxBuffer()
  print("Before Tx,  Size of Rx queue: {}".format(canbus.rxQueue.qsize()))
  canbus.transmit(tx_ctrl,1)
  print("transmit 1")

  print("Before Rx,  Size of Rx queue: {}".format(canbus.rxQueue.qsize()))
  while(a_rx_ctrl[0]!=CRC16):
    canbus.receive(a_rx_ctrl, 1)
    time.sleep(0.01)


  #Prepare first block - header */
  print("prepareing Init Packet")
  PrepareIntialPacket(aPacketData, p_file_name, file_size)

  while((ack_recpt==0 ) and ( result == COM_OK )):
    #Send Packet */
    print("Sending Init Packet")
    canbus.transmit(aPacketData[PACKET_START_INDEX:], PACKET_SIZE + PACKET_HEADER_SIZE)

    #Send CRC or Check Sum based on CRC16_F */
    temp_crc = Cal_CRC16(aPacketData[PACKET_DATA_INDEX:], PACKET_SIZE)
    canbus.transmit([(temp_crc >> 8)],1)
    canbus.transmit([temp_crc & 0xFF],1)
    print("Sending Init CRC = {}, high={}, low={}".format(temp_crc,temp_crc >> 8,temp_crc & 0xFF))

   
    #Wait for Ack and 'C' */
    if (canbus.receive(a_rx_ctrl, 1) == COM_OK):
      if (a_rx_ctrl[0] == ACK):
        print("Get ACK")
        canbus.receive(a_rx_ctrl, 1)
        print("Then Get {}".format(a_rx_ctrl[0]))
        ack_recpt = 1
      elif (a_rx_ctrl[0] == CA):
        print("Get CA")
        if ((canbus.receive(a_rx_ctrl, 1) == COM_OK) and (a_rx_ctrl[0] == CA)):
          time.sleep( 2 )
          result = COM_ABORT
      else:
        print("Invalid ret code, get {} and {}".format(a_rx_ctrl[0],a_rx_ctrl[1]))
    else:
      print("Error, Rx Timeout")
      errors+=1

    if (errors >= MAX_ERRORS):
      print("Reach Max Error")
      result = COM_ERROR
    

  p_buf_int = 0
  size = file_size



  #Here 1024 bytes length is used to send the packets */
  while ((size) and (result == COM_OK )):
  
    #Prepare next packet */
    print("prepare data packet")
    PreparePacket(p_buf[p_buf_int:], aPacketData, blk_number, size)
    ack_recpt = 0
    a_rx_ctrl[0] = 0
    errors = 0

    #Resend packet if NAK for few times else end of communication */
    while (( ack_recpt==0 ) and ( result == COM_OK )):
    
      #Send next packet */
      if (size >= PACKET_1K_SIZE):
      
        pkt_size = PACKET_1K_SIZE
      
      else:
      
        pkt_size = PACKET_SIZE
      
      #before sending every packet, clear the rx buffer in case of multiple 'c'
      canbus.clearRxBuffer()

      print("sending data packet")
      canbus.transmit(aPacketData[PACKET_START_INDEX:], pkt_size + PACKET_HEADER_SIZE)
      
      #Send CRC or Check Sum based on CRC16_F */

      temp_crc = Cal_CRC16(aPacketData[PACKET_DATA_INDEX:], pkt_size)
      canbus.transmit([temp_crc >> 8],1)
      canbus.transmit([temp_crc & 0xFF],1)
      print("Sending Data CRC = {}, high={}, low={}, errors={}".format(temp_crc,temp_crc >> 8,temp_crc & 0xFF,errors))

      #Wait for Ack */
      if (canbus.receive(a_rx_ctrl, 1) == COM_OK):
          
          if (a_rx_ctrl[0] == ACK):
            print("get ACK")
            ack_recpt = 1
            if (size > pkt_size):
              p_buf_int += pkt_size
              size -= pkt_size
              if (blk_number == (USER_FLASH_SIZE / PACKET_1K_SIZE)):
                result = COM_LIMIT #boundary error */
              else:
                blk_number+=1
            else:
              p_buf_int += pkt_size
              size = 0
          elif(a_rx_ctrl[0] == CA):
              print("Get CA")
              if ((canbus.receive(a_rx_ctrl, 1) == COM_OK) and (a_rx_ctrl[0] == CA)):
                time.sleep( 2 )
                result = COM_ABORT
          else:
              print("Get invaid ret code {}".format(a_rx_ctrl[0]))
      else:
        print("Error in trans dat, get {}".format(a_rx_ctrl[0]))
        errors+=1
      #Resend packet if NAK  for a count of 10 else end of communication */
      if (errors >= MAX_ERRORS):
        print("Reach Max Error")
        result = COM_ERROR
    
  #Sending End Of Transmission char */
  ack_recpt = 0
  a_rx_ctrl[0] = 0x00
  errors = 0
  while (( ack_recpt==0 ) and ( result == COM_OK )):
    print("Sending EOT")
    canbus.transmit([EOT],1)

    #Wait for Ack */
    if (canbus.receive(a_rx_ctrl, 1) == COM_OK):
      
      if (a_rx_ctrl[0] == ACK):
        print("Get EOT ACK")
        ack_recpt = 1
      elif (a_rx_ctrl[0] == CA):
        print("Get EOT CA")
        if ((canbus.receive(a_rx_ctrl, 1) == COM_OK) and (a_rx_ctrl[0] == CA)):
          time.sleep( 2 )
          result = COM_ABORT
    else:
      print("Error EOT")
      errors+=1
    if (errors >=  MAX_ERRORS):
      print("Reach Max Error EOT")
      result = COM_ERROR

  #Empty packet sent - some terminal emulators need this to close session */
  if ( result == COM_OK ):
  
    #Preparing an empty packet */
    aPacketData[PACKET_START_INDEX] = SOH
    aPacketData[PACKET_NUMBER_INDEX] = 0
    aPacketData[PACKET_CNUMBER_INDEX] = 0xFF

    for i in range(PACKET_DATA_INDEX , PACKET_SIZE + PACKET_DATA_INDEX):
      aPacketData[i] = 0x00

    #Send Packet */
    canbus.transmit(aPacketData[PACKET_START_INDEX:], PACKET_SIZE + PACKET_HEADER_SIZE)

    #Send CRC or Check Sum based on CRC16_F */  
    temp_crc = Cal_CRC16(aPacketData[PACKET_DATA_INDEX:], PACKET_SIZE)
    canbus.transmit([temp_crc >> 8],1)
    canbus.transmit([temp_crc & 0xFF],1)


    #Wait for Ack and 'C' */
    if (canbus.receive(a_rx_ctrl, 1) == COM_OK):
         if (a_rx_ctrl[0] == CA):
             time.sleep( 2 )
             result = COM_ABORT

  return result #file transmitted successfully */



def run():
  canbus.open(0)
  canbus.open(1)
  canbus.start_keyboard()
  canbus.start_receiving(0)

  binPath="hydrosoft_IMU_V3.bin"

  if(canbus.getInput()=='1'):
    time.sleep(0.1)
    Ymodem_Transmit(binPath)

run()