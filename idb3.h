/*
 *  idb3.h  is a library for accessing IDApro databases.
 *
 * Author: Willem Hengeveld <itsme@xs4all.nl>
 *
 *
 * Toplevel class: IDBFile, use getsection to get a stream to the desired section,
 * Then create an ID0File, ID1File, NAMFile for that section.
 * 
 */
#include <iostream>
#include <sstream>
#include <vector>
#include <set>
#include <cassert>
#include "formatter.h"

#define dbgprint(...)

// a sharedptr, so i can pass an istream around without
// worrying about who owns it.
typedef std::shared_ptr<std::istream> stream_ptr;

// create vector from `n`  invocations of `f`
template<typename T, typename FN>
std::vector<T> getvec(int n, FN f)
{
    std::vector<T> v;
    while (n--)
        v.push_back(f());
    return v;
}


////////////////////////////////////////////////////////////////////////
// Sometimes i need to pass backinserter iterators as <first, last> pair
// These functions make that possible.

// ANY - backinserter == INT_MAX -> always enough space after a backinserter
template<typename T, typename C>
int operator-(T lhs, typename std::back_insert_iterator<C> rhs)
{
    return INT_MAX;
}

// backinserter += INT  -> does nothing
template<typename C>
typename std::back_insert_iterator<C>& operator+=(typename std::back_insert_iterator<C>& rhs, int n)
{
    return rhs;
}



// streamhelper: get little/big endian integers of various sizes from a stream
// There are functions for 8, 16, 32, 64 bit little/big endian unsigned integers.
// And a function for reading a database dependent word (64bit for .i64, 32bit for .idb)
template<typename ISPTR>
class streamhelper {
    ISPTR _is;
    int _wordsize;  // the wordsize of the current database
public:
    streamhelper(ISPTR is, int wordsize)
        : _is(is), _wordsize(wordsize)
    {
    }
    uint8_t get8()
    {
        auto c = _is->get();
        if (c==-1)
            throw "EOF";
        return (uint8_t)c;
    }
    uint16_t get16le()
    {
        uint8_t lo = get8();
        uint8_t hi = get8();
        return (hi<<8) | lo;
    }
    uint16_t get16be()
    {
        uint8_t hi = get8();
        uint8_t lo = get8();
        return (hi<<8) | lo;
    }

    uint32_t get32le()
    {
        uint16_t lo = get16le();
        uint16_t hi = get16le();
        return (hi<<16) | lo;
    }
    uint32_t get32be()
    {
        uint16_t hi = get16be();
        uint16_t lo = get16be();
        return (hi<<16) | lo;
    }
    uint64_t get64le()
    {
        uint32_t lo = get32le();
        uint32_t hi = get32le();
        return (uint64_t(hi)<<32) | lo;
    }
    uint64_t get64be()
    {
        uint32_t hi = get32be();
        uint32_t lo = get32be();
        return (uint64_t(hi)<<32) | lo;
    }


    // function used to get the right wordsize for either .i64 or .idb file.
    uint64_t getword()
    {
        if (_wordsize==4)
            return get32le();
        else if (_wordsize==8)
            return get64le();
        throw "unsupported wordsize";
    }
    std::string getdata(int n)
    {
        std::string str(n, char(0));
        auto m = _is->readsome(&str.front(), n);
        str.resize(m);
        dbgprint("getdata -> %b\n", str);
        return str;
    }
    std::istream& seekg( std::istream::off_type off, std::ios_base::seekdir dir)
    {
        return _is->seekg(off, dir);
    }
    std::istream& seekg( std::istream::pos_type pos )
    {
        return _is->seekg(pos);
    }
};

// function for creating a streamhelper.
template<typename ISPTR>
auto makehelper(ISPTR is, int wordsize = 0)
{
    return streamhelper<ISPTR>(is, wordsize);
}


// EndianTools: a collection of static functions for reading/writing
// little/big endian integers of various sizes from an iterator range
struct EndianTools {
    template<typename P>
    static void set8(P first, P last, uint8_t w)
    {
        if (last-first < 1)
            throw "not enough space";
        *first = w;
    }
    template<typename P, typename T>
    static void setbe16(P first, P last, T w)
    {
        P p = first;
        if (last-p < 2)
            throw "not enough space";
        set8(p, last, w>>8);  p += 1;
        set8(p, last, w);
    }
    template<typename P, typename T>
    static void setbe32(P first, P last, T w)
    {
        P p = first;
        if (last-p < 4)
            throw "not enough space";
        setbe16(p, last, w>>16);  p += 2;
        setbe16(p, last, w);
    }
    template<typename P, typename T>
    static void setbe64(P first, P last, T w)
    {
        P p = first;
        if (last-p < 8)
            throw "not enough space";
        setbe32(p, last, w>>32);  p += 4;
        setbe32(p, last, w);
    }
    template<typename P, typename T>
    static void setle16(P first, P last, T w)
    {
        P p = first;
        if (last-p < 2)
            throw "not enough space";
        set8(p, last, w);  p += 1;
        set8(p, last, w>>8);
    }
    template<typename P, typename T>
    static void setle32(P first, P last, T w)
    {
        P p = first;
        if (last-p < 4)
            throw "not enough space";
        setle16(p, last, w);      p += 2;
        setle16(p, last, w>>16);
    }
    template<typename P, typename T>
    static void setle64(P first, P last, T w)
    {
        P p = first;
        if (last-p < 8)
            throw "not enough space";
        setle32(p, last, w);        p += 4;
        setle32(p, last, w>>32);
    }

    template<typename P>
    static uint8_t get8(P first, P last)
    {
        if (first>=last)
            throw "not enough space";
        return *first;
    }

    template<typename P>
    static uint16_t getbe16(P first, P last)
    {
        P p = first;
        if (last-p < 2)
            throw "not enough space";
        uint8_t hi =get8(p, last);   p += 1;
        uint8_t lo =get8(p, last);

        return (uint16_t(hi)<<8) | lo;
    }
    template<typename P>
    static uint32_t getbe32(P first, P last)
    {
        P p = first;
        if (last-p < 4)
            throw "not enough space";
        uint16_t hi =getbe16(p, last);   p += 2;
        uint16_t lo =getbe16(p, last);

        return (uint32_t(hi)<<16) | lo;
    }
    template<typename P>
    static uint64_t getbe64(P first, P last)
    {
        P p = first;
        if (last-p < 8)
            throw "not enough space";
        uint32_t hi =getbe32(p, last);   p += 4;
        uint32_t lo =getbe32(p, last);

        return (uint64_t(hi)<<32) | lo;
    }
    template<typename P>
    static uint16_t getle16(P first, P last)
    {
        P p = first;
        if (last-p < 2)
            throw "not enough space";
        uint8_t lo =get8(p, last);    p += 1;
        uint8_t hi =get8(p, last);

        return (uint16_t(hi)<<8) | lo;
    }
    template<typename P>
    static uint32_t getle32(P first, P last)
    {
        P p = first;
        if (last-p < 4)
            throw "not enough space";
        uint16_t lo =getle16(p, last);   p += 2;
        uint16_t hi =getle16(p, last);

        return (uint32_t(hi)<<16) | lo;
    }
    template<typename P>
    static uint64_t getle64(P first, P last)
    {
        P p = first;
        if (last-p < 8)
            throw "not enough space";
        uint32_t lo =getle32(p, last);   p += 4;
        uint32_t hi =getle32(p, last);

        return (uint64_t(hi)<<32) | lo;
    }

};

// stream buffer for sectionstream
// This is the class doing the actual work for sectionstream.
// This presents a view of a section of a random access stream.
class sectionbuffer : public std::streambuf {
    stream_ptr _is;

    std::streamoff _first;
    std::streamoff _last;

    std::streampos _curpos;

public:
    sectionbuffer(stream_ptr is,  uint64_t first, uint64_t last)
        : _is(is), _first(first), _last(last), _curpos(0)
    {
        _is->seekg(_first);
    }
protected:
    std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
    {
        std::streampos newpos;
        switch(way)
        {
        case std::ios_base::beg:
            newpos = off;
            break;
        case std::ios_base::cur:
            newpos = _curpos + off;
            break;
        case std::ios_base::end:
            newpos = (_last-_first) + off;
            break;
        default:
            throw std::ios_base::failure("bad seek direction");
        }
        return seekpos(newpos, which);
    }

    std::streampos seekpos(std::streampos sp, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
    {
        if (sp<0 || sp > (_last-_first))
            return -1;
        _is->seekg(sp+_first);
        return _curpos = sp;
    }
    std::streamsize showmanyc()
    {
        return (_last-_first)-_curpos;
    }
    std::streamsize xsgetn(char_type* s, std::streamsize n)
    {
        if (n<=0 || _curpos >= (_last-_first))
            return 0;
        auto want = std::min(std::streamsize(_last-_first - _curpos), n);
        //auto got = _is->readsome(s, want);
        _is->read(s, want);  auto got = want;
        _curpos += got;

        return got;
    }
    int_type underflow()
    {
        if (_curpos >= (_last-_first))
            return traits_type::eof();
        int r = _is->peek();
        return r;
    }
    int_type uflow()
    {
        if (_curpos >= (_last-_first))
            return traits_type::eof();
        int r = _is->get();
        if (r==traits_type::eof())
            return traits_type::eof();
        _curpos+=1;
        return r;
    }
};
// istream restricted to a section of a seakable stream
class sectionstream : public std::istream {
    sectionbuffer _buf;
public:
    template<typename ISPTR>
    sectionstream(ISPTR is, uint64_t from, uint64_t size)
        : std::istream(nullptr), _buf(is, from, from+size)
    {
        init(&_buf);
    }
};

///////////////////////////////////////////////////////////////
// read .idb file, returns sectionstreams for sections
//
// IDBFile knows how to read sections from all types of IDApro databases.
//
// Compression is not yet supported.
class IDBFile {
    stream_ptr _is;
    uint32_t _magic;
    int _fileversion;
    std::vector<uint64_t> _offsets;
    std::vector<uint32_t> _checksums;
public:
    enum {
MAGIC_IDA2 = 0x32414449,
MAGIC_IDA1 = 0x31414449,
MAGIC_IDA0 = 0x30414449,
    };
    IDBFile(stream_ptr is)
        : _is(is), _magic(0), _fileversion(-1)
    {
        readheader();
    }
    uint32_t magic() const { return _magic; }

    void readheader()
    {
        auto s = makehelper(_is);
        _magic = s.get32le();
        /*zero = */ s.get16le();
        auto values = getvec<uint32_t>(6, [&](){ return s.get32le(); });
        if (values[5]!=0xaabbccdd) {
            _fileversion = 0;
            for (auto v : values)
                _offsets.push_back(v);
            _offsets[5] = 0;
            _checksums.resize(6);
            return;
        }
        _fileversion = s.get16le();

        if (_fileversion < 5) {
            /*auto unknown =*/ s.get32le();
            for (auto v : values)
                _offsets.push_back(v);
            _offsets.pop_back();

            _checksums = getvec<uint32_t>(5, [&](){ return s.get32le(); });
            uint32_t idsofs = s.get32le();
            uint32_t idscheck = _fileversion==1 ? s.get16le() : s.get32le();
            _offsets.push_back(idsofs);
            _checksums.push_back(idscheck);

            // in filever==4 there is more in the .idb header:
            // 0x5c, 0, 0, <md5>, 128*NUL
        }
        else {
            // ver 5, 6 : 64 bit fileptrs
            _offsets.push_back((uint64_t(values[1])<<32)|values[0]);
            _offsets.push_back((uint64_t(values[3])<<32)|values[2]);
            _offsets.push_back(s.get64le());
            _offsets.push_back(s.get64le());
            _offsets.push_back(s.get64le());
            _checksums = getvec<uint32_t>(5, [&](){ return s.get32le(); });
            _offsets.push_back(s.get64le());
            _checksums.push_back(s.get32le());

            // more data in the .idb header:
            // 0x7c, 0, 0, <md5>, 128*NUL
        }
    }
    void dump()
    {
        print("IDB v%d, m=%08x\n", _fileversion, _magic);
        for (int i=0 ; i<std::max(_offsets.size(), _checksums.size()) ; i++)
            print("%d: %10llx %08x\n", i, i<_offsets.size() ? _offsets[i] : -1, i<_checksums.size() ? _checksums[i] : -1);
    }
    auto getinfo(int i)
    {
        _is->seekg(_offsets[i]);
        auto s = makehelper(_is);
        auto comp = s.get8();
        uint64_t size = _fileversion<5 ?  s.get32le() : s.get64le();
        uint64_t ofs = _offsets[i] + (_fileversion<5 ?  5 : 9);

        return std::make_tuple(comp, ofs, size);
    }

    stream_ptr getsection(int i)
    {
        auto info = getinfo(i);
        if (std::get<0>(info))
            throw "compression not supported";
        return std::make_shared<sectionstream>(_is, std::get<1>(info), std::get<2>(info));
    }
};

// search relation
enum relation_t {
    REL_LESS,
    REL_LESS_EQUAL,
    REL_EQUAL,
    REL_GREATER_EQUAL,
    REL_GREATER,
    REL_RECURSE,
};

// baseclass for Btree Pages
// baseclass for Btree database, subclassed by v1.5, v1.6, v2.0
class BasePage {
protected:
    stream_ptr _is;
    int _pagesize;
    uint32_t _nr;
    uint32_t _preceeding;
    int _count;

    // item for the entry table
    class Entry {
    public:
        uint32_t pagenr;
        int indent;
        int recofs;

        Entry() : pagenr(0), indent(0), recofs(0) { }
        Entry(Entry&& e) : pagenr(e.pagenr), indent(e.indent), recofs(e.recofs) { }
    };
    std::vector<Entry> _index;
    std::vector<std::string> _keys; // only for leaf pages

    // IntIter, used to be able to use upper_bound on `_index`
    class IntIter : public std::iterator<std::random_access_iterator_tag, int> {
        int _ix;
    public:
        IntIter(int x)  : _ix(x) { }
        IntIter() : _ix(0) { }
        IntIter(const IntIter& i) : _ix(i._ix) { }

        bool operator==(const IntIter& rhs) {return _ix==rhs._ix;}
        bool operator!=(const IntIter& rhs) {return _ix!=rhs._ix;}

        int operator*() {return _ix;}
        int operator[](int i) {return _ix+i;}

        IntIter& operator++() {++_ix;return *this;}
        IntIter operator++(int) {IntIter tmp(*this); operator++(); return tmp;}
        IntIter& operator--() {--_ix;return *this;}
        IntIter operator--(int) {IntIter tmp(*this); operator--(); return tmp;}

        IntIter& operator+=(int n) {  _ix += n; return *this; }
        IntIter& operator-=(int n) {  _ix -= n; return *this; }

        friend IntIter operator+(int n, IntIter p) {  return p+=n; }
        friend IntIter operator+(IntIter p, int n) {  return p+=n; }
        friend IntIter operator-(IntIter p, int n) {  return p-=n; }
        friend int operator-(const IntIter& p, const IntIter& q) {  return p._ix-q._ix; }

        bool operator<(const IntIter& rhs) { return _ix<rhs._ix; }
        bool operator<=(const IntIter& rhs) { return _ix<=rhs._ix; }
        bool operator>(const IntIter& rhs) { return _ix>rhs._ix; }
        bool operator>=(const IntIter& rhs) { return _ix>=rhs._ix; }

    };

    // unused, iterator returning Entry's
    class PageIter : public std::iterator<std::random_access_iterator_tag, Entry> {
        BasePage* _page;
        int _ix;
    public:
        PageIter(BasePage*page, int ix) : _page(page), _ix(ix) { }
        PageIter() : _page(nullptr), _ix(0) { }
        PageIter(const PageIter& i) : _page(i._page), _ix(i._ix) { }

        bool operator==(const PageIter& rhs) {return _ix==rhs._ix;}
        bool operator!=(const PageIter& rhs) {return _ix!=rhs._ix;}

        Entry& operator*() {return _page->getent(_ix);}
        Entry& operator[](int i) {return _page->getent(_ix+i);}

        PageIter& operator++() {++_ix;return *this;}
        PageIter operator++(int) {PageIter tmp(*this); operator++(); return tmp;}
        PageIter& operator--() {--_ix;return *this;}
        PageIter operator--(int) {PageIter tmp(*this); operator--(); return tmp;}

        PageIter& operator+=(int n) {  _ix += n; return *this; }
        PageIter& operator-=(int n) {  _ix -= n; return *this; }

        friend PageIter operator+(int n, PageIter p) {  return p+=n; }
        friend PageIter operator+(PageIter p, int n) {  return p+=n; }
        friend PageIter operator-(PageIter p, int n) {  return p-=n; }
        friend int operator-(const PageIter& p, const PageIter& q) {  return p._ix-q._ix; }

        bool operator<(const PageIter& rhs) { return _ix<rhs._ix; }
        bool operator<=(const PageIter& rhs) { return _ix<=rhs._ix; }
        bool operator>(const PageIter& rhs) { return _ix>rhs._ix; }
        bool operator>=(const PageIter& rhs) { return _ix>=rhs._ix; }
    };

public:
    BasePage(stream_ptr  is, uint32_t nr, int pagesize)
        : _is(is), _pagesize(pagesize), _nr(nr), _preceeding(0), _count(0)
    {
    }
    uint32_t nr() const { return _nr; }

    bool isindex() const { return _preceeding!=0; }
    bool isleaf() const { return _preceeding==0; }

    size_t indexsize() const { return _index.size(); }

    virtual Entry readent() = 0;

    void dump()
    {
        if (_preceeding)
            print("prec = %05x\n", _preceeding);
        for (int i=0 ; i<_index.size() ; i++)
            print("%-b = %-b\n", getkey(i), getval(i));
    }

    void readindex()
    {
        for (int i=0 ; i<_count ; i++)
            _index.emplace_back(readent());
        //print("got %d entries\n", _index.size());

        if (isleaf())
            readkeys();
    }
    // for a leafpage, calculate all key values
    void readkeys()
    {
        auto s = makehelper(_is);
        std::string key;
        for (auto & ent : _index) {
            _is->seekg(ent.recofs);
            int klen = s.get16le();
            key.resize(klen+ent.indent);
            _is->read(&key[ent.indent], klen);

            dbgprint("key i=%d, l=%d -> %b\n", ent.indent, klen, key);
            _keys.push_back(key);
        }
    }

    // get the subpage for the item at positon `i`
    uint32_t getpage(int i) const
    {
        if (!isindex())
            throw "getpage called on leaf";
        if (i<0)
            return _preceeding;
        if (i>=_index.size()) {
            print("#%06x i=%d, max=%d\n", _nr, i, _index.size());
            throw "page: i too large";
        }
        return _index[i].pagenr;
    }

    // get key for the item at position `i`
    std::string getkey(int i)
    {
        auto& ent = getent(i);
        if (isindex()) {
            _is->seekg(ent.recofs);
            auto s = makehelper(_is);
            int klen = s.get16le();

            dbgprint("indexkey(%d) -> l=%d\n", i, klen);
            return s.getdata(klen);
        }
        else if (isleaf()) {
            dbgprint("leafkey(%d)\n", i);
            return _keys[i];
        }
        throw "not a leaf of index";
    }
    // get value for the item at position `i`
    std::string getval(int i)
    {
        auto& ent = getent(i);
        _is->seekg(ent.recofs);
        auto s = makehelper(_is);
        int klen = s.get16le();
        _is->seekg(klen, std::ios_base::cur);
        int vlen = s.get16le();

        dbgprint("%04x: val(%d), kl=%d, vl=%d\n", ent.recofs, i, klen, vlen);
        return s.getdata(vlen);
    }

    Entry& getent(int i) {
        if (i<0 || i>=_index.size())
            throw "invalid key index";

        return _index[i];
    }

    // unused
    //auto begin() { return PageIter(this, 0); }
    //auto end() { return PageIter(this, _count); }

    struct result {
        relation_t act;
        int index;

        bool operator==(const result& rhs) const { return act==rhs.act && index==rhs.index; }
        bool operator!=(const result& rhs) const { return !(*this==rhs); }

        friend std::ostream& operator<<(std::ostream& os, const result& res)
        {
            os << '{';
            switch(res.act)
            {
                case REL_LESS:           os << "<"; break;
                case REL_LESS_EQUAL:     os << "<="; break;
                case REL_EQUAL:          os << "=="; break;
                case REL_GREATER_EQUAL:  os << ">="; break;
                case REL_GREATER:        os << ">"; break;
                case REL_RECURSE:        os << "r"; break;
                         default:        os << "?";
            }
            os << res.index;
            os << '}';
            return os;
        }
    };

    // search for the key in this page.
    // getkey(index) ... act ... key
    result find(const std::string& key)
    {
        //auto i = std::upper_bound(begin(), end(), key, [](const std::string& key, const Entry& ent){  return false; });
        auto i = std::upper_bound(IntIter(0), IntIter(_count), key, [this](const std::string& key, int ix){  return key < this->getkey(ix); });

        if (i==IntIter(0)) {
            if (isindex())
                return {REL_RECURSE, -1};
            return {REL_GREATER, 0}; // index[0] > key
        }
        --i;
        int ix = i-IntIter(0);
        if (getkey(ix) == key)
            return {REL_EQUAL, ix};
        if (isindex())
            return {REL_RECURSE, ix};
        return {REL_LESS, ix};       // index[ix] < key
    }
};
typedef std::shared_ptr<BasePage> Page_ptr;


// baseclass for Btree database, subclassed by v1.5, v1.6, v2.0
class BtreeBase {
protected:
    stream_ptr _is;
    uint32_t _firstindex;
    uint32_t _pagesize;
    uint32_t _firstfree;
    uint32_t _reccount;
    uint32_t _pagecount;
public:
    class Cursor {
        BtreeBase *_bt;
        struct ent {
            Page_ptr page;
            int index;

            ent(Page_ptr page, int index)
                : page(page), index(index)
            {
            }
            ent() : index(0) { }

            bool operator==(const ent& rhs) const { return page==rhs.page && index==rhs.index; }
            bool operator!=(const ent& rhs) const { return !(*this==rhs); }
        };
        std::vector<ent> _stack;

        void dump() const {
            std::stringstream x;
            for (auto& ent : _stack)
                x << stringformat(" %05x:%d", ent.page->nr(), ent.index);
            std::cout << x.str() << std::endl;
        }
    public:
        Cursor(BtreeBase *bt)
            : _bt(bt)
        {
        }

        void clear()
        {
            _stack.clear();
        }
        void next()
        {
            if (eof())
                throw "cursor:EOF";
            auto ent = _stack.back(); _stack.pop_back();
            if (ent.page->isleaf()) {
                // from leaf move towards root
                ent.index++;
                while (!_stack.empty() && ent.index==ent.page->indexsize()) {
                    ent = _stack.back(); _stack.pop_back();
                    ent.index++;
                }
                if (ent.index<ent.page->indexsize()) {
                    add(ent.page, ent.index);
                }
            }
            else {
                // from node move towards leaf
                add(ent.page, ent.index);
                ent.page = _bt->readpage(ent.page->getpage(ent.index));
                while (ent.page->isindex()) {
                    ent.index = -1;
                    add(ent.page, ent.index);
                    ent.page = _bt->readpage(ent.page->getpage(ent.index));
                }
                ent.index = 0;
                add(ent.page, ent.index);
            }
        }
        void prev()
        {
            if (eof())
                throw "cursor:EOF";
            auto ent = _stack.back(); _stack.pop_back();
            ent.index--;
            if (ent.page->isleaf()) {
                while (!_stack.empty() && ent.index<0) {
                    ent = _stack.back(); _stack.pop_back();
                }
                if (ent.index>=0)
                    add(ent.page, ent.index);
            }
            else {
                add(ent.page, ent.index);
                while (ent.page->isindex()) {
                    ent.page = _bt->readpage(ent.page->getpage(ent.index));
                    ent.index = ent.page->indexsize()-1;
                    add(ent.page, ent.index);
                }
            }
        }
        bool eof() const
        {
            return _stack.empty();
        }

        // for Btree.find to create cursor.
        void add(Page_ptr page, int index)
        {
            _stack.emplace_back(page, index);
        }

        // getting key/value from cursor pos
        auto getkey() const
        {
            if (eof())
                throw "cursor:EOF";
            auto ent = _stack.back();
            return ent.page->getkey(ent.index);
        }
        auto getval() const
        {
            if (eof())
                throw "cursor:EOF";
            auto ent = _stack.back();
            return ent.page->getval(ent.index);
        }

        bool operator==(const Cursor& rhs) const { return _stack == rhs._stack; }
        bool operator!=(const Cursor& rhs) const { return !(*this==rhs); }

        bool operator<(const Cursor& rhs) const { return getkey() < rhs.getkey(); }
    };


    BtreeBase(stream_ptr  is) : _is(is) { }
    virtual ~BtreeBase() { }

    virtual int version() const = 0;
    virtual void readheader() = 0;
    virtual Page_ptr  makepage(int nr) = 0;

    Page_ptr readpage(int nr)
    {
        auto page = makepage(nr);
        page->readindex();
        return page;
    }

    void dump()
    {
        print("btree v%02d ff=%d, pg=%d, root=%05x, #recs=%d #pgs=%d\n",
                version(), _firstfree, _pagesize, _firstindex, _reccount, _pagecount);

        dumptree(_firstindex);
    }
    void dumptree(int nr)
    {
        auto page = readpage(nr);
        page->dump();

        if (page->isindex()) {
            dumptree(page->getpage(-1));
            for (int i=0 ; i<page->indexsize() ; i++)
                dumptree(page->getpage(i));
        }
    }

    stream_ptr pagestream(int nr)
    {
        return std::make_shared<sectionstream>(_is, nr*_pagesize, _pagesize);
    }

    Cursor find(relation_t rel, const std::string& key)
    {
        auto page = readpage(_firstindex);

        Cursor cursor(this);
        relation_t act;
        while (true)
        {
            auto res = page->find(key);
            dbgprint("bt.find %d : %d\n", res.act, res.index);
            cursor.add(page, res.index);
            if (res.act != REL_RECURSE)
            {
                act = res.act;
                break;
            }
            page = readpage(page->getpage(res.index));
        }

        if (act == rel) {
            dbgprint("same -> pass\n");
            // pass
        }
        else if (rel==REL_EQUAL && act!=REL_EQUAL) {
            cursor.clear();
            dbgprint("not equal -> empty\n");
        }
        else if ((rel==REL_LESS_EQUAL || rel==REL_GREATER_EQUAL) && act==REL_EQUAL) {
            dbgprint("want: <=/>=, got: == -> pass\n");
            // pass
        }
        else if ((rel==REL_GREATER || rel==REL_GREATER_EQUAL) && act==REL_LESS) {
            dbgprint("want: >/>=, got: < -> next\n");
            cursor.next();
        }
        else if (rel==REL_GREATER  && act==REL_EQUAL) {
            dbgprint("want: >, got: == -> next\n");
            cursor.next();
        }
        else if ((rel==REL_LESS || rel==REL_LESS_EQUAL) && act==REL_GREATER) {
            dbgprint("want: </<=, got: > -> prev\n");
            cursor.prev();
        }
        else if (rel==REL_LESS  && act==REL_EQUAL) {
            dbgprint("want: <, got: == -> prev\n");
            cursor.prev();
        }

        return cursor;
    }
};
class Page15 : public BasePage {
public:
    Page15(stream_ptr  is, uint32_t nr, int pagesize)
        : BasePage(is, nr, pagesize)
    {
        auto s = makehelper(_is);
        _preceeding = s.get16le();
        _count = s.get16le();
    }

    virtual Entry readent()
    {
        auto s = makehelper(_is);
        if (isindex()) {
            Entry ent;
            ent.pagenr = s.get16le();
            ent.recofs = s.get16le()+1;
            dbgprint("@%04x: ix ent15 %08x %04x\n", (int)_is->tellg(), ent.pagenr, ent.recofs);
            return ent;
        }
        else if (isleaf()) {
            Entry ent;
            ent.indent = s.get8();
            /*ent.unknown = */s.get8();
            ent.recofs = s.get16le()+1;
            dbgprint("@%04x: lf ent15 %+4d %04x\n", (int)_is->tellg(), ent.indent, ent.recofs);
            return ent;
        }
        throw "page not a index or leaf";
    }
};

class Btree15 : public BtreeBase {
public:
    Btree15(stream_ptr  is) : BtreeBase(is) { }
    int version() const { return 15; }

    void readheader()
    {
        _is->seekg(0);
        auto s = makehelper(_is);
        _firstfree = s.get16le();
        _pagesize = s.get16le();
        _firstindex = s.get16le();
        _reccount = s.get32le();
        _pagecount = s.get16le();
    }

    virtual Page_ptr makepage(int nr) 
    {
        dbgprint("page15\n");
        return std::make_shared<Page15>(pagestream(nr), nr, _pagesize);
    }
};

class Page16 : public BasePage {
public:
    Page16(stream_ptr  is, uint32_t nr, int pagesize)
        : BasePage(is, nr, pagesize)
    {
        auto s = makehelper(_is);
        _preceeding = s.get32le();
        _count = s.get16le();
    }
    virtual Entry readent()
    {
        auto s = makehelper(_is);
        if (isindex()) {
            Entry ent;
            ent.pagenr = s.get32le();
            ent.recofs = s.get16le()+1;
            dbgprint("@%04x: ix ent16 %08x %04x\n", (int)_is->tellg(), ent.pagenr, ent.recofs);
            return ent;
        }
        else if (isleaf()) {
            Entry ent;
            ent.indent = s.get8();
            /*ent.unknown = */s.get8();
            /*ent.unknown1 = */s.get16le();
            ent.recofs = s.get16le()+1;
            dbgprint("@%04x: lf ent16 %+4d %04x\n", (int)_is->tellg(), ent.indent, ent.recofs);
            return ent;
        }
        throw "page not a index or leaf";
    }

};

class Btree16 : public BtreeBase {
public:
    Btree16(stream_ptr  is) : BtreeBase(is) { }
    int version() const { return 16; }

    void readheader()
    {
        _is->seekg(0);
        auto s = makehelper(_is);
        _firstfree = s.get32le();
        _pagesize = s.get16le();
        _firstindex = s.get32le();
        _reccount = s.get32le();
        _pagecount = s.get32le();
    }
    virtual Page_ptr makepage(int nr) 
    {
        dbgprint("page16\n");
        return std::make_shared<Page16>(pagestream(nr), nr, _pagesize);
    }

};
// v2 b-tree pages - since idav6.7
class Page20 : public Page16 {
public:
    Page20(stream_ptr  is, int nr, int pagesize)
        : Page16(is, nr, pagesize)
    {
    }
    virtual Entry readent()
    {
        auto s = makehelper(_is);
        if (isindex()) {
            Entry ent;
            ent.pagenr = s.get32le();
            ent.recofs = s.get16le();
            dbgprint("@%04x: ix ent20 %08x %04x\n", (int)_is->tellg(), ent.pagenr, ent.recofs);
            return ent;
        }
        else if (isleaf()) {
            Entry ent;
            ent.indent = s.get16le();
            /*ent.unknown = */s.get16le();
            ent.recofs = s.get16le();

            dbgprint("@%04x: lf ent20 %+4d %04x\n", (int)_is->tellg(), ent.indent, ent.recofs);
            return ent;
        }
        throw "page not a index or leaf";
    }

};
class Btree20 : public Btree16 {
public:
    Btree20(stream_ptr  is) : Btree16(is) { }
    int version() const { return 20; }
    virtual Page_ptr makepage(int nr) 
    {
        dbgprint("page20\n");
        return std::make_shared<Page20>(pagestream(nr), nr, _pagesize);
    }
};


// determine which btree type to create for the id0 stream.
inline std::unique_ptr<BtreeBase> MakeBTree(stream_ptr  is)
{
    std::unique_ptr<BtreeBase> bt;
    is->seekg(0);
    char data[64];
    is->read(data, 64);

    dbgprint("mkbt: %b\n", std::string(data, data+64));

    if (std::equal(data+13, data+13+25, "B-tree v 1.5 (C) Pol 1990")) {
        bt = std::make_unique<Btree15>(is);
    }
    else if (std::equal(data+19, data+19+25, "B-tree v 1.6 (C) Pol 1990")) {
        bt = std::make_unique<Btree16>(is);
    }
    else if (std::equal(data+19, data+19+9, "B-tree v2")) {
        bt = std::make_unique<Btree20>(is);
    }
    else {
        throw "unknown btree version";
    }

    bt->readheader();

    return bt;
}

// NodeKeys is used to create btree keys with the right format
// for the current database.
class NodeKeys {
    int _w;
public:
    NodeKeys(int wordsize)
        : _w(wordsize)
    {
    }


    template<typename P>
    void setwordle(P first, P last, uint64_t w)
    {
        if (_w==8)
            EndianTools::setle64(first, last, w);
        else if (_w==4)
            EndianTools::setle32(first, last, w);
    }
    template<typename P>
    void setwordbe(P first, P last, uint64_t w)
    {
        if (_w==8)
            EndianTools::setbe64(first, last, w);
        else if (_w==4)
            EndianTools::setbe32(first, last, w);
    }
    template<typename P>
    int make_name_key(P first, P last, uint64_t id)
    {
        if (last-first<1+_w)
            throw "not enough space";
        P p = first;
        *p++ = 'N';
        setwordbe(p, last, id); p += _w;

        return p-first;
    }
    template<typename P>
    int make_name_key(P first, P last, const std::string& name)
    {
        if (last-first<1+name.size())
            throw "not enough space";
        P p = first;
        *p++ = 'N';
        std::copy(name.begin(), name.end(), p);
        p += name.size();

        return p-first;
    }

    template<typename P>
    int make_node_key(P first, P last, uint64_t nodeid)
    {
        if (last-first<1+_w)
            throw "not enough space";
        P p = first;
        *p++ = '.';
        setwordbe(p, last, nodeid); p += _w;

        return p-first;
    }
    template<typename P>
    int make_node_key(P first, P last, uint64_t nodeid, char tag)
    {
        if (last-first<2+_w)
            throw "not enough space";
        P p = first;
        *p++ = '.';
        setwordbe(p, last, nodeid); p += _w;
        *p++ = tag;
        return p-first;
    }
    template<typename P>
    int make_node_key(P first, P last, uint64_t nodeid, char tag, const std::string& hashkey)
    {
        if (last-first<2+_w+hashkey.size())
            throw "not enough space";
        P p = first;
        *p++ = '.';
        setwordbe(p, last, nodeid); p += _w;
        *p++ = tag;  // usually 'H'
        std::copy(hashkey.begin(), hashkey.end(), p);
        p += hashkey.size();

        return p-first;
    }
    template<typename P, typename T>
    int make_node_key(P first, P last, uint64_t nodeid, char tag, T index)
    {
        if (last-first<2+2*_w)
            throw "not enough space";
        P p = first;
        *p++ = '.';
        setwordbe(p, last, nodeid); p += _w;
        *p++ = tag;
        setwordbe(p, last, index); p += _w;

        return p-first;
    }

    template<typename V>
    V make_name_key(uint64_t id)
    {
        V key;
        make_name_key(back_inserter(key), back_inserter(key), id);
        return key;
    }
    template<typename V>
    V make_name_key(const std::string& name)
    {
        V key;
        make_name_key(back_inserter(key), back_inserter(key), name);
        return key;
    }

    template<typename V>
    V make_node_key(uint64_t nodeid)
    {
        V key;
        make_node_key(back_inserter(key), back_inserter(key), nodeid);
        return key;
    }
    template<typename V>
    V make_node_key(uint64_t nodeid, char tag)
    {
        V key;
        make_node_key(back_inserter(key), back_inserter(key), nodeid, tag);
        return key;
    }
    template<typename V>
    V make_node_key(uint64_t nodeid, char tag, const std::string& hashkey)
    {
        V key;
        make_node_key(back_inserter(key), back_inserter(key), nodeid, tag, hashkey);
        return key;
    }
    template<typename V, typename T>
    V make_node_key(uint64_t nodeid, char tag, T index)
    {
        V key;
        make_node_key(back_inserter(key), back_inserter(key), nodeid, tag, index);
        return key;
    }
};

// convert node values to integer or string.
struct NodeValues {
    static uint64_t getuint(const std::string& str)
    {
        switch(str.size()) {
            case 1:
                return EndianTools::get8(str.begin(), str.end());
            case 2:
                return EndianTools::getle16(str.begin(), str.end());
            case 4:
                return EndianTools::getle32(str.begin(), str.end());
            case 8:
                return EndianTools::getle64(str.begin(), str.end());

        }
        throw "unsupported int type";
    }
    static uint64_t getuintbe(const std::string& str)
    {
        switch(str.size()) {
            case 1:
                return EndianTools::get8(str.begin(), str.end());
            case 2:
                return EndianTools::getbe16(str.begin(), str.end());
            case 4:
                return EndianTools::getbe32(str.begin(), str.end());
            case 8:
                return EndianTools::getbe64(str.begin(), str.end());

        }
        throw "unsupported int type";
    }
    static int64_t getint(const std::string& str)
    {
        return (int64_t)getuint(str);
    }
    static std::string getstr(std::string data)
    {
        // strip terminating zeroes
        while (!data.empty() && data.back()==0)
            data.pop_back();
        return data;
    }
};

// provide access to the main part of the IDApro database.
//
// use 'find', 'node' and 'blob' to access nodes in the database.
class ID0File {
    std::unique_ptr<BtreeBase> _bt;
    uint64_t _nodebase;
    int _wordsize;

public:
    enum { INDEX = 0 };  // argument for idb.getsection()

    ID0File(IDBFile& idb, stream_ptr  is)
        : _bt(MakeBTree(is))
    {
        if (idb.magic() == IDBFile::MAGIC_IDA2)
            _wordsize = 8;
        else
            _wordsize = 4;

        _nodebase = uint64_t(0xFF)<<((_wordsize-1)*8);
    }
    uint64_t nodebase() const { return _nodebase; }
    bool is64bit() const { return _wordsize==8; }
    void dump()
    {
        _bt->dump();
    }

    // function for creating a node key for the current database.
    template<typename...ARGS>
    std::string makekey(ARGS...args)
    {
        NodeKeys nk(_wordsize);
        return nk.make_node_key<std::string>(args...);
    }

    // function for creating a name key for the current database.
    template<typename...ARGS>
    std::string makename(ARGS...args)
    {
        NodeKeys nk(_wordsize);
        return nk.make_name_key<std::string>(args...);
    }

    // search for records in the current database by key.
    // relation gives the desired relation:
    // REL_LESS  : return records less than the key.
    //
    // returns a cursor object.
    auto find(relation_t rel, const std::string& key)
    {
        return _bt->find(rel, key);
    }

    // search for records in the current database.
    // relation gives the desired relation:
    // REL_LESS  : return records less than the key.
    //
    // returns a cursor object.
    template<typename...ARGS>
    auto find(relation_t rel, uint64_t nodeid, ARGS...args)
    {
        return _bt->find(rel, makekey(nodeid, args...));
    }

    // return a blob object as a string.
    std::string blob(uint64_t nodeid, char tag, uint64_t startid = 0, uint64_t lastid = 0xFFFFFFFF)
    {
        auto c = _bt->find(REL_GREATER_EQUAL, makekey(nodeid, tag, startid));
        auto endkey =  makekey(nodeid, tag, lastid);

        std::string blob;
        while (c.getkey() <= endkey) {
            blob += c.getval();
            c.next();
        }

        return blob;
    }

    // finds the nodeid by name.
    //
    // names can be labels like 'sub_1234', but also internal names like '$ structs', or 'Root Name'
    uint64_t node(const std::string& name)
    {
        auto c = _bt->find(REL_EQUAL, makename(name));
        if (c.eof())
            return 0;
        return NodeValues::getint(c.getval());
    }

    // callback is called for each nodeid in the list.
    // examples of lists: '$ structs', '$ enums'
    template<typename CB>
    void enumlist(uint64_t nodeid, char tag, CB cb)
    {
        auto c = _bt->find(REL_GREATER_EQUAL, makekey(nodeid, tag));
        auto endkey = makekey(nodeid, tag+1);
        while (c.getkey() <= endkey)
            cb(NodeValues::getint(c.getval()));
    }

    // 'easy' interface: return empty when record not found.
    // otherwise directly return the value.
    template<typename...ARGS>
    std::string getdata(ARGS...args)
    {
        auto c = find(REL_EQUAL, makekey(args...));
        if (c.eof())
            return {};
        return c.getval();
    }
    template<typename...ARGS>
    std::string getstr(ARGS...args)
    {
        // until ida6.7 strings were stored zero terminated.
        auto c = find(REL_EQUAL, makekey(args...));
        if (c.eof())
            return {};
        return NodeValues::getstr(c.getval());
    }
    template<typename...ARGS>
    uint64_t getuint(ARGS...args)
    {
        auto c = find(REL_EQUAL, makekey(args...));
        if (c.eof())
            return {};
        return NodeValues::getuint(c.getval());
    }
    uint64_t getuint(BtreeBase::Cursor& c)
    {
        return NodeValues::getuint(c.getval());
    }

    // returns the node name, resolves long names.
    std::string getname(uint64_t node)
    {
        auto c = find(REL_EQUAL, makekey(node, 'N'));
        if (c.eof())
            return {};
        auto val = c.getval();
        if (val.empty())
            return {};
        if (val[0]==0) {
            val.erase(val.begin());
            // bigname
            uint64_t nameid = NodeValues::getuintbe(val);
            val = blob(_nodebase, 'S', nameid*256, nameid*256+32);
        }
        return NodeValues::getstr(val);
    }
};

#ifndef BADADDR 
#define BADADDR uint64_t(-1)
#endif

// the ID1File contains information on segments, and stores the flags for each byte.
// basically this is the data for the idc GetFlags(ea)  function.
class ID1File {
    struct segment {
        uint32_t start_ea;
        uint32_t end_ea;
        uint32_t id1ofs;
    };
    typedef std::vector<segment> segmentlist_t;
    segmentlist_t _segments;

    stream_ptr _is;
    int _wordsize;

private:

    void open()
    {
        auto s = makehelper(_is, _wordsize);
        s.seekg(0);
        uint32_t magic = s.get32le();
        if ((magic&0xFFF0FFFF)==0x306156) { // Va0 .. Va4
            uint16_t nsegments = s.get16le();
            uint16_t npages    = s.get16le();
            (void)npages; // value not used

            _segments.resize(nsegments);
            for (unsigned i=0 ; i<nsegments ; i++) {
                _segments[i].start_ea = s.getword();
                _segments[i].end_ea   = s.getword();
                _segments[i].id1ofs   = s.getword();
            }
        }
        else if (magic==0x2a4156) {
            uint32_t unk1      = s.get32le();      // 3
            uint32_t nsegments = s.get32le(); 
            uint32_t unk2      = s.get32le();      // 0x800 -- # addrs per page?
            uint32_t npages    = s.get32le();
            (void)unk1; (void)unk2;  (void)npages;  // values not used

            _segments.resize(nsegments);
            uint32_t ofs= 0x2000;
            for (unsigned i=0 ; i<nsegments ; i++) {
                auto &seg= _segments[i];
                seg.start_ea = s.getword();
                seg.end_ea   = s.getword();
                seg.id1ofs= ofs;

                ofs += 4*(seg.end_ea-seg.start_ea);
            }
        }
        else {
            throw "invalid id1";
        }
    }
    segmentlist_t::const_iterator find_segment(uint64_t ea) const
    {
        for (segmentlist_t::const_iterator i=_segments.begin() ; i!=_segments.end() ; i++)
        {
            if ((*i).start_ea <= ea && ea<(*i).end_ea)
                return i;
        }
        return _segments.end();
    }

public:
    enum { INDEX = 1 };  // argument for idb.getsection()

    ID1File(IDBFile& idb, stream_ptr  is)
        : _is(is)
    {
        if (idb.magic() == IDBFile::MAGIC_IDA2)
            _wordsize = 8;
        else
            _wordsize = 4;

        open();
    }
    void dump_info()
    {
        print("id1: %d segments\n", _segments.size());
        for (unsigned i=0 ; i<_segments.size() ; i++)
            print("  %08x-%08x @ %08x\n", _segments[i].start_ea, _segments[i].end_ea, _segments[i].id1ofs);
    }

    uint32_t GetFlags(uint64_t ea) const
    {
        // for optimization maybe i need to implement some kind of flags page caching
        segmentlist_t::const_iterator i= find_segment(ea);
        if (i==_segments.end())
            return 0;

        auto s = makehelper(_is, _wordsize);
        s.seekg((*i).id1ofs+4*(ea-(*i).start_ea));

        return s.get32le();
    }
    uint64_t FirstSeg()
    {
        if (_segments.empty())
            return BADADDR;
        return _segments.front().start_ea;
    }
    uint64_t NextSeg(uint64_t ea)
    {
        segmentlist_t::const_iterator i= find_segment(ea);
        if (i==_segments.end())
            return BADADDR;
        i++;
        if (i==_segments.end())
            return BADADDR;
        return (*i).start_ea;
    }
    uint64_t SegStart(uint64_t ea)
    {
        segmentlist_t::const_iterator i= find_segment(ea);
        if (i==_segments.end())
            return BADADDR;
        return (*i).start_ea;
    }
    uint64_t SegEnd(uint64_t ea)
    {
        segmentlist_t::const_iterator i= find_segment(ea);
        if (i==_segments.end())
            return BADADDR;
        return (*i).end_ea;
    }
};

// NAMFile keeps a list of named items.
class NAMFile {
private:
    mutable std::vector<uint64_t> _namedoffsets;
    mutable bool _namesloaded;

    stream_ptr _is;
    int _wordsize;

    uint32_t _nnames;
    uint64_t _listofs;
public:
    enum { INDEX = 2 };  // argument for idb.getsection()

    NAMFile(IDBFile& idb, stream_ptr  is)
        : _namesloaded(false), _is(is)
    {
        if (idb.magic() == IDBFile::MAGIC_IDA2)
            _wordsize = 8;
        else
            _wordsize = 4;


        open();
    }

    void open()
    {
        auto s = makehelper(_is, _wordsize);
        s.seekg(0);
        uint32_t magic = s.get32le();

        if ((magic&0xFFF0FFFF)==0x306156) { // Va0 .. Va4
            uint16_t npages = s.get16le();  // nr of chunks
            uint16_t eof = s.get16le();
            uint64_t unknown = s.getword();
            (void)npages; (void)eof; // value not used

            _nnames = s.getword();  // nr of names
            _listofs = s.getword(); // page size

            dbgprint("nam: np=%d, eof=%d, nn=%d, ofs=%08x\n",
                    npages, eof, _nnames, _listofs);
            if (unknown)
                print("!! nam.unknown=%08x\n", unknown);
        }
        else if (magic==0x2a4156) {
            uint32_t unk1 = s.get32le();    // 3
            uint32_t npages = s.get32le();  // nr of chunks
            uint32_t unk2 = s.get32le();    // 0x800
            uint32_t eof = s.get32le();     // 0x15
            uint64_t unknown = s.getword(); // 0
            (void)unk1; (void)npages;  (void)unk2; (void)eof; (void)unknown; // values not used

            _listofs = 0x2000;
            _nnames = s.getword();  // nr of names

            dbgprint("nam: np=%d, eof=%d, nn=%d, ofs=%08x\n",
                    npages, eof, _nnames, _listofs);
        }
        else {
            throw "invalid NAM";
        }

        if (_wordsize==8)
            _nnames /= 2;
    }
    void loadoffsets() const
    {
        if (_namesloaded)
            return;
        _namedoffsets.reserve(_nnames);

        auto s = makehelper(_is, _wordsize);
        s.seekg(_listofs);
        for (unsigned i=0 ; i<_nnames ; i++)
            _namedoffsets.push_back(s.getword());

        _namesloaded = true;
    }
    int numnames() const
    {
        loadoffsets();
        return _namedoffsets.size();
    }
    template<typename FN>
    void enumerate(FN fn) const
    {
        loadoffsets();
        std::for_each(_namedoffsets.begin(), _namedoffsets.end(), fn);
    }
    // finds nearest named item
    uint64_t findname(uint64_t ea) const
    {
        loadoffsets();
        if (_namedoffsets.empty())
            return BADADDR;
        auto i= std::upper_bound(_namedoffsets.begin(), _namedoffsets.end(), ea);
        if (i==_namedoffsets.begin()) {
            // address before first: return first named item
            return *i;
        }
        i--;
        return *i;
    }

    uint64_t firstnamed() const
    {
        loadoffsets();
        if (_namedoffsets.empty())
            return BADADDR;
        return *_namedoffsets.begin();
    }
};

// packs/unpacks structured data
class BaseUnpacker  {
public:
    virtual bool eof() const = 0;
    virtual uint32_t next32() = 0;
    virtual uint64_t nextword() = 0;
    virtual ~BaseUnpacker() { }
};
template<typename P>
class Unpacker : public BaseUnpacker {
    P _p;
    P _last;
    bool _use64;
    public:

    Unpacker(P first, P last, bool use64)
        : _p(first), _last(last), _use64(use64)
    {
    }
    bool eof() const
    {
        return _p>=_last;
    }

    /*
     *  1 byte - values 0 .. 0x7f
     *  2 byte - values 0x80 .. 0x3fff,  orred with 0x8000
     *  4 byte - values 0x4000 .. 0x1fffffff, orred with 0xc0000000
     *
     *  0xFF + 4 byte - any 32 bit val  - in .idb
     *
     *  in .i64, 64 bit values are encoded as the low 32bit value followed by high
     *  32 bit value using any of the above encodings. So the byte order is kind of twisted.
     *
     */
    uint32_t next32()
    {
        if (_p>=_last)  throw "unpack: no data";
        uint8_t byte = *_p++;
        if (byte==0xff) {
            if (_p+4>_last)  throw "unpack: no data";
            uint32_t value = EndianTools::getbe32(_p, _last);
            _p += 4;
            return value;
        }
        if (byte<0x80) {
            return byte;
        }
        _p--;
        if (byte<0xc0) {
            if (_p+2>_last)  throw "unpack: no data";
            uint32_t value = EndianTools::getbe16(_p, _last) & 0x3FFF;  _p += 2;
            return value;
        }
        if (_p+4>_last)  throw "unpack: no data";
        uint32_t value = EndianTools::getbe32(_p, _last) & 0x1FFFFFFF;  _p += 4;
        return value;

    }
    uint64_t nextword()
    {
        uint64_t lo = next32();
        if (_use64)
        {
            uint64_t hi = next32();
            return lo|(hi<<32);
        }
        return lo;
    }

};
template<typename I, typename O>
static void idaunpack(I first, I last, O out)
{
    Unpacker<I> p(first, last, false);
    
    while (!p.eof())
        *out++ = p.next32();
}

template<typename P>
Unpacker<P> makeunpacker(P first, P last, bool use64)
{
    return Unpacker<P>(first, last, use64);
}

typedef std::vector<uint32_t> DwordVector;

// used mostly in lists, where the stored value is one less than the actually used value.
// lists like: $enums, $structs, $scripts, values of enums, masks of bitfields, values of bitmasks
// backref of bitfield value to mask.
inline uint64_t minusone(uint64_t id)
{
    if (id) return id-1;
    return 0;
}

// 'd'  xref-from  -> points to used type
// 'D'  xref-to    -> points to type users

class StructMember {
    ID0File& _id0;
    uint64_t _nodeid;
    uint64_t _skip;  // nr of bytes to skip before this member
    uint64_t _size;  // size in bytes of this member
    uint32_t _flags;
    uint32_t _props;
    uint64_t _ofs;
public:
    StructMember(ID0File& id0, BaseUnpacker& spec)
        : _id0(id0)
    {
        _nodeid = spec.nextword();
        _skip = spec.nextword();
        _size = spec.nextword();
        _flags = spec.next32();
        _props = spec.next32();
        _ofs = 0;
    }
    void setofs(uint64_t ofs) { _ofs = ofs; }

    uint64_t nodeid() const { return _nodeid + _id0.nodebase(); }
    uint64_t skip() const { return _skip; }
    uint64_t size() const { return _size; }
    uint32_t flags() const { return _flags; }
    uint32_t props() const { return _props; }
    uint64_t offset() const { return _ofs; }

    std::string name() const { return _id0.getname(nodeid()); }

    uint64_t enumid() const { return minusone(_id0.getuint(nodeid(), 'A', 11)); }
    uint64_t structid() const { return minusone(_id0.getuint(nodeid(), 'A', 3)); }
    std::string comment(bool repeatable) const { return _id0.getstr(nodeid(), 'S', repeatable ? 1 : 0); }
    std::string ptrinfo() const { return _id0.getdata(nodeid(), 'S', 9); }

    // types from typeinfo:
        // 11 00  _BYTE
        // 32 00  char
        // 22 00  unsigned __int8
        // 02 00  __int8
        // ='WORD"  WORD
        // 03 00  short
        // 07 00  int
        // 03 00  __int16
        // 28 00  _BOOL2
        // 10 00  _WORD
    std::string typeinfo() const { return _id0.getdata(nodeid(), 'S', 0x3000); }
};

// access structs and struct members.
class Struct {
    ID0File& _id0;
    uint64_t _nodeid;

    uint32_t _flags;
    std::vector<StructMember> _members;
    uint32_t _seqnr;

    class Iterator : public std::iterator<std::random_access_iterator_tag, StructMember> {
        const Struct* _s;
        int _ix;
    public:
        Iterator(const Struct*s, int ix) : _s(s), _ix(ix) { }
        Iterator() : _s(nullptr), _ix(0) { }
        Iterator(const Iterator& i) : _s(i._s), _ix(i._ix) { }

        bool operator==(const Iterator& rhs) {return _ix==rhs._ix;}
        bool operator!=(const Iterator& rhs) {return _ix!=rhs._ix;}

        const StructMember& operator*() {return _s->member(_ix);}
        const StructMember& operator[](int i) {return _s->member(_ix+i);}

        Iterator& operator++() {++_ix;return *this;}
        Iterator operator++(int) {Iterator tmp(*this); operator++(); return tmp;}
        Iterator& operator--() {--_ix;return *this;}
        Iterator operator--(int) {Iterator tmp(*this); operator--(); return tmp;}

        Iterator& operator+=(int n) {  _ix += n; return *this; }
        Iterator& operator-=(int n) {  _ix -= n; return *this; }

        friend Iterator operator+(int n, Iterator p) {  return p+=n; }
        friend Iterator operator+(Iterator p, int n) {  return p+=n; }
        friend Iterator operator-(Iterator p, int n) {  return p-=n; }
        friend int operator-(const Iterator& p, const Iterator& q) {  return p._ix-q._ix; }

        bool operator<(const Iterator& rhs) { return _ix<rhs._ix; }
        bool operator<=(const Iterator& rhs) { return _ix<=rhs._ix; }
        bool operator>(const Iterator& rhs) { return _ix>rhs._ix; }
        bool operator>=(const Iterator& rhs) { return _ix>=rhs._ix; }
    };

public:
    Struct(ID0File& id0, uint64_t nodeid)
        : _id0(id0), _nodeid(nodeid)
    {
        auto spec = _id0.blob(_nodeid, 'M');
        auto p = makeunpacker(spec.begin(), spec.end(), _id0.is64bit());

        _flags = p.next32();
        uint32_t nmember = p.next32();
        uint64_t ofs = 0;
        while (nmember--) {
            _members.emplace_back(_id0, p);
            ofs += _members.back().skip();
            _members.back().setofs(ofs);
            ofs += _members.back().size();
        }
        if (!p.eof())
            _seqnr = p.next32();
        else
            _seqnr = 0;
    }
    std::string name() const { return _id0.getname(_nodeid); }
    std::string comment(bool repeatable) const { return _id0.getstr(_nodeid, 'S', repeatable ? 1 : 0); }
    int nmembers() const { return _members.size(); }
    uint32_t flags() const { return _flags; }
    uint32_t seqnr() const { return _seqnr; }

    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, nmembers()); }
    const StructMember& member(int ix) const { return _members[ix]; }


};

class EnumMember {
    ID0File& _id0;
    uint64_t _nodeid;
    uint64_t _value;
public:
    EnumMember(ID0File& id0, uint64_t nodeid)
        : _id0(id0), _nodeid(nodeid)
    {
        _value = _id0.getuint(_nodeid, 'A', -3);
    }

    // 'A', -2  -> points to enum node
    uint64_t nodeid() const { return _nodeid; }
    uint64_t value() const { return _value; }
    std::string name() const { return _id0.getname(nodeid()); }

    std::string comment(bool repeatable) const { return _id0.getstr(nodeid(), 'S', repeatable ? 1 : 0); }

};
// get properties of an enum.
// bitfields and enums are both in the '$ enums' list.
class Enum {
    ID0File& _id0;
    uint64_t _nodeid;

public:
    Enum(ID0File& id0, uint64_t nodeid)
        : _id0(id0), _nodeid(nodeid)
    {
    }
    uint64_t nodeid() const { return _nodeid; }
    uint64_t count() const { return _id0.getuint(_nodeid, 'A', -1); }

    // >>20 : 0x11=hex, 0x22=dec, 0x77=oct, 0x66=bin, 0x33=char
    // values: FF_0NUMx|FF_1NUMx   x=H,D,O,B,CHAR
    //
    // >>16 : 0x2  = signed : FF_SIGN
    uint32_t representation() const { return _id0.getuint(_nodeid, 'A', -3); }

    // bit0 = bitfield  ENUM_FLAGS_IS_BF
    // bit1 = hidden    ENUM_FLAGS_HIDDEN   
    // bit2 = fromtil   ENUM_FLAGS_FROMTIL
    // bit5-3 =  width 0..7 = (0,1,2,4,8,16,32,64)
    // bit6 = ghost     ENUM_FLAGS_GHOST
    uint32_t flags() const { return _id0.getuint(_nodeid, 'A', -5); }

    // 'A',-8  -> index in $enums list

    std::string name() const { return _id0.getname(_nodeid); }
    std::string comment(bool repeatable) const { return _id0.getstr(_nodeid, 'S', repeatable ? 1 : 0); }

    auto first() const { return _id0.find(REL_GREATER_EQUAL, _id0.makekey(_nodeid, 'E')); }
    std::string lastkey() const { return _id0.makekey(_nodeid, 'F'); }

    EnumMember getvalue(BtreeBase::Cursor& c) const
    {
        return EnumMember(_id0, minusone(_id0.getuint(c)));
    }
};

class BitfieldValue {
    ID0File& _id0;
    uint64_t _nodeid;
    uint64_t _value;
    uint64_t _mask;
public:
    BitfieldValue(ID0File& id0, uint64_t nodeid)
        : _id0(id0), _nodeid(nodeid)
    {
        _value = _id0.getuint(_nodeid, 'A', -3);
        _mask = _id0.getuint(_nodeid, 'A', -6) - 1;
    }

    uint64_t nodeid() const { return _nodeid; }
    std::string name() const { return _id0.getname(nodeid()); }

    std::string comment(bool repeatable) const { return _id0.getstr(nodeid(), 'S', repeatable ? 1 : 0); }
    uint64_t value() const { return _value; }
    uint64_t mask() const { return _mask; }

    // 'A', -2  -> points to enum node
    // 'A', -6  -> minusone -> maskid from : (enum, 'm', maskid)
};
class BitfieldMask {
    ID0File& _id0;
    uint64_t _nodeid;
    uint64_t _mask;

public:
    BitfieldMask(ID0File& id0, uint64_t nodeid, uint64_t mask)
        : _id0(id0), _nodeid(nodeid), _mask(mask)
    {
    }
//  BitfieldMask(const BitfieldMask& bf)
//      : _id0(bf._id0), _nodeid(bf._nodeid), _mask(bf._mask)
//  {
//  }

    uint64_t nodeid() const { return _nodeid; }
    std::string name() const { return _id0.getname(nodeid()); }
    std::string comment(bool repeatable) const { return _id0.getstr(nodeid(), 'S', repeatable ? 1 : 0); }

    uint64_t mask() const { return _mask; }

    auto first() const { return _id0.find(REL_GREATER_EQUAL, _id0.makekey(_nodeid, 'E')); }
    std::string lastkey() const { return _id0.makekey(_nodeid, 'F'); }

    BitfieldValue getvalue(BtreeBase::Cursor& c) const
    {
        return BitfieldValue(_id0, minusone(_id0.getuint(c)));
    }
};

// get properties of a bitfield.
// bitfields and enums are both in the '$ enums' list.
class Bitfield {
    ID0File& _id0;
    uint64_t _nodeid;

public:
    Bitfield(ID0File& id0, uint64_t nodeid)
        : _id0(id0), _nodeid(nodeid)
    {
    }
    uint64_t count() const { return _id0.getuint(_nodeid, 'A', -1); }

    // >>20 : 0x11=hex, 0x22=dec, 0x77=oct, 0x66=bin, 0x33=char
    // values: FF_0NUMx|FF_1NUMx   x=H,D,O,B,CHAR
    //
    // >>16 : 0x2  = signed : FF_SIGN
    uint32_t representation() const { return _id0.getuint(_nodeid, 'A', -3); }

    // bit0 = bitfield  ENUM_FLAGS_IS_BF
    // bit1 = hidden    ENUM_FLAGS_HIDDEN   
    // bit2 = fromtil   ENUM_FLAGS_FROMTIL
    // bit5-3 =  width 0..7 = (0,1,2,4,8,16,32,64)
    // bit6 = ghost     ENUM_FLAGS_GHOST
    uint32_t flags() const { return _id0.getuint(_nodeid, 'A', -5); }

    // 'A',-8  -> index in $enums list

    std::string name() const { return _id0.getname(_nodeid); }
    std::string comment(bool repeatable) const { return _id0.getstr(_nodeid, 'S', repeatable ? 1 : 0); }

    // for bitmasks there is an extra level: 'm'  in between.
    auto first() const { return _id0.find(REL_GREATER_EQUAL, _id0.makekey(_nodeid, 'm')); }
    std::string lastkey() const { return _id0.makekey(_nodeid, 'n'); }

    BitfieldMask getmask(BtreeBase::Cursor& c)
    {
        auto key = c.getkey();
        uint64_t mask;

        // get the mask from the key index,
        // ... not really nescesary, since we can also get the mask
        // from the BitfieldValue : node('A',-6) - 1
        if (_id0.is64bit() ) {
            assert(key.size()==18);
            mask = EndianTools::getbe64(&key[10], &key[10]+8);
        }
        else {
            assert(key.size()==10);
            mask = EndianTools::getbe32(&key[6], &key[6]+4);
        }

        return BitfieldMask(_id0, minusone(_id0.getuint(c)), mask);
    }

};

// access scripts by nodeid,
// use name(), language() and body()  to access
// the contents of the script.
class Script {
    ID0File& _id0;
    uint64_t _nodeid;

public:
    Script(ID0File& id0, uint64_t nodeid)
        : _id0(id0), _nodeid(nodeid)
    {
    }
    std::string name() const { return _id0.getstr(_nodeid, 'S', 0); }
    std::string language() const { return _id0.getstr(_nodeid, 'S', 1); }
    std::string body() const { return NodeValues::getstr(_id0.blob(_nodeid, 'X')); }
};
template<typename T>
class List {
    ID0File& _id0;
    BtreeBase::Cursor _c;
    std::string _endkey;
public:
    List(ID0File& id0, uint64_t nodeid)
        : _id0(id0), _c(_id0.find(REL_GREATER, _id0.makekey(nodeid, 'A')))
    {
        _endkey = _id0.makekey(nodeid, 'A', -1);
    }
    bool eof() const { return !(_c.getkey() < _endkey); }
    T next() 
    { 
        uint64_t id = minusone(_id0.getuint(_c));
        _c.next();
        return T(_id0, id);
    }
};
