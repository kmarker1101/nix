#include "builtins.hh"
#include "compression.hh"
#include "tarfile.hh"

namespace nix {

void builtinUnpackChannel(const BasicDerivation & drv)
{
    auto getAttr = [&](const string & name) {
        auto i = drv.env.find(name);
        if (i == drv.env.end()) throw Error("attribute '%s' missing", name);
        return i->second;
    };

    Path out = getAttr("out");
    auto channelName = getAttr("channelName");
    auto src = getAttr("src");

    createDirs(out);

    auto source = sinkToSource([&](Sink & sink) {
        auto decompressor =
            hasSuffix(src, ".bz2") ? makeDecompressionSink("bzip2", sink) :
            hasSuffix(src, ".xz") ? makeDecompressionSink("xz", sink) :
            makeDecompressionSink("none", sink);
        readFile(src, *decompressor);
        decompressor->finish();
    });

    unpackTarfile(*source, out);

    auto entries = readDirectory(out);
    if (entries.size() != 1)
        throw Error("channel tarball '%s' contains more than one file", src);
    if (rename((out + "/" + entries[0].name).c_str(), (out + "/" + channelName).c_str()) == -1)
        throw SysError("renaming channel directory");
}

}
