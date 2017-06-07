#include "util/rw/FileReader.h"
#include "util/rw/CompressedReader.h"
class SIGFile {
    ReadWriter_ptr _file;
    ReadWriter_ptr _rd;
    SIGFile(const std::string& filename)
        : _file(ReadWriter_ptr(new FileReader(filename)))
    {
        std::string magic= _file->readstring(6);
        if (magic != "IDASIG")
            throw "not a .sig file";
        uint8_t comptype= _file->read8();
        // 5 -> ?
        // 6 -> ?
        // 7 -> deflated
        uint8_t unknown07= _file->read8();
        uint64_t bitmask= _file->read64le();
        ByteVector unknown10(18);
        _file->read(&unknown10[0], unknown10.size());
        uint16_t desclen= _file->read16le();
        uint8_t unknown24= _file->read8();
        uint32_t unknown25= _file->read32le();
        std::string desc= _file->readstring(desclen);
        _rd= ReadWriter_ptr(new CompressedReader(_file, 0x29+desclen));
    }
};
int main(int argc, char**argv)
{
    if (argc<2) {
        printf("Usage: dumpsig <file>\n");
        return 1;
    }
    SIGFile sig(argv[1]);

    sig.dump();

    return 0;
}
