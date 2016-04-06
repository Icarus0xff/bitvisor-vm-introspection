#!/usr/bin/python3

from tkinter import *
from tkinter import simpledialog as sdg
import subprocess
import threading
import time
import queue

vmm_conf = {'vmm.driver.usb.ehci': 0, 'vmm.driver.ata': 1, 'vmm.driver.usb.uhci': 1, 'vmm.status': 1, 'vmm.f11panic': 0, 'vmm.boot_active': 1, 'vmm.auto_reboot': 1, 'vmm.shell': 0, 'vmm.f12msg': 1, 'vmm.no_intr_intercept': 1, 'vmm.driver.concealEHCI': 1, 'vmm.ignore_tsc_invariant': 0, 'vmm.dbgsh': 1, }

ahci_conf = {'storage.conf1.type':'AHCI', 'storage.conf1.host_id':'-1', 'storage.conf1.device_id':'-1', 'storage.conf1.lba_low':'0', 'storage.conf1.lba_high':'0x7FFFFFFF', 'storage.conf1.keyindex':'0', 
'storage.conf1.crypto_name':'aes-xts', 'storage.conf1.keybits':'256', }

sys_info = {'CPU Name: ':'Null', 'CPU Vendor: ':'Null', 'VT-x: ':'Null'}

grid_row = 1


log = queue.Queue(10)
log.put('Welcome!')



class AhciEntry(Entry):
    def __init__(self, master, name):
        Entry.__init__(self, master)
        self.name = name
        

class Dialog_Ahci(sdg.Dialog):
    
    def body(self, master):
        self.entrys = []
        i = 0
        for (k,v) in ahci_conf.items():
            Label(master, text=k).grid(row=i,column = 1,sticky = 'W')
            e = AhciEntry(master,k)
            e.delete(0, END)
            e.insert(0,v)
            self.entrys.append(e)
            self.entrys[i].grid(row=i, column=2, sticky = 'W')
            i+=1
        return
        
    def apply(self):
        for e in self.entrys:
            ahci_conf[e.name] = e.get()
        
        return 

        
class Checkbox(object):
    status = 0
    text = 0
    onvalue = 1
    offvalue = 0
    var = None

    def __init__(self, root, status, text, var):
        self.status = status
        self.text = text
        self.var = var
        c = Checkbutton(root, text=self.text, onvalue=self.onvalue, offvalue=self.offvalue, variable=self.var, command=self.action)
        if status == 1:
            self.status = 1
            c.select()
        global grid_row
        c.grid(sticky = W)
        grid_row+=1

    def action(self):
        print(self.var)
        if(self.status == 0):
            self.status = 1
        else:
            self.status = 0
        vmm_conf[self.var] = self.status

class Menulist(object):
    def __init__(self, root):
        self.root = root
        menubar = Menu(root)
        #创建下拉菜单File，然后将其加入到顶级的菜单栏中
        filemenu = Menu(menubar,tearoff=0)
        filemenu.add_command(label="Make", command=self.make)
        filemenu.add_command(label="Installation", command=self.install)
        filemenu.add_command(label="Uninstall", command=self.uninstall)
        filemenu.add_command(label="Save", command=self.action_save)
        filemenu.add_separator()
        filemenu.add_command(label="Exit", command=root.quit)
        menubar.add_cascade(label="File", menu=filemenu)
        ## tools menu
        toolsmenu = Menu(menubar,tearoff=0)
        toolsmenu.add_command(label="USB config")
        toolsmenu.add_command(label="AHCI config", command = self.action_ahci)
        menubar.add_cascade(label="Tools", menu=toolsmenu)
        ##help menu
        helpmenu = Menu(menubar,tearoff=0)
        helpmenu.add_command(label="Help")
        helpmenu.add_command(label="About")
        menubar.add_cascade(label="Help", menu=helpmenu)
        root.config(menu = menubar)

    def action_ahci(self):
        d = Dialog_Ahci(self.root)


    def make(self):
        ##TODO:Make bitvisor
        path = 'boot/login-simple/'
        cmd = 'sudo make -C' + path
        try:
            out = subprocess.check_output(cmd, shell=True).decode('utf-8')
            print("sucessful")
            log.put("Make sucessful")
        except subprocess.CalledProcessError as e:
            out = e.output
            code = e.returncode
            print("error while making!")
            log.put("Error while making!")
        pass
            
    def action_save(self):
        try:
            output = open('conf.conf','w')
        except FileNotFoundError:
            print("Save file not found!")
            return

        for option in vmm_conf:
            text = option + '=' + str(vmm_conf[option]) + '\n'
            output.write(text)
        for option in ahci_conf:
            text = option + '=' + str(ahci_conf[option]) + '\n'
            output.write(text)
        print("Save complation.")    
        output.close()
    
    def install(self):
        ##TODO: run a shell batch to install bitvisor automatically
        ##grub_name = '/boot/grub/grub.cfg'
        
        grub_name = './test.txt'
        conf_line = 'menuentry "Hypro" {{ \n\
        insmod ext2 \n\
        set root={device} \n\
        multiboot /boot/bitvisor.elf \n\
        module /boot/{module1} \n\
        module /boot/{module2} \n\
        }}'
        
        cmd = 'sudo -S df -h | awk \'{print $1,$NF}\''
        out = subprocess.check_output(cmd, shell = True).decode('utf-8')
        setroot = '\'hd0,msdos'	
        
        lines = re.split(r'\n', out)
        for line in lines:
            m=re.match('^/dev/(sda([0-9]*)).*/$', line)
            if(m):
                setroot+=m.group(2)+'\''
        try:        
            with open(grub_name, 'r+') as f:
                grub = f.read()
                hypro = re.compile(r'.*Hypro.*',re.DOTALL)
                match = hypro.findall(grub)
                if(match):
                    print('you have installed hypro already!')
                    log.put('You have installed hypro already!')
                    ##insert copy module, bitvisor.elf statement
                    return
                conff = conf_line.format(device=setroot,module1='module1.bin',module2='module2.bin')
                print(conff,file=f)
        except FileNotFoundError:
            print(grub_name+' File not found!')
            log.put(grub_name+' File not found!')
        
        ##pass
    def uninstall(self):
        grub_name = './test.txt'
        try:        
            with open(grub_name, 'r') as f:
                grub = f.read()
            with open(grub_name, 'w') as f:
                r = 'menuentry.*Hypro.*{.*}'
                replace = re.compile(r, re.DOTALL)
                match = replace.findall(grub)
                if(match):
                    grub = re.sub(replace, '', grub)
                    print(grub,file=f)
                else:
                    print('you haven\'t install Hypro')
                    log.put('you haven\'t install Hypro')
              
        except FileNotFoundError:
            print(grub_name+' File not found!')
            log.put('you haven\'t install Hypro')
	
        
        pass

class LogThread(threading.Thread):
    
    def __init__(self, root, threadID, name, counter):
        self.text = Text(root, background = 'black', fg = 'green2')
        self.text.config(state = DISABLED)
        self.text.grid(columnspan=2, sticky = W)
        scroll = Scrollbar(root, orient = 'vertical')
        scroll.grid(column = 2,row = 10,sticky = E)
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.name = name
        self.counter = counter
        self._running = True
        
    def terminate(self):
        self._running = False
        
    def run(self):
        count = 100
        while self._running:
            try:
                str = log.get(timeout=1)+'\n'
                self.text.config(state = NORMAL)
                self.text.insert(INSERT, str)
                self.text.config(state = DISABLED)
            except queue.Empty:
                continue

class Gui(object):
    def __init__(self, root):
        menu = Menulist(root)
        self.read_configuration(root)
        self.put_checkbox(root)
        self.put_cpuinfo_label(root)
        thread1 = LogThread(root, 1, "Thread-1", 1)
        thread1.start()
        infolabel = Label(root, text = 'SystemInfo', font = "Times 10 bold")
        infolabel.grid(column = 1, row = 0, sticky = W)
        mainloop()
        thread1.terminate()
    
    def read_configuration(self, root):
        try:
            file = open("conf.conf")
        except FileNotFoundError:
            print('Configuration File not found')
            log.put('Configuration File not found')
            return
        VMMLable = Label(root,text="VMM Options",font="Times 10 bold")
        VMMLable.grid(column = 0, row = 0, sticky = W)
        
        for i in range(len(vmm_conf)):
            line = file.readline()
            match = re.match(r'(^vmm\.(.*))=([10])',line)
            if(match):
                vmm_conf[match.group(1)] = int(match.group(3))
        file.close()

    def put_cpuinfo_label(self, root):
        r = 1
        for (k,v) in sys_info.items():
            label = Label(root, text=k+v)
            label.grid(column = 1, row=r, sticky = W)
            r+=1
            
    def put_checkbox(self, root):
        for (k,v) in vmm_conf.items():
            if v==1:
                c = Checkbox(root, 1, k.lstrip('vmm.'), k)
            elif v==0:
                c = Checkbox(root, 0, k.lstrip('vmm.'), k)

def main():
    cmd = 'sudo cat /proc/cpuinfo'
    out = subprocess.check_output(cmd, shell = True).decode('utf-8')
    lines = re.split(r'[\n]',out)
    
    r1 = r'.*vendor_id.*:[ ]*(.*)'
    r2 = r'.*model name.*:[ ]*(.*)'
    r3 = r'.*(vmx).*'
    for line in lines:
        regex = re.compile(r1, re.DOTALL)
        match = re.match(regex, line)
        if(match):
            sys_info['CPU Vendor: '] = match.group(1)
            match = None
        regex = re.compile(r2, re.DOTALL)
        match = re.match(regex, line)
        if(match):
            sys_info['CPU Name: '] = match.group(1)
            match = None
        regex = regex = re.compile(r3, re.DOTALL)
        match = re.match(regex, line)
        if(match):
            sys_info['VT-x: '] = match.group(1)
            match = None
    root = Tk()
    g = Gui(root)
    

main()



