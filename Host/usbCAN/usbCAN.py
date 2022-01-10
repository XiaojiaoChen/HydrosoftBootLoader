from ctypes import *
from os import EX_CANTCREAT
import threading
import queue
import time


COM_OK       = 0
COM_ERROR    = 1
COM_ABORT    = 2
COM_TIMEOUT  = 3


MASTER_BROADCAST=0x07FF
MASTER_P2P_MASK =0x0400

RX_FILTER_MASK_ALL=0xFFFFFFFF

RX_FILTER_MASK_ONE=0x00000000




class VCI_INIT_CONFIG(Structure):
    _fields_ = [("AccCode", c_uint),
                ("AccMask", c_uint),
                ("Reserved", c_uint),
                ("Filter", c_ubyte),
                ("Timing0", c_ubyte),
                ("Timing1", c_ubyte),
                ("Mode", c_ubyte)
                ]


class VCI_CAN_OBJ(Structure):
    _fields_ = [("ID", c_uint),
                ("TimeStamp", c_uint),
                ("TimeFlag", c_ubyte),
                ("SendType", c_ubyte),
                ("RemoteFlag", c_ubyte),
                ("ExternFlag", c_ubyte),
                ("DataLen", c_ubyte),
                ("Data", c_ubyte*8),
                ("Reserved", c_ubyte*3)
                ]



#two channels could be used simultaneously
class USB_CAN:
    def __init__(self, CAN_ID=MASTER_BROADCAST, baud=1000000):



        self.VCI_USBCAN2 = 4
        self.STATUS_OK = 1
        self.canDLL = cdll.LoadLibrary('./libcontrolcan.so')

        self.CAN_ID = CAN_ID
        self.channelStatus = [0, 0]

        self.RX_FILTER_TYPE_ALL = 0
        self.RX_FILTER_TYPE_ONE = 1
        self.RxFilType = self.RX_FILTER_TYPE_ALL
        self.RxNodeID = 0

        self.receiving_alive=0
        self.keyboard_alive=0
        self.threads=[]
        self.kbdQueue = queue.Queue()

        self.rxQueue = queue.Queue()
        self.rxTimeout=200

        self.latestRxData=bytearray()
        self.latestRxDataLen=0

        rx_vci_can_obj_type = VCI_CAN_OBJ*2500
        self.rx_vci_can_obj = rx_vci_can_obj_type()

        self.tx_vci_can_obj = VCI_CAN_OBJ()

        ret = self.canDLL.VCI_OpenDevice(self.VCI_USBCAN2, 0, 0)

        if ret == self.STATUS_OK:
            print('[INFO]Open USB_CAN Device successful')
            ret = self.canDLL.VCI_ClearBuffer(self.VCI_USBCAN2, 0, 0)
            ret = self.canDLL.VCI_ClearBuffer(self.VCI_USBCAN2, 0, 1)
        else:
            print('[INFO]Open USB_CAN Device failed')



    def open(self, chn=0,filterType=0,rxID=0):

        self.RxNodeID = rxID
        self.RxFilType = filterType

        #determine filter parameters from filter Type and nodeID
        filterAcc = self.RxNodeID<<21
        if(self.RxFilType == 0):
            filterMask = RX_FILTER_MASK_ALL
        else:
            filterMask = RX_FILTER_MASK_ONE

        #Init CAN channel
        self.vci_initconfig = VCI_INIT_CONFIG(
            filterAcc, filterMask, 0, 0, 0x00, 0x14, 0)  # 1M baudrate, 87.5%,  normal mode, all ID acceptable
        ret = self.canDLL.VCI_InitCAN(
            self.VCI_USBCAN2, 0, chn, byref(self.vci_initconfig))
        if ret != self.STATUS_OK:
            print('[INFO]Init CAN Channel {} fail'.format(chn))

        #Start CAN channel
        ret = self.canDLL.VCI_StartCAN(self.VCI_USBCAN2, 0, chn)
        if ret == self.STATUS_OK:
            self.channelStatus[chn] = 1
            ret = self.canDLL.VCI_ClearBuffer(self.VCI_USBCAN2, 0, chn)
        else:
            print('[INFO]Start CAN Channel {} fail'.format(chn))
        return ret

    def setCANID(self,canID=MASTER_BROADCAST):
        self.CAN_ID = canID

    def start_keyboard(self):
        self.keyboard_alive = 1
        # print("[INFO]Keyboard Input Enabled")
        self.keyboardThread = threading.Thread(target=self.keyboard_thread)
        self.threads.append(self.keyboardThread)
        self.keyboardThread.start()
        
    def getInput(self):
        return self.kbdQueue.get()

    def keyboard_thread(self):
        while(self.keyboard_alive==1):
            try:
                input_str = input()
                if (len(input_str) > 0):
                    self.kbdQueue.put(input_str)
            except:
                print("[INFO]quit keyboard thread")
                break

    def start_receiving(self, chn=0):
        self.receiving_alive=1
        # print("[INFO]Receiving thread started")
        self.receivingThread = threading.Thread(target=self.receiving_thread,args=(chn,))
        self.threads.append(self.receivingThread)
        self.receivingThread.start()

    def receiving_thread(self, chn=0):
        if(self.channelStatus[chn] == 1):
            while(self.receiving_alive):
                rxNB=0
                while rxNB <= 0 and self.receiving_alive:
                    rxNB = self.canDLL.VCI_Receive(self.VCI_USBCAN2, 0, chn, byref(self.rx_vci_can_obj), 2500, 0)
                for i in range (rxNB):
                    rxid=self.rx_vci_can_obj[i].ID
                    # if(self.RxFilType==self.RX_FILTER_TYPE_ALL or (self.RxFilType==self.RX_FILTER_TYPE_ONE and rxid == self.RxNodeID)):
                    dlen=self.rx_vci_can_obj[i].DataLen
                    dlc=bytearray(self.rx_vci_can_obj[i].Data[:dlen])
                    for dlctata in dlc:
                        self.rxQueue.put(dlctata)
                    print(dlc.decode('iso-8859-1'), end='', flush=True)

                rxNB=0
        else:
            print("[INFO]Rx Channel {} Not opened".format(chn))

    def transmit(self,pdata,num,chn=0):
        ret=COM_OK
        frameNB=num//8
        remBytesNB=num%8
        pdataInd=0
        for i in range(frameNB):
            if(ret==COM_OK):
                ret = self.transmit_Frame(pdata[pdataInd:],8,chn)
                pdataInd+=8
                time.sleep(0.0001)
        if(ret==COM_OK and remBytesNB!=0):
            ret = self.transmit_Frame(pdata[pdataInd:],remBytesNB,chn)
        return ret

    def transmit_Frame(self, frameData, dalalen, chn=0):
        try:
            ret = COM_OK
            if(self.channelStatus[chn] == 1):
                self.tx_vci_can_obj.ID=self.CAN_ID
                self.tx_vci_can_obj.SendType=1
                self.tx_vci_can_obj.DataLen=dalalen
                for i in range(dalalen):
                    self.tx_vci_can_obj.Data[i]=frameData[i]
                if(self.canDLL.VCI_Transmit(self.VCI_USBCAN2, 0, chn, byref(self.tx_vci_can_obj), 1)!=1):
                    ret=COM_ERROR
                # else:
                #     print("[INFO]Tx frame ID:0x{:x}, Len {}".format(self.tx_vci_can_obj.ID,self.tx_vci_can_obj.DataLen))
            else:
                print("[INFO]Tx Error, Channel {} Not opened".format(chn))
            return ret
        except:
            print("[INFO]Tx Frame timeout")
            return COM_TIMEOUT

    def clearRxBuffer(self):
        self.canDLL.VCI_ClearBuffer(self.VCI_USBCAN2, 0, 0)
        self.canDLL.VCI_ClearBuffer(self.VCI_USBCAN2, 0, 1)
        while(self.rxQueue.qsize()!=0):
            self.rxQueue.get()


    def receive(self,pdata,num,chn=0):
        dataInd=0
        tstart=time.time()
        try:
            while(dataInd<num):
                if(self.rxQueue.qsize()!=0):
                    pdata[dataInd]=self.rxQueue.get()
                    dataInd+=1
                    if(dataInd==num):
                        return COM_OK
                if(time.time()-tstart>self.rxTimeout):
                    return COM_TIMEOUT
        except:
            return COM_ERROR



    def close(self):
        ret = self.canDLL.VCI_CloseDevice(self.VCI_USBCAN2, 0)
        print("[INFO]CAN device closed")
        self.receiving_alive=0
        self.keyboard_alive=0
        for threadOb in self.threads:
            threadOb.join()
        
if __name__ == "__main__":
    print("[INFO]This is a USB CAN Test program")
    usbcan = USB_CAN()
    usbcan.open(0)
    usbcan.open(1)
    try:
        usbcan.start_receiving(0)
        usbcan.start_keyboard()
    except:
        usbcan.close()
   