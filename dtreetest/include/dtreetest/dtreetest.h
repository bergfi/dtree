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

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <dtree/dtree.h>

template<typename TREE>
class dtreeTest {
public:

    dtreeTest(size_t scale): _tree() {
        _tree.setScale(scale);
    }

    template<bool (*F)(TREE& tree, const char* vector, size_t offset, const char* deltaData)>
    void test() {
        char original[] = "AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHHIIIIJJJJKKKKLLLLMMMMNNNNOOOOPPPPQQQQRRRRSSSS";
        char delta[] = "qqqqrrrrssssttttuuuuvvvvwwwwxxxxyyyyzzzz";

        for(size_t length = 0; length < sizeof(original)/4; ++length) {
            printf("vector of length %zu\n", length);
            for(size_t deltaLength = 0; deltaLength < sizeof(delta)/4; ++deltaLength) {
                for(size_t offset = 0; offset < sizeof(original)/4; ++offset) {
                    char save1 = original[length*4];
                    char save2 = delta[deltaLength*4];
                    original[length*4] = 0;
                    delta[deltaLength*4] = 0;
                    F(_tree, original, offset, delta);
                    original[length*4] = save1;
                    delta[deltaLength*4] = save2;
                }
            }
        }
    }

    void go() {

        printf("\n:: Testing getSparse()\n");

        testGetSparse(_tree, "0123456789ABCDEF", 0, 2, 2, 2);
        testGetSparse(_tree, "aAaAbBbBcCcCdDdDeEeEfFfFgGgGhHhHiIiIjJjJkKkKlLlL", 0, 3, 3, 3);
        testGetSparse(_tree, "aAaAbBbBcCcCdDdDeEeEfFfFgGgGhHhHiIiIjJjJkKkKlLlL", 1, 3, 4, 3);
        testSparse2<testGetSparse>();

        printf("\n:: Testing deltaSparse()\n");

        testDeltaSparse(_tree, "0123456789ABCDEF", 0, 2, 2, 2);
        testDeltaSparse(_tree, "aAaAbBbBcCcCdDdDeEeEfFfFgGgGhHhHiIiIjJjJkKkKlLlL", 0, 3, 3, 3);
        testDeltaSparse(_tree, "aAaAbBbBcCcCdDdDeEeEfFfFgGgGhHhHiIiIjJjJkKkKlLlL", 1, 3, 4, 3);
        testSparse2<testDeltaSparse>();

        printf("\n:: Testing deltaSparseStride()\n");

//        testDeltaSparseStride2(_tree, "0123456789ABCDEF", 0, 2, 2, 2);
//        testDeltaSparseStride2(_tree, "aAaAbBbBcCcCdDdDeEeEfFfFgGgGhHhHiIiIjJjJkKkKlLlL", 0, 3, 3, 3);
//        testDeltaSparseStride2(_tree, "aAaAbBbBcCcCdDdDeEeEfFfFgGgGhHhHiIiIjJjJkKkKlLlL", 1, 3, 4, 3);
//        testSparse2<testDeltaSparseStride2>();
        testSparse<testDeltaSparseStride>();

        printf("\n:: Testing extendAt()\n");

        testExtend(_tree, "0123456789AB"    , 2, "zZzZ");
        testExtend(_tree, "0123456789ABCDEF", 0, "zZzZ");
        testExtend(_tree, "0123456789ABCDEF", 0, "zZzZxXxX89ABCDEF");
        testExtend(_tree, "0123456789ABCDEF", 24, "zZzZxXxX89ABCDEF");
        testExtend(_tree, "0123", 24, "zZzZxXxX89ABCDEF");
        testExtend(_tree, "0123", 24, "zZzZ");
        testExtend(_tree, "0123", 1, "zZzZxXxX89ABCDEF");
        testExtend(_tree, "0123456789ABCDEF", 128, "zZzZxXxX89ABCDEF");
        testExtend(_tree, "0123456789ABCDEF", 4, "");
        testExtend(_tree, "", 3, "ABCD");
        testExtend(_tree, "", 0, "ABCD");
        testExtend(_tree, "0123456789ABCDEF", 0, "");
        testExtend(_tree, "", 0, "");
        testExtend(_tree, "0123", 0, "zZzZxXxX89ABCDEF");

        test<testExtend>();

        printf("\n:: Testing deltaMayExtend()\n");

        testDeltaMayExtend(_tree, "AAAABBBBCCCC"    , 2, "aaaa");
        testDeltaMayExtend(_tree, "AAAABBBBCCCCDDDD", 0, "aaaa");
        testDeltaMayExtend(_tree, "AAAABBBBCCCCDDDD", 0, "aaaabbbbccccdddd");
        testDeltaMayExtend(_tree, "AAAABBBBCCCCDDDD", 24, "aaaabbbbccccdddd");
        testDeltaMayExtend(_tree, "AAAA", 24, "aaaabbbbccccdddd");
        testDeltaMayExtend(_tree, "0123", 24, "zZzZ");
        testDeltaMayExtend(_tree, "0123", 1, "zZzZxXxX89ABCDEF");
        testDeltaMayExtend(_tree, "0123456789ABCDEF", 128, "zZzZxXxX89ABCDEF");
        testDeltaMayExtend(_tree, "0123456789ABCDEF", 4, "");
        testDeltaMayExtend(_tree, "", 3, "ABCD");
        testDeltaMayExtend(_tree, "", 0, "ABCD");
        testDeltaMayExtend(_tree, "0123456789ABCDEF", 0, "");
        testDeltaMayExtend(_tree, "", 0, "");
        testDeltaMayExtend(_tree, "0123456789ABCDEF", 2, "zZzZxXxX89ABCDEF");
        testDeltaMayExtend(_tree, "0123456789AB", 2, "zZzZxXxX");

        testDeltaMayExtend(_tree, "0123456789ABCDEF", 1, "zZzZxXxX89ABCDEF");
        testDeltaMayExtend(_tree, "0123456789ABCDEF", 2, "zZzZxXxX89ABCDEF");
        testDeltaMayExtend(_tree, "0123456789ABCDEF", 3, "zZzZxXxX89ABCDEF");

        testDeltaMayExtend(_tree, "0123456789ABCDEF", 3, "zZzZxXxX89ABCDEFqQqQrRrR");
        testDeltaMayExtend(_tree, "0123456789ABCDEFaAaAbBbB", 3, "zZzZxXxX89ABCDEFqQqQ");
        testDeltaMayExtend(_tree, "0123456789ABCDEFaAaAbBbB", 4, "zZzZxXxX89ABCDEF");
        testDeltaMayExtend(_tree, "0123456789ABCDEFaAaAbBbB", 3, "zZzZxXxX89ABCDEFqQqQ");

        test<testDeltaMayExtend>();

//        0123456789ABCDEF
//                zZzZxXxX89ABCDEF
//                       |
    }

    static bool testExtend(TREE& tree, const char* vector, size_t offset, const char* deltaData) {
        return testExtend(tree, (uint32_t*)vector, strlen(vector)>>2, offset, strlen(deltaData)>>2, (uint32_t*)deltaData);
    }

    static bool testExtend(TREE& tree, uint32_t* vector, size_t length, size_t offset, uint32_t deltaLength, uint32_t* deltaData) {

//        printf("  > testExtend(%s, %zu, %s)\n", (char*)vector, offset, (char*)deltaData);

        typename TREE::IndexInserted idx = tree.insert(vector, length, true);
        typename TREE::IndexInserted idx2 = tree.extendAt(idx.getState(), offset, deltaLength, deltaData, true);

        uint32_t bufferCorrect[length + offset + deltaLength + 1];
        memmove((char*)bufferCorrect, (void*)vector, length*sizeof(uint32_t));
        memset((char*)(bufferCorrect+length), 0, offset*sizeof(uint32_t));
        memmove((char*)(bufferCorrect+length+offset), (void*)deltaData, deltaLength*sizeof(uint32_t));
        memset((char*)(bufferCorrect+length+offset+deltaLength), 0, sizeof(uint32_t));

        uint32_t bufferResult[length + offset + deltaLength + 1];
        bufferResult[length + offset + deltaLength] = 0;
        tree.get(idx2.getState(), bufferResult, true);
        auto r = memcmp(bufferCorrect, bufferResult, (length + offset + deltaLength + 1)*sizeof(uint32_t));
        if(r==0) {
//            printf("OK!\n");
        } else {
            printf("\033[31mWRONG!\033[0m\n");
            tree.printBuffer("Expected", bufferCorrect, (length + offset + deltaLength)*sizeof(uint32_t), 0);
            tree.printBuffer("Obtained", bufferResult, (length + offset + deltaLength)*sizeof(uint32_t), 0);
        }
        return false;
    }

    static bool testDeltaMayExtend(TREE& tree, const char* vector, size_t offset, const char* deltaData) {
        return testDeltaMayExtend(tree, (uint32_t*)vector, strlen(vector)>>2, offset >> 2, strlen(deltaData)>>2, (uint32_t*)deltaData);
    }

    static bool testDeltaMayExtend(TREE& tree, uint32_t* vector, size_t length, size_t offset, uint32_t deltaLength, uint32_t* deltaData) {

//        printf("  > deltaMayExtend(%s, %zu, %s)\n", (char*)vector, offset, (char*)deltaData);

        size_t expectedLength = deltaLength == 0 ? length : std::max(length, offset + deltaLength);

        typename TREE::IndexInserted idx = tree.insert(vector, length, true);
        typename TREE::IndexInserted idx2 = tree.deltaMayExtend(idx.getState(), offset, deltaData, deltaLength, true);

        uint32_t bufferCorrect[length + offset + deltaLength + 1];
        memset((char*)(bufferCorrect), 0, (expectedLength+1)*sizeof(uint32_t));
        memmove((char*)bufferCorrect, (void*)vector, length*sizeof(uint32_t));
        if(deltaLength > 0) {
            memmove((char*)(bufferCorrect+offset), (void*)deltaData, deltaLength*sizeof(uint32_t));
        }

        uint32_t bufferResult[length + offset + deltaLength + 1];
        bufferResult[expectedLength] = 0;
        tree.get(idx2.getState(), bufferResult, true);
        auto r = memcmp(bufferCorrect, bufferResult, (expectedLength+1)*sizeof(uint32_t));
        if(r==0) {
//            printf("OK!\n");
        } else {
            printf("\033[31mWRONG!\033[0m\n");
            tree.printBuffer("Expected", bufferCorrect, (expectedLength)*sizeof(uint32_t), 0);
            tree.printBuffer("Obtained", bufferResult, (expectedLength)*sizeof(uint32_t), 0);
        }
        return false;
    }

    static bool testGetSparse(TREE& tree, const char* vector, size_t offset, size_t length, size_t offset2, size_t length2) {
        return testGetSparse(tree, (uint32_t*)vector, strlen(vector)>>2, offset, length, offset2, length2);
    }

    static bool testGetSparse( TREE& tree, uint32_t* vector, size_t length
                             , size_t offset, uint32_t deltaLength
                             , size_t offset2, uint32_t deltaLength2
                             ) {

//        printf("  > testGetSparse(%s, %zu, %zu, %u, %zu, %u)\n", (char*)vector, length, offset, deltaLength, offset2, deltaLength2);

        size_t expectedLength = deltaLength + deltaLength2;

        typename TREE::SparseOffset offsets[2];
        offsets[0] = (offset << 8) | deltaLength;
        offsets[1] = (offset2 << 8) | deltaLength2;

        uint32_t bufferResult[expectedLength + 1];
        bufferResult[expectedLength] = 0;

        typename TREE::IndexInserted idx = tree.insert(vector, length, true);
        tree.getSparse(idx.getState(), bufferResult, 2, offsets, true);

        uint32_t bufferCorrect[expectedLength + 1];
        if(deltaLength > 0) {
            memmove((char*)(bufferCorrect), (void*)(vector+offset), deltaLength*sizeof(uint32_t));
        }
        if(deltaLength2 > 0) {
            memmove((char*)(bufferCorrect+deltaLength), (void*)(vector+offset2), deltaLength2*sizeof(uint32_t));
        }
        memset((char*)(bufferCorrect+expectedLength), 0, sizeof(uint32_t));

        auto r = memcmp(bufferCorrect, bufferResult, (expectedLength+1)*sizeof(uint32_t));
        if(r==0) {
//            printf("OK!\n");
        } else {
            printf("\033[31mWRONG!\033[0m\n");
            tree.printBuffer("Expected", bufferCorrect, (expectedLength)*sizeof(uint32_t), 0);
            tree.printBuffer("Obtained", bufferResult, (expectedLength)*sizeof(uint32_t), 0);
        }
        return false;
    }

    static bool testDeltaSparse(TREE& tree, const char* vector, size_t offset, size_t length, size_t offset2, size_t length2) {
        return testDeltaSparse(tree, (uint32_t*)vector, strlen(vector)>>2, offset, length, offset2, length2);
    }

    static bool testDeltaSparse( TREE& tree, uint32_t* vector, size_t length
                               , size_t offset, uint32_t deltaLength
                               , size_t offset2, uint32_t deltaLength2
                               ) {

//        printf("  > testDeltaSparse(%s, %zu, %zu, %u, %zu, %u)\n", (char*)vector, length, offset, deltaLength, offset2, deltaLength2);

        char delta[] = "qqqqrrrrssssttttuuuuvvvvwwwwxxxxyyyyzzzz";

        size_t expectedLength = length;

        typename TREE::SparseOffset offsets[2];
        offsets[0] = (offset << 8) | deltaLength;
        offsets[1] = (offset2 << 8) | deltaLength2;

        typename TREE::IndexInserted idx = tree.insert(vector, length, true);
        typename TREE::IndexInserted idx2 = tree.deltaSparse(idx.getState(), (uint32_t*)delta, 2, offsets, true);

        uint32_t bufferCorrect[expectedLength + 1];
        memmove((char*)bufferCorrect, (void*)vector, length*sizeof(uint32_t));
        if(deltaLength > 0) {
            memmove((char*)(bufferCorrect+offset), (void*)(delta), deltaLength*sizeof(uint32_t));
        }
        if(deltaLength2 > 0) {
            memmove((char*)(bufferCorrect+offset2), (void*)(delta+deltaLength*sizeof(uint32_t)), deltaLength2*sizeof(uint32_t));
        }
        memset((char*)(bufferCorrect+expectedLength), 0, sizeof(uint32_t));

        uint32_t bufferResult[expectedLength + 1];
        tree.get(idx2.getState(), bufferResult, true);
        bufferResult[expectedLength] = 0;

        auto r = memcmp(bufferCorrect, bufferResult, (expectedLength+1)*sizeof(uint32_t));
        if(r==0) {
//            printf("OK!\n");
        } else {
            printf("\033[31mWRONG!\033[0m\n");
            tree.printBuffer("Expected", bufferCorrect, (expectedLength)*sizeof(uint32_t), 0);
            tree.printBuffer("Obtained", bufferResult, (expectedLength)*sizeof(uint32_t), 0);
        }
        return false;
    }

    static bool testDeltaSparseStride2(TREE& tree, const char* vector, size_t offset, size_t length, size_t offset2, size_t length2) {
        return testDeltaSparseStride2(tree, (uint32_t*)vector, strlen(vector)>>2, offset, length, offset2, length2);
    }

    static bool testDeltaSparseStride2( TREE& tree, uint32_t* vector, size_t length
                                      , size_t offset, uint32_t deltaLength
                                      , size_t offset2, uint32_t deltaLength2
                                      ) {

//        printf("  > testDeltaSparse(%s, %zu, %zu, %u, %zu, %u)\n", (char*)vector, length, offset, deltaLength, offset2, deltaLength2);

        char delta[] = "aaaabbbbccccddddeeeeffffgggghhhhiiiijjjjkkkkllllmmmmnnnnooooppppqqqqrrrrssssttttuuuuvvvvwwwwxxxxyyyyzzzz";

        size_t expectedLength = length;

        typename TREE::SparseOffset offsets[4];
        offsets[0] = offset;
        offsets[1] = deltaLength;
        offsets[2] = offset2;
        offsets[3] = deltaLength2;

        typename TREE::Index idx = tree.insert(vector, length);
        typename TREE::Index idx2 = tree.deltaSparseStride(idx, (uint32_t*)delta, 2, offsets, 2);

        uint32_t bufferCorrect[expectedLength + 1];
        memmove((char*)bufferCorrect, (void*)vector, length*sizeof(uint32_t));
        if(deltaLength > 0) {
            memmove((char*)(bufferCorrect+offset), (void*)(delta), deltaLength*sizeof(uint32_t));
        }
        if(deltaLength2 > 0) {
            memmove((char*)(bufferCorrect+offset2), (void*)(delta+deltaLength*sizeof(uint32_t)), deltaLength2*sizeof(uint32_t));
        }
        memset((char*)(bufferCorrect+expectedLength), 0, sizeof(uint32_t));

        uint32_t bufferResult[expectedLength + 1];
        tree.get(idx2, bufferResult);
        bufferResult[expectedLength] = 0;

        auto r = memcmp(bufferCorrect, bufferResult, (expectedLength+1)*sizeof(uint32_t));
        if(r==0) {
//            printf("OK!\n");
        } else {
            printf("\033[31mWRONG!\033[0m\n");
            tree.printBuffer("Expected", bufferCorrect, (expectedLength)*sizeof(uint32_t), 0);
            tree.printBuffer("Obtained", bufferResult, (expectedLength)*sizeof(uint32_t), 0);
        }
        return false;
    }

    static bool testDeltaSparseStride( TREE& tree, uint32_t* vector, size_t length
                                     , char* delta, size_t offsets, uint32_t* offsetData, size_t stride
                                     ) {

//        printf("  > testDeltaSparse(%s, %zu, %zu, %xu)\n", (char*)vector, length, offsets, stride);

        size_t expectedLength = length;

        typename TREE::IndexInserted idx = tree.insert(vector, length, true);
        typename TREE::IndexInserted idx2 = tree.deltaSparseStride(idx.getState(), (uint32_t*)delta, offsets, offsetData, stride, true);

        uint32_t bufferCorrect[expectedLength + 1];
        memmove((char*)bufferCorrect, (void*)vector, length*sizeof(uint32_t));
        uint32_t* offsetDataEnd = offsetData + stride * offsets;
        uint32_t usedDeltaSoFar = 0;
        for(uint32_t* o = offsetData; o < offsetDataEnd; o += stride) {
            uint32_t dLen = *(o+1);
            if(dLen > 0) {
                uint32_t dOff = *o;
                memmove((char*)(bufferCorrect+dOff), (void*)(delta+usedDeltaSoFar*sizeof(uint32_t)), dLen*sizeof(uint32_t));
                usedDeltaSoFar += dLen;
            }
        }
        memset((char*)(bufferCorrect+expectedLength), 0, sizeof(uint32_t));

        uint32_t bufferResult[expectedLength + 1];
        tree.get(idx2.getState(), bufferResult, true);
        bufferResult[expectedLength] = 0;

        auto r = memcmp(bufferCorrect, bufferResult, (expectedLength+1)*sizeof(uint32_t));
        if(r==0) {
//            printf("OK!\n");
        } else {
            printf("\033[31mWRONG!\033[0m\n");
            tree.printBuffer("Expected", bufferCorrect, (expectedLength)*sizeof(uint32_t), 0);
            tree.printBuffer("Obtained", bufferResult, (expectedLength)*sizeof(uint32_t), 0);
        }
        return false;
    }

    template<bool (*F)(TREE& tree, uint32_t* vector, size_t length, size_t offset, uint32_t deltaLength, size_t offset2, uint32_t deltaLength2)>
    void testSparse2() {
        char original[] = "AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHHIIIIJJJJKKKKLLLLMMMMNNNNOOOOPPPPQQQQRRRRSSSSTTTTUUUU";

        for(size_t length = 2; length < sizeof(original)/4; ++length) {
            printf("vector of length %zu\n", length);
            for(size_t offset = 0; offset < length - 1; ++offset) {
                for(size_t offsetEnd = offset + 1; offsetEnd < length; ++offsetEnd) {
                    for(size_t offset2 = offsetEnd; offset2 < length; ++offset2) {
                        for(size_t offsetEnd2 = offset2 + 1; offsetEnd2 <= length; ++offsetEnd2) {
                            F(_tree, (uint32_t*)original, length, offset, offsetEnd-offset, offset2, offsetEnd2-offset2);
                        }
                    }
                }
            }
        }
    }

    template<bool (*F)(TREE& tree, uint32_t* vector, size_t length, char* delta, size_t offsets, uint32_t* offsetData, size_t stride)>
    void testSparse() {
        char original[] = "AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHHIIIIJJJJKKKKLLLL";
        char delta[] = "aaaabbbbccccddddeeeeffffgggghhhhiiiijjjjkkkkllll";

        uint32_t offsetData[sizeof(delta)/4][2];

        // This calls F for every possible multi-delta configuration
        for(size_t length = 1; length < sizeof(original)/4; ++length) {
            printf("vector of length %zu\n", length);
            for(size_t offsets = 1; offsets <= length; ++offsets) {
                for(size_t currentDelta = offsets; currentDelta--;) {
                    offsetData[currentDelta][0] = currentDelta;
                    offsetData[currentDelta][1] = 1;
                }

                for(;;) {

                    F(_tree, (uint32_t*)original, length, delta, offsets, (uint32_t*)offsetData, 2);

                    offsetData[offsets-1][1]++;
                    for(size_t currentDelta = offsets; currentDelta--;) {
                        uint32_t threshold = currentDelta < (offsets-1) ? offsetData[currentDelta+1][0] : length;
                        if(offsetData[currentDelta][0] + offsetData[currentDelta][1] > threshold) {
                            if(offsetData[currentDelta][1] == 2) {
                                if(currentDelta == 0) {
                                    goto end;
                                }
                                offsetData[currentDelta-1][1]++;
                                offsetData[currentDelta][1] = 0;
                            } else {
                                offsetData[currentDelta][0]++;
                                offsetData[currentDelta][1] = 1;
                            }
                        }
                    }
                    for(size_t currentDelta = 1; currentDelta < offsets; currentDelta++) {
                        if(offsetData[currentDelta][1] == 0) {
                            offsetData[currentDelta][0] = offsetData[currentDelta-1][0] + offsetData[currentDelta-1][1];
                            offsetData[currentDelta][1] = 1;
                        }
                    }
                }
                end: {}
            }
        }

//        for(size_t length = 3; length < sizeof(original)/4; ++length) {
//            for(size_t offset = 0; offset < length - 2; ++offset) {
//                for(size_t offsetEnd = offset + 1; offsetEnd < length - 1; ++offsetEnd) {
//                    for(size_t offset2 = offsetEnd; offset2 < length - 1; ++offset2) {
//                        for(size_t offsetEnd2 = offset2 + 1; offsetEnd2 < length; ++offsetEnd2) {
//                            for(size_t offset3 = offsetEnd2; offset3 < length; ++offset3) {
//                                for(size_t offsetEnd3 = offset3 + 1; offsetEnd3 <= length; ++offsetEnd3) {
//                                    F(_tree, (uint32_t*)original, length, offset, offsetEnd-offset, offset2, offsetEnd2-offset2, offset3, offsetEnd3-offset3);
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//        }
    }

private:
    TREE _tree;
};
