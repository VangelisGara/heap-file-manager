## Heap file manager
Implementing a heap file manager, that will manage block level records for heap files.

The heap file manager will manage entries in the following format:
        
    15,​ ​“Marianna”,​ ​“Rezkalla”,​ ​“Hong​ ​Kong”
    4,​ ​“Christoforos”,​ ​“Svingos”,​ ​“Athens”
    300,​ ​“Sofia”,​ ​“Karvounari”,​ ​“San​ ​Francisco”
and will use block level functions to store, scan, print and get entries on heap files.

It also uses LRU/MRU for the iterative read and write of data.

The heap files block follow the following logic:
![Block Logic](https://github.com/VangelisGara/heap-file-manager/blob/master/images/Screenshot_20190808_010640.png)
 Each block can store 1KB, while the record size is 60 bytes, so each block contains 17 records.

## Compile & Execute

    make hp/bf
    ./build/runner

