
##cli based cliff framework
it needs intractive with running configuration server  
structure: Yang module.
command: set a-b-c field1 value1 field2 value2

setup generation: parse all entries, and pass to jinja template
command class generation: contruct class name to jinja template
cli -q

auto complete function:
class InteractiveCmd(InteractiveApp, object):
   def completedefault(self, text, line, start_index, end_index):
   #invoked when complete_<commandname> -method is not defined
