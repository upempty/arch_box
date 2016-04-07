
1. Testing the function which used internal function
   mock the internal function and using macro to distinguish the production code and mock function code.
   - [test.cpp file]
   #define TESTING
   
   - [production code]
   #ifndef TESTING
   internal function
   #endif
   
   
  
