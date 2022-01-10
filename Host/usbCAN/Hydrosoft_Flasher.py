
from os import EX_CANTCREAT
from YmodemFlasher import YMODEM_FLASHER
import time


firmwarePath="hydrosoft_IMU_V3.bin"


class HYDROSOFT_FLASHER:
  def __init__(self,chn=0) -> None:
    self.nodeIDList=[0,1,2,3]
    self.targetNodeID = 0
    self.flasher = YMODEM_FLASHER(chn)
    self.RxAllNode()

  def RxOneNode(self,nodeID):
    self.targetNodeID = nodeID
    self.flasher.open(self.flasher.canbus.RX_FILTER_TYPE_ONE,self.targetNodeID)
    print("[INFO]Open CAN channel {}".format(self.flasher.chn),end="")
    print("Rx Accept Only CAN ID = {}".format(self.targetNodeID))

  def RxAllNode(self):
    self.flasher.open(self.flasher.canbus.RX_FILTER_TYPE_ALL,0)
    print("[INFO]Open CAN channel {}".format(self.flasher.chn),end="")
    print("Rx Accept All")

  def close(self):
    self.flasher.close()

  def run(self):
    try:
      while(True):
        print("\n[INFO]===== CAN terminal ======")
        print(" Command: h ----------- Hold\n")
        print(" Command: f n---------- Flash node n\n")
        print(" Command: g ----------- Go to App")
        print("")
        print("[INFO]Waiting for keyboard input command:")
        inputStr=self.flasher.getInput()

        if(inputStr[0]=='g'):
          print("[INFO]Get Command 'g'")
          self.flasher.GoToApp()

        elif(inputStr[0]=='f'):
          print("[INFO]Get Command 'f'")
          if(len(inputStr)>1):
            self.targetNodeID=int(inputStr[1:])
          self.RxOneNode(self.targetNodeID)
          self.flasher.Flash(self.targetNodeID,firmwarePath)

        elif(inputStr[0]=='h'):
          print("[INFO]Get Command 'h'")
          self.flasher.HoldAll()

        time.sleep(1)
        #self.flasher.canbus.clearRxBuffer()
        
    except KeyboardInterrupt:
      self.close()


        
   