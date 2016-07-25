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
export $CSCOPE_DB=/..../cscope.out  
:cs add $CSCOPE_DB  
:cs find c fun1   
:cs find s fun1   

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

