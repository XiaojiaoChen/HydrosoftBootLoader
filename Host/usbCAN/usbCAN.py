from ctypes import *
import threading
import queue
import time


COM_OK       = 0
COM_ERROR    = 1
COM_ABORT    = 2
COM_TIMEOUT  = 3

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




class USB_CAN:
    def __init__(self, CAN_ID=0x0400, baud=1000000):

        self.VCI_USBCAN2 = 4
        self.STATUS_OK = 1
        self.canDLL = cdll.LoadLibrary('./libcontrolcan.so')

        self.CAN_ID = CAN_ID
        self.channelStatus = [0, 0]


        self.receiving_alive=0
        self.keyboard_alive=0
        self.threads=[]
        self.kbdQueue = queue.Queue()

        self.rxQueue = queue.Queue()
        self.rxTimeout=20

        self.latestRxData=bytearray()
        self.latestRxDataLen=0

        rx_vci_can_obj_type = VCI_CAN_OBJ*2500
        self.rx_vci_can_obj = rx_vci_can_obj_type()

        self.tx_vci_can_obj = VCI_CAN_OBJ()

        ret = self.canDLL.VCI_OpenDevice(self.VCI_USBCAN2, 0, 0)

        if ret == self.STATUS_OK:
            print('Open USB_CAN Device successful')
            ret = self.canDLL.VCI_ClearBuffer(self.VCI_USBCAN2, 0, 0)
            ret = self.canDLL.VCI_ClearBuffer(self.VCI_USBCAN2, 0, 1)
        else:
            print('Open USB_CAN Device failed')



    def open(self, chn=0):
        #Init and Start CAN channel
        self.vci_initconfig = VCI_INIT_CONFIG(
            0x80000008, 0xFFFFFFFF, 0, 0, 0x00, 0x14, 0)  # 1M baudrate, 87.5%,  normal mode, all ID acceptable
        ret = self.canDLL.VCI_InitCAN(
            self.VCI_USBCAN2, 0, chn, byref(self.vci_initconfig))
        if ret != self.STATUS_OK:
            print('Init CAN Channel {} fail'.format(chn))
        ret = self.canDLL.VCI_StartCAN(self.VCI_USBCAN2, 0, chn)
        if ret == self.STATUS_OK:
            self.channelStatus[chn] = 1
            ret = self.canDLL.VCI_ClearBuffer(self.VCI_USBCAN2, 0, chn)
        else:
            print('Start CAN Channel {} fail'.format(chn))
        return ret

    def setTxID(self,canID=0x0400):
        self.CAN_ID = canID

    def start_keyboard(self):
        self.keyboard_alive = 1
        print('Ready for keyboard input:')
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
                    #customize terminal input action here

            except:
                print("quit keyboard thread")
                break



   

    def start_receiving(self, chn=0):
        self.receiving_alive=1
        print("start receiving Can info:")
        self.receivingThread = threading.Thread(target=self.receiving_thread,args=(chn,))
        self.threads.append(self.receivingThread)
        self.receivingThread.start()

    def receiving_thread(self, chn=0):
        if(self.channelStatus[chn] == 1):
            while(self.receiving_alive):
                rxNB=0
                #Show Rx
                while rxNB <= 0 and self.receiving_alive:
                    rxNB = self.canDLL.VCI_Receive(self.VCI_USBCAN2, 0, chn, byref(self.rx_vci_can_obj), 2500, 0)
                    #print("Wait Rx")
                    # aa+=1
                for i in range (rxNB):
                    #customize receiving action here
                    #print("CAN channel {} Receive ID:{}, Len:{}, Data: {} ".format(chn,self.rx_vci_can_obj[i].ID,self.rx_vci_can_obj[i].DataLen,list(self.rx_vci_can_obj[i].Data)))
                    dlen=self.rx_vci_can_obj[i].DataLen
                    #print("Rx dalaLen={}".format(dlen))
                    dlc=bytearray(self.rx_vci_can_obj[i].Data[:dlen])
                    for dlctata in dlc:
                        self.rxQueue.put(dlctata)
                    print(dlc.decode('iso-8859-1'), end='', flush=True)
                    #print("dlc={}".format(dlc))
                rxNB=0
        else:
            print("Rx Channel {} Not opened".format(chn))

    def transmit(self,pdata,num):
        ret=COM_OK
        frameNB=num//8
        remBytesNB=num%8
        pdataInd=0
        for i in range(frameNB):
            if(ret==COM_OK):
                ret = self.transmit_Frame(pdata[pdataInd:],8)
                pdataInd+=8
                time.sleep(0.0001)
        if(ret==COM_OK and remBytesNB!=0):
            ret = self.transmit_Frame(pdata[pdataInd:],remBytesNB)
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
            else:
                print("Tx Error, Channel {} Not opened".format(chn))
            return ret
        except:
            print("Tx Frame timeout")
            return COM_TIMEOUT

    def clearRxBuffer(self):
        self.canDLL.VCI_ClearBuffer(self.VCI_USBCAN2, 0, 0)
        self.canDLL.VCI_ClearBuffer(self.VCI_USBCAN2, 0, 1)
        while(self.rxQueue.qsize()!=0):
            self.rxQueue.get()


    def receive(self,pdata,num,chn=0):
        dataInd=0
        tstart=time.time()
        while(dataInd<num):
            if(self.rxQueue.qsize()!=0):
                pdata[dataInd]=self.rxQueue.get()
                dataInd+=1
                if(dataInd==num):
                    return COM_OK
            if(time.time()-tstart>self.rxTimeout):
                return COM_TIMEOUT




    def close(self):
        ret = self.canDLL.VCI_CloseDevice(self.VCI_USBCAN2, 0)
        print("CAN device closed")
        self.receiving_alive=0
        self.keyboard_alive=0
        for threadOb in self.threads:
            threadOb.join(1)
        
if __name__ == "__main__":
    usbcan = USB_CAN()
    usbcan.open(0)
    usbcan.open(1)
    try:
        usbcan.start_receiving(1)
        usbcan.start_keyboard()
    except:
        usbcan.close()
   