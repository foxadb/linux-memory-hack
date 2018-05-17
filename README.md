# Process memory hack

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
The process shows the heap address of the string to hack.

Then run with *root permissions* the hack executable:
```
sudo ./hack <string memory address>
```
**sudo** is mandatory to have read/write access to `/proc/<pid>/mem` file.

The executable will find the pid alone by looking through the `/proc` directory.

### Example

Terminal 1:
```
./stringloop
0 - Change me please (0x557e0d999260)
1 - Change me please (0x557e0d999260)
2 - Change me please (0x557e0d999260)
3 - Change me please (0x557e0d999260)
4 - Change me please (0x557e0d999260)
5 - It is true magic (0x557e0d999260)
```

Terminal 2:
```
sudo ./hack 0x557e0d999260
Process 13971 found: ./stringloop
Open memfile: /proc/13971/mem
String value read at 0x557e0d999260: Change me please
Change it to: It is true magic
```