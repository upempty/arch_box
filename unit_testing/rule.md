
1. Unit Testing based on function's requirement, black box testing, as inside function, 
code style: if/while entries are different by programers.  
2. Testing the function which uses internal functions  
   mock the internal function and using macro to distinguish the production code and mock function code.
   - test.cpp file
   ```
    #define TESTING
   ```
   
   - production code
   ```
   #ifndef TESTING
   internal function
   #endif
   ```
   
   
  
