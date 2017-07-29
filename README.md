IDBTOOL
=======

A tool for extracting information from IDA databases.
`idbtool` knows how to handle databases from all IDA versions since v2.0, both `i64` and `idb` files.
You can also use `idbtool` to recover information from unclosed databases.

`idbtool` works without change with IDA v7.0.


Much faster than loading a file in IDA
--------------------------------------

With idbtool you can search thousands of .idb files in seconds.

More precisely: on my laptop it takes:

 *  1.5 seonds to extract 143 idc scripts from 119 idb and i64 files.
 *  3.8 seonds to print idb info for 441 files.
 *  5.6 seconds to extract 281 enums containing 4726 members from 35 files.
 * 67.8 seconds to extract 5942 structs containing 33672 members from 265 files.

Loading a approximately 5 Gbyte idb file in IDA, takes about 45 minutes.
While idb3.h takes basically no time at all, no more than a few milliseconds.


Download
========

Two versions of this tool exist:

One written in python
 * https://github.com/nlitsme/pyidbutil

One written in C++
 * https://github.com/nlitsme/idbutil

Both repositories contain a library which can be used for reading `.idb` or `.i64` files.


An IDApro plugin making use of `idb3.h` can be found here:
 * https://github.com/nlitsme/idbimport

This is a plugin making it easy to copy scripts, structs or enums from recent ida databases.


Usage
=====

Usage: 

    idbtool [options] [database file(s)] [-- address-list]

 * `-n` or `--names`  will list all named values in the database.
 * `-s` or `--scripts` will list all scripts stored in the database.
 * `-u` or `--structs` will list all structs stored in the database.
 * `-e` or `--enums` will list all enums stored in the database.
 * `-i` or `--info` will print some general info about the database. 

 * `-a`  list all names, including ..todo..
 * `-d`  dump btree page tree contents.
 * `--inc`, `--dec` list all records in ascending / descending order.
 * `-q` or `--query` search specific records in the database.
 * `-m` or `--limit` limit the number of results returned by `-q`.

All addresses after `--` will be printed as `symbol+offset`.

query
-----

Queries need to be specified last on the commandline.

example:

    idbtool [database file(s)]  --query  "Root Node;V"

Will list the source binary for all the databases specified on the commandline.

A query is a string with the following format:

 * [==,<=,>=,<,>]  - optional relation, default: ==
 * a base node key:
    * a DOT followed by the numeric value of the nodeid.
    * a HASH followed by the numeric value of the system-nodeid.
    * a QUESTION followed by the name of the node. -> a 'N'ame node
    * the name of the node.  -> the name is resolved, results in a '.'Dot node
 * an optional tag ( A for Alt, S for Supval, etc )
 * an optional index value

example queries:
 * `Root Node;V` -> prints record containing the source binary name
 * `?Root Node` -> prints the Name record pointing to the root
 * `>Root Node` -> prints the first 10 records starting with the root node id.
 * `<Root Node` -> prints the 10 records startng with the recordsbefore the rootnode.
 * `.0xff000001;N` -> prints the rootnode name entry.
 * `#1;N` -> prints the rootnode name entry.

List the highest node and following record in the database in two different ways,
the first: starting at the first record below `ffc00000`, and listing the next.
The second: starting at the first record after `ffc00000`, and listing the previous:
 * `--query "<#0xc00000"  --limit 2 --inc -v`
 * `--query ">#0xc00000"  --limit 2 --dec -v`

Note that this should be the nodeid in the `$ MAX NODE` record.

List the last two records:
 * `--limit 2 --dec  -v`

List the first two records, the `$ MAX LINK` and `$ MAX NODE` records:
 * `--limit 2 --inc -v`


A full database dump
--------------------

Several methods exist for printing all records in the database. This may be useful if
you want to investigate more of IDA''s internals. But can also be useful in recovering
data from corrupted databases.

 * `--inc`, `--dec` can be used to enumerate all b-tree records in either forward, or backward direction.
 * `--id0`  walks the page tree, instead of the b-tree, printing the contents of each page


LIBRARY
=======

The headerfile `idb3.h` contains a library for reading from IDApro databases.


## IDBFile

Class for accessing sections of an `.idb` or `.i64` file.

Constructor Parameters:
 * `std::shared_ptr<std::istream>` ( typedefed to `stream_ptr` )

Methods:
 * `stream_ptr getsection(int)`

 

## ID0File, ID1File, NAMFile

Constructor Parameters:
 * `IDBFile& idb`
 * `stream_ptr`

Constant
 * `INDEX`  - the argument for `idb.getsection`

## ID0File

methods
 * `Cursor find(relation_t, nodeid, ...)` 
    * `...`  can be: 
       * tag, index
       * tag, hash
       * tag
 * `Cursor find(relation_t, std::string key)`
 * `std::string blob(nodeid, tag, ...)`
 * `uint64_t node(std::string name)`

 * `bool is64bit()`
    * `true` for `.i64` files.

 * `uint64_t nodebase()`
    * return `0xFF000000(00000000)` for 32/64 bit databases.

 * `void enumlist(uint64_t nodeid, char tag, CB cb)`
    * call `cb` for each value in the list.

convenience methods
 * `std::string getdata(ARGS...args)`
 * `std::string getstr(ARGS...args)`
 * `uint64_t getuint(ARGS...args)`
 * `uint64_t getuint(BtreeBase::Cursor& c)`
 * `std::string getname(uint64_t node)`



## ID1File

methods
 * `uint32_t GetFlags(uint64_t ea)`


## NAMFile

methods
 * `uint64_t findname(uint64_t ea)`


## Cursor

methods
 * `void next()`
    * move cursor to the next btree record
 * `void prev()`
    * move cursor to the previous btree record
 * `bool eof()`
    * did we reach the start/end of the btree?
 * `std::string `getkey()`
    * return the key pointed to by the cursor
 * `std::string `getval()`
    * return the value pointed to by the cursor

TODe
====

 * add option to list all comments stored in the database
 * support compressed sections

Author
======

Willem Hengeveld <itsme@xs4all.nl>

