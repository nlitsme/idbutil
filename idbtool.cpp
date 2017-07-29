/*
 * idbtool: a tool for viewing idb files without running ida.
 *
 * Author: Willem Hengeveld <itsme@xs4all.nl>
 *
 */
#include <fstream>
#include <iostream>
#include "idb3.h"
#include "argparse.h"

#include "gmpxx.h"
#include "formatter.h"
#include "stringlibrary.h"

int verbose = 0;

/*
 *  decode license info from idb
 */

// defines for making the intention of our mpz_import parameters clear to the reader.
#define ORDER_LS_FIRST -1
#define ORDER_MS_FIRST 1

// function for converting a range of bytes to a gmp bignum object.
inline mpz_class lstompz(const uint8_t *first, const uint8_t *last)
{
    mpz_class m;
    int endian= 0;  // endianness does not matter for bytes
    int nails= 0;   // all bits used
    int order= ORDER_LS_FIRST;
    mpz_import(m.get_mpz_t(), last-first, order, sizeof(uint8_t), endian, nails, first);

    return m;
}

// function for converting a list of bytes to a gmp bignum object.
inline mpz_class lstompz(const std::string& v)
{
    return lstompz((uint8_t*)&v[0], (uint8_t*)&v[0]+v.size());
}

// function for converting a gmp bignum object to a list of bytes.
inline std::string mpztoms(int requiredbytes, const mpz_class& m)
{
    int endian= 0;  // endianness does not matter for bytes
    int nails= 0;   // all bits used
    int order= ORDER_MS_FIRST;

    if (m<0)
        print("PROBLEM: can't convert negative mpz to bytes\n");

    size_t n= mpz_sizeinbase(m.get_mpz_t(), 256);
    if (requiredbytes==0)
        requiredbytes= n;
    std::string v(requiredbytes, char(0));
    mpz_export(&v[v.size()-n], &n, order, sizeof(uint8_t), endian, nails, m.get_mpz_t());
    return v;
}

// decrypt the contents of the '$ original user' node.
std::string decryptuser(const std::string& encvector)
{
    mpz_class exp(0x13);
    uint8_t modbytes[]= {
0xED,0xFD,0x42,0x5C,0xF9,0x78,0x54,0x6E,0x89,0x11,0x22,0x58,0x84,0x43,0x6C,0x57, 0x14,0x05,0x25,0x65,0x0B,0xCF,0x6E,0xBF,0xE8,0x0E,0xDB,0xC5,0xFB,0x1D,0xE6,0x8F,
0x4C,0x66,0xC2,0x9C,0xB2,0x2E,0xB6,0x68,0x78,0x8A,0xFC,0xB0,0xAB,0xBB,0x71,0x80, 0x44,0x58,0x4B,0x81,0x0F,0x89,0x70,0xCD,0xDF,0x22,0x73,0x85,0xF7,0x5D,0x5D,0xDD,
0xD9,0x1D,0x4F,0x18,0x93,0x7A,0x08,0xAA,0x83,0xB2,0x8C,0x49,0xD1,0x2D,0xC9,0x2E, 0x75,0x05,0xBB,0x38,0x80,0x9E,0x91,0xBD,0x0F,0xBD,0x2F,0x2E,0x6A,0xB1,0xD2,0xE3,
0x3C,0x0C,0x55,0xD5,0xBD,0xDD,0x47,0x8E,0xE8,0xBF,0x84,0x5F,0xCE,0xF3,0xC8,0x2B, 0x9D,0x29,0x29,0xEC,0xB7,0x1F,0x4D,0x1B,0x3D,0xB9,0x6E,0x3A,0x8E,0x7A,0xAF,0x93,
    };
    mpz_class mod= lstompz(modbytes, modbytes+sizeof(modbytes));
    mpz_class val= lstompz(encvector);

    mpz_class res;
    mpz_powm(res.get_mpz_t(), val.get_mpz_t(), exp.get_mpz_t(), mod.get_mpz_t());

    return mpztoms(sizeof(modbytes)-1, res);
}

/*
 *  dump structs + unions
 */
void dumpstructmember(const StructMember& mem)
{
    print("     %02x %02x %08x %02x: %-40s", 
            mem.skip(), mem.size(), mem.flags(), mem.props(),
            mem.name());

    uint64_t enumid = mem.enumid();
    if (enumid)
        print(" enum %08x", enumid);

    uint64_t structid = mem.structid();
    if (structid)
        print(" struct %08x", structid);

    auto ptrinfo = mem.ptrinfo();
    if (!ptrinfo.empty())
        print(" ptr %b", ptrinfo);

    auto type= mem.typeinfo();
    if (!type.empty())
        print(" type %b", type);

    print("\n");
    return;
}


void dumpstruct(const Struct& s)
{
    print("struct %s,   0x%x, 0x%x\n", s.name(), s.flags(), s.seqnr());
    for (const auto& mem : s)
        dumpstructmember(mem);
}

/*
 * dump bitfields
 */
void dumpbfvalue(const BitfieldValue& val)
{
    print("   %16x %s\n", val.value(), val.name());
}
void dumpbfmask(const BitfieldMask& msk)
{
    print("    mask %x", msk.mask());
    auto name = msk.name();
    if (!name.empty())
        print(" - %s", name);
    print("\n");

    auto c = msk.first();
    while (c.getkey() < msk.lastkey()) {
        dumpbfvalue(msk.getvalue(c));
        c.next();
    }
}
void dumpbitfield(ID0File & id0, uint64_t bfnode)
{
    Bitfield e(id0, bfnode);
    print("bitfield %s, 0x%x, 0x%x, 0x%x\n", e.name(), e.count(), e.representation(), e.flags());
    auto c = e.first();
    while (c.getkey() < e.lastkey()) {
        dumpbfmask(e.getmask(c));
        c.next();
    }
}

/*
 * dump enums
 */
void dumpenummember(const EnumMember& e)
{
    print("    %08x %s\n", e.value(), e.name());
}
void dumpenum(ID0File& id0, const Enum& e)
{
    if (e.flags()&1) {
        dumpbitfield(id0, e.nodeid());
        return;
    }

    print("enum %s, 0x%x, 0x%x, 0x%x\n", e.name(), e.count(), e.representation(), e.flags());
    auto c = e.first();
    while (c.getkey() < e.lastkey()) {
        dumpenummember(e.getvalue(c));
        c.next();
    }

}

/*
 *  print list of structs / enums
 */
void printidbstructs(ID0File& id0)
{
    auto list = List<Struct>(id0, id0.node("$ structs"));

    while (!list.eof())
        try {
            dumpstruct(list.next());
        }
        catch(const char*msg)
        {
            print("struct entry with error found\n");
        }
}
void printidbenums(ID0File& id0)
{
    auto list = List<Enum>(id0, id0.node("$ enums"));

    while (!list.eof())
        dumpenum(id0, list.next());
}




void printcomments(ID0File& id0)
{
// todo
// tool which lists all comments found in the database

/*
 -- nalt.hpp: NSUP_CMT = 0
 .{0X00001ddd 53 supvals 0} => "normal comment" 00

 -- nalt.hpp: NSUP_REPCMT = 1
 .{0X00001dd7 53 supvals 1} => "repeatable comment" 00

 -- lines.hpp: E_PREV = 1000
 .{0X00001ddd 53 supvals 1000} => "anterior comment" 00
 .{0X00001ddd 53 supvals 1001} => "more" 00
 .{0X00001ddd 53 supvals 1002} => "and another anterior" 00

 -- lines.hpp: E_NEXT = 2000
 .{0X00001ddd 53 supvals 2000} => "posterior comment" 00
 .{0X00001ddd 53 supvals 2001} => "more" 00
 .{0X00001ddd 53 supvals 2002} => "and another posterior" 00
*/


}




/*
 .{0X00001dd9  name} => "globallabel" 00
 N:"globallabel" => 0x00001dd9

 .. func prop
 -- nalt.hpp: NSUP_LLABEL
 .{0X00001bb0 53 supvals 0x5000} => {82 27 0a:"locallabel"}
*/


void printnames(ID0File& id0, ID1File& id1, NAMFile& nam, bool listall)
{
    nam.enumerate([&](uint64_t ea){
        uint64_t f= id1.GetFlags(ea);
        std::string name= id0.getname(ea);
        if (listall || !(f&0x8000))
            print("%08x: [%08x] %s\n", ea, f, name);

        // todo: filter out nullsub, jpt_XXX, thunks (j_...)
    });
}

// print each address in the form: <label> + offset
void printaddrs(ID0File& id0, ID1File& id1, NAMFile& nam, const std::vector<uint64_t>& addrs)
{
    for (uint64_t ea : addrs) {
        auto seg0 = id1.SegStart(ea);
        auto seg1 = id1.SegEnd(ea);

        std::string segspec;
        if (seg0!=BADADDR) {
            if (seg0==ea)
                segspec = stringformat("seg:%08x start", seg0);
            else if (seg1==ea)
                segspec = stringformat("seg:%08x end", seg0);
            else
                segspec = stringformat("seg:%08x+0x%x", seg0, ea-seg0);
        }
        else {
            segspec = "not in a seg";
        }

        std::string namespec;
        uint64_t fea = nam.findname(ea);
        if (fea == BADADDR) {
            // no names in database
            namespec = "-";
        }
        else {
            std::string name= id0.getname(fea);
            if (fea==ea) {
                // found a name for this address

                namespec = stringformat("%s", name);
            }
            else if (fea < ea) {
                // found name before this address
                namespec = stringformat("%s+0x%x", name, ea-fea);
            }
            else {
                // found name after this address
                namespec = stringformat("%s-0x%x", name, fea-ea);
            }
        }
        print("%08x: %-23s %s\n", ea, segspec, namespec);
    }
}

/*
 * print all idc/python scripts
 */
void dumpscript(const Script& scr)
{
    print("======= %s %s =======\n%s\n", scr.language(), scr.name(), scr.body());
}

void printidbscripts(ID0File& id0)
{
    auto list = List<Script>(id0, id0.node("$ scriptsnippets"));

    while (!list.eof())
        dumpscript(list.next());
}

/*
 * print license info
 */
std::string timestring(uint32_t t)
{
    if (t==0)
        return "                ";
    time_t date = t;
    struct tm tm= *localtime(&date);
    return stringformat("%04d-%02d-%02d %02d:%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min);
}

void dumplicense(const char *tag, const std::string& user)
{
    if (user.size()!=127)
        return;

    auto et = EndianTools();
    if (et.getle32(&user[106], &user[110]))
        return;
    uint16_t licver= et.getle16(&user[2], &user[0]+user.size());
    if (licver==0) {
        // before v5.3
        std::string licensee= (char*)&user[20];
        print("%s %s   %s\n", tag, timestring(et.getle32(&user[4], &user[0]+user.size())), licensee);
    }
    else {
        std::string licensee= (char*)&user[34];
        print("%sv%04d %s ... %s   %02x-%02x%02x-%02x%02x-%02x  %s\n",
                tag, licver,
                timestring(et.getle32(&user[16], &user[0]+user.size())),
                timestring(et.getle32(&user[16+8], &user[0]+user.size())),
                (int)(uint8_t)user[28], (int)(uint8_t)user[29], (int)(uint8_t)user[30], (int)(uint8_t)user[31], (int)(uint8_t)user[32], (int)(uint8_t)user[33],
                licensee);
    }
}

/*
 * print idb info from the rootnode
 */
void printidbinfo(ID0File& id0)
{
    uint64_t loadernode= id0.node("$ loader name");
    print("loader: %s  %s\n", id0.getstr(loadernode, 'S', 0), id0.getstr(loadernode, 'S', 1));

    uint64_t rootnode= id0.node("Root Node");
    std::string params= id0.getdata(rootnode, 'S', 0x41b994);
    std::string cpu{&params[5], &params[5+8]};
    auto nulpos = cpu.find(char(0));
    if (nulpos != cpu.npos)
        cpu.resize(nulpos);
    print("cpu: %-8s,  idaversion=%04d: %s\n", cpu,
            id0.getuint(rootnode, 'A', -1), id0.getstr(rootnode, 'S', 1303));
    print("nopens=%d, ctime=%s, crc=%08x, binary md5=%b\n", 
            id0.getuint(rootnode, 'A', -4),
            timestring(id0.getuint(rootnode, 'A', -2)),
            id0.getuint(rootnode, 'A', -5),
            id0.getdata(rootnode, 'S', 1302));

    std::string originaluser= id0.getdata(id0.node("$ original user"), 'S', 0);
//  this is privkey encrypted ( decrypts to user1 contents )
//  note: number is stored LSB first
    std::string user0= decryptuser(originaluser);

    std::string user1= id0.getdata(id0.node("$ user1"), 'S', 0);
    if (verbose)
        print("\n%b\n%b\n", user0, user1);

    dumplicense("orig: ", user0);
    dumplicense("curr: ", user1);
}

/*
 * print all nodes in sequential order
 */
void dumpnodes(ID0File& id0, bool ascending, int limit)
{
    auto c = ascending ? id0.find(REL_GREATER_EQUAL, "")
                       : id0.find(REL_LESS_EQUAL, "\xFF\xFF\xFF\xFF");
    while (!c.eof() && limit!=0)
    {
        print("%-b = %-b\n", c.getkey(), c.getval());
        if (ascending)
            c.next();
        else
            c.prev();

        if (limit>0)
            limit--;
    }
}

/*
 * perform simple queries on the .idb database
 */
enum {FL_EQ=1, FL_GT=2, FL_LT=4};

// translate ==, =>, <=, <. >  tokens to REL_xxx constants, passed to `id0.find`
relation_t xlat_relation(int flags)
{
    if (flags==(FL_EQ|FL_GT)) return REL_GREATER_EQUAL;
    if (flags==(FL_GT)) return REL_GREATER;
    if (flags==(FL_EQ|FL_LT)) return REL_LESS_EQUAL;
    if (flags==(FL_LT)) return REL_LESS;
    if (flags==FL_EQ) return REL_EQUAL;
    if (flags==0) return REL_EQUAL;
    throw "unknown relation";
}

// parses the part of the key after the relation into a node key.
// optionally looksup a node by name.
std::string createkey(ID0File& id0, const char *key, const char *keyend)
{
    // .<nodeid>;tag;idx   -> '.'<nodeid><tag><idx>
    // ?<nodename>         -> 'N'<name>
    // <nodename>;tag;idx  -> lookup name -> '.'<nodeid><tag><idx>
    if (key==keyend)
        return "";
    if (*key=='?')
        return id0.makename(std::string(key+1, keyend));
    auto i0 = std::find(key, keyend, ';');
    uint64_t nodeid;
    if (*key=='.' || *key=='#') {
        // a numeric node id.
        auto r = parseunsigned(key+1, i0, 0);
        if (r.second!=i0)
            throw "invalid keyspec";
        nodeid = r.first;
        if (*key=='#')
            nodeid += id0.nodebase();
    }
    else {
        // lookup the node by name
        nodeid = id0.node(std::string(key, i0));
    }
    if (i0==keyend)
        return id0.makekey(nodeid);
    if (i0+1==keyend)
        throw "invalid keyspec";
    char tag = i0[1];
    auto i1 = std::find(i0+1, keyend, ';');
    if (i1==keyend)
        return id0.makekey(nodeid, tag);
    if (i1+1==keyend)
        throw "invalid keyspec";
    if (tag=='H')
        return id0.makekey(nodeid, tag, std::string(i1+1, keyend));
    auto r = parseunsigned(i1+1, keyend, 0);
    if (r.second!=keyend)
        throw "invalid keyspec";
    uint64_t ix = r.first;

    return id0.makekey(nodeid, tag, ix);
}

// parse and execute the query.
void queryidb(ID0File& id0, const std::string& query, bool ascending, int limit)
{
    if (query.size()<=2)
        return;
    int flags = 0;
    const char *key = nullptr;
    switch(query[0])
    {
        case '=': flags |= FL_EQ; break;
        case '>': flags |= FL_GT; break;
        case '<': flags |= FL_LT; break;
        default: key = &query[0];
    }
    switch(query[1])
    {
        case '=': flags |= FL_EQ; break;
        case '>': flags |= FL_GT; break;
        case '<': flags |= FL_LT; break;
        default: if (!key) key = &query[1];
    }
    if (!key) key = &query[2];
    if (flags==0) flags = FL_EQ;

    auto c = id0.find(xlat_relation(flags), createkey(id0, key, &query[0]+query.size()));

    while (!c.eof() && limit!=0)
    {
        print("%-b = %-b\n", c.getkey(), c.getval());
        if (flags == FL_EQ)
            break;
        if (ascending)
            c.next();
        else
            c.prev();
        if (limit>0)
            limit--;
    }
}

void usage()
{
    printf("idbtool OPTIONS  <files> [-- ADDRLIST]\n");
    printf("    -s | --scripts    print all scripts\n"); //  -c | --comments   print all comments
    printf("    -t | --structs    print all structs          -i | --info       print database info\n");
    printf("    -e | --enums      print all enums            -d | --id0        low level db dump\n");
    printf("    -n | --names      print generated names      -inc | --inc      dump all records in ascending order\n");
    printf("    -a                print all names            -dec | --dec      dump all records in descending order\n");
    printf("when the ADDRLIST is specified, the addresses in the list are printed as 'name+offset'\n");

    printf("    -q | --query  QUERY                          -m LIMIT          number of records printed\n");
    printf("example queries:\n");
    printf("  * '?Root Node' -> prints the Name node pointing to the root\n");
    printf("  * '>Root Node' -> prints the first 10 records after the root node\n");
    printf("  * '<Root Node' -> prints the 10 records startng with the recordsbefore the rootnode.\n");
    printf("  * '.0xff000001;N' -> prints rootnode name entry.\n");
    printf("  * '#1;N' -> prints rootnode name entry.\n");

}

#define LISTALL_NAMES     1
#define PRINT_NAMES       2
#define PRINT_SCRIPTS     4
#define PRINT_STRUCTS     8
#define PRINT_COMMENTS   16
#define PRINT_ENUMS      32
#define PRINT_INFO       64
#define DUMP_ASCENDING  128
#define DUMP_DESCENDING 256
#define DUMP_DATABASE   512
#define QUERY_IDB      1024

// perform the options specified on the commandline on a specific idb file.
void processidb(const std::string& fn, int flags, const std::string& query, const std::vector<uint64_t>& addrs, int limit)
{
    IDBFile idb(std::make_shared<std::ifstream>(fn));
    ID0File id0(idb, idb.getsection(ID0File::INDEX));
    ID1File id1(idb, idb.getsection(ID1File::INDEX));
    NAMFile nam(idb, idb.getsection(NAMFile::INDEX));

    if (flags&PRINT_INFO)
        printidbinfo(id0);
    if (flags&PRINT_SCRIPTS)
        printidbscripts(id0);
    if (flags&PRINT_COMMENTS)
        printcomments(id0);
    if (flags&PRINT_STRUCTS)
        printidbstructs(id0);
    if (flags&PRINT_ENUMS)
        printidbenums(id0);
    if (flags&PRINT_NAMES)
        printnames(id0, id1, nam, flags&LISTALL_NAMES);

    if (!addrs.empty())
        printaddrs(id0, id1, nam, addrs);

    if (flags&QUERY_IDB)
        queryidb(id0, query, !(flags&DUMP_DESCENDING), limit);
    else if (flags&(DUMP_ASCENDING|DUMP_DESCENDING))
        dumpnodes(id0, flags&DUMP_ASCENDING, limit);

    if (flags&DUMP_DATABASE) {
        id1.dump_info();
        id0.dump();
    }
}


int main(int argc, char**argv)
{
    bool addingidbs=true;
    std::vector<std::string> idbnames;
    std::vector<uint64_t> addrs;
    std::string query;
    int limit = -1;

    int flags= 0;

    for (auto& arg : ArgParser(argc, argv))
       switch (arg.option())
       {
            case 'v': verbose = arg.count(); break;
            case '-': if (arg.match("--verbose")) verbose = 1;
                      else if (arg.match("--names"))   flags |= PRINT_NAMES;
                      else if (arg.match("--scripts")) flags |= PRINT_SCRIPTS;
                      else if (arg.match("--structs")) flags |= PRINT_STRUCTS;
                      else if (arg.match("--enums"))   flags |= PRINT_ENUMS;
                      //else if (arg.match("--comments"))flags |= PRINT_COMMENTS;
                      else if (arg.match("--id0"))     flags |= DUMP_DATABASE;
                      else if (arg.match("--info"))    flags |= PRINT_INFO;
                      else if (arg.match("--limit"))   limit = arg.getint();
                      else if (arg.match("--query")) {
                          flags |= QUERY_IDB;
                          query = arg.getstr();
                      }
                      else if (arg.match("--inc")) flags |= DUMP_ASCENDING;
                      else if (arg.match("--dec")) flags |= DUMP_DESCENDING;
                      else if (arg.optionterminator()) {
                          // '--' separates the db list from the addr list.
                          addingidbs= false;
                      }
                      break;
            case 'a': flags |= LISTALL_NAMES; break;
            case 'n': flags |= PRINT_NAMES; break;
            case 's': flags |= PRINT_SCRIPTS; break;
            case 'u': flags |= PRINT_STRUCTS; break;
            //case 'c': flags |= PRINT_COMMENTS; break;
            case 'e': flags |= PRINT_ENUMS; break;
            case 'd': if (arg.match("-dec"))
                          flags |= DUMP_DESCENDING;
                      else
                          flags |= DUMP_DATABASE; 
                      break;
            case 'i': if (arg.match("-inc"))
                          flags |= DUMP_ASCENDING;
                      else if (arg.match("-id0"))
                          flags |= DUMP_DATABASE; 
                      else
                          flags |= PRINT_INFO;
                      break;
            case 'q': flags |= QUERY_IDB;
                      query = arg.getstr();
                      break;
            case 'm': limit = arg.getint();
                      break;
            default:
                      usage();
                      return 1;
           case -1:
                if (addingidbs)
                    idbnames.push_back(arg.getstr());
                else
                    addrs.push_back(arg.getint());

    }
    if (idbnames.empty()) {
        usage();
        return 1;
    }

    for (auto const&arg : idbnames)
    {
        if (idbnames.size()>1)
            print("==> %s <==\n", arg);
        try {
        processidb(arg, flags, query, addrs, limit);
        }
        catch(const std::exception & e) {
            print("EXCEPTION: %s\n", e.what());
        }
        catch(const char * msg) {
            print("ERROR: %s\n", msg);
        }
        if (idbnames.size()>1)
            print("\n");
    }

    return 0;
}

/*  -- ida type id's
off_210FA0      dd offset aVoid         ; "void"              
                dd offset a__int8       ; "__int8"            0x01
                dd offset a__int16_0    ; "__int16"           0x02
                dd offset a__int32_0    ; "__int32"           0x03
                dd offset a__int64_0    ; "__int64"           0x04
                dd offset a__int128_0   ; "__int128"          0x05
                dd offset aChar         ; "char"              0x06
                dd offset aInt          ; "int"               0x07
                dd offset aBool         ; "bool"              0x08
                dd offset a_bool1       ; "_BOOL1"            0x09
                dd offset a_bool2       ; "_BOOL2"            0x0a
                dd offset a_bool4       ; "_BOOL4"            0x0b
                dd offset aFloat        ; "float"             0x0c
                dd offset aDouble       ; "double"            0x0d
                dd offset aLongDouble   ; "long double"       0x0e
                dd offset aShortFloat   ; "short float"       0x0f
                dd offset a__seg        ; "__seg"             0x10
                dd offset a_unknown     ; "_UNKNOWN"          0x11
                dd offset a_byte        ; "_BYTE"             0x12
                dd offset a_word        ; "_WORD"             0x13
                dd offset a_dword       ; "_DWORD"            0x14
                dd offset a_qword       ; "_QWORD"            0x15
                dd offset a_oword       ; "_OWORD"            0x16
                dd offset a_tbyte       ; "_TBYTE"            0x17
                dd offset aSigned_0     ; "signed "           0x18
                dd offset aUnsigned     ; "unsigned "         0x19
                dd offset a__cdecl      ; "__cdecl"           0x1a
                dd offset a__stdcall    ; "__stdcall"         0x1b
                dd offset a__pascal     ; "__pascal"          0x1c
                dd offset a__fastcall   ; "__fastcall"        0x1d
                dd offset a__thiscall   ; "__thiscall"        0x1e
                dd offset asc_1C1708    ; ""                  0x1f
                dd offset a__userpurge  ; "__userpurge"       0x20
                dd offset a__usercall   ; "__usercall"        0x21
                dd offset a__int8_0     ; ": __int8 "         0x22
                dd offset a__int16      ; ": __int16 "        0x23
                dd offset a__int32      ; ": __int32 "        0x24
                dd offset a__int64      ; ": __int64 "        0x25
                dd offset a__int128     ; ": __int128 "       0x26
                dd offset a__int256     ; ": __int256 "       0x27
                dd offset a__int512     ; ": __int512 "       0x28

 */

