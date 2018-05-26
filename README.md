# Simple Linux memory hack example

This project shows a basic example on how to hack the memory of a Linux process.

## Building the project

```
mkdir build
cd build
cmake ..
make
```

## Running the code

In first place, run the executable example to hack:
```
./stringloop
```
The process shows the heap address of the string which will be hacked.

Then run with *root permissions* the hack executable:
```
sudo ./hack
```
**sudo** is mandatory to have read/write access to `/proc/<pid>/mem` file.

The executable will find the pid alone by looking through the `/proc` directory, then finding the address of the string to hack.

### Example

Terminal 1:
```
./stringloop
0 - Change me please (0x556c0936d260)
1 - Change me please (0x556c0936d260)
2 - Change me please (0x556c0936d260)
3 - Change me please (0x556c0936d260)
4 - It is true magic (0x556c0936d260)
```

Terminal 2:
```
sudo ./hack
Process 5445 found: ./stringloop
Heap address: 0x556c0936d000
String address: 0x556c0936d260
Writing operation successful :)
```
