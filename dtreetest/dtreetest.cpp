#include <getopt.h>

#include <dtreetest/dtreetest.h>
#include <libfrugi/Settings.h>

//#include "wrappers.h"
#include <dtree/dtree.h>

using namespace libfrugi;

void runTest(std::string const& name) {

    Settings& settings = Settings::global();

    if(name == "dtree.s") {
//        dtreeTest<dtree<SingleLevelhashSet<HashSet<RehasherExit, Linear> > > > (settings["buckets_scale"].asUnsignedValue()).go();
    } else if(name == "dtree.m") {
//        dtreeTest<dtree<MultiLevelhashSet<HashSet<RehasherExit, Linear> > > > (settings["buckets_scale"].asUnsignedValue()).go();
    } else if(name == "dtree.sr") {
        dtreeTest<dtree<SeparateRootSingleHashSet<HashSet128<RehasherExit, Linear>, HashSet<RehasherExit, Linear> > > > (settings["buckets_scale"].asUnsignedValue()).go();
    } else {
        printf("No compression data structure selected\n");
    }
}

int main(int argc, char** argv) {

    Settings& settings = Settings::global();

    settings["threads"] = 32;
    settings["duplicateratio"] = 0.0;
    settings["collisionratio"] = 1.0;
    settings["inserts"] = 100000;
    settings["buckets_scale"] = 28;
    settings["page_size_scale"] = 28;
    settings["stats"] = 0;
    settings["bars"] = 128;

    int c = 0;
    while ((c = getopt(argc, argv, "i:c:d:s:t:T:p:-:")) != -1) {
//    while ((c = getopt_long(argc, argv, "i:d:s:t:T:p:-:", long_options, &option_index)) != -1) {
        switch(c) {
            case 't':
                if(optarg) {
                    settings["threads"] = std::stoi(optarg);
                }
                break;
            case 'd':
                if(optarg) {
                    settings["duplicateratio"] = std::string(optarg);
                }
                break;
            case 'c':
                if(optarg) {
                    settings["collisionratio"] = std::string(optarg);
                }
                break;
            case 'T':
                if(optarg) {
                    settings["test"] = std::string(optarg);
                }
                break;
            case 'i':
                settings["inserts"] = std::stoi(optarg);
                break;
            case 's':
                settings["buckets_scale"] = std::stoi(optarg);
                break;
            case 'p':
                settings["page_size_scale"] = std::stoi(optarg);
                break;
            case '-':
                settings.insertKeyValue(optarg);
            default:
                break;
        }
    }

    int htindex = optind;
    while(argv[htindex]) {
        runTest(std::string(argv[htindex]));
        ++htindex;
    }


    return 0;
}
