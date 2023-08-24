#include "unittestframework.h"

#include <climits>
#include <idblib/idb3.h>

std::string CreateTestIndexPage(int pagesize)
{
    std::string page(pagesize, char(0));

    auto  oi = page.begin();
    auto  ei = page.begin() + pagesize/2;

    auto  od = page.begin() + pagesize/2;
    auto  ed = page.end();

    auto et = EndianTools();
    et.setle32(oi, ei, 122); oi += 4;
    et.setle16(oi, ei, 3); oi += 2;

    auto addkv = [&](const std::string& key, const std::string& val, int pagenr) {
        et.setle32(oi, ei, pagenr); oi += 4;
        et.setle16(oi, ei, od-page.begin()); oi += 2;

        et.setle16(od, ed, key.size()); od += 2;
        std::copy(key.begin(), key.end(), od);  od += key.size();
        et.setle16(od, ed, val.size()); od += 2;
        std::copy(val.begin(), val.end(), od);  od += val.size();
    };
    addkv("Nabcde", std::string{"\x01\x00\x00\xFF",4}, 123);
    addkv("Nbcdef", std::string{"\x02\x00\x00\xFF",4}, 125);
    addkv("Ncdef",  std::string{"\x03\x00\x00\xFF",4}, 127);

    return page;
}

TEST_CASE("TestIndexPage") {
    auto tstdata = CreateTestIndexPage(2048);
    auto page = std::make_unique<Page20>(std::make_shared<std::stringstream>(tstdata), 1, 2048);
    page->readindex();

    CHECK( page->isindex() == true );
    CHECK( page->isleaf() == false );

    CHECK( page->getpage(-1) == 122 );
    CHECK( page->getpage(0) == 123 );
    CHECK( page->getpage(1) == 125 );
    CHECK( page->getpage(2) == 127 );

    CHECK_THROWS( page->getpage(3) );

    CHECK( page->getkey(0) == "Nabcde" );
    CHECK( page->getkey(1) == "Nbcdef" );
    CHECK( page->getkey(2) == "Ncdef" );

    CHECK_THROWS( page->getkey(3) );

    CHECK_FALSE(page->getval(0) == std::string{"fail"});

    CHECK( page->getval(0) == (std::string{"\x01\x00\x00\xFF",4}) );
    CHECK( page->getval(1) == (std::string{"\x02\x00\x00\xFF",4}) );
    CHECK( page->getval(2) == (std::string{"\x03\x00\x00\xFF",4}) );

    CHECK_FALSE(page->find("fail") == (BasePage::result{REL_EQUAL,2}));

    CHECK( page->find("N") == (BasePage::result{REL_RECURSE,-1}) );
    CHECK( page->find("Nabcde") == (BasePage::result{REL_EQUAL,0}) );
    CHECK( page->find("Nbcdef") == (BasePage::result{REL_EQUAL,1}) );
    CHECK( page->find("Nbzzzz") == (BasePage::result{REL_RECURSE,1}) );
    CHECK( page->find("Ncdef") == (BasePage::result{REL_EQUAL,2}) );
    CHECK( page->find("Nzzzz") == (BasePage::result{REL_RECURSE,2}) );
}
std::string CreateTestLeafPage(int pagesize)
{
    std::string page(pagesize, char(0));

    auto  oi = page.begin();
    auto  ei = page.begin() + pagesize/2;

    auto  od = page.begin() + pagesize/2;
    auto  ed = page.end();

    auto et = EndianTools();
    et.setle32(oi, ei, 0); oi += 4;
    et.setle16(oi, ei, 3); oi += 2;

    auto addkv = [&](const std::string& key, const std::string& val, int indent) {
        et.setle32(oi, ei, indent); oi += 4;
        et.setle16(oi, ei, od-page.begin()); oi += 2;

        et.setle16(od, ed, key.size()-indent); od += 2;
        std::copy(key.begin()+indent, key.end(), od);  od += key.size()-indent;
        et.setle16(od, ed, val.size()); od += 2;
        std::copy(val.begin(), val.end(), od);  od += val.size();
    };
    addkv("Nabcde", std::string{"\x01\x00\x00\xFF",4}, 0);
    addkv("Nbcdef", std::string{"\x02\x00\x00\xFF",4}, 1);
    addkv("Ncdef",  std::string{"\x03\x00\x00\xFF",4}, 1);

    return page;
}
void TestLeafPage()
{
    auto tstdata = CreateTestLeafPage(2048);
    auto page = std::make_unique<Page20>(std::make_shared<std::stringstream>(tstdata), 1, 2048);
    page->readindex();

    CHECK( page->isindex() == false );
    CHECK( page->isleaf() == true );

    CHECK_THROWS( page->getpage(0) );

    CHECK( page->getkey(0) == "Nabcde" );
    CHECK( page->getkey(1) == "Nbcdef" );
    CHECK( page->getkey(2) == "Ncdef" );

    CHECK_THROWS( page->getkey(3) );

    CHECK_FALSE( page->getval(0) == std::string{"fail"} );

    CHECK( page->getval(0) == (std::string{"\x01\x00\x00\xFF",4}) );
    CHECK( page->getval(1) == (std::string{"\x02\x00\x00\xFF",4}) );
    CHECK( page->getval(2) == (std::string{"\x03\x00\x00\xFF",4}) );

    CHECK_FALSE( page->find("fail") == (BasePage::result{REL_EQUAL,2}) );

    CHECK( page->find("N") == (BasePage::result{REL_GREATER,0}) );
    CHECK( page->find("Nabcde") == (BasePage::result{REL_EQUAL,0}) );
    CHECK( page->find("Nbcdef") == (BasePage::result{REL_EQUAL,1}) );
    CHECK( page->find("Nbzzzz") == (BasePage::result{REL_LESS,1}) );
    CHECK( page->find("Ncdef") == (BasePage::result{REL_EQUAL,2}) );
    CHECK( page->find("Nzzzz") == (BasePage::result{REL_LESS,2}) );
}
/* streamhelper unittest */
TEST_CASE("test_streamhelper")
{
    auto f = makehelper(std::make_shared<std::stringstream>("3456789a"));

    // read various chunks of the stream.
    CHECK( f.getdata(3) == "345" );
    CHECK( f.getdata(8) == "6789a" );
    CHECK( f.getdata(8) == "" );
    f.seekg(-1, std::ios_base::end);
    CHECK( f.getdata(8) == "a" );
    f.seekg(3);
    CHECK( f.getdata(2) == "67" );
    f.seekg(-2,std::ios_base::cur);
    CHECK( f.getdata(2) == "67" );
    f.seekg(2,std::ios_base::cur);
    CHECK( f.getdata(2) == "a" );

    f.seekg(0);
    CHECK( f.get32le() == 0x36353433 );
    CHECK( f.get32be() == 0x37383961 );

    // seek to end of stream
    f.seekg(8);

    // read at EOF should return an empty string.
    CHECK( f.getdata(1) == "" );

    // seek beyond end of stream should throw an exception
    CHECK_THROWS( f.seekg(9) );
    CHECK_THROWS( f.getdata(1) );
}
/* unittest for EndianTools */
TEST_CASE("test_EndianTools")
{
    EndianTools et;

    uint8_t b[64];
    int i;
    for (i=0 ; i<64 ; i++)
        b[i] = 0xAA;

    CHECK_THROWS( et.setle64(b, b+7, 0x12345678LL) );

    et.setle64(b, b+8, 0x9abcdef12345678LL);

    uint8_t little_endian_number[9] = { 0x78, 0x56, 0x34, 0x12, 0xef, 0xcd, 0xab, 0x09, 0xAA };
    CHECK(std::equal(b, b+9, little_endian_number));

    CHECK_THROWS( et.getle64(b, b+7) );
    CHECK(et.getle64(b, b+8)==0x9abcdef12345678LL);

    et.setbe64(b, b+8, 0x9abcdef12345678LL);

    uint8_t big_endian_number[9] = { 0x09,0xab,0xcd,0xef,0x12,0x34,0x56,0x78,0xAA };
    CHECK(std::equal(b, b+9, big_endian_number));

    CHECK_THROWS( et.getbe64(b, b+7) );
    CHECK(et.getbe64(b, b+8)==0x9abcdef12345678LL);
}
/* unittest for sectionstream */
TEST_CASE("test_StreamSection")
{
    auto f = makehelper(std::make_shared<sectionstream>(std::make_shared<std::stringstream>("0123456789abcdef"), 3, 8));

    CHECK( f.getdata(3) == "345" );   // should return what was asked for.
    CHECK( f.getdata(8) == "6789a" ); // should return less than requested
    CHECK( f.getdata(8) == "" );      // should return nothing
    f.seekg(-1, std::ios_base::end);
    CHECK( f.getdata(8) == "a" );
    f.seekg(3);
    CHECK( f.getdata(2) == "67" );
    f.seekg(-2,std::ios_base::cur);
    CHECK( f.getdata(2) == "67" );
    f.seekg(2,std::ios_base::cur);
    CHECK( f.getdata(2) == "a" );

    f.seekg(8);
    CHECK( f.getdata(1) == "" );
    //CHECK_THROWS( f.seekg(9) );
}

TEST_CASE("test_NodeValues")
{
    CHECK( NodeValues::getuint("\x12\x34\x45\x56\x67\x78\x89\x9a") == 0x9a89786756453412 );
    CHECK( NodeValues::getuint("\x12\x34\x45\x56") == 0x56453412 );
    CHECK( NodeValues::getuint("\x12\x34") == 0x3412 );
    CHECK( NodeValues::getuint("\x12") == 0x12 );
}

TEST_CASE("test_Packer")
{
    std::string val("\x00\x04\x88\xf1\x00\x04\xc0\x20\x00\x04\x01\x88\xf2\x00\x04\xc0\x20\x00\x04\x01\x88\xf3\x00\x04\xc0\x25\x50\x04\x11\x88\xf4\x00\x04\xc0\x25\x50\x04\x11\x02", 39);

    DwordVector nums;
    idaunpack(val.begin(), val.end(), std::back_inserter(nums));

    DwordVector wrong = { 0x00 };
    CHECK( nums != wrong );
    CHECK_FALSE( nums == wrong );

    DwordVector check = { 0x00,0x04,0x8f1,0x00,0x04,0x0200004,0x01,0x8f2,0x00,0x04,0x0200004,0x01,0x8f3,0x00,0x04,0x0255004,0x11,0x8f4,0x00,0x04,0x0255004,0x11,0x02 };

    CHECK(nums == check);
}


