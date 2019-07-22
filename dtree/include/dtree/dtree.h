#pragma once

#include <cassert>
#include <cstring>
#include <array>
#include <vector>
#include <dtree/hashset.h>

template<typename HS>
class SingleLevelhashSet {
public:
    SingleLevelhashSet(size_t scale): _hashSet(scale) {
    }

    size_t scale() {
        return _hashSet._scale;
    }

protected:
    __attribute__((always_inline))
    uint32_t storage_fop(uint64_t v, size_t) {
        return _hashSet.insert(v);
    }

    __attribute__((always_inline))
    uint64_t storage_find(uint64_t v, size_t) {
        return _hashSet.find(v);
    }

    __attribute__((always_inline))
    uint64_t storage_get(uint32_t idx, size_t) {
        //assert(idx && "construct called with 0-idx, indicates possible trouble");
        return _hashSet.get(idx);
    }

    typename HS::mapStats const& getStats() {
        return _hashSet->getStats();
    }
protected:
    HS _hashSet;
};

template<typename HS>
class MultiLevelhashSet {
public:
    MultiLevelhashSet(size_t scale, size_t maps=2): _mask(maps-1), _hashSets() {
        //assert( (1U << (31 - __builtin_clz(maps))) - 1 == _mask && "Need power of two");
        assert( ((maps | _mask) == (maps + _mask)) && "Need power of two");
        _hashSets.reserve(maps);
        for(;maps--;) {
            _hashSets.emplace_back(scale);
        }
    }

    size_t scale() {
        return _hashSets[0]._scale;
    }

protected:
    __attribute__((always_inline))
    uint32_t storage_fop(uint64_t v, size_t level) {
        level &= _mask;
        return _hashSets[level].insert(v);
    }

    __attribute__((always_inline))
    uint64_t storage_find(uint64_t v, size_t level) {
        level &= _mask;
        return _hashSets[level].find(v);
    }

    __attribute__((always_inline))
    uint64_t storage_get(uint32_t idx, size_t level) {
//        assert(idx && "construct called with 0-idx, indicates possible trouble");
        level &= _mask;
        return _hashSets[level].get(idx);
    }

    typename HS::mapStats const& getStats() {

        typename HS::mapStats stats;
        for(auto& h: _hashSets) {
            stats += h.getStats();
        }
        return stats;
    }
protected:
    size_t _mask;
    std::vector<HS> _hashSets;
};

/**
 * @class dtree
 * @author Freark van der Berg
 * @date 02/03/19
 * @file dtree.h
 * @brief Dynamic compression tree. This tree stores variable-length vectors in a compressed way.
 * The compression is achieved by storing 32bit to 64bit mappings in a hashset. When a vector is inserted,
 * it is deconstructed into such mappings. Each 64bit chunk is mapped to a 32bit ID. Then, these 32bit IDs
 * are pairwise joined together, resulting in 64bit again, and then mapped again to a 32bit ID.
 * This is repeated until one 32bit ID remains. For vectors that do not have a length that is a power
 * of two, this results in an unbalanced tree. In this case, the left child tree of the top-node is in fact
 * balanced and maps to a part of the vector of a length that is the biggest power of two lower than the
 * length of the vector. In fact, in any tree, the left child always maps to a balanced tree.
 *
 * In the following example, we insert a vector of 24 bytes. The tree will look thusly wise:
 *
 *   [ a][ b][ c][ d][ e][ f]
 *     [ 1]    [ 2]    [ 3]
 *         [ 4]
 *                 [ 5]
 *
 * Mappings stored:
 *   - [1] -> [a][b]
 *   - [2] -> [c][d]
 *   - [3] -> [e][f]
 *   - [4] -> [1][2]
 *   - [5] -> [4][3]
 *
 * Using this method, when a vector is inserted, a unique @c Index is returned: this @c Index uniquely
 * identifies that vector. An @c Index is 64bit: 32bit to specify the ID of the vector and 32bit to
 * specify its length. In the example, the @c Index would be [24][5], since the vector is 24 bytes long
 * and the ID 5 maps to that vector.
 *
 * Given this @c Index, we can still perform operations on the vector it identifies. For example, we can
 * insert and compress a new vector, based on the first one but with a detla, without actually recomputing
 * the first one. The delta is a new piece of vector of a specific length at a specific offset.
 * Let's do this in the previous example. Given @c Index [24][5], let's apply a delta [g] at offet 3. The
 * delta is applied recursively until the leaves are reached. So, Index [24][5], delta [g]@3 goes as follows:
 *
 * - Index [24][5]: delta [g]@3
 *   - Index [16][4]: delta [g]@3
 *     - Index [8][1]: no change
 *     - Index [8][2]: delta [g]@1
 *       - Insert mapping [6] -> [c][g]
 *     - Insert mapping [7] -> [1][6]
 *   - Index [8][3]: no change
 *   - Insert mapping [8] -> [7][3]
 *
 * Thus, the resulting new Index is [24][8] and we added the following mappings:
 *   - [6] -> [c][g]
 *   - [7] -> [1][6]
 *   - [8] -> [7][3]
 *
 * The tree for this new vector will look like:
 *
 *   [ a][ b][ c][ g][ e][ f]
 *     [ 1]    [ 6]    [ 3]
 *         [ 7]
 *                 [ 8]
 *
 * With a small example, the reuse is small, but the bigger the difference between length of the vector
 * and length of the detla, the larger the compression, in general.
 *
 * Another factor is the position. Suppose we want to extend the vector with a delta [q][r][s][t] of 16 bytes.
 * When recursing, we have to virtually recurse through the new tree, in order to create it, since we may
 * need to add levels. We do this by using the new length and recomputing the tree from that.
 *
 * The new vector would look like:
 *   [ a][ b][ c][ g][ e][ f][ q][ r][ s][ t]
 *
 * - Length 40:
 *   - Length 32:
 *     - Length 16: no change
 *     - Length 16
 *       - Length 8: no change
 *       - Length 8: insert mapping [9] -> [q][r]
 *       - Insert mapping [10] -> [3][9]
 *     - Insert mapping [11] -> [7][10]
 *   - Length 8: insert mapping [12] -> [s][t]
 *   - Insert mapping [13] -> [11][12]
 *
 *   [ a][ b][ c][ g][ e][ f][ q][ r][ s][ t]
 *     [ 1]    [ 6]    [ 3]    [ 9]    [12]
 *         [ 7]            [10]
 *                 [11]
 *                                 [13]
 *
 * Thus, the resulting new Index is [40][13] and we added the following mappings:
 *   - [9]  -> [q][r]
 *   - [10] -> [3][9]
 *   - [11] -> [7][10]
 *   - [12] -> [s][t]
 *   - [13] -> [11][12]
 *
 * Now let's change that [g] back to a [d] in the new vector and see what happens:
 *
 * - Index [40][13]: delta [d]@3
 *   - Index [32][11]: delta [d]@3
 *     - Index [16][7]: delta [d]@3
 *       - Index [8][1]: no change
 *       - Index [8][6]: delta [g]@1
 *         - Reuse mapping [2] -> [c][d]
 *       - Reuse mapping [4] -> [1][2]
 *     - Index [16][10]: no change
 *     - Insert mapping [14] -> [4][10]
 *   - Index [8][12]: no change
 *   - Insert mapping [15] -> [14][12]
 *
 *   [ a][ b][ c][ d][ e][ f][ q][ r][ s][ t]
 *     [ 1]    [ 2]    [ 3]    [ 9]    [12]
 *         [ 4]            [10]
 *                 [14]
 *                                 [15]
 *
 * Thus, the resulting new Index is [40][15] and we added the following mappings:
 *   - [14] -> [4][10]
 *   - [15] -> [14][12]
 *
 * At this stage, our compression tree four three vectors (and some subvectors):
 *   - [a][b][c][d][e][f]
 *   - [a][b][c][g][e][f]
 *   - [a][b][c][g][e][f][q][r][s][t]
 *   - [a][b][c][d][e][f][q][r][s][t]
 *
 * And these are stored using the following mappings:
 *   - [1] -> [a][b]
 *   - [2] -> [c][d]
 *   - [3] -> [e][f]
 *   - [4] -> [1][2]
 *   - [5] -> [4][3]
 *   - [6] -> [c][g]
 *   - [7] -> [1][6]
 *   - [8] -> [7][3]
 *   - [9]  -> [q][r]
 *   - [10] -> [3][9]
 *   - [11] -> [7][10]
 *   - [12] -> [s][t]
 *   - [13] -> [11][12]
 *   - [14] -> [4][10]
 *   - [15] -> [14][12]
 *
 * The cost to store is 120 bytes, since we store the [a][b] part at a unique index in an array.
 * The vectors mapped are 128 bytes in total, so we are starting to see a small compression.
 */
template<typename Storage>
class dtree: public Storage {
public:

    static constexpr bool REPORT = 0;

    /**
     * Index is a unique mapping to a vector. The upper 32 bits are the
     * length of the vector, the lower 32 bits are the unique index.
     */
    using Index = uint64_t;

    /**
     * PartialIndex is a unique mapping to a vector, without the length
     * specification. This is not part of the public interface.
     */
    using PartialIndex = uint32_t;

    /**
     * RootIndex is used to index a root node. It is 32-bit:
     * [ 8                  ][ 16              ][ 8         ]
     * [ SuperIndex to tree ][ additional info ][ user info ]
     */
    using RootIndex = uint32_t;

    using SparseOffset = uint32_t;

public:

    static const Index NotFound = 0ULL;

    /**
     * @brief Construct a new instance.
     * @param scale The underlying hash map can maximally hold 2^scale 64-bit entries.
     * @param rootIndexScale The underlying hash map can maximally hold 2^rootIndexScale 64-bit entries.
     */
    dtree(size_t scale): Storage(scale) {
        assert(scale > 0);
        assert(scale <= 32);
    }

    /**
     * @brief Determines whether or not the specified vector is in the tree.
     * @param data The data of the vector.
     * @param length The length of the vector in number of 32bit units.
     * @return Index to the vector, or NotFound when no such vector is in the tree.
     */
    Index find(uint32_t* data, uint32_t length) {
        auto result = findRecursing(data, length);
        if(result == 0x100000000ULL) {
            if(REPORT) printf("NOT Found %s(%u) -> ?\n", (char*)data, length);
            return NotFound;
        }
        if(REPORT) printf("Found %s(%u) -> %16zx\n", (char*)data, length, result);
        return result;
    }

    /**
     * @brief Deconstructs the specified data into the compression tree.
     * Returns an @c Index that unique identifies the deconstructed vector.
     * @param data The data with length @c length to insert.
     * @param length Length of @c data in number of 32bit units.
     * @return Unique index that can be used to retrieve the data.
     */
    Index insert(uint32_t* data, uint32_t length) {
        uint64_t result = length == 0 ? 0 : ((uint64_t)deconstruct(data, length)) | (((uint64_t)length) << 34);
        if(REPORT) printBuffer("Inserted", (char*)data, length << 2, result);
        return result;
    }

    /**
     * @brief Deconstructs the specified vector into the compression tree.
     * Returns an @c Index that unique identifies the deconstructed vector.
     * @param data The data with length @c length to insert.
     * @param length Length of @c data in bytes.
     * @return Unique index that can be used to retrieve the data.
     */
    Index insertBytes(uint8_t* data, uint32_t length) {
        uint64_t result = ((uint64_t)deconstructBytes(data, length)) | (((uint64_t)length) << 32);
        if(REPORT) printBuffer("Inserted", (char*)data, length, result);
        return result;
    }

    /**
     * @brief Constructs the entire vector using the specified Index. Make sure the length of the vector
     * is a multiple of 4 bytes, otherwise use @c getBytes().
     * @param idx Index of the vector to construct.
     * @param buffer Buffer where the vector will be stored.
     * @return false
     */
    bool get(Index idx, uint32_t* buffer) {
        construct(idx, idx >> 34, buffer);
        if(REPORT) printBuffer("Constructed", (char*)buffer, idx >> 32, idx);
        return false;
    }

    /**
     * @brief Partially constructs a vector using the specified Index. Make sure the length of the vector
     * is a multiple of 4 bytes, otherwise use @c getBytes().
     * It acts like memmove(buffer, vector+offset, length);
     * @param id Index of the vector to construct.
     * @param offset Offset to which part that will be stored into @c buffer within the vector
     * @param length Length of the part that will be stored into @c buffer.
     * @param buffer Buffer where the partial vector will be stored. Needs to be at least @c length 32-bit units long.
     * @return false
     */
    bool getPartial(Index idx, uint32_t offset, uint32_t length, uint32_t* buffer) {
        constructPartial(idx, idx >> 34, offset, length, buffer);
        if(REPORT) printBuffer("Constructed", (char*)buffer, idx >> 32, idx);
        return false;
    }

    bool getSparseUnits(Index idx, uint32_t* buffer, uint32_t offsets, uint32_t* offset) {
        constructSparseUnits(idx, idx >> 34, 0, buffer, offsets, offset);
        if(REPORT) {
            printBuffer("Constructed sparse units", (char*)buffer, offsets, 0);
        }
        return false;
    }

    bool getSparse(Index idx, uint32_t* buffer, uint32_t offsets, SparseOffset* offset) {
        constructSparse(idx, idx >> 34, 0, buffer, offsets, offset);
        if(REPORT) {
            size_t s = 0;
            uint32_t* end = offset + offsets;
            while(offset < end) s += *offset++ & 0xFF;
            printBuffer("Constructed sparse", (char*)buffer, s, 0);
        }
        return false;
    }

    Index deltaSparse(Index idx, uint32_t* deltaData, uint32_t offsets, SparseOffset* offset) {
        uint64_t result = ((uint64_t)deltaSparseApply(idx, idx >> 34, deltaData, offsets, offset)) | (idx & 0xFFFFFFFF00000000ULL);
        if(REPORT) {
            uint32_t buffer[idx >> 34];
            get(result, buffer);
            printBuffer("Inserted", (char*)buffer, (uint32_t)(idx >> 32), result);
        }
        return result;
    }

    Index deltaSparseStride(Index idx, uint32_t* deltaData, uint32_t offsets, uint32_t* offset, uint32_t stride) {
        uint64_t result = ((uint64_t)deltaSparseApply(idx, idx >> 34, 0, deltaData, offsets, offset, stride)) | (idx & 0xFFFFFFFF00000000ULL);
        if(REPORT) {
            uint32_t buffer[idx >> 34];
            get(result, buffer);
            printBuffer("Inserted", (char*)buffer, (uint32_t)(idx >> 32), result);
        }
        return result;
    }

    /**
     * @brief Returns a single 64bit part of the vector associated with @c idx.
     * @param idx Index of the vector of which a 64bit part is desired.
     * @param offset Offset within the vector to the desired single 64bit entry. MUST be multiple of 2.
     * @return The desired 64bit part.
     */
    uint64_t getSingle(Index idx, uint32_t offset) {
        uint32_t length = idx >> 32ULL;
        if(offset >= length) return 0;
        return getSingleRecursive(idx, idx >> 32ULL, offset);
    }

    /**
     * @brief Deconstructs a new vector that is based on the vector specified by @c idx, but with the delta
     * described by @c deltaLength and @c deltaData applied at offset @c offset within the vector. The delta
     * MUST be within the original vector.
     * @requires offset + deltaLength <= (idx >> 34)
     * @param idx Index of the vector to construct.
     * @param offset The offset within the vector at which the delta will be applied, in 32-bit units.
     * @param deltaData The contents of the delta.
     * @param deltaLength The length of the delta, in 32-bit units
     * @return A new unique index that can be used to retrieve the new vector.
     */
    Index delta(Index idx, uint32_t offset, const uint32_t* deltaData, uint32_t deltaLength) {
        uint64_t result = ((uint64_t)deltaApply(idx, idx >> 34, offset, deltaLength, deltaData)) | (idx & 0xFFFFFFFF00000000ULL);
        if(REPORT) {
            uint32_t buffer[idx >> 34];
            get(result, buffer);
            printBuffer("Inserted", (char*)buffer, (uint32_t)(idx >> 32), result);
        }
        return result;
    }

    /**
     * @brief Deconstructs a new vector that is based on the vector specified by @c idx, but with the delta
     * described by @c deltaLength and @c deltaData applied at offset @c offset within the vector. The delta
     * is allowed to be after the end of the original vector, thus extending the original vector.
     * If @c deltaLength == 0, the original Index @c idx is returned.
     * @param idx Index of the vector to base the new vector on.
     * @param offset The offset within the vector at which the delta will be applied, in 32-bit units.
     * @param deltaData The contents of the delta.
     * @param deltaLength The length of the delta, in 32-bit units
     * @return A new unique index that can be used to retrieve the new vector.
     */
    Index deltaMayExtend(Index idx, uint32_t offset, uint32_t* deltaData, uint32_t deltaLength) {
        if(deltaLength == 0) return idx;
        uint32_t length = idx >> 34;
        uint32_t newLength = std::max(length, offset + deltaLength);
        uint64_t result = ((uint64_t)deltaApplyMayExtend(idx, length, offset, deltaData, deltaLength)) | (((uint64_t)newLength) << 34);
        if(REPORT) {
            uint32_t buffer[newLength];
            get(result, buffer);
            printBuffer("Inserted", (char*)buffer, newLength << 2, result);
        }
        return result;
    }

    /**
     * @brief Deconstructs a new vector that is based on the vector specified by @c idx, but shrunk
     * to @c shrinkTo units. The vector will be shrunk to the left.
     * @param idx Index of the vector to base the new vector on.
     * @param shrinkTo Number of units (32bit integers) of the new vector.
     * @return Index to the newly deconstructed vector.
     */
    Index shrink(Index idx, uint32_t shrinkTo) {
        return shrinkRecursive(idx, idx >> 34, shrinkTo);
    }

    /**
     * @brief Deconstructs a new vector that is based on the vector specified by @c idx, but extended
     * with @c data of length @c deltaLength. Zeroes are used for padding if needed due to the
     * specified @c alignment.
     *
     * The new vector will look like:
     *   Data:   [original][0 padding if needed][delta]
     *
     * @param idx Index of the vector to base the new vector on.
     * @param alignment The start of the delta will be divisible by this number
     * @param deltaLength The length of the delta
     * @param data
     * @return
     */
    Index extend(Index idx, uint32_t alignment, uint32_t deltaLength, uint32_t* data) {
        uint32_t length = idx >> 34;
        alignment--;
        return extendAt(idx, (length + alignment) & ~alignment, deltaLength, data);
    }

    /**
     * @brief Deconstructs a new vector that is based on the vector specified by @c idx, but extended
     * first with @c offset zeroes and then with @c data of length @c deltaLength
     *
     * The new vector will look like:
     *   Data:   [original][offset number of 0's][delta]
     *
     * As with all
     * @param idx Index of the vector to base the new vector on.
     * @param offset The number of zeroes between the original data and the delta
     * @param deltaLength The length of the delta
     * @param data
     * @return
     */
    Index extendAt(Index idx, uint32_t offset, uint32_t deltaLength, uint32_t* data) {
        uint64_t newLength = (idx >> 34) + (uint64_t)offset + (uint64_t)deltaLength;
        uint64_t result;
        if(idx == 0) {
            result = deltaLength == 0 ? 0 : (insertZeroPrepended(data, deltaLength, offset) | (newLength << 34));
        } else if(deltaLength == 0) {
            result = zeroExtend(idx, idx >> 34, (idx >> 34) + offset);
        } else {
            result = extendRecursive(idx, idx >> 34, offset, deltaLength, data);
        }
        result |= (newLength << 34);
        if(REPORT) {
            uint32_t buffer[1 + newLength];
            buffer[newLength] = 0;
            get(result, buffer);
            printBuffer("Inserted", (char*)buffer, (uint32_t)(idx >> 32), result);
        }
        return result;
    }

    /**
     * @brief Inserts a new vector that is based on the vector specified by @c idx, but with the delta
     * described by @c deltaLength and @c data applied at offset @c offset within the vector.
     * @param idx Index of the vector to construct.
     * @param offset The offset within the vector at which the delta will be applied, in 8-bit units.
     * @param deltaLength The length of the delta, in 8-bit units
     * @param data The contents of the delta.
     * @return A new unique index that can be used to retrieve the new vector.
     */
    Index deltaB(Index idx, uint32_t offset, uint32_t deltaLength, uint8_t* data) {
        uint64_t lengthPart = idx & 0xFFFFFFFF00000000ULL;
        uint32_t lengthInBytes = lengthPart >> 32;
        uint64_t result = deltaApplyInBytes(idx, lengthInBytes, offset, deltaLength, data) | lengthPart;
        if(REPORT) {
            uint32_t buffer[lengthInBytes/4];
            get(result, buffer);
            printBuffer("Inserted", (char*)buffer, lengthInBytes, result);
        }
        return result;
    }

    template<typename CONTAINER>
    void getDensityStats(size_t bars, CONTAINER& elements, size_t map) {
//        _hashSets[map].getDensityStats(bars, elements);
    }

    template<typename CONTAINER>
    void getProbeStats(size_t bars, CONTAINER& elements, size_t map) {
//        _hashSets[map].getProbeStats(bars, elements);
    }

    void printBuffer(const char* action, char* buffer, uint32_t length, uint64_t result) {
        printf("%s ", action);
        char* end = buffer + length;
        while(buffer < end) {
            //printf("%02x", *buffer);
            printf("%c", *buffer);
            buffer++;
        }
        printf("(%u) -> %16zx\n", length, result);
    }

//    [offset]*[length][offset][data]
//
//    [0][0][4][0][abcd]
//    [0][1][2][0][ef]
//    [1][0][2][0][gh]
//    [2][0][2][0][ij]
//    [2][2][4][0][klmn]
//
//    ->
//
//    [0][0][4][0]
//    [0][1][2][0]
//    [1][0][2][0]
//    [2][0][2][0]
//    [2][2][4][0]
//    [abcd][ef][gh][ij][klmn]
//
//    [0][0][1][2][2]
//    [0][1][0][0][2]
//    [4][2][2][2][4]
//    [0][0][0][0][0]

//    Index multiDelta(uint32_t src, uint32_t length, uint32_t deltas, uint32_t indices, uint32_t currentIndex, uint32_t* indexData, uint32_t* data) {
//        uint32_t filteredIndices[deltas];
//        uint32_t mapped[deltas];
//        uint32_t n[deltas];
//        uint32_t filteredIndices_n = 0;
//        uint32_t maxIndex = currentIndex + deltas * indices;
//
//        if(currentIndex == indices - 2) {
//            deltaSparseApply(src, length, );
//        }
//
//
//        for(int i = currentIndex; i < maxIndex; i += indices) {
//            uint32_t f = indexData[i];
//            for(uint32_t j = filteredIndices_n; j--;) {
//                if(filteredIndices[j]==f) {
//                    n[j]++;
//                    goto alreadyIn;
//                }
//            }
//            n[filteredIndices_n] = 1;
//            filteredIndices[filteredIndices_n++] = f;
//            alreadyIn: {}
//        }
//
//        getSparseUnits(src, mapped, filteredIndices_n, filteredIndices);
//
//        uint32_t* currentIndexData = indexData;
//        uint32_t* currentData = data;
//        for(uint32_t fi = filteredIndices_n; fi--;) {
//            multiData(mapped[fi], n[fi], indices, currentIndex + 1, currentIndexData, data);
//            currentIndexData += n[fi] * indices;
//        }
//    }

private:

    __attribute__((always_inline))
    size_t lengthToLevel(size_t length) {
        // length -> __builtin_clz(length-1) -> level
        //      1 ->                      32 ->   n/a
        //      2 ->                      31 ->     0
        //    3-4 ->                      30 ->     1
        //    5-8 ->                      29 ->     2
        //   9-16 ->                      28 ->     3
        return 31 - __builtin_clz(length-1);
    }

    __attribute__((always_inline))
    uint32_t deconstruct(uint64_t v, size_t level) {
        if(v==0ULL) return 0;
        uint32_t idx = this->template storage_fop(v, level);
        assert(idx && "hash table full");
        return idx;
    }

    __attribute__((always_inline))
    uint64_t findRecursing(uint64_t v, size_t level) {
        if(v==0ULL) return 0ULL;
        return this->template storage_find(v, level);
    }

    __attribute__((always_inline))
    uint64_t construct(uint32_t idx, size_t level) {
        if(idx == 0) return 0;
        uint64_t mapped = this->template storage_get(idx, level);
        if(REPORT) printf("Got %8x(%u) -> %16zx\n", idx, 8, mapped);
        return mapped;
    }

    uint32_t deconstruct(uint32_t* data, uint32_t length) {
        if(length == 1) {
            return *data;
        } else if (length == 2) {
            return deconstruct(*(uint64_t*)data, 0);
        }

        size_t level = lengthToLevel(length);

        uint32_t leftLength = 1 << level;
        uint32_t l = deconstruct(data, leftLength);
        uint32_t r = deconstruct(data+leftLength, length-leftLength);
        return deconstruct(((uint64_t)r) << 32 | (uint64_t)l, level);
    }

    uint32_t deconstructBytes(uint8_t* data, uint32_t length) {
        if(length <= 4) {
            uint32_t d;
            memmove(&d, data, length);
            return d;
        } else if(length <= 8) {
            return deconstruct(*(uint64_t*)data, 0);
        }
        size_t level = lengthToLevel(length);
        uint32_t leftLength = 1 << level;
        uint32_t l = deconstructBytes(data, leftLength);
        uint32_t r = deconstructBytes(data+leftLength, length-leftLength);
        return deconstruct(((uint64_t)r) << 32 | (uint64_t)l, level - 2);
    }

    uint64_t findRecursing(uint32_t* data, uint32_t length) {
        if(length == 1) {
            return *data;
        } else if (length == 2) {
            return findRecursing(*(uint64_t*)data, 0);
        }

        size_t level = lengthToLevel(length);
        uint32_t leftLength = 1 << level;
        uint64_t l = findRecursing(data, leftLength);
        if(l == 0x100000000ULL) {
            return 0x100000000ULL;
        }
        uint64_t r = findRecursing(data+leftLength, length-leftLength);
        if(r == 0x100000000ULL) {
            return 0x100000000ULL;
        }
        return findRecursing(((uint64_t)r) << 32 | (uint64_t)l, level);
    }

    uint64_t getSingleRecursive(uint32_t idx, uint32_t length, uint32_t offset) {
        if(length == 2) {
            return construct(idx, 0);
        }
        size_t level = lengthToLevel(length);
        uint64_t mapped = construct(idx, level);
        uint32_t leftLength = 1 << level;
        if(offset < leftLength) {
            return getSingle(mapped, leftLength, offset);
        } else {
            return getSingle(mapped >> 32ULL, length - leftLength, offset - leftLength);
        }
    }

    void construct(uint32_t idx, uint32_t length, uint32_t* buffer) {
        if(length == 1) {
            if(REPORT) printf("Got %8x(%u) -> %16x\n", idx, length, idx);
            *buffer = idx;
            return;
        }

        if(idx == 0) {
            memset(buffer, 0, length * sizeof(uint32_t));
            return;
        }

        if(length == 2) {
            *(uint64_t*)buffer = construct(idx, 0);
            return;
        }

        size_t level = lengthToLevel(length);
        uint64_t mapped = construct(idx, level);

        uint32_t leftLength = 1 << level;
        construct(mapped & 0xFFFFFFFFULL, leftLength, buffer);

        // If there is a part remaining
        if(leftLength < length) {
            construct(mapped >> 32ULL, length-leftLength, buffer + leftLength);
        }
    }

    void constructPartial(uint32_t idx, uint32_t length, uint32_t offset, uint32_t wantedLength, uint32_t* buffer) {
        if(REPORT) {
            for(int i=__builtin_clz(1 << lengthToLevel(length))-26; i--;) printf("    ");
            printf("\033[35mconstructPartial\033[0m(%x, %u, %u, %u)\n", idx, length, offset, wantedLength);
        }
        if(length == 1) {
            if(REPORT) printf("Got %8x(%u) -> %16x\n", idx, length, idx);
            *buffer = idx;
            return;
        }

        if(idx == 0) {
            memset(buffer, 0, wantedLength * sizeof(uint32_t));
            return;
        }

        if(length == 2) {
            uint64_t mapped = construct(idx, 0);
            if(wantedLength == 2) {
                *(uint64_t*)buffer = mapped;
            } else {
                if(offset == 0) {
                    *buffer = (uint32_t)mapped;
                } else {
                    *buffer = mapped >> 32ULL;
                }
            }
            return;
        }

        size_t level = lengthToLevel(length);
        uint64_t mapped = construct(idx, level);

        uint32_t leftLength = 1 << level;

        if(offset < leftLength) {
//            if(offset + wantedLength < leftLength) {
//            }
            uint32_t leftWantedLength = leftLength - offset;
            if(wantedLength > leftWantedLength) {
                constructPartial(mapped & 0xFFFFFFFFULL, leftLength, offset, leftWantedLength, buffer);
                constructPartial(mapped >> 32ULL, length-leftLength, 0, wantedLength - leftWantedLength, buffer + leftWantedLength);
            } else {
                constructPartial(mapped & 0xFFFFFFFFULL, leftLength, offset, wantedLength, buffer);
            }
        } else {
            constructPartial(mapped >> 32ULL, length-leftLength, offset - leftLength, wantedLength, buffer);
        }

    }

    void constructSparseUnits(uint32_t idx, uint32_t length, uint32_t internalOffset, uint32_t* buffer, uint32_t offsets, uint32_t* offset) {

        if(length == 1) {
            *(uint32_t*)buffer = idx;
            return;
        }

        if(length == 2) {
            uint64_t mapped = construct(idx, 0);
            if(offsets == 2) {
                *(uint64_t*)buffer = mapped;
            } else if (offset[0] == internalOffset) {
                *buffer = mapped;
            } else {
                *buffer = mapped >> 32ULL;
            }
            return;
        }

        size_t level = lengthToLevel(length);
        uint64_t mapped = construct(idx, level);
        uint32_t leftLength = 1 << level;

        uint32_t leftOffsets = 0;
        uint32_t offsetLeft = internalOffset + leftLength;
        while(leftOffsets < offsets && offset[leftOffsets] < offsetLeft) ++leftOffsets;

        // If the left side is touched
        if(leftOffsets > 0) {
            constructSparseUnits(mapped, leftLength, internalOffset, buffer, leftOffsets, offset);
        }

        // If the right side is touched
        if(leftOffsets < offsets) {
            constructSparseUnits(mapped, length - leftLength, offsetLeft, buffer + leftOffsets, offsets - leftOffsets, offset + leftOffsets);
        }
    }

    void constructSparse(uint32_t idx, uint32_t length, uint32_t internalOffset, uint32_t* buffer, uint32_t offsets, SparseOffset* offset) {

        if(REPORT) {
            for(int i=__builtin_clz(1 << lengthToLevel(length))-26; i--;) printf("    ");
            printf("\033[36mconstructSparse\033[0m(%x, %u, %u, %p, [", idx, length, internalOffset >> 8, buffer);
            uint32_t* o = offset;
            uint32_t* e = offset + offsets;
            while(o<e) {
                if(o != offset) printf(", ");
                printf("%x", *o);
                o++;
            }
            printf("])\n");
        }

        if(offsets == 1) {
            constructPartial(idx, length, (offset[0] - internalOffset) >> 8, *offset & 0xFFULL, buffer);
            return;
        }

        if(length == 2) {
            *(uint64_t*)buffer = construct(idx, 0);
            return;
        }

        size_t level = lengthToLevel(length);
        uint64_t mapped = construct(idx, level);

        uint32_t leftLength = 1 << level;
        uint32_t leftLength2 = leftLength << 8;

        uint32_t leftOffsets = 0;
        uint32_t leftOffsetSizeTotal = 0;
        uint32_t offsetLeft = internalOffset + leftLength2;
        while(leftOffsets < offsets && offset[leftOffsets] < offsetLeft) {
            leftOffsetSizeTotal += offset[leftOffsets] & 0xFF;
            ++leftOffsets;
        }

//        // Fix the offsets of the right offsets
//        for(uint32_t r = leftOffsets; r < offsets; ++r) {
//            offset[r] -= leftLength2;
//        }

//        printf("left offsets:        %u\n", leftOffsets);
//        printf("leftOffsetSizeTotal: %u\n", leftOffsetSizeTotal);

        // If the left side is touched
        if(leftOffsets > 0) {


            // If there is overlap in change from left and right, we need to split them up...
            uint32_t last = leftOffsets - 1;
            int32_t overlap = ((((int32_t)(offset[last] - offsetLeft)) >> 8) + (offset[last] & 0xFF));
//            printf("overlap: %i (%i + %u)\n", overlap, (((int32_t)(offset[last] - offsetLeft)) >> 8), (offset[last] & 0xFF));
            if(overlap > 0) {

                // Cut off the part that affects the right side
                offset[last] -= overlap;

                // problem is that we need to add offsetLeft again, so maybe changing the offsets on the fly (the commented while loop) is not so bad...

                // Construct the left part
                constructSparse(mapped, leftLength, internalOffset, buffer, leftOffsets, offset);

                // Overwrite the last "left offset" with a new offset that describes
                // the part that was just cut off
                offset[last] = overlap + offsetLeft;

                // Decrement the number of left offsets such that this new one is picked
                // up later by the recursive call for the right side
                leftOffsets--;

                // Fix the size total too
                leftOffsetSizeTotal -= overlap;
            } else {
                // Construct the left part
                constructSparse(mapped, leftLength, internalOffset, buffer, leftOffsets, offset);
            }

        }

        // If the right side is touched
        if(leftOffsets < offsets) {
            constructSparse(mapped >> 32ULL, length - leftLength, offsetLeft, buffer + leftOffsetSizeTotal, offsets - leftOffsets, offset + leftOffsets);
        }
    }

    uint32_t deltaSparseUnitsApply(uint32_t idx, uint32_t length, uint64_t* data, uint32_t offsets, uint32_t* offset) {

        if(length == 1) {
            return *(uint32_t*)data;
        }

        if(length == 2) {
            return deconstruct(*(uint64_t*)data);
        }

        size_t level = lengthToLevel(length);
        uint64_t mapped = construct(idx, level);

        uint32_t leftLength = 1 << level;

        uint32_t leftOffsets = 0;
        while(leftOffsets < offsets && offset[leftOffsets] < leftLength) ++leftOffsets;

        // If the left side is touched
        uint32_t leftIndex = leftOffsets == 0 ? mapped
                                              : deltaSparseUnitsApply(mapped, leftLength, data, leftOffsets, offset)
                                              ;

        // If the right side is touched
        uint32_t rightIndex = leftOffsets == offsets ? (mapped >> 32ULL)
                                                     : deltaSparseUnitsApply(mapped, length - leftLength, data + leftOffsets, offsets - leftOffsets, offset + leftOffsets)
                                                     ;

        return deconstruct(((uint64_t)leftIndex) | (((uint64_t)rightIndex) << 32));

    }

    uint32_t deltaSparseApply(uint32_t idx, uint32_t length, uint32_t* buffer, uint32_t offsets, SparseOffset* offset) {

        if(REPORT) {
            for(int i=__builtin_clz(1 << lengthToLevel(length))-26; i--;) printf("    ");
            printf("deltaSparseApply(%u, %u, [", idx, length);
            uint32_t* o = offset;
            uint32_t* e = offset + offsets;
            while(o<e) {
                if(o != offset) printf(", ");
                printf("%x", *o);
                o++;
            }
            printf("])\n");
        }

        if(offsets == 1) {
            return deltaApply(idx, length, offset[0] >> 8, offset[0] & 0xFFULL, buffer);
        }

        size_t level = lengthToLevel(length);
        uint64_t mapped = construct(idx, level);

        uint32_t leftLength = 1 << level;
        uint32_t leftLength2 = leftLength << 8;

        uint32_t leftOffsets = 0;
        uint32_t leftOffsetSizeTotal = 0;
        while(leftOffsets < offsets && offset[leftOffsets] < leftLength2) {
            leftOffsetSizeTotal += offset[leftOffsets] & 0xFF;
            ++leftOffsets;
        }

        // Fix the offsets of the right offsets
        for(uint32_t r = leftOffsets; r < offsets; ++r) {
            offset[r] -= leftLength2;
        }

        uint64_t leftIndex;
        uint64_t rightIndex;

        // If the left side is touched
        if(leftOffsets > 0) {

            // If there is overlap in change from left and right, we need to split them up...
            uint32_t last = leftOffsets - 1;
            int32_t overlap = ((offset[last] >> 8) + (offset[last] & 0xFF)) - (int32_t)leftLength;

            if(overlap > 0) {

                // Cut off the part that affects the right side
                offset[last] -= overlap;

                // Construct the left part
                leftIndex = deltaSparseApply(mapped, leftLength, buffer, leftOffsets, offset);

                // Overwrite the last "left offset" with a new offset that describes
                // the part that was just cut off
                offset[last] = overlap;

                // Decrement the number of left offsets such that this new one is picked
                // up later by the recursive call for the right side
                leftOffsets--;

                // Fix the size total too
                leftOffsetSizeTotal -= overlap;
            } else {
                // Construct the left part
                leftIndex = deltaSparseApply(mapped, leftLength, buffer, leftOffsets, offset);
            }

        } else {
            leftIndex = mapped & 0x00000000FFFFFFFFULL;
        }

        // If the right side is touched
        if(leftOffsets < offsets) {
            rightIndex = deltaSparseApply(mapped >> 32ULL, length - leftLength, buffer + leftOffsetSizeTotal, offsets - leftOffsets, offset + leftOffsets);
            rightIndex <<= 32ULL;
        } else {
            rightIndex = mapped & 0xFFFFFFFF00000000ULL;
        }
        return deconstruct(leftIndex | rightIndex, level);
    }

    uint32_t deltaSparseApply(uint32_t idx, uint32_t length, uint32_t internalOffset, uint32_t* buffer, uint32_t offsets, uint32_t* offset, uint32_t stride) {

        if(REPORT) {
            for(int i=__builtin_clz(1 << lengthToLevel(length))-26; i--;) printf("    ");
            printf("deltaSparseApply(%u, %u, %u, %p, [", idx, length, internalOffset, buffer);
            uint32_t* o = offset;
            uint32_t* e = offset + offsets * stride;
            while(o<e) {
                if(o != offset) printf(", ");
                printf("%u@%u", *(o+1), *o);
                o += stride;
            }
            printf("])\n");
        }

        if(offsets == 1) {

            // Detect too long delta due to crossing left-right bound:
            int32_t o = offset[0] - internalOffset;

            if(o < 0) {
                return deltaApply(idx, length, 0, offset[1] + o, buffer - o);
            } else {
                int32_t overlap = (int32_t)o + (int32_t)offset[1] - (int32_t)length;
                return deltaApply(idx, length, o, overlap > 0 ? offset[1] - overlap : offset[1], buffer);
            }
        }

        if(length == 2) {
            return deconstruct(*(uint64_t*)(buffer + internalOffset - offset[0]), 0);
        }

        size_t level = lengthToLevel(length);
        uint64_t mapped = construct(idx, level);

        uint32_t leftLength = 1 << level;

        uint32_t leftOffsets = 0;
        uint32_t leftOffsetSizeTotal = 0;
        uint32_t offsetIndex = 0;
        uint32_t offsetLeft = internalOffset + leftLength;
        while(leftOffsets < offsets && offset[offsetIndex] < offsetLeft) {
            leftOffsetSizeTotal += offset[offsetIndex + 1];
            ++leftOffsets;
            offsetIndex += stride;
        }

        uint64_t leftIndex;
        uint64_t rightIndex;

        // If the left side is touched
        if(leftOffsets > 0) {

            // If there is overlap in change from left and right, we need to split them up...
            uint32_t last = offsetIndex - stride;
            int32_t overlap = ((offset[last]) + (offset[last+1])) - (int32_t)offsetLeft;

            if(overlap > 0) {

                // Construct the left part
                leftIndex = deltaSparseApply(mapped, leftLength, internalOffset, buffer, leftOffsets, offset, stride);


                // Decrement the number of left offsets such that this new one is picked
                // up later by the recursive call for the right side
                leftOffsets--;

                // Fix the size total too
                leftOffsetSizeTotal -= offset[last+1];
            } else {
                // Construct the left part
                leftIndex = deltaSparseApply(mapped, leftLength, internalOffset, buffer, leftOffsets, offset, stride);
            }

        } else {
            leftIndex = mapped & 0x00000000FFFFFFFFULL;
        }

        // If the right side is touched
        if(leftOffsets < offsets) {
            rightIndex = deltaSparseApply(mapped >> 32ULL, length - leftLength, offsetLeft, buffer + leftOffsetSizeTotal, offsets - leftOffsets, offset + leftOffsets * stride, stride);
            rightIndex <<= 32ULL;
        } else {
            rightIndex = mapped & 0xFFFFFFFF00000000ULL;
        }
        return deconstruct(leftIndex | rightIndex, level);
    }

    uint32_t deltaApply(uint32_t idx, uint32_t length, uint32_t offset, uint32_t deltaLength, const uint32_t* data) {

        if(REPORT) {
            for(int i=__builtin_clz(1 << lengthToLevel(length))-26; i--;) printf("    ");
            printf("deltaApply(%u, %uB, %uB, %uB, %p)\n", idx, length << 2, offset << 2, deltaLength << 2, data);
        }

        if(length == 1) {
            if(REPORT) printf("Got %8x(%u) -> %16x\n", idx, length, idx);
            return *data;
        }

        if(length == 2) {
            uint64_t mapped = construct(idx, 0);

            // This is >= instead of == because in some edge cases deltaApply() may be called with a
            // deltaLength larger than the length, in which case only length bytes should be copied.
            if (deltaLength >= 2) {
                return deconstruct(*(uint64_t*)data, 0);
            } else {
                if(offset == 0) {
                    mapped &= 0xFFFFFFFF00000000ULL;
                    mapped |= *data;
                } else {
                    mapped &= 0x00000000FFFFFFFFULL;
                    mapped |= ((uint64_t)*data) << 32;
                }
                return deconstruct(mapped, 0);
            }
        }

        size_t level = lengthToLevel(length);
        uint64_t mapped = construct(idx, level);

        uint32_t leftLength = 1 << level;
        uint64_t mappedNew = 0;

        if(offset < leftLength) {
            uint32_t leftDeltaLength = leftLength - offset;
            if(leftDeltaLength < deltaLength) {
                mappedNew = (uint64_t)(deltaApply(mapped & 0xFFFFFFFFULL, leftLength, offset, leftDeltaLength, data))
                       | (((uint64_t)deltaApply(mapped >> 32ULL, length-leftLength, 0, deltaLength - leftDeltaLength, data + leftDeltaLength)) << 32)
                       ;
            } else {
//                *((uint32_t*)(mapped)) = deltaApply(mapped & 0xFFFFFFFFULL, leftLength, offset, leftDeltaLength, data);
                mappedNew = (uint64_t)deltaApply(mapped & 0xFFFFFFFFULL, leftLength, offset, deltaLength, data);
                mappedNew |= mapped & 0xFFFFFFFF00000000ULL;
            }
        } else {
//            *((uint32_t*)(mapped)+1) = deltaApply(mapped >> 32ULL, length-leftLength, offset - leftLength, deltaLength, data);
            mappedNew = (uint64_t)deltaApply(mapped >> 32ULL, length-leftLength, offset - leftLength, deltaLength, data) << 32;
            mappedNew |= mapped & 0xFFFFFFFFULL;
        }

        if(mapped == mappedNew) {
            return idx;
        } else {
            return deconstruct(mappedNew, level);
        }
    }

    uint32_t deltaApplyInBytes(uint32_t idx, uint32_t lengthInBytes, uint32_t offsetInBytes, uint32_t deltaLengthInBytes, uint8_t* data) {
        if(lengthInBytes == 4) {
            if(REPORT) printf("Got %8zx(%u) -> %16zx\n", idx, lengthInBytes, idx);
            memmove(((uint8_t*)&idx)+offsetInBytes, data, deltaLengthInBytes);
            return idx;
        }

        if(lengthInBytes == 8) {
            uint64_t mapped = construct(idx, 0);
            uint32_t offsetInBits = (offsetInBytes*8);
            memmove(((uint8_t*)&mapped)+offsetInBytes, data, deltaLengthInBytes);
            return deconstruct(mapped);
        }

        size_t level = lengthToLevel(lengthInBytes);
        uint64_t mapped = construct(idx, level);

        uint32_t leftLength = 1 << level;
        uint64_t mappedNew = 0;

        if(offsetInBytes < leftLength) {
            uint32_t leftDeltaLength = leftLength - offsetInBytes;
            if(leftDeltaLength < deltaLengthInBytes) {
                mappedNew = (uint64_t)(deltaApplyInBytes(mapped & 0xFFFFFFFFULL, leftLength, offsetInBytes, leftDeltaLength, data))
                       | (((uint64_t)deltaApplyInBytes(mapped >> 32ULL, lengthInBytes-leftLength, 0, deltaLengthInBytes - leftDeltaLength, data + leftDeltaLength)) << 32)
                       ;
            } else {
                mappedNew = (uint64_t)deltaApplyInBytes(mapped & 0xFFFFFFFFULL, leftLength, offsetInBytes, deltaLengthInBytes, data);
                mappedNew |= mapped & 0xFFFFFFFF00000000ULL;
            }
        } else {
            mappedNew = (uint64_t)deltaApplyInBytes(mapped >> 32ULL, lengthInBytes-leftLength, offsetInBytes - leftLength, deltaLengthInBytes, data) << 32;
            mappedNew |= mapped & 0xFFFFFFFFULL;
        }

        if(mapped == mappedNew) {
            return idx;
        } else {
            return deconstruct(mappedNew);
        }
    }

    uint32_t shrinkRecursive(uint32_t idx, uint32_t length, uint32_t shrinkTo) {
        if(length == 1) return idx;
        if(length == 2) {
            if(shrinkTo == 2) {
                return idx;
            } else {
                return construct(idx, 0);
            }
        }

        size_t level = lengthToLevel(length);
        uint32_t leftLength = 1 << level;

        if(leftLength < shrinkTo) {
            uint64_t mapped = construct(idx, level);
            uint32_t rightIndex = mapped >> 32;
            mapped &= 0xFFFFFFFF00000000ULL;
            mapped |= ((uint64_t)shrinkRecursive(rightIndex, length - leftLength, shrinkTo - leftLength)) << 32;
            return deconstruct(mapped);
        } else if(leftLength > shrinkTo) {
            uint64_t mapped = construct(idx, level);
            // Recursively shrink the left index
            return shrinkRecursive(mapped, leftLength, shrinkTo);
        } else {
            return idx;
        }

    }

    uint32_t extendRecursive(uint32_t idx, uint32_t length, uint32_t offset, int32_t deltaLength, uint32_t* data) {
        uint64_t newLength = length + (uint64_t)offset + (uint64_t)deltaLength;

        if(REPORT) printf("Extending %x(%u) at %u with ...(%u)\n", idx, length << 2, offset << 2, deltaLength << 2);

        if(newLength == 1) {
            return *data;
        }
        if(newLength == 2) {

            // This can only happen for [original][delta]
            return deconstruct( ((uint64_t)idx) | (((uint64_t)(*(uint32_t*)data)) << 32), 0);

//            // Options are:
//            // [original[delta]
//            //
//            printf("             %4u %4u %4u lolo\n", length, offset, deltaLength);
//
//            // If the length is
//            if(offset == 0) {
//                return deconstruct(*(uint64_t*)data);
//            }
//
//            // Given the offset is 1 and if the length is 0, the left part is
//            // 0 and the right part comes from the delta
//            if(length == 0) {
//                return deconstruct( ((uint64_t)(*(uint32_t*)data)) << 32);
//            }
//
//            // Given offset is 1 and length is 1, the left part must be from the original
//            // and the right part
//            return deconstruct( ((uint64_t)idx) | (((uint64_t)(*(uint32_t*)data)) << 32) );
        }
//        if(length == 2) {
//            if(offset == 0) {
//                if (deltaLength == 2) {
//                    return deconstruct(*(uint64_t*)data);
//                } else {
//                    mapped &= 0xFFFFFFFF00000000ULL;
//                    mapped |= *data;
//                }
//            } else {
//                mapped &= 0x00000000FFFFFFFFULL;
//                mapped |= ((uint64_t)*data) << 32;
//            }
//            return deconstruct(mapped);
//        }

        size_t level = lengthToLevel(newLength);
        uint32_t leftLength = 1 << level;
        uint32_t zeroExtendedLength = length + offset;
        uint32_t rightOffset = zeroExtendedLength - leftLength;

        uint32_t leftIndex, rightIndex;

        // If the left part is exactly what we already have
        if(leftLength == length) {
            leftIndex = idx;
            if(REPORT) printf("Detected identical part: %x(%u)\n", leftIndex, leftLength);
            rightIndex = insertZeroPrepended(data, deltaLength, offset) ;

        // If the left part is part of what we already have
        } else if(leftLength < length) {
            uint64_t mapped = construct(idx, level);
            leftIndex = mapped & 0xFFFFFFFF;
            if(REPORT) printf("Detected unchanged part: %x(%u)\n", leftIndex, leftLength);
            rightIndex = extendRecursive(mapped >> 32, length - leftLength, offset, deltaLength, data);

        // If the left part contains original data and zeroes
        } else if(zeroExtendedLength >= leftLength) {
            leftIndex = zeroExtend(idx, length, leftLength);
            rightIndex = insertZeroPrepended(data, deltaLength, rightOffset);

        // If the left part contains original data, maybe zeroes and definitely delta
        } else {
            leftIndex = extendRecursive(idx, length, offset, -rightOffset, data);
            rightIndex = deconstruct(data - (int32_t)rightOffset, newLength - (int32_t)leftLength);
        }

        return deconstruct(((uint64_t)leftIndex) | (((uint64_t)rightIndex) << 32), level);
/*
            // If the left part contains part of the original
            if(length > 0) {
                if(REPORT) printf("Need to extend left part using delta, rightOffset=%u\n", rightOffset);
                newIndex = extendRecursive(idx, length, offset, -rightOffset, data);
            } else {
                if(REPORT) printf("Detected zeros on the left\n");
                newIndex = 0;
            }


        uint64_t rightIndex = 0;
        if(deltaLength <= 0) {
            if(length == 0) {
                if(REPORT) printf("Detected zeros on the right\n");
                // leave 0
            } else {
                if(mapped == 0) mapped = _hashSet.get(idx);
                rightIndex = extendRecursive(mapped >> 32, );
            }

        // If the remainder is only delta
        } else if(rightOffset <= 0) {
            if(REPORT) printf("Deconstructing the rest (%u)\n", deltaLength - rightOffset);
            rightIndex = deconstruct(data-rightOffset, deltaLength - rightOffset);

        // If there is part of the original in the right part
        } else if(length > leftLength) {
            if(mapped == 0) mapped = _hashSet.get(idx);
            rightIndex = extendRecursive(mapped >> 32, length - leftLength, offset, deltaLength, data);

        // If there are only 0's and delta remaining
        } else {
            if(mapped == 0) mapped = _hashSet.get(idx);
            rightIndex = extendRecursive(0, 0, rightOffset, newLength - zeroExtendedLength, data);
        }

        return deconstruct(((uint64_t)rightIndex) << 32 | (uint64_t)newIndex);
*/
    }

    uint32_t zeroExtend(uint32_t idx, uint32_t length, uint32_t extendTo) {
        if(REPORT) printf("Zero-extending %x(%u) to %u\n", idx, length << 2, extendTo << 2);

        size_t level = lengthToLevel(extendTo);
        uint32_t leftLength = 1 << level;

        // If the left part is part of what we already have
        if(extendTo == length) {
            return idx;
        } else if(leftLength == length) {
            return deconstruct(idx, level);
        } else if(leftLength < length) {
            uint64_t mapped = construct(idx, level);
            uint64_t newIndex = mapped & 0xFFFFFFFFULL;
            if(REPORT) printf("Detected unchanged part: %zx(%u)\n", newIndex, leftLength << 2);
            newIndex |= ((uint64_t)zeroExtend(mapped >> 32, length - leftLength, extendTo - leftLength)) << 32;
            return deconstruct(newIndex, level);

        // If the left part contains 0's
        } else {
            return deconstruct(zeroExtend(idx, length, leftLength), level);
        }
    }

    uint32_t insertZeroPrepended(uint32_t* data, uint32_t length, uint32_t offset) {
        if(REPORT) printf("Zero-prepending vector of length %u with %u zeroes\n", length << 2, offset << 2);

        size_t level = lengthToLevel(length + offset);
        uint32_t leftLength = 1 << level;

        // If the offset is 0, this is just a normal deconstruct
        if(offset == 0) {
            return deconstruct(data, length);

        // If the lengthLength is exactly the offset, the right side is
        // just a normal deconstruct and the left are only 0's
        } else if(leftLength == offset) {
            return deconstruct(((uint64_t)deconstruct(data, length)) << 32, level);

        // If the right part contains zeroes, the left part is
        // only zeroes and the right part needs to be recursively handled
        } else if(leftLength < offset) {
            return deconstruct(((uint64_t)insertZeroPrepended(data, length, offset - leftLength)) << 32, level);

        // If the left part contains part of the data, the right part
        // is normal data and the left part needs to be recursively handled
        } else {
            uint32_t diff = leftLength - offset;
            uint64_t v = ((uint64_t)insertZeroPrepended(data, diff, offset))
                       | (((uint64_t)deconstruct(data + diff, length - diff)) << 32)
                       ;
            return deconstruct(v, level);
        }
    }

    uint32_t deltaApplyMayExtend(uint32_t idx, uint32_t length, uint32_t offset, uint32_t* deltaData, uint32_t deltaLength) {

        if(REPORT) printf("deltaApplyMayExtend %x(%u) at %u with ...(%u)\n", idx, length << 2, offset << 2, deltaLength << 2);

        // If we are trying to extend a 0-length vector, just prepend a number of 0's
        if(idx == 0) {
            return deltaLength == 0 ? 0 : insertZeroPrepended(deltaData, deltaLength, offset);
        }

        // If the delta extends the vector...
        uint32_t newLength = offset + deltaLength;
        if(newLength > length) {

            // If the offset is 0, this means the delta completely overwrites the original
            if(offset == 0) {

                // If the length of the delta is 1 there is not need to deconstruct
                if(deltaLength == 1) return *deltaData;
                else return deconstruct(deltaData, deltaLength);
            }

            // If the original vector and the delta overlap
            if(offset < length) {

                // [ original ]
                //          [ delta ]
                //
                size_t level = lengthToLevel(newLength);
                uint32_t leftLength = 1 << level;
                uint64_t mapped = construct(idx, level);

                // If the delta affects the left part
                if(offset < leftLength) {
                    if(REPORT) printf("Right part is purely delta, leftLength=%uB\n", leftLength << 2);
                    uint32_t deltaLengthLeft = leftLength - offset;
                    uint32_t leftIndex;
                    if(length > leftLength) {
                        leftIndex= deltaApplyMayExtend(mapped, leftLength, offset, deltaData, deltaLengthLeft);
                    } else {
                        leftIndex= deltaApplyMayExtend(idx, length, offset, deltaData, deltaLengthLeft);
                    }
                    uint32_t rightIndex = deconstruct(deltaData + deltaLengthLeft, deltaLength - deltaLengthLeft);
                    return deconstruct(((uint64_t)leftIndex) | (((uint64_t)rightIndex) << 32), level);
                }

                // If the left part stays unchanged
                else {
                    if(REPORT) printf("Left part is unchanged, leftLength=%uB\n", leftLength << 2);
                    uint32_t rightIndex = deltaApplyMayExtend(mapped >> 32, length - leftLength, offset - leftLength, deltaData, deltaLength);
                    mapped &= 0x00000000FFFFFFFFULL;
                    mapped |= ((uint64_t)rightIndex) << 32;
                    return deconstruct(mapped, level);
                }

            }

            // If the delta is completely beyond the vector,
            // we can simply call the extend-only extendRecursive()
            else {
                idx = extendRecursive(idx, length, offset - length, deltaLength, deltaData);
            }

            // Return the new Index
            return idx;
        }

        // If the delta is completely within the bounds of the original vector,
        // we can simply call the non-extending deltaApply()
        else {
            return deltaApply(idx, length, offset, deltaLength, deltaData);
        }
    }

//    uint32_t extendRecursive(uint32_t idx, uint32_t length, uint32_t offset, int32_t deltaLength, uint32_t* data) {
//        uint64_t newLength = length + (uint64_t)offset + (uint64_t)deltaLength;
//
//        if(REPORT) printf("Extending %x(%u) at %u with ...(%u)\n", idx, length, offset, deltaLength);
//
//        if(newLength == 1) {
//            return *data;
//        }
//        if(newLength == 2) {
//            if(offset == 0) {
//                return deconstruct(*(uint64_t*)data);
//            } else if(length == 0) {
//                return deconstruct( ((uint64_t)(*(uint32_t*)data)) << 32);
//            } else {
//                return deconstruct( ((uint64_t)idx) | (((uint64_t)(*(uint32_t*)data)) << 32) );
//            }
//        }
////        if(length == 2) {
////            if(offset == 0) {
////                if (deltaLength == 2) {
////                    return deconstruct(*(uint64_t*)data);
////                } else {
////                    mapped &= 0xFFFFFFFF00000000ULL;
////                    mapped |= *data;
////                }
////            } else {
////                mapped &= 0x00000000FFFFFFFFULL;
////                mapped |= ((uint64_t)*data) << 32;
////            }
////            return deconstruct(mapped);
////        }
//
//        uint32_t leftLength = 1 << lengthToLevel(newLength);
//        uint64_t newIndex = idx;
//        uint32_t zeroExtendedLength = length + offset;
//        uint32_t rightOffset = zeroExtendedLength - leftLength;
//
//        // If the left part needs changing
//        if(leftLength > length) {
//
//            // If the left part is changed by the delta
//            if(length > 0) {
//                if(REPORT) printf("Need to extend left part using delta, rightOffset=%u\n", rightOffset);
//                newIndex = extendRecursive(idx, length, offset, -rightOffset, data);
//            } else {
//                if(REPORT) printf("Detected zeros on the left\n");
//                newIndex = 0;
//            }
//        } else {
//            if(REPORT) printf("Detected unchanged part: %x(%u)\n", newIndex, leftLength);
//        }
//
//        uint64_t rightIndex = 0;
//        if(deltaLength < 0) {
//            if(REPORT) printf("Detected zeros on the right\n");
//            // leave 0
//        } else if(rightOffset <= 0) {
//            if(REPORT) printf("Deconstructing the rest (%u)\n", deltaLength - rightOffset);
//            rightIndex = deconstruct(data-rightOffset, deltaLength - rightOffset);
//        } else if(length > leftLength) {
//            rightIndex = extendRecursive(idx, length - leftLength, rightOffset, newLength - zeroExtendedLength, data);
//        } else {
//            rightIndex = extendRecursive(idx, 0, rightOffset, newLength - zeroExtendedLength, data);
//        }
//
//        return deconstruct(((uint64_t)rightIndex) << 32 | (uint64_t)newIndex);
//
//    }

};
