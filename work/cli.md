
##cli based cliff framework
1. it needs intractive with running configuration server  
2. structure: Yang module.
3. command: set a-b-c field1 value1 field2 value2
4. generation setup  
> setup generation: parse all entries, and pass to jinja template  
> command class generation: contruct class name to jinja template  
5. cli -q  

6. feature  
<p>auto complete function:</p>
class InteractiveCmd(InteractiveApp, object):  
   def completedefault(self, text, line, start_index, end_index):
   \#invoked when complete_<commandname> -method is not defined
