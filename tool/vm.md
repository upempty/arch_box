
### Vagrant/VM:
centos image: https://atlas.hashicorp.com/jhcook/boxes/centos7/versions/3.10.0.327.13.1/providers/virtualbox.box

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


