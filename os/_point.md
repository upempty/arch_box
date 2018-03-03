1. print debug info in makefile(test.mak)  
make -f test.mak 
```
var=1  
$(info echo abc-$(var))  
test:  
  @echo "a"
```

2. refresh  
sleep()  
clear()  
printf for map  
  
...  
sleep()  
clear()  
printf for map  
...  

### vimgrep  
```  
:vimgrep /pattern/ **
:cn---next
:cp---pre
:copen(:cw)
:cclose
:res +5    ---to resize window
CTRL + o   ---return old position
CTRL + i   ---forward the position
:Ex folder ---list folder
:only      ---current window as only window to show full screen, hidden others windows
CTRL + w, release , then _      --max curr window size
CTRL + w, release , then =      --equal size up and down window
```  
### hidden/extend code seg:  
zf  
zo  
## color schem
:colorschem default  
:colorschem morning  
## vim+ctags+cscope  
**ctags -R**    
  ctrl + ]  
  ctrl + t  
  
**cscope -Rbq**    
export CSCOPE_DB=/..../cscope.out  
:cs add $CSCOPE_DB   
  
//in parent directory of cscope.out  
//:cs find c fun1   
:cs find s fun1   
:cs find f file  
:cs find t sring(slow...)  
vim -t xxx(tag)  
:ts <tag>  
:tn  
:tp  
:ts  


*cscope setting in ~/.vimrc*   
```
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" cscope setting
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
if has("cscope")
  set csprg=/usr/bin/cscope
  set csto=1
  set cst
  set nocsverb
  " add any database in current directory
  if filereadable("cscope.out")
      cs add cscope.out
  endif
  set csverb
endif
```

## tmux  
tmux 
ctrl + b + %
ctrl + b + ->(arrow)



### Vagrant/VM:
centos image: ???

### Vagrant image recovery  
1. find used vbox uuid via C:\Users\f1cheng\VirtualBox VMs\vag_default_1437448740438_26687's vag_default_1437448740438_26687.vbox's uuid. 
```  
box-disk1.vmdk stored the data in C:\Users\f1cheng\VirtualBox VMs\vag_default_1437448740438_26687\
```  
  
2. update this uuid in below files which files is in vagrant up folder:D:\virtualBox\vag\  
```  
D:\virtualBox\vag\.vagrant\machines\default\virtualbox\action_provision
D:\virtualBox\vag\.vagrant\machines\default\virtualbox\id
```  
