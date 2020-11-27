
IDApro databases
==================

An IDApro database consists of one large file which contains several sections.
At the start of the `idb` or `i64` file there is a list of fileoffsets pointing
to these sections.  Sections can optionally be stored compressed.
When a database is opened by IDA the sections are extracted from the main data file
and stored in separate files. When you only need to read from the database, and don't
want to change anything, the splitting into `id0`, `id1`, `nam` and `til` files is not
nescesary, IDApro does this anyway, since it expect the user to make changes to the database.
Very old IDApro versions ( v1.6 and v2.0 ) store the sections separately, such that
there could only be one database per directory.

| index | extension | contents                   |
| :---- | :-------- | :------------------------- |
|   0   |  id0      | A btree key/value database |
|   1   |  id1      | Flags for each byte        |
|   2   |  nam      | A list of named offsets    |
|   3   |  seg      | Unknown                    |
|   4   |  til      | Type information           |
|   5   |  id2      | Unknown                    |

Older ida versions don't have the id2 file.
Newer ida versions don't have the seg file.
Newer ida versions use 64 bit file offsets, so IDA can support files larger than 4GB.
There is no difference in the IDB header between 32 bit ( with `.idb` extension ) and 64 bit ( with `.i64` extension ) databases.

## ID0 section.

The ID0 sections contains a b-tree database, This is a single large key-value database, like leveldb.
There are three main groups of key types:

 * Bookkeeping, so IDApro can quickly decide what the next free nodeid is. These keys all start with a '$' (dollar) sign.
    * `$ MAX LINK`
    * `$ MAX NODE`
    * `$ NET DESC`
 * Nodes, keys starting with a '.' (dot).
    * followed by an address, or internal nodeid.
       * 32 bit databases use 32 bit addresses, 64 bit databases use 64 bit addresses here.
       * internal nodeid's have the upper 8 bits set to one,
         so `0xFF000000` for a 32 bit database, or `0xFF00000000000000` for a 64 bit database.
    * a tag,  `A` for altvals, `S` for supvals, etc. See netnode.hpp in the idasdk.
    * optionally followed by an index or hashkey value, depending on the tag.
    * both the address and index value are encoded in bigendian byte order.
 * Name index, keys starting with an `N`, followed by a name.
   The value being a 32 or 64 bit offset.
    * names up to 511 are encoded as plain strings. longer names start with a NUL byte, followed by a blob index.
      pointing to a blob at special nodeid `0xFF000000(00000000)`.
    * the maximum name length is 32 * 1024 characters.
 * Very old ida versions had keys starting with lowercase 'n', and '-' (minus).
 * The maximum key size if 512 bytes, including dots, 'N', etc.

The range of internal nodeid's is the reason you cannot have code or data in
your disassembly at addresses starting with `0xFF000000(00000000)`. IDA will allow you to
create such segments manually. Doing so will usually result in corrupted databases.

There are two types of names:
 * Internal, pointing to internal nodeid's. Examples: `$ structs`, `Root Node`. Most have a space in them.
 * Labels, pointing to addresses in the disassembly.

The maximum value size is 1024 bytes.
Several types of values:
 * Integers, encoded in little endian byte order.
 * Strings are sometimes NUL terminated, sometimes not.
 * In several cases structured information is stored in a _packed_ format, see below.

### packed values

Packed values are used among others for structure and segment definitions.

In packed data:
 * Values in the range 0x00-0x7f are stored in a single byte.
 * Values in the range 0x80-0x3fff are stored ORRED with 0x8000.
 * Values in the range 0x4000-0x1fffffff are stored ORRED with 0xC000000.
 * Larger 32 bit values are stored prefixed with a 0xFF byte.
 * 64 bit values are stored as two consecutive numbers.
 * All values are stored in big-endian byte order.


### The B-tree format

The file is organised in 8kbyte pages, where the first page contains a header with
pagesize, pointer to a list of free pages, pointer to the root of the page tree,
the number of records, and number of pages.

There are two types of pages, leaf pages, which don't contain pointers to other
pages, but only key-value records. And index pages, with a _preceeding_
pointer, and where all key-value records contain a pointer to a page where all
keys in the pointed-to page have values greater than the key containing the
page pointer.  This makes it very efficient to lookup records by key.

The page tree looks like this. Between brackets are key values, the pointer marked
with a `*` (STAR) is the _preceeding_ pointer. Values are not shown.


                       *-------->[00]
             *------>[02]---+    [01]
    root ->[08]---+  [05]-+ |    
           [17]-+ |       | +--->[03]
                | |       |      [04]
                | |       |      
                | |       +----->[06]
                | |              [07]
                | |
                | |    *-------->[09]
                | +->[11]---+    [10]
                |    [14]-+ |  
                |         | +--->[12]
                |         |      [13]
                |         |
                |         +----->[15]
                |                [16]
                |       
                |      *-------->[18]
                +--->[20]---+    [19]
                     [23]-+ |  
                          | +--->[21]
                          |      [22]
                          |
                          +----->[24]
                                 [25]




Each page has a small header, with a pointer to a preceeding page, and a record count.
For Leaf pages the _preceeding_ pointer is zero.

Following the header there is an index containing offsets to the actual records in the page,
and a pointer to the next level index or leaf page.
The records are stored as _keylength_, keydata, _datalength_, data.
All records in the level below an index are guaranteed to have a key greater than the key
in the index.

In leaf pages consecutive entries will often have keys which are very similar. The index stores
an offset into the key from which the keys differ, only the part that differs is stored.

| key                                | binary representation   | compressed key
| :--------------------------------- | :---------------------- | :------------------
| ('.', 0xFF000002, 'N')             | 2eff0000024e            | (0, 2eff0000024e)         
| ('.', 0xFF000002, 'S', 0x00000001) | 2eff0000025300000001    | (5, 5300000001)
| ('.', 0xFF000002, 'S', 0x00000002) | 2eff0000025300000002    | (9, 02)


## The ID1 section

The ID1 section contains the flag values as returned by the idc `GetFlags` function.
It starts with a list of file regions, followed by flags for each byte.


## Netnodes

The highlevel view of the `ID0` database is that of netnodes, as partially documented
in the idasdk.

The most important nodes are:
 * `Root Node`
 * lists: `$ structs`, `$ enums`, `$ scripts`
   * the values in a list are stored in the altnodes of the list node.
   * the values are one more than the actual nodeid pointed to:
     a list pointing to struct id's 0xff000bf6, 0xff000c01
     would contain : 0xff000bf7, 0xff000c02
 * `$ funcs`
 * `$ fileregions`, `$ segs`, '$ srareas'
 * '$ entry points'


### structs

The main struct node:

| node | contents
| :--- | :----
| (id, 'N')    | the struct name
| (id, 'M', 0) | packed member info, nodeids for members.


The struct member nodes:

| node | contents
| :--- | :----
| (id, 'N')    | the member name
| (id, 'M', 0) | packed member info
| (id, 'A', 3) | enum id
| (id, 'A', 11) | struct id
| (id, 'A', 16) | string type
| (id, 'S', 0) | member comment
| (id, 'S', 1) | repeatable member comment
| (id, 'S', 9) | offset spec
| (id, 'S', 0x3000) | typeinfo


### history

The `$ curlocs` list contains several location histories:

For example, the `$ IDA View-A` netnode contains the following keys:
 * `A 0` - highest history supval item 
 * `A 1` - number of history items
 * `A 2` - object type: `idaplace_t`
 * `S <num>` - packed history item: itemlinenr, ea\_t, int, int, colnr, rownr

 

### normal addresses

In the SDK, in the file `nalt.hpp` there are many more items defined.
These are some of the regularly used ones.

| key | value | description
| :-- | :---- | :----------
| (addr, 'D', fromaddr) | reftype | data xref from
| (addr, 'd', toaddr) | reftype | data xref to
| (addr, 'X', fromaddr) | reftype | code xref from
| (addr, 'x', toaddr) | reftype | code xref to
| (addr, 'N')         | string | global label
| (addr, 'A', 1)      | jumptableid+1 | jumptable target
| (addr, 'A', 2)      | nodeid+1 | hexrays info
| (addr, 'A', 3)      | structid+1  | data type
| (addr, 'A', 8)      | dword    | additional flags
| (addr, 'A', 0xB)    | enumid+1 | first operand enum type
| (addr, 'A', 0x10)   | dword  | string type
| (addr, 'A', 0x11)   | dword  | align type
| (addr, 'S', 0)      | string | comment
| (addr, 'S', 1)      | string | repeatable comment
| (addr, 'S', 4)      | data   | constant pool reference
| (addr, 'S', 5)      | data   | array
| (addr, 'S', 8)      | data   | jumptable info
| (addr, 'S', 9)      | packed | first operand offset spec
| (addr, 'S', 0xA)    | packed | second operand offset spec
| (addr, 'S', 0x1B)    | data  |  ?
| (addr, 'S', 1000+linenr) | string | anterior comment
| (addr, 'S', 0x1000) | packed | SP change point
| (addr, 'S', 0x3000) | data | function prototype
| (addr, 'S', 0x3001) | data | argument list
| (addr, 'S', 0x4000+n) | packed blob | register renaming
| (addr, 'S', 0x5000) | packed blob | function's local labels
| (addr, 'S', 0x6000) | data | register args
| (addr, 'S', 0x7000) | packed | function tails
| (addr, 'S', 0x7000) | dword  | tail backreference
| 
