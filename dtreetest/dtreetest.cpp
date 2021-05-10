/*
 * Dtree - a concurrent compression tree for variable-length vectors
 * Copyright © 2018-2021 Freark van der Berg
 *
 * This file is part of Dtree.
 *
 * Dtree is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Dtree is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Dtree.  If not, see <https://www.gnu.org/licenses/>.
 */

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
        printf("No such compression data structure: %s\n", name.c_str());
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
    if(htindex < argc) {
        while(argv[htindex]) {
            runTest(std::string(argv[htindex]));
            ++htindex;
        }
    } else {
        printf("No compression data structure selected\n");
    }

    return 0;
}
