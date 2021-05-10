/*
 * Dtree - a concurrent compression tree for variable-length vectors
 * Copyright © 2018-2021 Freark van der Berg
 *
 * This file is part of Dtree.
 *
 * Dtree is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
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

#include <cassert>
#include <cstring>
#include <array>
#include <functional>
#include <iostream>
#include <vector>

#include <dtree/hashset.h>

#ifndef dtree_likely
#   define dtree_likely(x) __builtin_expect((x),1)
#   define dtree_unlikely(x) __builtin_expect((x),0)
#endif


template<typename HS>
class SingleLevelhashSet {
public:
    SingleLevelhashSet(): _hashSet() {
    }

    size_t scale() {
        return _hashSet._scale;
    }

    void init() {
        _hashSet.init();
    }

    static constexpr uint64_t NotFound() {
        return HS::NotFound();
    }

    void setScale(size_t scale) {
        _hashSet.setScale(scale);
    }

    size_t getScale() const {
        return _hashSet._scale;
    }

protected:
    __attribute__((always_inline))
    uint64_t storage_fop(uint64_t v, uint32_t, bool) {
        return _hashSet.insert(v);
    }

    __attribute__((always_inline))
    uint64_t storage_find(uint64_t v, uint32_t, bool) {
        return _hashSet.find(v);
    }

    __attribute__((always_inline))
    uint64_t storage_get(uint64_t idx, uint32_t, bool) {
        //assert(idx && "construct called with 0-idx, indicates possible trouble");
        return _hashSet.get(idx);
    }

public:
    typename HS::mapStats getStats() {
        typename HS::mapStats stats;
        stats += _hashSet.getStats();
        return std::move(stats);
    }

    typename HS::probeStats getProbeStats() {
        typename HS::probeStats stats;
        stats += _hashSet.getProbeStats();
        return std::move(stats);
    }

    template<typename CONTAINER>
    void getDensityStats(size_t bars, CONTAINER& elements, size_t map) {
        _hashSet.getDensityStats(bars, elements);
    }
protected:
    HS _hashSet;
};

template<typename HSROOT, typename HS>
class SeparateDWordRootSingleHashSet {
public:
    SeparateDWordRootSingleHashSet(): _hashSetRoot(), _hashSet() {
    }

    void setScale(size_t scale) {
        _hashSetRoot.setScale(scale);
        _hashSet.setScale(scale);
    }

    void setRootScale(size_t scale) {
        _hashSetRoot.setScale(scale);
    }

    void setDataScale(size_t scale) {
        _hashSet.setScale(scale);
    }

    size_t getRootScale() const {
        return _hashSetRoot._scale;
    }

    size_t getDataScale() const {
        return _hashSet._scale;
    }

    void init() {
        _hashSetRoot.init();
        _hashSet.init();
    }

    static constexpr uint64_t NotFound() {
        return HS::NotFound();
    }

protected:
    __attribute__((always_inline))
    uint64_t storage_fop(uint64_t v, uint32_t, uint64_t length, bool isRoot) {
        if(dtree_unlikely(isRoot)) {
            length |= (v==0) * 0x8000000000000000ULL;
            v -= (v==0);
            return _hashSetRoot.insert(v, length);
        }
        else return _hashSet.insert(v);
    }

    __attribute__((always_inline))
    uint64_t storage_find(uint64_t v, uint32_t, uint64_t length, bool isRoot = false) {
        if(dtree_unlikely(isRoot)) {
            length |= (v==0) * 0x8000000000000000ULL;
            v -= (v==0);
            return _hashSetRoot.find(v, length);
        }
        else return _hashSet.find(v);
    }

    __attribute__((always_inline))
    uint64_t storage_get(uint64_t idx, uint32_t, uint64_t& length, bool isRoot = false) {
        if(dtree_unlikely(isRoot)) {
            uint64_t v = _hashSetRoot.get(idx, length);
            v += (length >= 0x8000000000000000ULL);
            length &= 0x7FFFFFFFFFFFFFFFULL;
            return v;
        }
        return _hashSet.get(idx);
    }

public:
    typename HS::mapStats getStats() {
        typename HS::mapStats stats;
        stats += _hashSetRoot.getStats();
        stats += _hashSet.getStats();
        return stats;
    }

    typename HS::mapStats getRootStats() {
        return _hashSetRoot.getStats();
    }

    typename HS::mapStats getDataStats() {
        return _hashSet.getStats();
    }

    typename HS::probeStats getProbeStats() {
        typename HS::probeStats stats;
        stats += _hashSetRoot.getProbeStats();
        stats += _hashSet.getProbeStats();
        return stats;
    }

    template<typename CONTAINER>
    typename HS::mapStats getDensityStats(size_t bars, CONTAINER& elements, size_t map) {
        if(map == 0) {
            return _hashSetRoot.getDensityStats(bars, elements);
        } else {
            return _hashSet.getDensityStats(bars, elements);
        }
    }

    void getAllSizes(std::unordered_map<size_t,size_t>& allSizes) {
        _hashSetRoot.forAll([&allSizes](size_t v, size_t v2) {
            size_t length = v2 & 0x7FFFFFFFFFFFFFFFULL;
            allSizes[length]++;
        });
    }

protected:
    HSROOT _hashSetRoot;
    HS _hashSet;
};

template<typename HSROOT, typename HS>
class SeparateRootSingleHashSet {
public:
    SeparateRootSingleHashSet(): _hashSetRoot(), _hashSet() {
    }

    void setScale(size_t scale) {
        _hashSetRoot.setScale(scale);
        _hashSet.setScale(scale);
    }

    void setRootScale(size_t scale) {
        _hashSetRoot.setScale(scale);
    }

    void setDataScale(size_t scale) {
        _hashSet.setScale(scale);
    }

    size_t getRootScale() const {
        return _hashSetRoot._scale;
    }

    size_t getDataScale() const {
        return _hashSet._scale;
    }

    void init() {
        _hashSetRoot.init();
        _hashSet.init();
    }

    static constexpr uint64_t NotFound() {
        return HS::NotFound();
    }

protected:
    __attribute__((always_inline))
    uint64_t storage_fop(uint64_t v, uint32_t, uint64_t length, bool isRoot) {
        if(dtree_unlikely(isRoot)) {
//            length |= (v==0) * 0x8000000000000000ULL;
//            v -= (v==0);
//            printf("inserting in root %zx\n", v);
            return _hashSetRoot.insert(v);
        }
        else return _hashSet.insert(v);
    }

    __attribute__((always_inline))
    uint64_t storage_find(uint64_t v, uint32_t, uint64_t length, bool isRoot = false) {
        if(dtree_unlikely(isRoot)) {
//            length |= (v==0) * 0x8000000000000000ULL;
//            v -= (v==0);
            return _hashSetRoot.find(v);
        }
        else return _hashSet.find(v);
    }

    __attribute__((always_inline))
    uint64_t storage_get(uint64_t idx, uint32_t, uint64_t& length, bool isRoot = false) {
        if(dtree_unlikely(isRoot)) {
            uint64_t v = _hashSetRoot.get(idx);
//            v += (length >= 0x8000000000000000ULL);
//            length &= 0x7FFFFFFFFFFFFFFFULL;
            return v;
        }
        return _hashSet.get(idx);
    }

public:
    typename HS::mapStats getStats() {
        typename HS::mapStats stats;
        stats += _hashSetRoot.getStats();
        stats += _hashSet.getStats();
        return stats;
    }

    typename HS::mapStats getRootStats() {
        return _hashSetRoot.getStats();
    }

    typename HS::mapStats getDataStats() {
        return _hashSet.getStats();
    }

    typename HS::probeStats getProbeStats() {
        typename HS::probeStats stats;
        stats += _hashSetRoot.getProbeStats();
        stats += _hashSet.getProbeStats();
        return stats;
    }

    template<typename CONTAINER>
    typename HS::mapStats getDensityStats(size_t bars, CONTAINER& elements, size_t map) {
        if(map == 0) {
            return _hashSetRoot.getDensityStats(bars, elements);
        } else {
            return _hashSet.getDensityStats(bars, elements);
        }
    }

    void getAllSizes(std::unordered_map<size_t,size_t>& allSizes) {
    }

protected:
    HS _hashSetRoot;
    HS _hashSet;
};

template<typename HS>
class MultiLevelhashSet {
public:
    MultiLevelhashSet(size_t maps=2): _mask(maps-1), _hashSets() {
        //assert( (1U << (31 - __builtin_clz(maps))) - 1 == _mask && "Need power of two");
        assert( ((maps | _mask) == (maps + _mask)) && "Need power of two");
        _hashSets.reserve(maps);
        for(;maps--;) {
            _hashSets.emplace_back();
        }
    }

    size_t scale() {
        return _hashSets[0]._scale;
    }

    void setScale() const {
        for(auto& map: _hashSets) {
            map.emplace_back();
        }
    }

    void init() {
        for(auto& map: _hashSets) {
            map.init();
        }
    }

    static constexpr uint64_t NotFound() {
        return HS::NotFound();
    }

protected:
    __attribute__((always_inline))
    uint64_t storage_fop(uint64_t v, uint32_t level, bool) {
        level &= _mask;
        return _hashSets[level].insert(v);
    }

    __attribute__((always_inline))
    uint64_t storage_find(uint64_t v, uint32_t level, bool) {
        level &= _mask;
        return _hashSets[level].find(v);
    }

    __attribute__((always_inline))
    uint64_t storage_get(uint64_t idx, uint32_t level, bool) {
//        assert(idx && "construct called with 0-idx, indicates possible trouble");
        level &= _mask;
        return _hashSets[level].get(idx);
    }

    typename HS::mapStats getStats() {

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

struct DTreeIndex {
public:
    static constexpr DTreeIndex NotFound() {
        return DTreeIndex(0ULL);
    }

public:
    constexpr DTreeIndex(): _data(0) {}

    constexpr DTreeIndex(uint64_t const& d): _data(d) {}

    constexpr DTreeIndex(size_t id, size_t length): _data((id & 0x000000FFFFFFFFFF) | (length << 40)) {}

public:
    uint64_t getData() const { return _data; }

    constexpr bool operator==(DTreeIndex const& other) const {
        return _data == other._data;
    }

    constexpr bool operator!=(DTreeIndex const& other) const {
        return _data != other._data;
    }

    DTreeIndex& operator=(size_t data) {
        _data = data;
        return *this;
    }

    bool exists() const {
        return _data != 0;
    }

    uint64_t getID() const {
        return _data & 0x000000FFFFFFFFFF;
    }

    uint64_t getLength() const {
        return _data >> 40;
    }

    void setLength(size_t length) {
        _data &= 0x000000FFFFFFFFFF;
        _data |= (length << 40);
    }

public:
    friend std::ostream& operator<<(std::ostream& os, DTreeIndex const& state) {
        os << "<";
        os << state.getData();
        os << ">" << std::endl;
        return os;
    }

protected:
    uint64_t _data;
};

//struct DTreeIndexInserted {
//public:
//    static constexpr DTreeIndexInserted NotFound() {
//        return DTreeIndexInserted(0ULL);
//    }
//
//public:
//    constexpr DTreeIndexInserted(): _data(0) {}
//
//    constexpr DTreeIndexInserted(uint64_t const& d): _data(d) {}
//
//public:
//    uint64_t getData() const { return _data; }
//
//    bool operator==(DTreeIndex const& other) const {
//        return _data == other._data;
//    }
//
//    bool operator!=(DTreeIndex const& other) const {
//        return _data != other._data;
//    }
//
//    bool exists() const {
//        return _data != 0;
//    }
//
//    bool newlyInserted() const {
//        return _data >> 63;
//    }
//
//    DTreeIndex setLength(size_t length) const {
//        uint64_t data = _data & 0x000000FFFFFFFFFF;
//        data |= (length << 40);
//        return DTreeIndex(data);
//    }
//
//public:
//    friend std::ostream& operator<<(std::ostream& os, DTreeIndexInserted const& state) {
//        os << "<";
//        os << state.getData();
//        os << ">" << std::endl;
//        return os;
//    }
//
//protected:
//    uint64_t _data;
//};

struct DTreeIndexInserted {

    DTreeIndexInserted(): _stateID{0}, _inserted(0) {}

    DTreeIndexInserted(DTreeIndex stateID, bool isInserted): _stateID(stateID), _inserted(isInserted) {
    }

    DTreeIndexInserted(uint64_t stateID, size_t length): _stateID(stateID & 0x7FFFFFFFFFFFFFFFULL, length), _inserted(stateID >> 63) {
    }

    DTreeIndex getState() const {
        return _stateID;
    }

    bool isInserted() const {
        return _inserted;
    }
protected:
    DTreeIndex _stateID;
    uint64_t _inserted;
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
template<typename Storage, typename INDEX = DTreeIndex, typename INDEXINSERTED = DTreeIndexInserted>
class dtree: public Storage {
public:

    enum class STORAGE_FLAG {
        CONSTRUCT_USING_ROOT,
        DECONSTRUCT_USING_ROOT,
    };

    static constexpr bool REPORT = 0;

    /**
     * Index is a unique mapping to a vector. The upper 32 bits are the
     * length of the vector, the lower 32 bits are the unique index.
     */
    using Index = INDEX;
    using IndexInserted = INDEXINSERTED;

    struct SparseOffset {

    public:
        __attribute__((always_inline))
        constexpr SparseOffset(uint32_t const& offset, uint32_t const& length): _data((offset << 8) | length) {}

        __attribute__((always_inline))
        constexpr SparseOffset(uint32_t const& data): _data(data) {}

        __attribute__((always_inline))
        constexpr SparseOffset(): _data(0) {}
    public:

        __attribute__((always_inline))
        uint32_t getOffset() const {
            return _data >> 8;
        }

        __attribute__((always_inline))
        uint32_t getLength() const {
            return _data & 0xFF;
        }

        __attribute__((always_inline))
        uint32_t getOffsetPart() const {
            return _data & 0xFFFFFF00;
        }

        __attribute__((always_inline))
        bool operator<(SparseOffset const& other) {
            return _data < other._data;
        }

        __attribute__((always_inline))
        uint32_t getData() const {
            return _data;
        }

        uint32_t _data;
    };

    struct MultiOffset {

    public:
        enum class Options {
            NONE,
            READ,
            WRITE,
            READ_WRITE
        };
    public:
        __attribute__((always_inline))
        constexpr MultiOffset(uint32_t const& offset, uint32_t const& options): _data(offset | (options << 24)) {}

        __attribute__((always_inline))
        constexpr MultiOffset(uint32_t const& data): _data(data) {}

        __attribute__((always_inline))
        constexpr MultiOffset(): _data(0) {}
    public:

        __attribute__((always_inline))
        void init(uint32_t const& offset, uint32_t const& options) {
            _data = offset | (options << 24);
        }

        __attribute__((always_inline))
        uint32_t getOffset() const {
            return _data & 0xFFFFFF;
        }

        __attribute__((always_inline))
        Options getOptions() const {
            return (Options)(_data >> 24);
        }

        __attribute__((always_inline))
        bool operator<(SparseOffset const& other) {
            return _data < other._data;
        }

        __attribute__((always_inline))
        uint32_t getData() const {
            return _data;
        }

        bool operator==(MultiOffset const& other) const {
            return _data == other._data;
        }

        bool operator==(uint32_t const& data) const {
            return _data == data;
        }

        uint32_t _data;
    };

    struct LengthAndOffset: private MultiOffset {
    public:
        __attribute__((always_inline))
        constexpr LengthAndOffset(uint32_t const& length, uint32_t const& offsets): MultiOffset(length, offsets) {}

        __attribute__((always_inline))
        constexpr LengthAndOffset(uint32_t const& data): MultiOffset(data) {}

        __attribute__((always_inline))
        constexpr LengthAndOffset(): MultiOffset() {}
    public:

        __attribute__((always_inline))
        void init(uint32_t const& length, uint32_t const& offsets) {
            this->_data = length | (offsets << 24);
        }

        __attribute__((always_inline))
        uint32_t getLength() const {
            return this->_data & 0xFFFFFF;
        }

        __attribute__((always_inline))
        uint32_t getOffsets() const {
            return this->_data >> 24;
        }
    };

    struct Projection {
    public:
        constexpr Projection(SparseOffset* const& offsets): _offsets(offsets) {}
        template<typename T>
        constexpr Projection(T* const& offsets): _offsets((SparseOffset*)offsets) {}
    public:

        SparseOffset* const& getOffsets() const {
            return _offsets;
        }

        SparseOffset* _offsets;
    };

    struct MultiProjection {
    public:

        struct SingleProjection {

            SingleProjection() {}

            SingleProjection(uint32_t options, uint32_t length, std::initializer_list<uint32_t> offsets) {
                int i = 0;
                for(auto& o: offsets) {
                    _data[i++].init(options, o);
                }
                lando.init(i, length); // TODO: determine if we want nr of offsets or stride
            }

            typename MultiOffset::Options getOptions() const {
                return _data[0].getOptions();
            }

            LengthAndOffset getLengthAndOffsets() const {
                return lando;
            }

            MultiOffset* getOffsets() {
                return _data;
            }

            MultiOffset& getOffset(uint32_t level) {
                return _data[level];
            }

            MultiOffset const& getOffset(uint32_t level) const {
                return _data[level];
            }

            uint32_t getLength(uint32_t level) const {
                return (level < lando.getOffsets() - 1) ? 2 : lando.getLength();
            }

            friend std::ostream& operator<<(std::ostream& out, SingleProjection const& p) {
                out << p.getLengthAndOffsets().getLength();
                out << "@( ";
                for(size_t o = 0; o < p.getLengthAndOffsets().getOffsets(); ++o) {
                    out << p.getOffset(o).getOffset() << " ";
                }
                out << ")";
                return out;
            }

            LengthAndOffset lando;
            MultiOffset _data[];
        };
    public:

        size_t getProjections() const {
            return projections;
        }

        size_t getStride() const {
            return maxDepth + 1;
        }

        SingleProjection& getProjection(size_t idx) {
            return *(SingleProjection*)&_projections[idx * getStride()];
        }

        SingleProjection const& getProjection(size_t idx) const {
            return *(SingleProjection*)&_projections[idx * getStride()];
        }

        void addProjection(uint32_t options, uint32_t length, std::initializer_list<uint32_t> offsets) {
            assert(projections < maxProjections);
            new(&getProjection(projections)) SingleProjection(options, length, offsets);
            projections++;
        }

        friend std::ostream& operator<<(std::ostream& out, MultiProjection const& mp) {
            out << "MultiProjection<";
            for(size_t p = 0; p < mp.getProjections(); ++p) {
                SingleProjection const& projection = mp.getProjection(p);
                out << " " << projection;
            }
            out << " >";
            return out;
        }

        void dump() const {
            std::cout << *this;
        }

        uint32_t projections;
        uint32_t maxProjections;
        uint32_t maxDepth;
        uint32_t _projections[];
    };

    struct DTreeNode {
    public:
        constexpr DTreeNode(): _data(0) {}

        constexpr DTreeNode(uint64_t const& d): _data(d) {}

        constexpr DTreeNode(uint32_t const& left, uint32_t const& right): _data((uint64_t)left | ((uint64_t)right << 32ULL)) {}

    public:
        uint64_t getData() const { return _data; }

        constexpr bool operator==(DTreeIndex const& other) const {
            return _data == other.getData();
        }

        constexpr bool operator!=(DTreeIndex const& other) const {
            return _data != other.getData();
        }

        DTreeIndex& operator=(size_t data) {
            _data = data;
            return *this;
        }

        bool exists() const {
            return _data != 0;
        }

        uint64_t getLeftPart() const {
            return _data & 0xFFFFFFFFULL;
        }

        uint64_t getRightPart() const {
            return _data & 0xFFFFFFFF00000000ULL;
        }

        uint32_t getLeft() const {
            return _data & 0xFFFFFFFFULL;
        }

        uint32_t getRight() const {
            return _data >> 32ULL;
        }

        void addPart(uint64_t part) {
            _data |= part;
        }

        void setLeft(uint32_t left) {
            // This compiles down to a single instruction
            _data &= 0xFFFFFFFF00000000ULL;
            _data |= (uint64_t)left;
        }

        void setRight(uint32_t right) {
            // This compiles down to a single instruction
            _data &= 0x00000000FFFFFFFFULL;
            _data |= ((uint64_t)right) << 32;
        }

    public:
        friend std::ostream& operator<<(std::ostream& os, DTreeNode const& state) {
            os << "<";
            os << std::hex << state.getData();
            os << ">" << std::endl;
            return os;
        }

    protected:
        uint64_t _data;
    };

    struct DTreeRootNode {

        DTreeRootNode(DTreeNode node, uint64_t length): _node(node), _length(length) {}
        DTreeNode& getNode() { return _node; }
        uint64_t getLength() { return _length; }
    protected:
        DTreeNode _node;
        uint64_t _length;
    };

public:

    /**
     * @brief Construct a new instance.
     * @param rootIndexScale The underlying hash map can maximally hold 2^rootIndexScale 64-bit entries.
     */
    dtree(): Storage(), insertedZeroes(false) {
    }

    std::atomic<bool> insertedZeroes;
    void checkForInsertedZeroes(uint64_t& idxResult) {
        if(dtree_unlikely(idxResult == 0ULL)) {
            if(dtree_unlikely(!insertedZeroes.load(std::memory_order_relaxed))) {
                bool F = false;
                if(insertedZeroes.compare_exchange_strong(F, true, std::memory_order_seq_cst)) {
                    idxResult = 0x8000000000000000ULL;
                }
            }
        }
    }

    /**
     * @brief Determines whether or not the specified vector is in the tree.
     * @param data The data of the vector.
     * @param length The length of the vector in number of 32bit units.
     * @return Index to the vector, or NotFound when no such vector is in the tree.
     */
    Index find(uint32_t* data, uint32_t length, bool isRoot) {
        auto result = findRecursing(data, length, isRoot);
        if(result == Storage::NotFound()) {
            if(REPORT) printBuffer("NOT Found", data, length, result);
            return INDEX::NotFound();
        }
        if(REPORT) printBuffer("Found", data, length, result);
        return Index(result, length);//findNode(result, length, isRoot);
    }

    /**
     * @brief Deconstructs the specified data into the compression tree.
     * Returns an @c Index that unique identifies the deconstructed vector.
     * @param data The data with length @c length to insert.
     * @param length Length of @c data in number of 32bit units.
     * @return Unique index that can be used to retrieve the data.
     */
    IndexInserted insert(uint32_t* data, uint32_t length, bool isRoot) {
        uint64_t result = length == 0 ? 0 : deconstruct(data, length, isRoot);
        checkForInsertedZeroes(result);
        if(REPORT) printBuffer("Inserted", data, length, result);
        assert(length > 0);
        return IndexInserted(result, length);
    }

    /**
     * @brief Deconstructs the specified vector into the compression tree.
     * Returns an @c Index that unique identifies the deconstructed vector.
     * @param data The data with length @c length to insert.
     * @param length Length of @c data in bytes.
     * @return Unique index that can be used to retrieve the data.
     */
//    Index insertBytes(uint8_t* data, uint32_t length) {
//        uint64_t result = (uint64_t)deconstructBytes(data, length, true);
//        if(REPORT) printBuffer("Inserted", (char*)data, length, result);
//        return Index(result);
//    }

    /**
     * @brief Constructs the entire vector using the specified Index. Make sure the length of the vector
     * is a multiple of 4 bytes, otherwise use @c getBytes().
     * @param idx Index of the vector to construct.
     * @param buffer Buffer where the vector will be stored.
     */
    void getLength(Index idx, uint64_t& length) const {
        //construct(idx.getID(), length);
        length = idx.getLength();
    }

    DTreeRootNode getRootNode(Index idx, bool isRoot) {
        uint64_t length;
        uint64_t node = construct(idx.getID(), 0, length, isRoot);
        //return DTreeRootNode(node, isRoot ? length : idx.getLength());
        return DTreeRootNode(node, idx.getLength());
    }

    Index findNode(DTreeNode node, uint64_t length, bool isRoot) {
        return findRecursing(node.getData(), 0, length, isRoot);
    }

//    IndexInserted addRootNode(uint64_t idx, uint64_t length, bool isRoot) {
//        return deconstruct(idx, 0, length * isRoot);
//    }

    /**
     * @brief Constructs the entire vector using the specified Index. Make sure the length of the vector
     * is a multiple of 4 bytes, otherwise use @c getBytes().
     * @param idx Index of the vector to construct.
     * @param buffer Buffer where the vector will be stored.
     * @return false
     */
    bool get(Index idx, uint32_t* buffer, bool isRoot) {
        if(idx.getLength() == 0) return true;

        DTreeRootNode root = getRootNode(idx, isRoot);

        if(REPORT) printf("get(%zx, %zu, %u)\n", root.getNode().getData(), root.getLength(), isRoot);

        if(root.getLength() <= 2) {
//            if(root.getLength() == 0) {
//                return false;
//            }
            if(root.getLength() == 1) {
                if(REPORT) printf("Got %8zx(%zu) -> %16zx\n", idx.getData(), root.getLength(), root.getNode().getData());
                *buffer = root.getNode().getData();
                return true;
            } else {
                if(REPORT) printf("Got %8zx(%zu) -> %16zx\n", idx.getData(), root.getLength(), root.getNode().getData());
                *(uint64_t*)buffer = root.getNode().getData();
                return true;
            }
        }

        if(root.getNode() == 0) {
            memset(buffer, 0, root.getLength() * sizeof(uint32_t));
            return true;
        }

        uint32_t maxPowerTwoInLength = 1U << (31 - __builtin_clz(root.getLength()));
        if(maxPowerTwoInLength == root.getLength()) {
            constructP2Mapped(root.getNode().getData(), root.getLength(), buffer);
        } else {
            constructP2(root.getNode().getLeft(), maxPowerTwoInLength, buffer, false);
            construct(root.getNode().getRight(), root.getLength()-maxPowerTwoInLength, buffer + maxPowerTwoInLength, false);
        }

        if(REPORT) printBuffer("Constructed", buffer, idx.getLength(), idx.getID());
        return true;
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
    bool getPartial(Index idx, uint32_t offset, uint32_t length, uint32_t* buffer, bool isRoot) {
        constructPartial(idx.getID(), idx.getLength(), offset, length, buffer, isRoot);
        if(REPORT) printBuffer("Constructed partially", buffer, length, idx.getID());
        return true;
    }

    void getPartial(Index idx, MultiProjection& projection, bool isRoot, uint32_t* buffer) {
        multiConstruct(idx, isRoot, projection, 0, 0, projection.getProjections(), buffer);
    }

//    bool getSparseUnits(Index idx, uint32_t* buffer, uint32_t offsets, uint32_t* offset, bool isRoot) {
//        constructSparseUnits(idx.getID(), idx.getLength(), 0, buffer, offsets, offset, isRoot);
//        if(REPORT) {
//            printBuffer("Constructed sparse units", (char*)buffer, offsets, 0);
//        }
//        return false;
//    }

    bool getSparse(Index idx, uint32_t* buffer, uint32_t offsets, Projection projection, bool isRoot) {

        constructSparse(idx.getID(), idx.getLength(), 0, buffer, offsets, projection, isRoot);
        if(REPORT) {
            size_t s = 0;
            SparseOffset* offset = projection.getOffsets();
            SparseOffset* end = offset + offsets;
            while(offset < end) {
                s += offset->getLength();
                offset++;
            }
            printBuffer("Constructed sparse", buffer, s, 0);
        }
        return false;
    }

    IndexInserted deltaSparse(Index idx, uint32_t* deltaData, uint32_t offsets, Projection offset, bool isRoot) {
        uint32_t length = idx.getLength();
        uint64_t result = deltaSparseApply(idx.getID(), length, 0, deltaData, offsets, offset, isRoot);
        checkForInsertedZeroes(result);

        IndexInserted R(result, length);
        if(REPORT) {
            uint32_t buffer[R.getState().getLength()];
            get(R.getState(), buffer, isRoot);
            printBuffer("Inserted", buffer, idx.getLength(), result);
        }
        assert(length > 0);
        return R;
    }

    IndexInserted deltaSparseStride(Index idx, uint32_t* deltaData, uint32_t offsets, uint32_t* offset, uint32_t stride, bool isRoot) {
        uint32_t length = idx.getLength();
        uint64_t result = deltaSparseApply(idx.getID(), length, 0, deltaData, offsets, offset, stride, isRoot);
        checkForInsertedZeroes(result);
        if(REPORT) {
            uint32_t buffer[length];
            get(result, buffer, isRoot);
            printBuffer("Inserted", buffer, length, result);
        }
        assert(length > 0 && "length is 0");
        return IndexInserted(result, length);
    }

    /**
     * @brief Returns a single 64bit part of the vector associated with @c idx.
     * @param idx Index of the vector of which a 64bit part is desired.
     * @param offset Offset within the vector to the desired single 64bit entry. MUST be multiple of 2.
     * @return The desired 64bit part.
     */
//    uint64_t getSingle(Index idx, uint32_t offset, bool isRoot) {
//        uint32_t length = idx.getLength();
//        if(offset >= length) return 0;
//        return getSingleRecursive(idx.getID(), length, offset, isRoot);
//    }

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
    IndexInserted delta(Index idx, uint32_t offset, const uint32_t* deltaData, uint32_t deltaLength, bool isRoot) {

        DTreeRootNode root = getRootNode(idx, isRoot);

        uint64_t result = deltaApplyMapped(root.getNode(), root.getLength(), offset, deltaLength, deltaData, isRoot);
        if(root.getNode().getData() == result) {
            result = idx;
        } else {
            uint32_t level = lengthToLevel(root.getLength());
            result = deconstruct(result, level, root.getLength(), isRoot);
        }
        checkForInsertedZeroes(result);
        IndexInserted R(result, root.getLength());
        if(REPORT) {
            uint32_t buffer[R.getState().getLength()];
            get(R.getState(), buffer, isRoot);
            printBuffer("Inserted", buffer, idx.getLength(), result);
        }
        assert(root.getLength() > 0);
        return R;
    }

    IndexInserted delta(Index idx, MultiProjection& projection, bool isRoot, uint32_t* buffer) {
        return multiDelta(idx, isRoot, projection, 0, 0, projection.getProjections(), buffer);
    }


    IndexInserted multiDeltaNaive(Index idx, bool isRoot, MultiProjection& projection, uint32_t level, uint32_t start, uint32_t end, uint32_t* buffer) {
        uint64_t idxWithoutLength = idx.getID();
        uint64_t length = idx.getLength();
        uint64_t mapped = construct(idxWithoutLength, 0, length, level == 0 && isRoot);

        if constexpr(REPORT) printf("%*c", level*4, ' ');
        if constexpr(REPORT) printf("multiDeltaNaive(%zx, %u, %u, %u, %p)\n", idx, level, start, end, buffer);

        uint32_t* bufferPosition = buffer;

        auto projections = projection.getProjections();

        uint64_t jump[projections];
        uint64_t jumps = 0;

        // Go through all projections to find the required data in the current tree
        for(uint32_t pid = start; pid < end;) {
            if constexpr(REPORT) printf("%*c", level*4, ' ');
            if constexpr(REPORT) printf("pid %u\n", pid);
            auto& p = projection.getProjection(pid);

            LengthAndOffset lando = p.getLengthAndOffsets();
            uint32_t len = lando.getLength();

            // If the current projection projects to the current tree
            if(level == lando.getOffsets() - 1) {
                mapped = deltaMapped2(mapped, length, projection.getProjection(pid).getOffset(level).getOffset(), len, bufferPosition);
                if constexpr(REPORT) {
                    printf("%*c", level*4, ' ');
                    printf("apply local delta, deltaMapped2() -> %zx, copied", mapped);
                    for(uint32_t l = 0; l < len; ++l) {
                        printf(" %x", bufferPosition[l]);
                    }
                    printf("\n");
                }
                ++pid;
            }

                // If the current projection projects to another tree, get the index of that tree in order to jump
            else {
                uint32_t currentOffset = p.getOffset(level).getOffset();
                assert((currentOffset & 0x1) == 0);
                pid++;
                while(projection.getProjection(pid).getOffset(level).getOffset() == currentOffset) {
                    bufferPosition += projection.getProjection(pid).getLengthAndOffsets().getLength();
                    pid++;
                }

                traverse2(mapped, length, currentOffset, 2, &std::remove_pointer<decltype(this)>::type::traverseP2_construct, &std::remove_pointer<decltype(this)>::type::traverseP2_construct12, (uint32_t*)(&jump[jumps]));
                if constexpr(REPORT) printf("%*c", level*4, ' ');
                if constexpr(REPORT) printf("get subtree index, traverse2(%zx, %u, %u, 2, %p) -> %zx\n", mapped, length, currentOffset, (uint32_t*)(&jump[jumps]), jump[jumps]);
                jumps++;
            }
            bufferPosition += len;

        }

        jumps = 0;

        // Go through all projections to find the jumps and traverse them
        bufferPosition = buffer;
        for(uint32_t pid = start; pid < end;) {
            auto& p = projection.getProjection(pid);

            LengthAndOffset lando = p.getLengthAndOffsets();

            //
            if(level < lando.getOffsets() - 1) {
                uint32_t currentOffset = p.getOffset(level).getOffset();
                assert((currentOffset & 0x1) == 0);

                uint32_t pidEnd = pid + 1;
                auto bufferPositionCurrent = bufferPosition;
                while(projection.getProjection(pidEnd).getOffset(level) == currentOffset) {
                    bufferPosition += projection.getProjection(pidEnd).getLengthAndOffsets().getLength();
                    pidEnd++;
                }

                if constexpr(REPORT) printf("%*c", level*4, ' ');
                if constexpr(REPORT) printf("go into subtree\n");
                auto newIdx = multiDeltaNaive(jump[jumps], false, projection, level+1, pid, pidEnd, bufferPositionCurrent);
                uint64_t m = newIdx.getState().getData();
                mapped = deltaMapped2(mapped, length, currentOffset, 2, (uint32_t*) &m);
                if constexpr(REPORT) printf("%*c", level*4, ' ');
                if constexpr(REPORT) printf("apply new subtree index %zx, deltaMapped2() -> %zx\n", m, mapped);
                pid = pidEnd;
            } else {
                ++pid;
            }
            bufferPosition += lando.getLength();

        }
        if constexpr(REPORT) printf("%*c", level*4, ' ');
        if constexpr(REPORT) printf("returning %zx\n", mapped);
        uint64_t result = deconstruct(mapped, 0, length, isRoot && level == 0);
        checkForInsertedZeroes(result);
        return IndexInserted(result, length);
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
    IndexInserted deltaMayExtend(Index idx, uint32_t offset, const uint32_t* deltaData, uint32_t deltaLength, bool isRoot) {
        if(deltaLength == 0) return DTreeIndexInserted(idx, false);
        uint64_t length;
        getLength(idx, length);
        uint32_t newLength = std::max(length, (uint64_t)offset + deltaLength);
        uint64_t result = deltaApplyMayExtend(idx.getID(), length, offset, deltaData, deltaLength, isRoot);
        checkForInsertedZeroes(result);
        if(REPORT) {
            auto R = IndexInserted(result, newLength);
            uint32_t buffer[newLength];
            get(R.getState().getData(), buffer, isRoot);
            printBuffer("Inserted", buffer, newLength, result);
        }
        assert(newLength > 0 && "length is 0");
        return IndexInserted(result, newLength);
    }

    /**
     * @brief Deconstructs a new vector that is based on the vector specified by @c idx, but shrunk
     * to @c shrinkTo units. The vector will be shrunk to the left.
     * @param idx Index of the vector to base the new vector on.
     * @param shrinkTo Number of units (32bit integers) of the new vector.
     * @return Index to the newly deconstructed vector.
     */
//    IndexInserted shrink(Index idx, uint32_t shrinkTo, bool isRoot) {
//        return IndexInserted(shrinkRecursive(idx.getID(), idx.getLength(), shrinkTo), shrinkTo, isRoot);
//    }

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
    IndexInserted extend(Index idx, uint32_t alignment, uint32_t deltaLength, uint32_t const* data, bool isRoot) {
        uint64_t length;
        getLength(idx, length);
        assert(alignment > 0 && "alignment should be at least 1");
        assert((alignment & (alignment-1)) == 0 && "alignment should be power of two");
        alignment--;
        size_t padding = ((length + alignment) & ~alignment) - length;
        return extendAt(idx, padding, deltaLength, data, isRoot);
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
    IndexInserted extendAt(Index idx, uint32_t offset, uint32_t deltaLength, uint32_t const* data, bool isRoot) {
        uint64_t result;
        uint64_t length;
        getLength(idx, length);
        if(idx == 0) {
            result = deltaLength == 0 ? 0 : insertZeroPrepended(data, deltaLength, offset, isRoot);
        } else if(deltaLength == 0) {
            result = zeroExtend(idx.getID(), length, length + offset, isRoot, isRoot);
        } else {
            result = extendRecursive(idx.getID(), length, offset, deltaLength, data, isRoot, isRoot);
        }
        checkForInsertedZeroes(result);
        uint64_t newLength = length + (uint64_t)offset + (uint64_t)deltaLength;
        if(REPORT) {
            uint32_t buffer[1 + newLength];
            buffer[newLength] = 0;
            get(result, buffer, isRoot);
            printBuffer("Inserted", buffer, idx.getLength(), result);
        }
        return IndexInserted(result, newLength);
    }

    IndexInserted extend(Index idx, uint32_t extendWith, bool isRoot) {
        uint64_t result;
        uint64_t length;
        getLength(idx, length);
        result = zeroExtend(idx.getID(), length, length + extendWith, isRoot, isRoot);
//        checkForInsertedZeroes(result); // not needed because we can only get zeroes if we start with zeroes
        uint64_t newLength = length + (uint64_t)extendWith;
        if(REPORT) {
            uint32_t buffer[1 + newLength];
            buffer[newLength] = 0;
            get(result, buffer, isRoot);
            printBuffer("Inserted", buffer, idx.getLength(), result);
        }
        return IndexInserted(result, newLength);
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
//    Index deltaB(Index idx, uint32_t offset, uint32_t deltaLength, uint8_t* data) {
//        uint64_t lengthPart = idx & 0xFFFFFFFF00000000ULL;
//        uint32_t lengthInBytes = lengthPart >> 32;
//        uint64_t result = deltaApplyInBytes(idx, lengthInBytes, offset, deltaLength, data) | lengthPart;
//        if(REPORT) {
//            uint32_t buffer[lengthInBytes/4];
//            get(result, buffer);
//            printBuffer("Inserted", (char*)buffer, lengthInBytes, result);
//        }
//        return result;
//    }

//    template<typename CONTAINER>
//    void getProbeStats(size_t bars, CONTAINER& elements, size_t map) {
////        _hashSets[map].getProbeStats(bars, elements);
//    }

    void printBuffer(const char* action, uint32_t* buffer, uint32_t length, uint64_t result) {
        printf("%s", action);
        uint32_t* end = buffer + length;
        while(buffer < end) {
            //printf("%02x", *buffer);
            printf(" %x", *buffer);
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

    template<typename FUNC>
    void setHandlerFull(FUNC&& handler_full) {
        _handler_full = handler_full;
    }

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
    uint64_t deconstruct(uint64_t v, uint32_t level, uint64_t length, bool isRoot) {
//        if(v==0ULL) return 0;
        uint64_t idx = this->storage_fop(v, level, length, isRoot);
#ifndef NDEBUG
        if(idx == Storage::NotFound()) {
            _handler_full(v, isRoot);
        }
#endif

//        printf("deconstruct(%16zx,%u): %16zx\n", v, isRoot, idx);
        if(REPORT) printf("Dec %8zx(%u) <- %16zx (%s)\n", idx & 0x7FFFFFFFFFFFFFFF, 8, v, isRoot ? "root":"");
        return idx;
    }

    __attribute__((always_inline))
    uint64_t deconstruct(uint64_t v, uint32_t level) {
//        if(v==0ULL) return 0;
        uint64_t idx = this->storage_fop(v, level, 2, false);
#ifndef NDEBUG
        if(idx == Storage::NotFound()) {
            _handler_full(v, false);
        }
#endif
//        printf("deconstruct(%16zx,%u): %16zx\n", v, isRoot, idx);
        if(REPORT) printf("Dec %8zx(%u) <- %16zx\n", idx & 0x7FFFFFFFFFFFFFFF, 8, v);

        return idx;
    }

    __attribute__((always_inline))
    uint64_t findRecursing(uint64_t v, uint32_t level, uint64_t length, bool isRoot = false) {
//        if(v==0ULL) return 0ULL;
        uint64_t idx = this->storage_find(v, level, length, isRoot);
        return idx;
    }

    __attribute__((always_inline))
    uint64_t construct(uint64_t idx, uint32_t level, uint64_t& length, bool isRoot = false) {
//        if(idx == 0) return 0;
        uint64_t mapped = this->storage_get(idx, level, length, isRoot);
//        assert(mapped != Storage::NotFound() && "Cannot construct: index is not in the map");
        if(REPORT) printf("Got %8zx(%u) -> %16zx (%s)\n", idx, 8, mapped, isRoot ? "root":"");
        return mapped;
    }

    __attribute__((always_inline))
    uint64_t construct(uint64_t idx, uint32_t level, bool isRoot = false) {
//        if(idx == 0) return 0;
        uint64_t g;
        uint64_t mapped = this->storage_get(idx, level, g, isRoot);
//        assert(mapped != Storage::NotFound() && "Cannot construct: index is not in the map");
        if(REPORT) printf("Got %8zx(%u) -> %16zx (%s)\n", idx, 8, mapped, isRoot ? "root":"");
        return mapped;
    }

//    a b c d e f
//     1   2   3
//       4
//           5
//
//    5
//    43
//    12ef
//
//    a b c d e f
//    123
//    43
//    5
//
//    6: 4 + 2
//
//    a b c d e f g h i j
//    12345
//    675
//    85
//    9
//
//    10: decode(8), copy(2)
//    decode(4+4), copy(2)
//    decode(2+2+2+2), decode(2)
//
//    a b c d e f g h i j k
//    12345k
//    678
//    98
//    a
//
//    11: decode(8), copy(3)
//    decode(4+4), copy(3)
//    decode(2+2+2+2), decode(2), copy(1)
//
//    a b c d e f g
//    123g
//    45
//    6
//
//    7: 4 + 3
//    6
//    45
//
//    171: 128 + 43
//
//
    void constructInline(uint32_t* dataSource, uint32_t length, bool isRoot) {
        uint64_t * data = (uint64_t*)dataSource;

    }

    void deconstructInline(uint32_t* data, uint32_t length, bool isRoot) {
        uint64_t * dataSource = (uint64_t*)data;

        uint32_t currentLength = length;
        while(currentLength > 2) {
            uint32_t currentLengthDiv2 = currentLength >> 1;

            // dataSource
            // [ ]_[ ] [ ]_[ ] [ ]_[ ] [ ]_[ ]
            //    |       |       |       |
            //   /   ----/-------/       /
            //  |   /    /  /-----------/
            // [ ] [ ] [i] [ ]
            // data
            for(uint32_t i = 0; i < currentLengthDiv2; ++i) {
                data[i] = deconstruct(dataSource[i], 0);
            }

            // dataSource
            // <--------- currentLength --------->
            // [ ]_[ ] [ ]_[ ] [ ]_[ ] [ ]_[ ] [ ]
            //    |       |       |       |    /
            //   /   ----/-------/       /    /
            //  |   /    /  /-----------/    /
            //  |   |   |   |   /-----------/
            // [ ] [ ] [ ] [ ] [ ]
            // <-------------> currentLengthDiv2
            // data
            if(currentLength & 0x1) {
                data[currentLengthDiv2] = data[currentLengthDiv2 << 1];
                currentLength = currentLengthDiv2 + 1;
            } else {
                currentLength = currentLengthDiv2;
            }
        }
    }

    uint64_t deconstruct(const uint32_t* data, uint32_t length, bool isRoot) {
        if (dtree_unlikely(length == 1)) {
            return *data;
        } else if (length == 2) {
//            return deconstruct(*(uint64_t *)data, 0, length, isRoot);
            return deconstruct((uint64_t)data[0] | (((uint64_t)data[1]) << 32), 0, length, isRoot);
        }


        uint32_t level = lengthToLevel(length);
        uint32_t lengthDiv2 = length / 2;
        uint32_t buffer[lengthDiv2 + 1];
        for (uint32_t i = 0; i < lengthDiv2; ++i) {
            buffer[i] = deconstruct(((uint64_t *)data)[i], level);
        }
        if(length & 0x1) {
            buffer[lengthDiv2] = data[lengthDiv2 * 2];
            deconstructInline(buffer, lengthDiv2 + 1, isRoot);
        } else {
            deconstructInline(buffer, lengthDiv2, isRoot);
        }
        return deconstruct((uint64_t)buffer[0] | (((uint64_t)buffer[1]) << 32), 0, length, isRoot);
    }
//    uint64_t deconstructP2(uint32_t* buffer, uint32_t length, bool isRoot) {
//        uint64_t * bufferSource = (uint64_t*)buffer;
//
//        for(; length > 2; ) {
//            length >>=1;
//            for(uint32_t i = 0; i < length; ++i) {
//                buffer[i] = deconstruct(bufferSource[i], 0, false);
//            }
//        }
//        uint32_t level = lengthToLevel(length);
//        return deconstruct(bufferSource[0], level, isRoot);
//    }

//    uint64_t deconstruct(uint32_t* data, uint32_t length, bool isRoot) {
//        if (dtree_unlikely(length == 1)) {
//            return *data;
//        } else if (length == 2) {
//            return deconstruct(*(uint64_t *) data, 0, isRoot);
//        }
//
//        uint32_t level = lengthToLevel(length);
//
//        uint32_t maxPowerTwoInLength = 1U << (31 - __builtin_clz(length));
//        uint32_t maxPowerTwoInLengthDiv2 = maxPowerTwoInLength / 2;
//        uint32_t buffer[maxPowerTwoInLengthDiv2];
//        for (uint32_t i = 0; i < maxPowerTwoInLengthDiv2; ++i) {
//            buffer[i] = deconstruct(((uint64_t *)data)[i], level - 1, false);
//        }
//        if (length == maxPowerTwoInLength) {
//            return deconstructP2(buffer, maxPowerTwoInLengthDiv2, isRoot);
//        } else {
//            uint32_t l = deconstructP2(buffer, maxPowerTwoInLengthDiv2, false);
//            uint32_t r = deconstruct(data + maxPowerTwoInLength, length - maxPowerTwoInLength, false);
//            return deconstruct(((uint64_t) r) << 32 | (uint64_t) l, level, isRoot);
//        }
//    }

//    uint64_t deconstruct(uint32_t* data, uint32_t length, bool isRoot) {
//        if(dtree_unlikely(length == 1)) {
//            return *data;
//        } else if (length == 2) {
//            return deconstruct(*(uint64_t*)data, 0, isRoot);
//        }
//
//        uint32_t level = lengthToLevel(length);
//
//        uint32_t leftLength = 1 << level;
//        uint32_t l = deconstruct(data, leftLength, false);
//        uint32_t r = deconstruct(data+leftLength, length-leftLength, false);
//        return deconstruct(((uint64_t)r) << 32 | (uint64_t)l, level, isRoot);
//    }

//    uint64_t deconstructBytes(uint8_t* data, uint32_t length) {
//        if(length <= 4) {
//            uint32_t d;
//            memmove(&d, data, length);
//            return d;
//        } else if(length <= 8) {
//            return deconstruct(*(uint64_t*)data, 0);
//        }
//        uint32_t level = lengthToLevel(length);
//        uint32_t leftLength = 1 << level;
//        uint32_t l = deconstructBytes(data, leftLength);
//        uint32_t r = deconstructBytes(data+leftLength, length-leftLength);
//        return deconstruct(((uint64_t)r) << 32 | (uint64_t)l, level - 2);
//    }

    uint64_t findRecursing(uint32_t* data, uint32_t length, bool isRoot) {
        if(length == 1) {
            return *data;
        } else if (length == 2) {
            return findRecursing(*(uint64_t*)data, 0, length, isRoot);
        }

        uint32_t level = lengthToLevel(length);
        uint32_t leftLength = 1 << level;
        uint64_t l = findRecursing(data, leftLength, false);
        if(l == Storage::NotFound()) {
            return Storage::NotFound();
        }
        uint64_t r = findRecursing(data+leftLength, length-leftLength, false);
        if(r == Storage::NotFound()) {
            return Storage::NotFound();
        }
        return findRecursing(((uint64_t)r) << 32 | (uint64_t)l, level, length, isRoot);
    }

    uint64_t getSingleRecursive(uint64_t idx, uint32_t length, uint32_t offset, bool isRoot) {
        if(length == 2) {
            return construct(idx, 0);
        }
        uint32_t level = lengthToLevel(length);
        uint64_t mapped = construct(idx, level, length, isRoot);
        uint32_t leftLength = 1 << level;
        if(offset < leftLength) {
            return getSingleRecursive(mapped, leftLength, offset, false);
        } else {
            return getSingleRecursive(mapped >> 32ULL, length - leftLength, offset - leftLength, false);
        }
    }

    void constructP2(uint64_t idx, uint32_t length, uint32_t* buffer, bool isRoot) {
        uint64_t* bufferDest = (uint64_t*)buffer;

        if(REPORT) printf("constructP2(%zx, %u, %u)\n", idx, length, isRoot);

        bufferDest[0] = construct(idx, length, isRoot);

        for(uint32_t levelLength = 2; levelLength < length; levelLength <<=1) {
            for(uint32_t i = levelLength; i--;) {
                bufferDest[i] = construct(buffer[i], false);
            }
        }
    }

    void constructP2Mapped(uint64_t mapped, uint32_t length, uint32_t* buffer) {
        uint64_t* bufferDest = (uint64_t*)buffer;

        if(REPORT) printf("constructP2Mapped(%zx, %u)\n", mapped, length);

        bufferDest[0] = mapped;
        for(uint32_t levelLength = 2; levelLength < length; levelLength <<=1) {
            for(uint32_t i = levelLength; i--;) {
                bufferDest[i] = construct(buffer[i], false);
            }
        }
    }

    void constructP2NoRoot(uint64_t idx, uint32_t length, uint32_t* buffer) {
        uint64_t* bufferDest = (uint64_t*)buffer;

        for(uint32_t levelLength = 2; levelLength < length; levelLength <<=1) {
            for(uint32_t i = levelLength; i--;) {
                bufferDest[i] = construct(buffer[i], false);
            }
        }
    }

    // TODO: do the same as the new deconstruct()
    void construct(uint64_t idx, uint32_t length, uint32_t* buffer, bool isRoot) {
        if(REPORT) printf("construct(%zx, %u, %u)\n", idx, length, isRoot);
        if(length == 1) {
            if(REPORT) printf("Got %8zx(%u) -> %16zx\n", idx, length * 4, idx);
            *buffer = idx;
            return;
        }
        uint64_t mapped = construct(idx, length, isRoot);

        if(idx == 0) {
            memset(buffer, 0, length * sizeof(uint32_t));
            return;
        }

        uint32_t maxPowerTwoInLength = 1U << (31 - __builtin_clz(length));
        if(maxPowerTwoInLength == length) {
            constructP2(idx, length, buffer, isRoot);
        } else {
            constructP2(mapped & 0xFFFFFFFFULL, maxPowerTwoInLength, buffer, false);
            construct(mapped >> 32ULL, length-maxPowerTwoInLength, buffer + maxPowerTwoInLength, false);
        }
    }

    void constructMapped(DTreeNode node, uint32_t length, uint32_t* buffer, bool isRoot) {
        if(length <= 2) {
            if(node.getLength() == 0) {
                return;
            }
            if(node.getLength() == 1) {
                *buffer = node.getNode();
                return;
            } else {
                *(uint64_t*)buffer = node.getNode();
                return;
            }
        }

        if(node == 0) {
            memset(buffer, 0, length * sizeof(uint32_t));
            return;
        }

        uint32_t maxPowerTwoInLength = 1U << (31 - __builtin_clz(length));
        if(maxPowerTwoInLength == node.getLength()) {
            constructP2Mapped(node, length, buffer);
        } else {
            constructP2(node.getLeft(), maxPowerTwoInLength, buffer, false);
            construct(node.getRight(), length-maxPowerTwoInLength, buffer + maxPowerTwoInLength, false);
        }

    }

//    void construct(uint64_t idx, uint32_t length, uint32_t* buffer, bool isRoot) {
//        if(length == 1) {
//            if(REPORT) printf("Got %8zx(%u) -> %16zx\n", idx, length, idx);
//            *buffer = idx;
//            return;
//        }
//
//        if(idx == 0) {
//            memset(buffer, 0, length * sizeof(uint32_t));
//            return;
//        }
//
//        if(length == 2) {
//            *(uint64_t*)buffer = construct(idx, 0, isRoot);
//            return;
//        }
//
//        uint32_t level = lengthToLevel(length);
//        uint64_t mapped = construct(idx, level, isRoot);
//
//        uint32_t leftLength = 1 << level;
//        construct(mapped & 0xFFFFFFFFULL, leftLength, buffer, false);
//
//        // If there is a part remaining
//        if(leftLength < length) {
//            construct(mapped >> 32ULL, length-leftLength, buffer + leftLength, false);
//        }
//    }

    void constructPartial(uint64_t idx, uint64_t length, uint32_t offset, uint32_t wantedLength, uint32_t* buffer, bool isRoot) {
        if(REPORT) {
//            for(int i=__builtin_clz(1 << lengthToLevel(length))-26; i--;) printf("    ");
            printf("\033[35mconstructPartial\033[0m(%zx, %zu, %u, %u, %u)\n", idx, length, offset, wantedLength, isRoot);
        }
        if(length == 1) {
            if(REPORT) printf("Got %8zx(%zu) -> %16zx (%u)\n", idx, length, idx, isRoot);
            *buffer = idx;
            return;
        }

        if(idx == 0) {
            memset(buffer, 0, wantedLength * sizeof(uint32_t));
            return;
        }

        if(length == 2) {
            uint64_t mapped = construct(idx, 0, length, isRoot);
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

        uint32_t level = lengthToLevel(length);
        uint64_t mapped = construct(idx, level, length, isRoot);
        if(REPORT) printf("Got %8zx(%zu) -> %16zx (%u)\n", idx, length, mapped, isRoot);

        uint32_t leftLength = 1 << level;

        if(offset < leftLength) {
//            if(offset + wantedLength < leftLength) {
//            }
            uint32_t leftWantedLength = leftLength - offset;
            if(wantedLength > leftWantedLength) {
                constructPartial(mapped & 0xFFFFFFFFULL, leftLength, offset, leftWantedLength, buffer, false);
                constructPartial(mapped >> 32ULL, length-leftLength, 0, wantedLength - leftWantedLength, buffer + leftWantedLength, false);
            } else {
                constructPartial(mapped & 0xFFFFFFFFULL, leftLength, offset, wantedLength, buffer, false);
            }
        } else {
            constructPartial(mapped >> 32ULL, length-leftLength, offset - leftLength, wantedLength, buffer, false);
        }

    }

    void constructSparseUnits(uint64_t idx, uint64_t length, uint32_t internalOffset, uint32_t* buffer, uint32_t offsets, uint32_t* offset, bool isRoot) {

        if(length == 1) {
            *(uint32_t*)buffer = idx;
            return;
        }

        if(length == 2) {
            uint64_t mapped = construct(idx, 0, length, isRoot);
            if(offsets == 2) {
                *(uint64_t*)buffer = mapped;
            } else if (offset[0] == internalOffset) {
                *buffer = mapped;
            } else {
                *buffer = mapped >> 32ULL;
            }
            return;
        }

        uint32_t level = lengthToLevel(length);
        uint64_t mapped = construct(idx, level, length, isRoot);
        uint32_t leftLength = 1 << level;

        uint32_t leftOffsets = 0;
        uint32_t offsetLeft = internalOffset + leftLength;
        while(leftOffsets < offsets && offset[leftOffsets] < offsetLeft) ++leftOffsets;

        // If the left side is touched
        if(leftOffsets > 0) {
            constructSparseUnits(mapped & 0xFFFFFFFFULL, leftLength, internalOffset, buffer, leftOffsets, offset, false);
        }

        // If the right side is touched
        if(leftOffsets < offsets) {
            constructSparseUnits(mapped >> 32, length - leftLength, offsetLeft, buffer + leftOffsets, offsets - leftOffsets, offset + leftOffsets, false);
        }
    }

    void constructSparse(uint64_t idx, uint64_t length, uint32_t internalOffset, uint32_t* buffer, uint32_t offsets, Projection projection, bool isRoot) {

        SparseOffset* offset = projection.getOffsets();

        if(REPORT) {
            for(int i=__builtin_clz(1 << lengthToLevel(length)); i--;) printf("    ");
            printf("\033[36mconstructSparse\033[0m(%zx, %zu, %u, %p, [", idx, length, internalOffset >> 8, buffer);
            SparseOffset* o = offset;
            SparseOffset* e = offset + offsets;
            while(o<e) {
                if(o != offset) printf(", ");
                printf("%u@%u", o->getLength(), o->getOffset());
                o++;
            }
            printf("])\n");
        }

        if(dtree_unlikely(offsets == 1)) {
            constructPartial(idx, length, (offset->getData() - internalOffset) >> 8, offset->getLength(), buffer, isRoot);
            return;
        }

        // If we have more than two offsets and length == 2, we can simply construct the length 2 vector
        if(dtree_unlikely(length == 2)) {
            *(uint64_t*)buffer = construct(idx, 0, length, isRoot);
            return;
        }

        uint64_t mapped = construct(idx, 0, length, isRoot);
        uint32_t level = lengthToLevel(length);

        uint32_t leftLength = 1 << level;
        uint32_t leftLength2 = leftLength << 8;

        // This is the start of the right side of the tree in the entire vector, encoded as SparseOffset
        uint32_t offsetLeft = internalOffset + leftLength2;

        uint32_t leftOffsets = 0;
        uint32_t leftOffsetSizeTotal = 0;
        while(leftOffsets < offsets && offset[leftOffsets].getData() < offsetLeft) {
            leftOffsetSizeTotal += offset[leftOffsets].getLength();
            ++leftOffsets;
        }

        // If the left side is touched
        if(leftOffsets > 0) {


            // If there is overlap in change from left and right, we need to split them up...
            uint32_t last = leftOffsets - 1;
            int32_t overlap = (((int32_t)(offset[last].getData() - offsetLeft)) >> 8) + offset[last].getLength();
            if(overlap > 0) {

                // Cut off the part that affects the right side
                offset[last]._data -= overlap;

                // Construct the left part
                constructSparse(mapped & 0xFFFFFFFFULL, leftLength, internalOffset, buffer, leftOffsets, offset, false);

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
                constructSparse(mapped & 0xFFFFFFFFULL, leftLength, internalOffset, buffer, leftOffsets, offset, false);
            }

        }

        // If the right side is touched
        if(leftOffsets < offsets) {
            constructSparse(mapped >> 32ULL, length - leftLength, offsetLeft, buffer + leftOffsetSizeTotal, offsets - leftOffsets, offset + leftOffsets, false);
        }
    }

    uint64_t deltaSparseUnitsApply(uint64_t idx, uint64_t length, uint64_t* data, uint32_t offsets, uint32_t* offset, bool isRoot) {

        if(length == 1) {
            return *(uint32_t*)data;
        }

        if(length == 2) {
            return deconstruct(*(uint64_t*)data, 0, length, isRoot);
        }

        uint32_t level = lengthToLevel(length);
        uint64_t mapped = construct(idx, level, length, isRoot);

        uint32_t leftLength = 1 << level;

        uint32_t leftOffsets = 0;
        while(leftOffsets < offsets && offset[leftOffsets] < leftLength) ++leftOffsets;

        // If the left side is touched
        uint64_t leftIndex = leftOffsets == 0 ? (mapped & 0xFFFFFFFFULL)
                                              : deltaSparseUnitsApply(mapped, leftLength, data, leftOffsets, offset, false)
                                              ;

        // If the right side is touched
        uint64_t rightIndex = leftOffsets == offsets ? (mapped & 0xFFFFFFFF00000000ULL)
                                                     : (deltaSparseUnitsApply(mapped, length - leftLength, data + leftOffsets, offsets - leftOffsets, offset + leftOffsets) << 32, false)
                                                     ;

        return deconstruct(((uint64_t)leftIndex) | (((uint64_t)rightIndex)), length, isRoot);

    }

    uint64_t deltaSparseApply(uint64_t idx, uint64_t length, uint32_t internalOffset, uint32_t* delta, uint32_t offsets, Projection projection, bool isRoot) {

        SparseOffset* offset = projection.getOffsets();

        if(REPORT) {
            for(int i=__builtin_clz(1 << lengthToLevel(length)); i--;) printf("    ");
            printf("\033[36mdeltaSparseApply\033[0m(%zx, %zu, %u, %p, [", idx, length, internalOffset >> 8, delta);
            SparseOffset* o = offset;
            SparseOffset* e = offset + offsets;
            while(o<e) {
                if(o != offset) printf(", ");
                printf("%u@%u", o->getLength(), o->getOffset());
                o++;
            }
            printf("])\n");
        }

        if(dtree_unlikely(offsets == 1)) {
            return deltaApply(idx, length, (offset->getData() - internalOffset) >> 8, offset->getLength(), delta, isRoot);
        }

        // If we have more than two offsets and length == 2, we can simply apply both
        if(dtree_unlikely(length == 2)) {
            return deconstruct(((uint64_t)delta[0]) | ((uint64_t)delta[1] << 32), 0, 2, isRoot );
        }

        uint64_t mapped = construct(idx, 0, length, isRoot);
        uint32_t level = lengthToLevel(length);

        uint32_t leftLength = 1 << level;
        uint32_t leftLength2 = leftLength << 8;

        // This is the start of the right side of the tree in the entire vector, encoded as SparseOffset
        uint32_t offsetLeft = internalOffset + leftLength2;

        uint32_t leftOffsets = 0;
        uint32_t leftOffsetSizeTotal = 0;
        while(leftOffsets < offsets && offset[leftOffsets].getData() < offsetLeft) {
            leftOffsetSizeTotal += offset[leftOffsets].getLength();
            ++leftOffsets;
        }
        uint64_t newMapped = mapped;

        // If the left side is touched
        if(leftOffsets > 0) {


            // If there is overlap in change from left and right, we need to split them up...
            uint32_t last = leftOffsets - 1;
            int32_t overlap = (((int32_t)(offset[last].getData() - offsetLeft)) >> 8) + offset[last].getLength();
            if(overlap > 0) {

                // Cut off the part that affects the right side
                offset[last]._data -= overlap;

                // Construct the left part
                newMapped = (newMapped & 0xFFFFFFFF00000000ULL) | (deltaSparseApply(mapped & 0xFFFFFFFFULL, leftLength, internalOffset, delta, leftOffsets, offset, false) & 0xFFFFFFFFULL);

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
                newMapped = (newMapped & 0xFFFFFFFF00000000ULL) | (deltaSparseApply(mapped & 0xFFFFFFFFULL, leftLength, internalOffset, delta, leftOffsets, offset, false) & 0xFFFFFFFFULL);
            }

        }

        // If the right side is touched
        if(leftOffsets < offsets) {
            newMapped = (newMapped & 0xFFFFFFFFULL) | (deltaSparseApply(mapped >> 32ULL, length - leftLength, offsetLeft, delta + leftOffsetSizeTotal, offsets - leftOffsets, offset + leftOffsets, false) << 32);
        }
        return deconstruct(newMapped, level, length, isRoot);
    }

    uint64_t deltaSparseApply(uint64_t idx, uint64_t length, uint32_t internalOffset, uint32_t* buffer, uint32_t offsets, uint32_t* offset, uint32_t stride, bool isRoot) {

        if(REPORT) {
            for(int i=__builtin_clz(1 << lengthToLevel(length))-26; i--;) printf("    ");
            printf("deltaSparseApply(%zx, %zu, %u, %p, [", idx, length, internalOffset, buffer);
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
                return deltaApply(idx, length, 0, offset[1] + o, buffer - o, isRoot);
            } else {
                int32_t overlap = (int32_t)o + (int32_t)offset[1] - (int32_t)length;
                return deltaApply(idx, length, o, overlap > 0 ? offset[1] - overlap : offset[1], buffer, isRoot);
            }
        }

        if(length == 2) {
            return deconstruct(*(uint64_t*)(buffer + internalOffset - offset[0]), 0, length, isRoot);
        }

        uint32_t level = lengthToLevel(length);
        uint64_t mapped = construct(idx, level, length, isRoot);

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
                leftIndex = deltaSparseApply(mapped & 0xFFFFFFFFULL, leftLength, internalOffset, buffer, leftOffsets, offset, stride, false);


                // Decrement the number of left offsets such that this new one is picked
                // up later by the recursive call for the right side
                leftOffsets--;

                // Fix the size total too
                leftOffsetSizeTotal -= offset[last+1];
            } else {
                // Construct the left part
                leftIndex = deltaSparseApply(mapped & 0xFFFFFFFFULL, leftLength, internalOffset, buffer, leftOffsets, offset, stride, false);
            }

        } else {
            leftIndex = mapped & 0xFFFFFFFFULL;
        }

        // If the right side is touched
        if(leftOffsets < offsets) {
            rightIndex = deltaSparseApply(mapped >> 32ULL, length - leftLength, offsetLeft, buffer + leftOffsetSizeTotal, offsets - leftOffsets, offset + leftOffsets * stride, stride, false);
            rightIndex <<= 32ULL;
        } else {
            rightIndex = mapped & 0xFFFFFFFF00000000ULL;
        }
        return deconstruct(leftIndex | rightIndex, level, length, isRoot);
    }

    uint64_t deltaApply(uint64_t idx, uint64_t length, uint32_t offset, uint32_t deltaLength, const uint32_t* data, bool isRoot) {
        assert(length > 0);

        if(REPORT) {
//            for(int i=__builtin_clz(1 << lengthToLevel(length))-26; i--;) printf("    ");
            printf("deltaApply(%zx, %zuB, %uB, %uB, %p)\n", idx, length << 2, offset << 2, deltaLength << 2, data);
        }

        if(length == 1) {
            if(REPORT) printf("Got %8zx(%zu) -> %16zx\n", idx, length, idx);
            assert(offset == 0);
            assert(deltaLength >= 0);
            return *data;
        }

        DTreeNode node = construct(idx, 0, length, isRoot);

        if(length == 2) {

            // This is >= instead of == because in some edge cases deltaApply() may be called with a
            // deltaLength larger than the length, in which case only length bytes should be copied.
            if (deltaLength >= 2) {
                return deconstruct(*(uint64_t*)data, 0, length, isRoot);
            } else {
                if(offset == 0) {
                    node.setLeft(*data);
                } else {
                    node.setRight(*data);
                }
                return deconstruct(node.getData(), 0, length, isRoot);
            }
        }

        uint32_t level = lengthToLevel(length);

        uint32_t leftLength = 1 << level;
        uint64_t mappedNew = 0;

        if(offset < leftLength) {
            uint32_t leftDeltaLength = leftLength - offset;
            if(leftDeltaLength < deltaLength) {
                mappedNew = (deltaApply(node.getLeft(), leftLength, offset, leftDeltaLength, data, false) & 0xFFFFFFFFULL)
                          | ((deltaApply(node.getRight(), length-leftLength, 0, deltaLength - leftDeltaLength, data + leftDeltaLength, false)) << 32)
                          ;
            } else {
//                *((uint32_t*)(mapped)) = deltaApply(mapped & 0xFFFFFFFFULL, leftLength, offset, leftDeltaLength, data);
                mappedNew = deltaApply(node.getLeft(), leftLength, offset, deltaLength, data, false) & 0xFFFFFFFFULL;
                mappedNew |= node.getRightPart();
            }
        } else {
//            *((uint32_t*)(mapped)+1) = deltaApply(mapped >> 32ULL, length-leftLength, offset - leftLength, deltaLength, data);
            mappedNew = deltaApply(node.getRight(), length-leftLength, offset - leftLength, deltaLength, data, false) << 32;
            mappedNew |= node.getLeftPart();
        }

        if(node.getData() == mappedNew) {
            return idx;
        } else {
            return deconstruct(mappedNew, level, length, isRoot);
        }
    }

    uint64_t deltaApplyMapped(DTreeNode node, uint64_t length, uint32_t offset, uint32_t deltaLength, const uint32_t* data, bool isRoot) {

        assert(length > 0);

        if(REPORT) {
            printf("deltaApplyMapped(%zx, %zuB, %uB, %uB, %p)\n", node.getData(), length << 2, offset << 2, deltaLength << 2, data);
        }

        if(length <= 2) {

            // This is >= instead of == because in some edge cases deltaApply() may be called with a
            // deltaLength larger than the length, in which case only length bytes should be copied.
            if (deltaLength >= 2) {
                return *(uint64_t*)data;
            } else {
                if(offset == 0) {
                    node.setLeft(*data);
                } else {
                    node.setRight(*data);
                }
                if(length == 1) assert(node.getRight() == 0);
                return node.getData();
            }
        }

        uint32_t level = lengthToLevel(length);

        uint32_t leftLength = 1 << level;
        uint64_t mappedNew = 0;

        if(offset < leftLength) {
            uint32_t leftDeltaLength = leftLength - offset;
            if(leftDeltaLength < deltaLength) {
                mappedNew = (deltaApply(node.getLeft(), leftLength, offset, leftDeltaLength, data, false) & 0xFFFFFFFFULL)
                          | ((deltaApply(node.getRight(), length-leftLength, 0, deltaLength - leftDeltaLength, data + leftDeltaLength, false)) << 32)
                          ;
            } else {
                mappedNew = deltaApply(node.getLeft(), leftLength, offset, deltaLength, data, false) & 0xFFFFFFFFULL;
                mappedNew |= node.getRightPart();
            }
        } else {
            mappedNew = deltaApply(node.getRight(), length-leftLength, offset - leftLength, deltaLength, data, false) << 32;
            mappedNew |= node.getLeftPart();
        }

        return mappedNew;
    }

//    uint32_t deltaApplyInBytes(uint64_t idx, uint32_t lengthInBytes, uint32_t offsetInBytes, uint32_t deltaLengthInBytes, uint8_t* data) {
//        if(lengthInBytes == 4) {
//            if(REPORT) printf("Got %8zx(%u) -> %16zx\n", idx, lengthInBytes, idx);
//            memmove(((uint8_t*)&idx)+offsetInBytes, data, deltaLengthInBytes);
//            return idx;
//        }
//
//        if(lengthInBytes == 8) {
//            uint64_t mapped = construct(idx, 0);
//            uint32_t offsetInBits = (offsetInBytes*8);
//            memmove(((uint8_t*)&mapped)+offsetInBytes, data, deltaLengthInBytes);
//            return deconstruct(mapped);
//        }
//
//        uint32_t level = lengthToLevel(lengthInBytes);
//        uint64_t mapped = construct(idx, level);
//
//        uint32_t leftLength = 1 << level;
//        uint64_t mappedNew = 0;
//
//        if(offsetInBytes < leftLength) {
//            uint32_t leftDeltaLength = leftLength - offsetInBytes;
//            if(leftDeltaLength < deltaLengthInBytes) {
//                mappedNew = (uint64_t)(deltaApplyInBytes(mapped & 0xFFFFFFFFULL, leftLength, offsetInBytes, leftDeltaLength, data))
//                       | (((uint64_t)deltaApplyInBytes(mapped >> 32ULL, lengthInBytes-leftLength, 0, deltaLengthInBytes - leftDeltaLength, data + leftDeltaLength)) << 32)
//                       ;
//            } else {
//                mappedNew = (uint64_t)deltaApplyInBytes(mapped & 0xFFFFFFFFULL, leftLength, offsetInBytes, deltaLengthInBytes, data);
//                mappedNew |= mapped & 0xFFFFFFFF00000000ULL;
//            }
//        } else {
//            mappedNew = (uint64_t)deltaApplyInBytes(mapped >> 32ULL, lengthInBytes-leftLength, offsetInBytes - leftLength, deltaLengthInBytes, data) << 32;
//            mappedNew |= mapped & 0xFFFFFFFFULL;
//        }
//
//        if(mapped == mappedNew) {
//            return idx;
//        } else {
//            return deconstruct(mappedNew);
//        }
//    }

    uint64_t shrinkRecursive(uint64_t idx, uint32_t length, uint32_t shrinkTo, bool isRoot) {
        if(length == 1) return idx;
        if(length == 2) {
            if(shrinkTo == 2) {
                return idx;
            } else {
                return construct(idx, 0, length, isRoot);
            }
        }

        uint32_t level = lengthToLevel(length);
        uint32_t leftLength = 1 << level;

        uint64_t mapped = construct(idx, level, length, isRoot);
        if(leftLength < shrinkTo) {
            uint32_t rightIndex = mapped >> 32;
            mapped &= 0xFFFFFFFF00000000ULL;
            mapped |= ((uint64_t)shrinkRecursive(rightIndex, length - leftLength, shrinkTo - leftLength, false)) << 32;
            return deconstruct(mapped, 0, shrinkTo, isRoot);
        } else if(shrinkTo < leftLength) {

            abort();
            // this will go wrong because the root is at a different level
            // if we use isRoot = false. then the new one will not be inserted as root
            // if we pass on isRoot, then the next construct() might use isRoot=true,
            // even though that one should actually use false.
            // WE CAN FIX THIS WITH A FOR-LOOP AT THE START OF THIS FUNCTION

            // Recursively shrink the left index
            return shrinkRecursive(mapped & 0xFFFFFFFFULL, leftLength, shrinkTo, false);
        } else {
            return mapped & 0x7FFFFFFFFFFFFFFFULL;
        }

    }

    // https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAM1QDsCBlZAQwBtMQBGAFlICsupVs1qhkAUgBMAISnTSAZ0ztkBPHUqZa6AMKpWAVwC2tEAFYA7KS3oAMnlqYAcsYBGmYiAAcpAA6oFQnVaPUMTcyt/QLU6e0cXI3dPHyUVGNoGAmZiAlDjU0tFZUxVYMzsgjjnNw9vRSycvPDChQbKh2rE2q8ASkVUA2JkDgByKQBmB2RDLABqcXGdVvx6ADoEBexxAAYAQR3dhwJZ7OAzCAMjgDZuAH1j5lJZy/ob%2B9nXJ5eCN%2BPkL%2Bud2O6ABryBs0wPXmFmks2ImAIg1oJzkrjkEhk6DkmAWsPEFgAIgcjidiMArhdAe9Hs8qcdPrSwe9/oyfuCQazfhDQWz3lQofjYfDEcRkcxUei5FiZDiZFRcdCiXsSacLJSmQ8eVyGd8uSzdeyteDMEa%2BabjsABTC4QikSiZGiZBjpNLpLLpPKZMAFfilYd6KTgF51bzNZzwTq6bN9VGOQb3ibw2akxbzbMEFahbbRfbpI7pM7Xe7PdJvTINuM8YSDsSA6crrdJK4Q1yafH6TzxpJmZ3u8C04n27MqE9XKh9LNgKPx6x05mDrNFynZgAvebjAm5/OF3ELpdrhab4DrgBiq/mMghsxAq93eyX58P6dPj8vVGvt8re8XwrtK7v%2BzVnsIx9KwIAjGYIykKYIzbFBqDgTochyLMCgDEMmAXuMnBQQQ4FwT0fQANYgOMFirF4kg3FcnAWNsFhmGYZFmEI4HcFBRhcNs2zQfh8HgVBCggDxeGwSBpBwLASBoEYvh4OwZAUBAMlyQpKDCKIVzcaQVDyQQHhCRArh8aQrgONkACe4E4aQMlGFoBAAPK0KwVliaQWBGCIwDsCZ%2BDwqUABumBCe5mAAB4lAY%2BnWVBRzKCZrB4K4xCWXoWCxaQBDEHgnEjDhfQ0PQTBsBwPD8CAkhCN5KDITIQjJUJkB9KgvjpKFAC0jnjLMHUAOpsKwgnFKUGgQDYTSmJw1jaFUCRJIIURBHQk2LQEy20HNNSeNNqQlOk5SNPo%2BSCHto0ZG0W1dDt9QVKtu2XR0821JwfRoYMwxcKB4GQbx7kISM4VeFcHU3NGGnHlcqzbNDswQLghAkFh02zHosnyR4WGSFCSEyHIuF8YRpAIJgzBYJ4EDEZVXgUQAnGYVyUVckjjNskjAyxYEjOxpCcZw2kwXBpAA4JwlZYTEkwIgKCoOjCnkJQKkY54QZVbprD6cQhnGe5Zm0JZmV2Q5zmuX5mBeaIvnuf5%2B14MFoVCxFUUxflcX0Al7lJSlaUYKMNnZblsWFXQjAsL55UCOM1WiLVeP1V7TWU8LbXBJ13XDWkwSaNo90zXYT3bWt0TBLnS3pFdC27SNB1tLnZ01xUFcvbdR1hFNLftPEhevf0H1ld9EFQYL/GA8DoPcJOyDILMNOSHDCNEMQyNPGjqmYxM4w43V0gE2JRMk2TtRJyR4zjKskgWBYXiMVcVy09wtFaZzbEcVxPHD8LAmKGLokEQPkhDxMiLcWe8%2BjBS1lnbgQA%3D%3D
    // https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAM1QDsCBlZAQwBtMQBGAFlICsupVs1qhkAUgBMAISnTSAZ0ztkBPHUqZa6AMKpWAVwC2tEADYAzKS3oAMnlqYAcsYBGmYuYAMpAA6oFQnVaPUMTcyt/QLU6e0cXI3dPMx8lFRjaBgJmYgJQ41NLRWVMVWCsnII45zcPb0Vs3PzwooVGqocaxLqUgEpFVANiZA4AcikLB2RDLABqcQsdNvx6ADoEBexxLwBBCamZzHnF5awqdc3tvclJ2mmDOYWlgnxUC4st3avZn9m24gMqlmABEACrETDODBHcQAdlku1%2Bs18BlcrDwyBA3yRvzQtDamAAHr5iCDwZCnNCIL0QLMAProZjZCBeXrzeFw4FXbE42Z4gnE0lgiFQrAQAwOAhmbh0gh8uhtKRmWboGn0xnM1Xs2SwrlfRG8%2BX4ghEklkkWUsUS%2BgWSSyo2KyTK9hUAikWbWgi2%2B38ghK2bEPDABAENUMpnMKCe6Wy3ouuVwnSzKOSmOhwPBhOLJ6zW0AVVstl6bLhOr1ewNPxRaIxWMrOOjMrlwEwBGBEepDoT8IDraGtHVEYW0nZ5Z5ON9ptJrlQ%2BlmqF8HiZJAWXIswIgwop0K7/tQBAQHjZvu148NvYI/cH2WOa%2BB84PHlW4eyw7Pv053PrSMngtmM7nBclyIYgwDAVdN3JUUjl9PdH2IY8FW7BEdnPJEIUvYgBxfZhZjA1cH0PYhnw1Zg32/D9dS/VDDS3aC90XYhl2ICDAgAL0we1SJLDkKN5HDbxVIcLBQtDfgwq8ACoDzwBRyJo3lP31BScQA1hZiJWSCAUTsT1Ld9eQkrDr1w/D11mLx5PPJSKxUpFG3tFsqkwV0AAVKl0pDTz4nEjOw0j5idCzCQAMTC8KwoLWwrMNGyDI9VMm1mJyACUgxDdzck841vLsw0/JMwLlS8UKIvCrwKsqiqopixSqOU89PW9ZtW1sFyCGytpcrEn4CoE/0SrK8KapE%2BK4p8pq7Ragg0szTrkPi9C%2B2M/qPk2XNJBG0TYvq2zzwAN1QPB0D%2BVr2syjqHLleMLp47a6rHHzDuO06ZvSggLvFRL7QzDLKjusbdvi56TqUZzXS%2Bm0ptmeM2TpOkmQIQNXAME0oFoI7aHRRxi26nr%2BqdAjBqGkKqqqrbFt%2BfrYR0AjIalJs43a2qcXGvKfhB17ZpDenmoDd64YRggkbwFG0YgDGHGxzBcf0nykQJswicJMnKpJkKKflqmAsTOn6bTXpftDY5acWDbNfZ0dqKREl91KE10DrS2rpMlnP1G5TJVmIxmAcCAvZyYBkHdZAEBySTJNmQP9oB%2Bs6MtI4MbFQPkF6Fmk8wVYwbaiHODTj2VIzrPW25jrJHz%2B7kUDegqAgKRJCkABWQxxEbnRaHr90i6c9tmWLFmCssgvOVGfpWBAUZG9GUhTFGLxp9QCfaZkOQ/kGYYYRuThp4ICf5%2BLUgAGsQEbnxx9Gbhp9n%2BfSEX0Zp4UEAfF3ufR9IOBYCQNAjF8PB2DICgEBv6/3/iAYAABOTgpAqB/xNMQR%2BEBXB72nq4BwOQACeE9t6kG/kYLQBAADyWNMGv1IFgH2oh2DILIXgCEZR9qYEfqQokpRUZjGwZKZQ1D0SuCYsQdBegsBYJ3oGIwyD%2Bg0HoEwNgHAeD8EEMIUQKA5ByCEKLR%2BkB%2BgLgyEwgAtAQiwD8ShlA0BAGwzRTBQJsNUBISRBBRCCHQCx9iAiONoDY2ongoFpHtuUdozjvHGIyBUXIHjuheIaJUAJkTQmdFsXUPOAwhgjC4GPCeU8Z7ULvoSAAHGYXR0pkrIGQLMcBqxODJlwIQEggULBQNmHoH%2Bf8PC1Lzg0lRMgd7iP6MfU%2BQgJ6X0yaQu%2BD8n6kBfvvNJoxJBXyyRPLpr8D4MPgcEEA3AgA%3D


    // TODO: limit to 5 parameters to avoid passing arguments on the stack
    uint64_t extendRecursive(uint64_t idx, uint64_t length, uint32_t offset, int32_t deltaLength, const uint32_t* data, bool isRoot, bool toRoot) {
        uint64_t newLength = length + (uint64_t)offset + (uint64_t)deltaLength;

        if(REPORT) printf("Extending %zx(%zu) at %u with ...(%u)\n", idx, length << 2, offset << 2, deltaLength << 2);

        if(newLength == 1) {
            return *data;
        }
        if(newLength == 2) {

            // This can only happen for [original][delta]
            return deconstruct( ((uint64_t)idx) | (((uint64_t)(*(uint32_t*)data)) << 32), 0, newLength, toRoot);

        }

        uint32_t level = lengthToLevel(newLength);
        uint32_t leftLength = 1 << level;
        uint32_t zeroExtendedLength = length + offset;
        uint32_t rightOffset = zeroExtendedLength - leftLength;

        uint32_t leftIndex, rightIndex;

        // If the left part is exactly what we already have
        if(leftLength == length) {
            leftIndex = isRoot ? deconstruct(construct(idx, level, isRoot), level, length, false) : idx;
            if(REPORT) printf("Detected identical part: %x(%u)\n", leftIndex, leftLength);
            rightIndex = insertZeroPrepended(data, deltaLength, offset, false);

        // If the left part is part of what we already have
        } else if(leftLength < length) {
            uint64_t mapped = construct(idx, level, length, isRoot);
            leftIndex = mapped & 0xFFFFFFFFULL;
            if(REPORT) printf("Detected unchanged part: %x(%u)\n", leftIndex, leftLength);
            rightIndex = extendRecursive(mapped >> 32, length - leftLength, offset, deltaLength, data, false, false);

        // If the left part contains original data and zeroes
        } else if(zeroExtendedLength >= leftLength) {
            leftIndex = zeroExtend(idx, length, leftLength, isRoot, false);
            rightIndex = insertZeroPrepended(data, deltaLength, rightOffset, false);

        // If the left part contains original data, maybe zeroes and definitely delta
        } else {
            leftIndex = extendRecursive(idx, length, offset, -rightOffset, data, isRoot, false);
            rightIndex = deconstruct(data - (int32_t)rightOffset, newLength - (int32_t)leftLength, false);
        }

        return deconstruct(((uint64_t)leftIndex) | (((uint64_t)rightIndex) << 32), level, newLength, toRoot);
    }

    uint64_t zeroExtend(uint64_t idx, uint64_t length, uint32_t extendTo, bool isRoot, bool toRoot) {
        if(REPORT) printf("Zero-extending %zx(%zu) to %u\n", idx, length << 2, extendTo << 2);

        if(extendTo == 1) {
            return idx & 0xFFFFFFFFULL;
        }

        uint32_t level = lengthToLevel(extendTo);
        uint32_t leftLength = 1 << level;

        // If the left part is part of what we already have
        if(extendTo == length) {
            return idx;
        } else if(leftLength == length) {
//            return deconstruct(idx, level, extendTo, toRoot);
            if(REPORT)
                printf("Detected unchanged part and filling rest with 0s: %zx(%u)\n", idx, leftLength << 2);
            if(isRoot) {
                uint64_t mapped = construct(idx, level, length, isRoot);
                uint32_t left = deconstruct(mapped, level, leftLength, false);
                return deconstruct((uint64_t) left, level, extendTo, isRoot);
            } else {
                return deconstruct(idx, level, extendTo, isRoot);
            }
        } else if(leftLength < length) {
            uint64_t mapped = construct(idx, level, length, isRoot);
            uint64_t newIndex = mapped & 0xFFFFFFFFULL;
            if(REPORT) printf("Detected unchanged part: %zx(%u)\n", newIndex, leftLength << 2);
            newIndex |= ((uint64_t)zeroExtend(mapped >> 32, length - leftLength, extendTo - leftLength, false, false)) << 32;
            return deconstruct(newIndex, level, extendTo, toRoot);

        // If the left part contains 0's
        } else {
            return deconstruct(zeroExtend(idx, length, leftLength, isRoot, false) & 0xFFFFFFFFULL, level, extendTo, toRoot);
        }
    }

    uint64_t insertZeroPrepended(const uint32_t* data, uint32_t length, uint32_t offset, bool isRoot) {
        if(REPORT) printf("Zero-prepending vector of length %u with %u zeroes\n", length << 2, offset << 2);

        uint32_t newLength = length + offset;
        uint32_t level = lengthToLevel(newLength);
        uint32_t leftLength = 1 << level;

        // If the offset is 0, this is just a normal deconstruct
        if(offset == 0) {
            return deconstruct(data, length, isRoot);

        // If the leftLength is exactly the offset, the right side is
        // just a normal deconstruct and the left are only 0's
        } else if(leftLength == offset) {
            return deconstruct(((uint64_t)deconstruct(data, length, false)) << 32, level, newLength, isRoot);

        // If the right part contains zeroes, the left part is
        // only zeroes and the right part needs to be recursively handled
        } else if(leftLength < offset) {
            return deconstruct(((uint64_t)insertZeroPrepended(data, length, offset - leftLength, false)) << 32, level, newLength, isRoot);

        // If the left part contains part of the data, the right part
        // is normal data and the left part needs to be recursively handled
        } else {
            uint32_t diff = leftLength - offset;
            uint64_t v = (insertZeroPrepended(data, diff, offset, false) & 0xFFFFFFFFULL)
                       | (((uint64_t)deconstruct(data + diff, length - diff, false)) << 32)
                       ;
            return deconstruct(v, level, newLength, isRoot);
        }
    }

    uint64_t deltaApplyMayExtend(uint64_t idx, uint64_t length, uint32_t offset, const uint32_t* deltaData, uint32_t deltaLength, bool isRoot) {

        if(REPORT) printf("deltaApplyMayExtend %zx(%zu) at %u with ...(%u)\n", idx, length << 2, offset << 2, deltaLength << 2);

        // If we are trying to extend a 0-length vector, just prepend a number of 0's
        if(length == 0) {
            return deltaLength == 0 ? 0 : insertZeroPrepended(deltaData, deltaLength, offset, isRoot);
        }

        // If the delta extends the vector...
        uint32_t newLength = offset + deltaLength;
        if(newLength > length) {

            // If the offset is 0, this means the delta completely overwrites the original
            if(offset == 0) {

                // If the length of the delta is 1 there is not need to deconstruct
                if(deltaLength == 1) return *deltaData;
                else return deconstruct(deltaData, newLength, isRoot);
            }

            // If the original vector and the delta overlap
            if(offset < length) {

                // [ original ]
                //          [ delta ]
                //
                uint32_t level = lengthToLevel(newLength);
                uint32_t leftLength = 1 << level;
                uint64_t mapped = construct(idx, level, length, isRoot);

                // If the delta affects the left part
                if(offset < leftLength) {
                    if(REPORT) printf("Right part is purely delta, leftLength=%uB\n", leftLength << 2);
                    uint32_t deltaLengthLeft = leftLength - offset;
                    uint32_t leftIndex;
                    if(length > leftLength) {
                        leftIndex = deltaApplyMayExtend(mapped & 0xFFFFFFFFULL, leftLength, offset, deltaData, deltaLengthLeft, false);
                    } else {
                        leftIndex = deltaApplyMayExtend(idx, length, offset, deltaData, deltaLengthLeft, false);
                    }
                    uint32_t rightIndex = deconstruct(deltaData + deltaLengthLeft, deltaLength - deltaLengthLeft, false);
                    return deconstruct(((uint64_t)leftIndex) | (((uint64_t)rightIndex) << 32), level, newLength, isRoot);
                }

                // If the left part stays unchanged
                else {
                    if(REPORT) printf("Left part is unchanged, leftLength=%uB\n", leftLength << 2);
                    uint32_t rightIndex = deltaApplyMayExtend(mapped >> 32, length - leftLength, offset - leftLength, deltaData, deltaLength, false);
                    mapped &= 0x00000000FFFFFFFFULL;
                    mapped |= ((uint64_t)rightIndex) << 32;
                    return deconstruct(mapped, level, newLength, isRoot);
                }

            }

            // If the delta is completely beyond the vector,
            // we can simply call the extend-only extendRecursive()
            else {
                idx = extendRecursive(idx, length, offset - length, deltaLength, deltaData, isRoot, isRoot);
            }

            // Return the new Index
            return idx;
        }

        // If the delta is completely within the bounds of the original vector,
        // we can simply call the non-extending deltaApply()
        else {
            return deltaApply(idx, length, offset, deltaLength, deltaData, isRoot);
        }
    }

    void constructTree(uint64_t mapped, uint32_t length, uint64_t* buffer) {
        uint32_t* __attribute__((__may_alias__)) bufferSrc = (uint32_t*)buffer;
        uint64_t* __attribute__((__may_alias__)) bufferDest = buffer;

        if(REPORT) printf("constructTree(%zx, %u)\n", mapped, length);

        bufferDest[1] = mapped;

        // mapped = [5 6]
        // length = 8
        // a b c d e f g h
        //  1   2   3   4
        //    5       6
        //        7
        //
        // buffer: . . [5 6] [1 2] [3 4] [a b] [c d] [e f] [g h]

        for(uint32_t i = 2; i < length; ++i) {
            bufferDest[i] = construct(bufferSrc[i], false);
//            printf("%2u: %16zx <- %8x\n", i, bufferDest[i], bufferSrc[i]); fflush(stdout);
        }
    }

    uint64_t deconstructTree(uint32_t length, uint64_t* buffer) {
        uint64_t* __attribute__((__may_alias__)) bufferSrc = buffer;
        uint32_t* __attribute__((__may_alias__)) bufferDest = (uint32_t*)buffer;

        // mapped = [5 6]
        // length = 8
        // a b c d e f g h
        //  1   2   3   4
        //    5       6
        //        7
        //
        // buffer: . . [5 6] [1 2] [3 4] [a b] [c d] [e f] [g h]

        for(uint32_t i = length - 1; i > 1; --i) {
            bufferDest[i] = deconstruct(bufferSrc[i], false);
//            printf("%2u: %16zx -> %8x\n", i, bufferSrc[i], bufferDest[i]); fflush(stdout);
        }
        if(REPORT) printf("deconstructTree(%u, %p) -> %zx\n", length, buffer, bufferSrc[1]);
        return bufferSrc[1];
    }

    //    uint32_t extendRecursive(uint64_t idx, uint32_t length, uint32_t offset, int32_t deltaLength, uint32_t* data) {
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


    void multiConstruct(Index idx, bool isRoot, MultiProjection& projection, uint32_t level, uint32_t start, uint32_t end, uint32_t* buffer) {
        uint64_t idxWithoutLength = idx.getID();
        uint64_t length = idx.getLength();
        uint64_t mapped = construct(idxWithoutLength, 0, length, level == 0 && isRoot);

        if constexpr(REPORT) printf("%*c", level*4, ' ');
        if constexpr(REPORT) printf("multiConstruct(%zx, %u, %u, %u, %p)\n", idx, level, start, end, buffer);
        if constexpr(REPORT) printf("%*c", level*4, ' ');
        if constexpr(REPORT) std::cout << projection << std::endl;

        uint32_t* bufferPosition = buffer;

        auto projections = projection.getProjections();

        uint64_t jump[projections];
        uint64_t jumps = 0;

        if constexpr(REPORT) printf("%*c", level*4, ' ');
        if constexpr(REPORT) printf("perform constructs and gather jumps\n");

        // Go through all projections to find the required data in the current tree
        for(uint32_t pid = start; pid < end;) {
            if constexpr(REPORT) printf("%*c", level*4, ' ');
            if constexpr(REPORT) printf("pid %u\n", pid);
            auto& p = projection.getProjection(pid);

            LengthAndOffset lando = p.getLengthAndOffsets();
            uint32_t len = lando.getLength();

            // If the current projection projects to the current tree, get the data and write it to the buffer
            if(level == lando.getOffsets() - 1) {
                if constexpr(REPORT) printf("%*c", level*4, ' ');
                if constexpr(REPORT) printf("traverse2 for construct\n");
                traverse2(mapped, length, p.getOffset(level).getOffset(), len, &std::remove_pointer<decltype(this)>::type::traverseP2_construct, &std::remove_pointer<decltype(this)>::type::traverseP2_construct12, bufferPosition);
                ++pid;
            }

            // If the current projection projects to another tree, get the index of that tree in order to jump
            else {
                uint32_t currentOffset = p.getOffset(level).getOffset();
                assert((currentOffset & 0x1) == 0);
                ++pid;
                while(pid < end && projection.getProjection(pid).getOffset(level).getOffset() == currentOffset) {
                    if constexpr(REPORT) printf("%*c", level*4, ' ');
                    if constexpr(REPORT) printf("pid %u has same offset\n", pid);
                    pid++;
                    bufferPosition += projection.getProjection(pid).getLengthAndOffsets().getLength();
                }

                if constexpr(REPORT) printf("%*c", level*4, ' ');
                if constexpr(REPORT) printf("traverse2(%zx, %u, %u, 2, %p)\n", mapped, length, currentOffset, (uint32_t*)(&jump[jumps]));
                traverse2(mapped, length, currentOffset, 2, &std::remove_pointer<decltype(this)>::type::traverseP2_construct, &std::remove_pointer<decltype(this)>::type::traverseP2_construct12, (uint32_t*)(&jump[jumps]));
                if constexpr(REPORT) printf("%*c", level*4, ' ');
                if constexpr(REPORT) printf("traverse2 for jump, jump[%u] = %zx\n", jumps, jump[jumps]);
                jumps++;
            }
            bufferPosition += len;
        }

        if constexpr(REPORT) printf("%*c", level*4, ' ');
        if constexpr(REPORT) printf("found %u jumps\n", jumps);

        jumps = 0;

        if constexpr(REPORT) printf("%*c", level*4, ' ');
        if constexpr(REPORT) printf("perform jumps\n");
        // Go through all projections to find the jumps and traverse them
        bufferPosition = buffer;
        for(uint32_t pid = start; pid < end;) {
            auto& p = projection.getProjection(pid);

            LengthAndOffset lando = p.getLengthAndOffsets();
            uint32_t len = lando.getLength();

            //
            if(level < lando.getOffsets() - 1) {
                uint32_t currentOffset = p.getOffset(level).getOffset();
                assert((currentOffset & 0x1) == 0);

                uint32_t pidEnd = pid + 1;
                auto bufferPositionCurrent = bufferPosition;
                while(pidEnd < end && projection.getProjection(pidEnd).getOffset(level).getOffset() == currentOffset) {
                    pidEnd++;
                    bufferPosition += projection.getProjection(pidEnd).getLengthAndOffsets().getLength();
                }

                if constexpr(REPORT) printf("%*c", level*4, ' ');
                if constexpr(REPORT) printf("jumping...\n");
                multiConstruct(jump[jumps++], false, projection, level+1, pid, pidEnd, bufferPositionCurrent);
                pid = pidEnd;
            } else {
                ++pid;
            }
            bufferPosition += len;
        }
        if constexpr(REPORT) printf("%*c", level*4, ' ');
        if constexpr(REPORT) printf("DONE\n");
    }

    uint32_t multiConstructJumpsOnly(Index idx, MultiProjection& projection, uint32_t level, uint32_t start, uint32_t end, uint64_t* jump) {
        uint64_t idxWithoutLength = idx.getID();
        uint64_t length = idx.getLength();
        uint64_t mapped = construct(idxWithoutLength, 0, length, level == 0);

        auto projections = projection.getProjections();

        uint64_t jumps = 0;

        // Go through all projections to find the required data in the current tree
        for(uint32_t pid = 0; pid < projections;) {
            auto& p = projection.getProjection(pid);

            LengthAndOffset lando = p.getLengthAndOffsets();

            // If the current projection projects to the current tree, skip
            if(level == lando.getOffsets() - 1) {
                ++pid;
            }

            // If the current projection projects to another tree, get the index of that tree in order to jump
            else {
                uint32_t currentOffset = p.getOffset(level).getOffset();
                assert((currentOffset & 0x1) == 0);
                while(projection.getProjection(pid).getOffset(level) == currentOffset) pid++;

                traverse2(mapped, length, currentOffset, 2, &std::remove_pointer<decltype(this)>::type::traverseP2_construct, &std::remove_pointer<decltype(this)>::type::traverseP2_construct12, jump+jumps);
                jumps++;
            }

        }
        return jumps;

    }

    IndexInserted multiDelta(Index idx, bool isRoot, MultiProjection& projection, uint32_t level, uint32_t start, uint32_t end, uint32_t* buffer) {
//        traverse2(idx, projection, level, &std::remove_pointer<decltype(this)>::type::traverseP2_delta, &std::remove_pointer<decltype(this)>::type::traverseP2_delta12);
        if(REPORT) printf("\033[35mmultiDelta\033[0m %p\n", buffer);
        return traverse3(idx, isRoot, projection, start, end, level, &std::remove_pointer<decltype(this)>::type::traverseP2_delta, &std::remove_pointer<decltype(this)>::type::traverseP2_delta12, &buffer);
    }

//    uint32_t* multiConstructBIGBUFFER(Index idx, MultiProjection& projection, uint32_t level, uint32_t start, uint32_t end, uint32_t* buffer) {
//        uint64_t idxWithoutLength = idx.getID();
//        uint64_t length = idx.getLength();
//
//        uint64_t tree[1 << lengthToLevel()];
//        tree[0] = idx;
//        tree[1] = construct(idx, 0, length, level == 0);
//        constructTree(idx, length, tree, projection, 0, start, end);
//
//
//        //
//        //   ____data____
//        //   \   \  /   /
//        //    \   \/   /
//        //     \  /   /
//        //      \ \  /
//        //       \ \/
//        //  ______\/___
//        //  \     /   /
//        //   \   /   /
//        //    \  \  /
//        //     \ / /
//        //     [idx]
//        //
//        //
//        //   __cdef__gh__
//        //   \ \  /  \/ /
//        //    \ \/   / /
//        //     \ \  / /
//        //      \ \/ /
//        //       \ \/
//        //  __ab__\/___
//        //  \     /   /
//        //   \   /   /
//        //    \  \  /
//        //     \ / /
//        //     [idx]
//        // MultiProjection:
//        // [2][2]
//        // [4][6][2]
//        // [2][6][8]
//
//        auto projections = projection.getProjections();
//
//        for(uint32_t pid = 0; pid < projections; ++pid) {
//            auto& p = projection.getProjection(pid);
//
//            // If we have reached the end of this projection
//            if(level < p.getOffsets() - 1) {
////                memcpy(buffer, ((uint32_t*)tree)+)
//            } else {
//                uint32_t currentOffset = p.getOffset(level);
//                assert((currentOffset & 0x1) == 0);
//
//                uint32_t pidEnd = pid + 1;
//                while(projection.getProjection(pid).getOffset(level) == currentOffset) pidEnd++;
//
//                currentOffset >>= 1;
//                buffer = multiConstruct(&tree[currentOffset], projection, level + 1, pid, pidEnd, buffer);
//            }
//        }
//        return buffer;
//
//    }

//
//    IndexInserted constructTree(uint64_t idx, uint32_t length, uint64_t* buffer, typename MultiProjection::SingleProjection* projection, uint32_t level, uint32_t projections) {
//        uint32_t* srcBuffer = (uint32_t*)buffer;
//        if(REPORT) printf("constructTree %zu %u %u\n", idx, length, level);
//
////        uint32_t lengths[PROJECTIONS];
////        for(size_t i = 0; i < PROJECTIONS; ++i) {
////            auto lengthAndOffsets = projection.getProjection(i).getLengthAndOffsets();
////            auto offsetsMinusOne = lengthAndOffsets.getOffsets() - 1;
////
////            lengths[i] = level < offsetsMinusOne ? 2  // Only need to expand the node containing the index to the next tree
////                       : level > offsetsMinusOne ? 0  // The projection projected to an earlier tree
////                       : lengthAndOffsets.getLength() // The projection projects to a part of the current tree
////                       ;
////        }
//
//        uint32_t currentProjection = 0;
//
//        // Skip already finished projections
////        while(lengths[currentProjection] == 0) currentProjection++;
//
//        auto projectionEnd = projection + projections;
//
//        //
//        while(projection < projectionEnd) {
//            uint32_t currentOffset = projection->getOffset(level);
//            uint32_t currentLength = projection->getLength(level);
//            uint32_t bufferPosition = 2;
//            uint32_t lengthLevel = lengthToLevel(length);
//
//
//            // Vector: 89ab67
//            // 8 9 a b
//            //  4   5   6 7
//            //    2      3
//            //       idx
//            //
//            // [idx] [2 3] [4 5] [6 7] [8 9] [a b]
//
//            // Vector: ghijklmn67
//            //
//            // g h i j k l m n
//            //  8   9   a   b
//            //    4       5    6 7
//            //        2         3
//            //             idx
//            //
//            // [idx] [2 3] [4 5] [6 7] [8 9] [a b] . . . . [g h] [i j] [k l] [m n]
//
//            struct {
//                uint32_t pos;
//                uint32_t length;
//                uint32_t projectionLength;
//            } todo[lengthLevel];
//            uint32_t nTodo = 0;
//
//            traverse(buffer, bufferPosition, length, currentOffset, currentLength, nTodo, todo);
//
//            for(; nTodo--;) {
//                traverse(buffer, todo[nTodo].pos, todo[nTodo].length, 0, todo[nTodo].projectionLength, nTodo, todo);
//            }
//
//            projection++;
//        }
//    }
//
//    template<typename TODO>
//    void traverse(uint64_t* buffer, uint32_t bufferPosition, uint32_t length, uint32_t currentLocalOffset, uint32_t currentLength, uint32_t& nTodo, TODO&& todo) {
//        uint32_t lengthLevel = lengthToLevel(length);
//        uint32_t leftLength = 1 << lengthLevel;
//        uint32_t rightLength = length - leftLength;
//        uint32_t* srcBuffer = (uint32_t*)buffer;
//
//        if(REPORT) printf("constructTree-traverse %u %u %u %u %u\n", bufferPosition, length, currentLocalOffset, currentLength, nTodo);
//
//        // <leftLength>
//        // [..........][....]
//        //
//        //
//        //
//        //     [ left | right ]
//        //
//
//        // Go deep into the tree, building a single branch
//        while(currentLocalOffset > 1) {
//
//            // If the projection is only in the right part of the tree
//            if(currentLocalOffset >= leftLength) {
//                bufferPosition++;
//                currentLocalOffset -= leftLength;
//                leftLength = 1 << lengthToLevel(rightLength);
//                rightLength -= leftLength;
//            } else {
//                // If part of the projection is in the right part of the tree
//                int32_t overflow = currentLocalOffset + currentLength - leftLength;
//                if(overflow > 0) {
//                    todo[nTodo++] = {bufferPosition+1, rightLength, overflow};
//                    if(REPORT) printf("  added TODO %u %u %u\n", bufferPosition+1, rightLength, overflow);
//                }
//                leftLength >>= 1;
//                rightLength = leftLength;
//            }
//            buffer[bufferPosition] = construct(srcBuffer[bufferPosition], 0);
//            bufferPosition <<= 1;
//            if(REPORT) printf("constructTree-traverse find-left %u %u %u %u %u\n", bufferPosition, length, currentLocalOffset, currentLength, nTodo);
//        }
//        while(currentLocalOffset + currentLength <= leftLength) {
//            leftLength >>= 1;
//            buffer[bufferPosition] = construct(srcBuffer[bufferPosition], 0);
//            bufferPosition <<= 1;
//            if(REPORT) printf("constructTree-traverse find-right %u %u %u %u %u\n", bufferPosition, length, currentLocalOffset, currentLength, nTodo);
//        }
//
//        if(REPORT) printf("constructTree-traverse found %u %u %u %u %u\n", bufferPosition, length, currentLocalOffset, currentLength, nTodo);
//
//        // At this point we found a perfectly balanced section that we wish to expand
//        for(uint32_t curSize = 2; curSize <= leftLength; curSize<<=1) {
//            uint32_t b = bufferPosition;
//            uint32_t bEnd = bufferPosition + curSize;
//            while(b < bEnd) {
//                if(REPORT) printf("  -> %u\n", b);
//                buffer[b] = construct(srcBuffer[b], 0);
//                b++;
//            }
//            bufferPosition <<= 1;
//        }
//    }
    void traverseP2_construct12(uint64_t mapped, uint32_t length, uint32_t currentLocalOffset, uint32_t currentLength, uint32_t*& dest) {
        memcpy(dest, (uint32_t*)&mapped + currentLocalOffset, currentLength * sizeof(uint32_t));
        dest += currentLength;
//        if(currentLength == 2) {
//            *dest++ = mapped & 0xFFFFFFFFULL;
//            *dest++ = mapped >> 32;
//        } else {
//            *dest++ = currentLocalOffset ? mapped >> 32 : mapped & 0xFFFFFFFFULL;
//        }
    }
    void traverseP2_construct(uint64_t mapped, uint32_t length, uint32_t currentLocalOffset, uint32_t currentLength, uint32_t*& dest) {
        if(REPORT) printf("traverseP2_construct %zx %u %u %u %p\n", mapped, length, currentLocalOffset, currentLength, dest);

        uint32_t lengthLevel = lengthToLevel(length);
        uint32_t leftLength = length / 2;

        struct {
            uint32_t idx;
            uint32_t length;
            uint32_t pLength;
        } todo[lengthLevel];
        uint32_t nTodos = 0;

        for(;;) {
            while(currentLength <= leftLength) {

                // Only right part is touched
                if(leftLength <= currentLocalOffset) {
                    currentLocalOffset -= leftLength;
                    if(REPORT) printf("traverseP2_construct: took right %zx %u %u@%u\n", mapped, leftLength, currentLength, currentLocalOffset);
                    leftLength /= 2;
                    mapped = construct(mapped >> 32, 0);
                } else {
                    int32_t rightPartLength = currentLocalOffset + currentLength - leftLength;
                    if(rightPartLength > 0) {
                        if(REPORT) printf("traverseP2_construct: adding todo %zx %u %u@0\n", mapped >> 32, leftLength, rightPartLength);
                        todo[nTodos++] = {(uint32_t)(mapped >> 32), leftLength, (uint32_t)rightPartLength};
                        currentLength -= rightPartLength;
                    }
                    if(REPORT) printf("traverseP2_construct: took left %zx %u %u@%u\n", mapped, leftLength, currentLength, currentLocalOffset);
                    leftLength /= 2;
                    mapped = construct(mapped & 0xFFFFFFFFULL, 0);
                }
                if(leftLength == 1) {
                    memcpy(dest, (uint32_t*)&mapped + currentLocalOffset, currentLength * sizeof(uint32_t));
                    if(REPORT) printf("traverseP2_construct: COPIED TO %p %zu\n", dest, currentLength * sizeof(uint32_t));
                    goto todo;
                }
            }

            {
                uint32_t fullLength = 2 * leftLength;
                uint64_t buffer[fullLength];
                constructTree(mapped, fullLength, buffer);
                memcpy(dest, (uint32_t*)buffer + fullLength + currentLocalOffset, currentLength * sizeof(uint32_t));
                if(REPORT) printf("traverseP2_construct: COPIED TO %p %zu\n", dest, currentLength * sizeof(uint32_t));
            }

            todo:
            dest += currentLength;
            if(nTodos) {
                nTodos--;
                currentLocalOffset = 0;
                currentLength = todo[nTodos].pLength;
                leftLength = todo[nTodos].length / 2;
                mapped = construct(todo[nTodos].idx, 0);
                if(leftLength == 1) {
                    memcpy(dest, (uint32_t*)&mapped + currentLocalOffset, currentLength * sizeof(uint32_t));
                    if(REPORT) printf("traverseP2_construct: COPIED TO %p %zu\n", dest, currentLength * sizeof(uint32_t));
                    goto todo;
                }
                if(REPORT) printf("traverseP2_construct: %u todo, %u %u@0\n", nTodos+1, leftLength, currentLength);
            } else {
                if(REPORT) printf("traverseP2_construct: all done, dest = %p\n", dest);
                break;
            }
        }
    }

    __attribute__((always_inline))
    uint64_t traverseP2_delta12(uint64_t mapped, uint32_t length, uint32_t offset, uint32_t lengthToGo, uint32_t** src) {
        if(REPORT) {
            printf("\033[35mtraverseP2_delta12\033[0m %zx %u, off %u, togo %u, src %p\n", mapped, length, offset, lengthToGo, *src);
        }
        if(length == 2) {
            uint32_t idx;
//            if(pStart + 1 < pEnd) {
//                mapped = (((uint64_t)src[0]) | (((uint64_t)(src[1])) << 32));
//                idx = deconstruct(mapped, 0, 2, level == 0);
//                src += 2;
//            } else {
//                if(level < lando.getOffsets() - 1) {
//                    idx = traverse2(mapped, projection, level+1, &std::remove_pointer<decltype(this)>::type::traverseP2_delta, &std::remove_pointer<decltype(this)>::type::traverseP2_delta12);
//                    src += 2;
//                } else {
                    memcpy(((uint32_t*)&mapped) + offset, *src, lengthToGo * sizeof(uint32_t));
                    //idx = deconstruct(mapped, 0, 2, level == 0);
                    *src += lengthToGo;
                    return mapped;
//                }
//            }
            return idx;
        } else {
            return *(*src)++;
        }
    }

    uint64_t traverseP2_delta(uint64_t mapped, uint32_t length, uint32_t currentLocalOffset, uint32_t currentLength, uint32_t** src) {
        if(REPORT) printf("\033[35mtraverseP2_delta\033[0m %zx %u %u %u %p\n", mapped, length, currentLocalOffset, currentLength, *src);

        if(length == 2) {
            memcpy((uint32_t*)&mapped + currentLocalOffset, *src, currentLength * sizeof(uint32_t));
            if(REPORT) printf("traverseP2_delta: COPIED FROM %p %zu\n", *src, currentLength * sizeof(uint32_t));
            *src += currentLength;
            return mapped;
        }

        uint32_t lengthLevel = lengthToLevel(length);
        uint32_t leftLength = length / 2;

        uint64_t chain[lengthLevel+1];
        bool right[lengthLevel+1];

        uint32_t level = 0;

        chain[0] = mapped;

        uint32_t lengthToGo = currentLength;

        for(;;) {
            while(currentLength <= leftLength) {

                if(leftLength == 1) {
                    currentLength = std::min(2-currentLocalOffset, lengthToGo);
                    memcpy((uint32_t*)&chain[level] + currentLocalOffset, *src, currentLength * sizeof(uint32_t));
                    if(REPORT) printf("traverseP2_delta: COPIED FROM %p %zu\n", *src, currentLength * sizeof(uint32_t));
                    lengthToGo -= currentLength;
                    *src += currentLength;
                    goto todo;
                }
                uint64_t& prevChain = chain[level];
                level++;

                // Only right part is touched
                if(leftLength <= currentLocalOffset) {
                    currentLocalOffset -= leftLength;
                    if(REPORT) printf("traverseP2_delta: took right %zx %u %u@%u\n", prevChain, leftLength, currentLength, currentLocalOffset);
                    leftLength /= 2;
                    chain[level] = construct(prevChain >> 32, 0);
                    right[level] = 1;
                } else {
                    if(REPORT) printf("traverseP2_delta: took left %zx %u %u@%u\n", mapped, leftLength, currentLength, currentLocalOffset);
                    leftLength /= 2;
                    chain[level] = construct(prevChain & 0xFFFFFFFFULL, 0);
                    right[level] = 0;
                }
            }

            {
                uint32_t fullLength = 2 * leftLength;
                uint64_t buffer[fullLength];
                constructTree(chain[level], fullLength, buffer);
                currentLength = std::min(fullLength-currentLocalOffset, lengthToGo);
                memcpy((uint32_t*)buffer + fullLength + currentLocalOffset, *src, currentLength * sizeof(uint32_t));
                chain[level] = deconstructTree(fullLength, buffer);
                if(REPORT) printf("traverseP2_construct: COPIED FROM %p %zu TO %u\n", *src, currentLength * sizeof(uint32_t), currentLocalOffset);
                lengthToGo -= currentLength;
                *src += currentLength;
            }

            todo:

        // a b c d e f g h
        //  1   2   3   4
        //    5       6
        //       (7)
        //
        // chain[0] = [5 6]   right[0] = ?
        // chain[1] = [1 2]   right[1] = 0
        // chain[2] = [c D]   right[2] = 1
        // level = 2

            if(REPORT) printf("lengthToGo: %u\n", lengthToGo);

            if(!lengthToGo) {
                break;
            }

            while(right[level]) {
                uint32_t m = deconstruct(chain[level], 0);
                level--;
                memcpy(((uint32_t*)&chain[level])+1, &m, sizeof(uint32_t));
                if(REPORT) printf("%2u: %zx -- %x --> %zx right\n", level, chain[level + 1], m, chain[level]);
                leftLength *=2;
            }
            uint32_t m = deconstruct(chain[level], 0);
            memcpy(((uint32_t*)&chain[level-1]), &m, sizeof(uint32_t));
            if(REPORT) printf("%2u: %zx -- %u --> %zx left\n", level, chain[level], m, chain[level - 1]);
            chain[level] = construct(chain[level-1] >> 32, 0);
            right[level] = 1;
            if(REPORT) printf("setup %zx\n", chain[level]);
            currentLength = lengthToGo;
            currentLocalOffset = 0;
        }

        if(REPORT) printf("done, rebuilding chain\n");
        while(level) {
            uint32_t m = deconstruct(chain[level], 0);
            if(REPORT) printf("level %u: %zx -> %x (%u)\n", level, chain[level], m, right[level]);
            bool r = right[level];
            level--;
            memcpy(((uint32_t*)&chain[level])+r, &m, sizeof(uint32_t));
//            leftLength *=2;
        }
        if(REPORT) printf("returning %zx\n", chain[0]);
        return chain[0];
        // __xx____
        // \      /
        //  \    /
        //   \  /
        //    \/





//        if(REPORT) printf("traverseP2_delta %zx %u %u %u %u %p\n", mapped, length, level, pStart, pEnd, src);
//        uint32_t lengthLevel = lengthToLevel(length);
//        uint32_t leftLength = length / 2;
//
//        struct {
//            uint32_t idx;
//            uint32_t subLevel;
//            uint32_t pLength;
//        } todo[lengthLevel];
//        uint32_t nTodos = 0;
//
//        uint64_t chain[lengthLevel];
//        uint64_t left[lengthLevel];
//        uint32_t currentLevel = 1;
//        chain[0] = mapped;
//
//        //       _
//        // a b c d e f g h
//        //  1   2   3   4
//        //    5       6
//        //       (7)
//        //
//        // mappedOriginal = [5 6]
//        //
//        // chain[0] = [5 6]   left[0] = ?
//        // chain[1] = [1 2]   left[1] = 1
//        // chain[2] = [c D]   left[2] = 0
//        //
//
//        for(;;) {
//            while(currentLength <= leftLength) {
//
//                // Only right part is touched
//                if(leftLength <= currentLocalOffset) {
//                    currentLocalOffset -= leftLength;
//                    if(REPORT) printf("traverseP2_delta: took right %zx %u %u@%u\n", mapped, leftLength, currentLength, currentLocalOffset);
//                    leftLength /= 2;
//                    chain[currentLevel] = construct(chain[currentLevel-1] >> 32, 0);
//                    left[currentLevel] = 0;
//                } else {
//                    int32_t rightPartLength = currentLocalOffset + currentLength - leftLength;
//                    if(rightPartLength > 0) {
//                        if(REPORT) printf("traverseP2_delta: adding todo %zx %u %u@0\n", mapped >> 32, leftLength, rightPartLength);
//                        todo[nTodos++] = {mapped >> 32, currentLevel, rightPartLength};
//                        currentLength -= rightPartLength;
//                    }
//                    if(REPORT) printf("traverseP2_delta: took left %zx %u %u@%u\n", mapped, leftLength, currentLength, currentLocalOffset);
//                    leftLength /= 2;
//                    chain[currentLevel] = construct(chain[currentLevel-1] & 0xFFFFFFFFULL, 0);
//                    left[currentLevel] = 1;
//                }
//                if(leftLength == 1) {
//                    memcpy((uint32_t*)&chain[currentLevel] + currentLocalOffset, src, currentLength * sizeof(uint32_t));
//                    if(REPORT) printf("traverseP2_delta: COPIED FROM %p %u\n", src, currentLength * sizeof(uint32_t));
//                    goto backtrackChain;
//                }
//                currentLevel++;
//            }
//
//            {
//                uint32_t fullLength = 2 * leftLength;
//                uint64_t buffer[fullLength];
//                constructTree(mapped, fullLength, (uint32_t*)buffer);
//                memcpy((uint32_t*)buffer + fullLength + currentLocalOffset, src, currentLength * sizeof(uint32_t));
//                chain[currentLevel] = deconstructTree(fullLength, (uint32_t*)buffer);
//                if(REPORT) printf("traverseP2_delta: COPIED FROM %p %u\n", src, currentLength * sizeof(uint32_t));
//            }
//
//            backtrackChain:
//
//            src += currentLength;
//            if(nTodos) {
//                uint32_t todoLevel = todo[nTodos].subLevel;
//                // chain[0..currentLevel] contain the chain of old nodes
//                // chain[currentLevel] contains the deepest, new node
//                for(;todoLevel < currentLevel;currentLevel--) {
//                    uint32_t m = deconstruct(chain[currentLevel], 0);
//                    memcpy((uint32_t*)chain[currentLevel-1] + left[currentLevel], &m, sizeof(uint32_t));
//                }
//                nTodos--;
//                currentLocalOffset = 0;
//                currentLength = todo[nTodos].pLength;
//                leftLength = (length >> todoLevel) / 2;
//                currentLevel++;
//                chain[currentLevel] = construct(todo[nTodos].idx, 0);
//                left[currentLevel] = 0;
//                if(leftLength == 1) {
//                    memcpy((uint32_t*)&chain[currentLevel] + currentLocalOffset, src, currentLength * sizeof(uint32_t));
//                    if(REPORT) printf("traverseP2_delta: COPIED FROM %p %u\n", src, currentLength * sizeof(uint32_t));
//                    goto backtrackChain;
//                }
//                if(REPORT) printf("traverseP2_delta: %u todo, %zx %u %u@0\n", nTodos+1, leftLength, currentLength);
//            } else {
//                if(REPORT) printf("traverseP2_delta: all done\n");
//
//                // chain[0..currentLevel] contain the chain of old nodes
//                // chain[currentLevel] contains the deepest, new node
//                for(;currentLevel;currentLevel--) {
//                    uint32_t m = deconstruct(chain[currentLevel], 0);
//                    memcpy((uint32_t*)chain[currentLevel-1] + left[currentLevel], &m, sizeof(uint32_t));
//                }
//                return chain[0];
//            }
//        }
    }

    uint64_t deltaMapped2(uint64_t mapped, uint32_t length, uint32_t currentLocalOffset, uint32_t currentLength, uint32_t* src) {
        if(REPORT) printf("\033[35mdeltaMapped2\033[0m %zx %u %u %u %p\n", mapped, length, currentLocalOffset, currentLength, src);

        if(length == 2) {
            memcpy((uint32_t*)&mapped + currentLocalOffset, src, currentLength * sizeof(uint32_t));
            if(REPORT) printf("deltaMapped2: COPIED FROM %p %zu\n", src, currentLength * sizeof(uint32_t));
            src += currentLength;
            return mapped;
        }

        uint32_t lengthLevel = lengthToLevel(length);
        uint32_t leftLength = 1 << lengthLevel;
        uint32_t rightLength = length - leftLength;

        uint64_t chain[lengthLevel+2];
        bool right[lengthLevel+2];
        uint32_t lengths[leftLength+2];

        uint32_t level = 0;

        chain[0] = mapped;

        uint32_t lengthToGo = currentLength;

        for(;;) {
            while(currentLength <= leftLength) {

                if(leftLength == 1) {
                    currentLength = std::min(2-currentLocalOffset, lengthToGo);
                    memcpy((uint32_t*)&chain[level] + currentLocalOffset, src, currentLength * sizeof(uint32_t));
                    if(REPORT) printf("deltaMapped2: COPIED FROM %p %zu\n", src, currentLength * sizeof(uint32_t));
                    lengthToGo -= currentLength;
                    src += currentLength;
                    goto todo;
                }
                uint64_t& prevChain = chain[level];
                level++;

                // Only right part is touched
                if(leftLength <= currentLocalOffset) {
                    currentLocalOffset -= leftLength;
                    if(REPORT) printf("deltaMapped2: took right %zx %u %u@%u\n", prevChain, leftLength, currentLength, currentLocalOffset);
                    lengths[level] = rightLength;
                    leftLength = 1 << lengthToLevel(rightLength);
                    rightLength -= leftLength;
                    chain[level] = construct(prevChain >> 32, 0);
                    right[level] = 1;
                } else {
                    if(REPORT) printf("deltaMapped2: took left %zx %u %u@%u\n", mapped, leftLength, currentLength, currentLocalOffset);
                    lengths[level] = leftLength;
                    leftLength /= 2;
                    rightLength = leftLength;
                    chain[level] = construct(prevChain & 0xFFFFFFFFULL, 0);
                    right[level] = 0;
                }
            }

            {
                uint32_t fullLength = leftLength + rightLength;
                uint64_t buffer[fullLength];
                constructTree(chain[level], fullLength, buffer);
                currentLength = std::min(fullLength-currentLocalOffset, lengthToGo);
                memcpy((uint32_t*)buffer + fullLength + currentLocalOffset, src, currentLength * sizeof(uint32_t));
                chain[level] = deconstructTree(fullLength, buffer);
                if(REPORT) printf("deltaMapped2: COPIED FROM %p %zu\n", src, currentLength * sizeof(uint32_t));
                lengthToGo -= currentLength;
                src += currentLength;
            }

            todo:

            // a b c d e f g h
            //  1   2   3   4
            //    5       6
            //       (7)
            //
            // chain[0] = [5 6]   right[0] = ?
            // chain[1] = [1 2]   right[1] = 0
            // chain[2] = [c D]   right[2] = 1
            // level = 2

            if(REPORT) printf("lengthToGo: %u\n", lengthToGo);

            if(!lengthToGo) {
                break;
            }

            while(right[level]) {
                uint32_t m = deconstruct(chain[level], 0);
                level--;
                memcpy(((uint32_t*)&chain[level])+1, &m, sizeof(uint32_t));
                if(REPORT) printf("%2u: %zx -- %x --> %zx right\n", level, chain[level + 1], m, chain[level]);
            }
            leftLength = 1 << lengthToLevel(lengths[level]);
            rightLength = lengths[level] - leftLength;
            uint32_t m = deconstruct(chain[level], 0);
            memcpy(((uint32_t*)&chain[level-1]), &m, sizeof(uint32_t));
            if(REPORT) printf("%2u: %zx -- %u --> %zx left\n", level, chain[level], m, chain[level - 1]);
            chain[level] = construct(chain[level-1] >> 32, 0);
            right[level] = 1;
            if(REPORT) printf("setup %zx\n", chain[level]);
            currentLength = lengthToGo;
            currentLocalOffset = 0;
        }

        if(REPORT) printf("done, rebuilding chain\n");
        while(level) {
            uint32_t m = deconstruct(chain[level], 0);
            if(REPORT) printf("level %u: %zx -> %x (%u)\n", level, chain[level], m, right[level]);
            bool r = right[level];
            level--;
            memcpy(((uint32_t*)&chain[level])+r, &m, sizeof(uint32_t));
        }
        if(REPORT) printf("returning %zx\n", chain[0]);
        return chain[0];
    }

    /**
     * Traverses the tree, calling @c tb on parts that are balanced and @c ts for parts of lengths 1 and 2
     * @param mapped
     * @param length
     * @param currentLocalOffset
     * @param currentLength
     * @param dest
     */
    template<typename TRAVERSE_BALANCED, typename TRAVERSE_SINGLE, typename... ARGS>
    __attribute__((always_inline))
    void traverse2(uint64_t mapped, uint32_t length, uint32_t currentLocalOffset, int32_t currentLength, TRAVERSE_BALANCED&& tb, TRAVERSE_SINGLE&& ts, ARGS... tbArgs) {

        if(REPORT) printf("traverse2 %zx %u %u %u\n", mapped, length, currentLocalOffset, currentLength);

        if(length <= 2) {
            (this->*ts)(mapped, length, currentLocalOffset, currentLength, tbArgs...);
            return;
        }

        uint32_t lengthLevel = lengthToLevel(length);
        uint32_t leftLength = 1 << lengthLevel;
        uint32_t rightLength = length - leftLength;

        // \   16   /\   8  /\  4 /\ 2/
        //  \      /  \    /  \  /  \/
        //   \    /    \  /    \/ />[ 1]
        //    \  /      \/ /-->[  ]
        //     \/ /---->[  ]
        //     [  ]
        //
        // \   16   /\ 2/
        //  \      /  \/
        //   \    / __/
        //    \  / /
        //     \/ /
        //     [  ]
        //

        for(; leftLength > rightLength; mapped = construct(mapped, 0)) {

            // If part of the projection is in the left part of the tree
            int32_t touchedLengthInLeft = leftLength - currentLocalOffset;
            if(touchedLengthInLeft > 0) {
                touchedLengthInLeft = std::min(touchedLengthInLeft, currentLength);
                (this->*tb)(construct(mapped & 0xFFFFFFFFULL, 0), leftLength, currentLocalOffset, touchedLengthInLeft, tbArgs...);
                currentLength -= touchedLengthInLeft;
                if(currentLength <= 0) return;
                currentLocalOffset = 0;
            } else {
                currentLocalOffset -= leftLength;
            }
            mapped >>= 32;
            if(rightLength <= 3) {
                if(rightLength >= 2) {
                    mapped = construct(mapped, 0);
                    if(rightLength == 3) {
                        // If part of the projection is in the left part of the tree
                        int32_t touchedLengthInLeft = 2 - currentLocalOffset;
                        if(touchedLengthInLeft > 0) {
                            touchedLengthInLeft = std::min(touchedLengthInLeft, currentLength);
                            (this->*ts)(construct(mapped & 0xFFFFFFFFULL, 0), 2, currentLocalOffset, touchedLengthInLeft, tbArgs...);
                            currentLength -= touchedLengthInLeft;
                            if(currentLength <= 0) return;
                            currentLocalOffset = 0;
                        } else {
                            currentLocalOffset -= 2;
                        }
                        mapped >>= 32;
                        rightLength -= 2;
                    }
                }
                (this->*ts)(mapped, rightLength, currentLocalOffset, currentLength, tbArgs...);
                return;
            }
            leftLength = 1 << lengthToLevel(rightLength);
            rightLength -= leftLength;
        }
        (this->*tb)(mapped, leftLength + rightLength, currentLocalOffset, currentLength, tbArgs...);
    }

    /**
     * Traverses the tree, calling traverseP2_construct on parts that are balanced
     * @param mapped
     * @param length
     * @param currentLocalOffset
     * @param currentLength
     * @param dest
     */
//    template<typename TRAVERSE_BALANCED, typename TRAVERSE_SINGLE, typename... ARGS>
//    __attribute__((always_inline))
//    uint32_t traverse2(Index idx, MultiProjection& projection, uint32_t level, TRAVERSE_BALANCED&& tb, TRAVERSE_SINGLE&& ts, ARGS... tbArgs) {
//        uint64_t idxWithoutLength = idx.getID();
//        uint64_t length = idx.getLength();
//        uint64_t mapped = construct(idxWithoutLength, 0, length, level == 0);
//
//        if(REPORT) printf("traverse2 %zx %u\n", mapped, length);
//        uint32_t projections = projection.getProjections();
//
//        if(length <= 2) {
//            return (this->*ts)(mapped, length, projection, level, 0, projections, tbArgs...);
//        }
//
//        uint32_t lengthLevel = lengthToLevel(length);
//        uint32_t leftLength = 1 << lengthLevel;
//        uint32_t rightLength = length - leftLength;
//
//        // \   16   /\   8  /\  4 /\ 2/
//        //  \      /  \    /  \  /  \/
//        //   \    /    \  /    \/ />[ 1]
//        //    \  /      \/ /-->[  ]
//        //     \/ /---->[  ]
//        //     [  ]
//        //
//        // \   16   /\   8  /
//        //  \      /  \    /
//        //   \    /    \  /
//        //    \  /      \/
//        //     \/ /-----/
//        //     [  ]
//        //
//        // \   16   /\ 2/
//        //  \      /  \/
//        //   \    / __/
//        //    \  / /
//        //     \/ /
//        //     [  ]
//        //
//
//        uint32_t currentLocalOffset = 0;
//        uint32_t currentProjection = 0;
//        uint32_t currentProjectionEnd = 0;
//
//        uint32_t chain[lengthLevel];
//        uint32_t chainIdx = 0;
//
//        for(; leftLength > rightLength; mapped = construct(mapped, 0)) {
//
//            // We will now determine what series of projections we will pass on
//            // and remember it as [currentProjection, currentProjectionEnd)
//
//            // Remember the offset of the last projection in the upcoming series of projections we pass on
//            uint32_t projectionOffset;
//            while(currentProjectionEnd < projections && (projectionOffset = projection.getProjection(currentProjectionEnd).getOffset(level)) < currentLocalOffset + leftLength) currentProjectionEnd++;
//
//            // If there are any projections to pass on, pass them on and determine if the last projection
//            // overlaps with the next power-of-two link
//            if(currentProjection < currentProjectionEnd) {
//                chain[chainIdx++] = (this->*tb)(construct(mapped & 0xFFFFFFFFULL, 0), leftLength, projection, level, currentProjection, currentProjectionEnd, tbArgs...);
//                if(projectionOffset + projection.getProjection(currentProjectionEnd-1).getLength(level) > currentLocalOffset + leftLength) currentProjectionEnd--;
//            }
//
//            // If there are no more projections to pass on, we are done
//            if(currentProjectionEnd >= projections) return;
//
//            // Prepare for next series
//            currentProjection = currentProjectionEnd;
//            mapped >>= 32;
//
//            // If we reach the end of the power-of-two-chain, handle that specifically
//            if(rightLength <= 3) {
//                if(rightLength >= 2) {
//                    mapped = construct(mapped, 0);
//                    if(rightLength == 3) {
//                        while(currentProjectionEnd < projections && projection.getProjection(currentProjectionEnd).getOffset(level) < currentLocalOffset + leftLength) currentProjectionEnd++;
//
//                        // If part of the projection is in the left part of the tree
//                        if(currentProjection < currentProjectionEnd) {
//                            chain[chainIdx++] = (this->*ts)(construct(mapped & 0xFFFFFFFFULL, 0), 2, projection, level, currentProjection, currentProjectionEnd, tbArgs...);
//                        }
//                        mapped >>= 32;
//                        rightLength -= 2;
//                    }
//                }
//                if(currentProjectionEnd < projections) {
//                    chain[chainIdx++] = (this->*ts)(mapped, rightLength, projection, level, currentProjectionEnd, projections, tbArgs...);
//                }
//                goto processChain;
//            }
//
//            // Prepare for the next power-of-two link
//            leftLength = 1 << lengthToLevel(rightLength);
//            rightLength -= leftLength;
//        }
//        chain[chainIdx++] = (this->*tb)(mapped, leftLength + rightLength, projection, level, currentProjection, currentProjectionEnd, tbArgs...);
//        processChain:
//        --chainIdx;
//        for(;--chainIdx;) {
//            // Combine chain[chainIdx-1] and chain[chainIdx]
//            chain[chainIdx] = deconstruct(((uint64_t)chain[chainIdx]) | (((uint64_t)chain[chainIdx+1]) << 32), 0);
//        }
//        return deconstruct(((uint64_t)chain[0]) | (((uint64_t)chain[1]) << 32), 0, 2, level == 0);
//    }

    template<typename TRAVERSE_BALANCED, typename TRAVERSE_SINGLE, typename... ARGS>
    __attribute__((always_inline))
    void traverse3_gotoJump(MultiProjection& projection, uint32_t pStart, uint32_t pEnd, uint32_t level, uint32_t originalLevel, uint32_t currentLocalOffset, uint32_t currentOffset, uint32_t nextOffset, uint32_t leftLength, uint64_t* chain, bool* right, TRAVERSE_BALANCED&& tb, TRAVERSE_SINGLE&& ts, ARGS... tbArgs) {
        if(REPORT) printf("traverse3_gotoJump: lvl %u, %zx %u@%u\n", originalLevel, chain[originalLevel], leftLength, currentLocalOffset);

        uint32_t currentLevel = originalLevel;
        while(2 <= leftLength) {

            uint64_t& prevChain = chain[currentLevel];
            currentLevel++;

            // Only right part is touched
            if(leftLength <= currentLocalOffset) {
                currentLocalOffset -= leftLength;
                if(REPORT) printf("traverse3_gotoJump: took right %zx %u@%u\n", prevChain, leftLength, currentLocalOffset);
                leftLength /= 2;
                chain[currentLevel] = construct(prevChain >> 32, 0);
                right[currentLevel] = 1;
            } else {
                if(REPORT) printf("traverse3_gotoJump: took left %zx %u@%u\n", prevChain, leftLength, currentLocalOffset);
                leftLength /= 2;
                chain[currentLevel] = construct(prevChain & 0xFFFFFFFFULL, 0);
                right[currentLevel] = 0;
            }
        }
        IndexInserted newIdx = traverse3(chain[currentLevel], false, projection, pStart, pEnd, level+1, tb, ts, tbArgs...);
        chain[currentLevel] = newIdx.getState().getData();
        while(originalLevel < currentLevel) {
            uint32_t m = deconstruct(chain[currentLevel], 0);
            if(REPORT) printf("level %u: %zx -> %x (%u)\n", currentLevel, chain[currentLevel], m, right[currentLevel]);
            bool r = right[currentLevel];
            currentLevel--;
            memcpy(((uint32_t*)&chain[currentLevel])+r, &m, sizeof(uint32_t));
            leftLength *=2;
        }
        if(REPORT) printf("traverse3_gotoJump: jump done, got index %zx at level %u\n", chain[originalLevel], currentLevel);
    }

    /**
     * Traverses the tree, calling traverseP2_construct on parts that are balanced
     * @param mapped
     * @param length
     * @param currentLocalOffset
     * @param currentLength
     * @param dest
     */
    template<typename TRAVERSE_BALANCED, typename TRAVERSE_SINGLE, typename... ARGS>
    inline
    IndexInserted traverse3(Index idx, bool isRoot, MultiProjection& projection, uint32_t pStart, uint32_t pEnd, uint32_t level, TRAVERSE_BALANCED&& tb, TRAVERSE_SINGLE&& ts, ARGS... tbArgs) {
        uint64_t idxWithoutLength = idx.getID();
        uint64_t length = idx.getLength();
        uint64_t mapped = construct(idxWithoutLength, 0, length, isRoot && level == 0);
        if(REPORT) {
            for(uint32_t l=level;l--;) printf("  ");
            printf("\033[36mtraverse3\033[0m %zx %u %u %u %u\n", idxWithoutLength, (uint32_t)length, pStart, pEnd, level);
            for(uint32_t p = pStart; p < pEnd; ++p) {
                auto& proj = projection.getProjection(p);
                auto lando = proj.getLengthAndOffsets();

                printf("  - %u at", lando.getLength());
                for(uint32_t o = 0; o < lando.getOffsets(); ++o) {
                    printf(" %u (%u)", proj.getOffset(o).getOffset(), (uint32_t)proj.getOffset(o).getOptions());
                }
                printf("\n");
            }
        }
        uint32_t lengthLevel = lengthToLevel(length);
        uint32_t leftLength = 1 << lengthLevel;
        uint32_t rightLength = length - leftLength;

        uint64_t chain[lengthLevel+2];
        bool right[lengthLevel+2];
        uint32_t lengths[leftLength+2];
        uint32_t currentLevel = 0;
        chain[0] = mapped;
        lengths[0] = length;

        //       _
        // a b c d e f g h
        //  1   2   3   4
        //    5       6
        //       (7)
        //
        // mappedOriginal = [5 6]
        //
        // chain[0] = [5 6]   left[0] = ?
        // chain[1] = [1 2]   left[1] = 1
        // chain[2] = [c D]   left[2] = 0
        //

        uint32_t currentProjection = pStart;
        uint32_t currentOffset = projection.getProjection(currentProjection).getOffset(level).getOffset();
        uint32_t currentLength = projection.getProjection(currentProjection).getLength(level);
        uint32_t nextOffset;
        uint32_t nextLength;
        if(currentProjection + 1 < pEnd) {
            nextOffset = projection.getProjection(currentProjection + 1).getOffset(level).getOffset();
            nextLength = projection.getProjection(currentProjection + 1).getLength(level);
        } else {
            nextOffset = length;
            nextLength = 0;
        }

        uint32_t lengthToGo = currentLength;
        uint32_t globalOffset = 0;

        /// TODO...

        while(currentProjection < pEnd) {

//            uint32_t currentOffset = nextOffset;
//            uint32_t currentLength = nextLength;
//            if(currentProjection + 1 < pEnd) {
//                nextOffset = projection.getProjection(currentProjection+1).getOffset(level).getOffset();
//                nextLength = projection.getProjection(currentProjection+1).getLength(level);
//            } else {
//                nextOffset = length;
//            }

            while(true) {

                currentLevel++;
                if(REPORT) {
                    for(uint32_t l=level;l--;) printf("  ");
                    printf("- lvl %u (%u+%u), proj %u, off %u (%u), noff %u (%u), gloff %u, chain[currentLevel-1] = %zx\n", currentLevel, leftLength, rightLength, currentProjection, currentOffset, currentLength, nextOffset, nextLength, globalOffset, chain[currentLevel-1]);
                }

                uint32_t currentLocalOffset = currentOffset - globalOffset;
                uint32_t nextLocalOffset = nextOffset - globalOffset;

                // Only right part is touched
                if(leftLength <= currentLocalOffset) {
                    if(false) {
                        uint64_t right = (this->*tb)(construct(chain[currentLevel-1] >> 32, 0), leftLength, currentLocalOffset, lengthToGo, tbArgs...);
                        right = deconstruct(right, 0);
                        right <<= 32;
                        chain[currentLevel-1] = right | (chain[currentLevel-1] & 0xFFFFFFFFULL);
                        lengthToGo = 0;
                    } else {
                        if(rightLength == 1) {
                            currentLevel--;
                            uint64_t right = (this->*ts)(chain[currentLevel] >> 32, 1, 0, 1, tbArgs...);
                            right <<= 32;
                            chain[currentLevel] = right | (chain[currentLevel] & 0xFFFFFFFFULL);
                            lengthToGo = 0;
                            currentProjection++;
                            break;
                        }
                        currentLocalOffset -= leftLength;
                        globalOffset += leftLength;
                        lengths[currentLevel] = rightLength;
                        leftLength = 1 << lengthToLevel(rightLength);
                        rightLength -= leftLength;
                        chain[currentLevel] = construct(chain[currentLevel - 1] >> 32, 0);
                        right[currentLevel] = 1;
                        if(REPORT) {
                            for(uint32_t l = level; l--;) printf("  ");
                            printf("  only right part touched %8zx -> %16zx\n", chain[currentLevel - 1] >> 32, chain[currentLevel]);
                        }
                    }
                }

                // The left part is only touched by currentProjection
                else if(leftLength <= nextLocalOffset) {


                    // <-------> leftLength
                    //
                    //  \      / \      /
                    //   \    /   \    /
                    //    \  /     \  /
                    //     \/       \/
                    //      \       /
                    //       \     /
                    //     [    |    ]    chain[currentLevel-1]
                    //          |
                    //     [    |    ]    chain[currentLevel-2], might not exist

                    // If the next offset doesn't touch the right part
                    uint32_t touchedHere = leftLength * 2;
                    if(touchedHere <= nextLocalOffset) {
                        if(REPORT) {
                            for(uint32_t l=level;l--;) printf("  ");
                            if(REPORT) printf("  whole further tree only touched by currentProjection %u\n", currentProjection);
                        }
                        assert(leftLength == rightLength && "ok, this is not correct after all");
                        currentLevel--;
                        if(level < projection.getProjection(currentProjection).getLengthAndOffsets().getOffsets() - 1) {
                            uint32_t currentProjectionEnd = currentProjection + 1;
                            if(currentOffset == nextOffset) {
                                while(currentProjectionEnd < pEnd && ((nextOffset = projection.getProjection(currentProjectionEnd).getOffset(level).getOffset())) == currentOffset) {
                                    if(REPORT) printf("traverse3_gotoJump: determined projection %u is in the same jump\n", currentProjectionEnd);
                                    currentProjectionEnd++;
                                }
                            }
                            lengths[currentLevel] = touchedHere;
//                            printf("BEFOR traverse3_gotoJump:\n");
//                            printf("      currentLevel          = %u:\n", currentLevel);
//                            printf("      chain[currentLevel]   = %zx:\n", chain[currentLevel]);
//                            printf("      lengths[currentLevel] = %u:\n", lengths[currentLevel]);
//                            printf("      leftLength            = %u:\n", leftLength);
//                            printf("      rightLength           = %u:\n", rightLength);
//                            printf("      currentProjection     = %u:\n", currentProjection);
//                            printf("      currentOffset         = %u:\n", currentOffset);
//                            printf("      currentLength         = %u:\n", currentLength);
//                            printf("      nextOffset            = %u:\n", nextOffset);
//                            printf("      nextLength            = %u:\n", nextLength);
                            traverse3_gotoJump(projection, currentProjection, currentProjectionEnd, level, currentLevel, currentLocalOffset, currentOffset, nextOffset, leftLength, chain, right, tb, ts, tbArgs...);
//                            printf("AFTER traverse3_gotoJump:\n");
//                            printf("      chain[currentLevel]   = %zx:\n", chain[currentLevel]);
//                            printf("      lengths[currentLevel] = %u:\n", lengths[currentLevel]);
//                            printf("      leftLength            = %u:\n", leftLength);
//                            printf("      rightLength           = %u:\n", rightLength);
//                            printf("      currentProjection     = %u:\n", currentProjection);
//                            printf("      currentOffset         = %u:\n", currentOffset);
//                            printf("      currentLength         = %u:\n", currentLength);
//                            printf("      nextOffset            = %u:\n", nextOffset);
//                            printf("      nextLength            = %u:\n", nextLength);
                            currentProjection = currentProjectionEnd;
                            if(currentProjection >= pEnd) {
                                if(REPORT) {
                                    for(uint32_t l=level;l--;) printf("  ");
                                    printf("  no more projections %u\n", __LINE__);
                                }
                                currentOffset = length;
                                break;
                            }
                            currentOffset = nextOffset;
                            currentLength = projection.getProjection(currentProjection).getLength(level);
                            lengthToGo = currentLength;
                            if(currentProjection + 1 < pEnd) {
                                nextOffset = projection.getProjection(currentProjection + 1).getOffset(level).getOffset();
                                nextLength = projection.getProjection(currentProjection + 1).getLength(level);
                            } else {
                                nextOffset = length;
                            }
                            break;
//                            if(currentProjection >= pEnd) {
//                                if(REPORT) {
//                                    for(uint32_t l=level;l--;) printf("  ");
//                                    printf("  no more projections %u\n", __LINE__);
//                                }
//                                currentOffset = length;
//                                break;
//                            }
                        } else {
                            touchedHere = std::min(touchedHere - currentLocalOffset, lengthToGo);
                            chain[currentLevel] = (this->*tb)(chain[currentLevel], leftLength*2, currentLocalOffset, touchedHere, tbArgs...);
                        }
                        lengthToGo -= touchedHere;
                        if(lengthToGo) {
                            currentOffset += touchedHere;
                            currentLength -= touchedHere;
                            if(REPORT) printf("  projection not done yet, remaining %u, offset %u\n", lengthToGo, currentOffset);
                        } else {
                            currentProjection++;
                            currentOffset = nextOffset;
                            currentLength = nextLength;
                            lengthToGo = currentLength;
                            if(currentProjection + 1 < pEnd) {
                                nextOffset = projection.getProjection(currentProjection + 1).getOffset(level).getOffset();
                                nextLength = projection.getProjection(currentProjection + 1).getLength(level);
                                if(REPORT) printf("  projection %u done, setup current %u %u@%u and next %u@%u\n", currentProjection-1, currentProjection, currentLength, currentOffset, nextLength, nextOffset);
                            } else {
                                nextOffset = length;
                                if(REPORT) printf("  projection %u done, setup current %u %u@%u and next to finish\n", currentProjection-1, currentProjection, currentLength, currentOffset);
                            }
                        }
                        break;
                    } else {
                        if(REPORT) {
                            for(uint32_t l=level;l--;) printf("  ");
                            printf("  left part only touched by currentProjection %u, noff %u\n", currentProjection, nextOffset);
                        }
                        if(level < projection.getProjection(currentProjection).getLengthAndOffsets().getOffsets() - 1) {
                            chain[currentLevel] = construct(chain[currentLevel-1] & 0xFFFFFFFFULL, 0);
                            lengths[currentLevel] = leftLength;
                            uint32_t currentProjectionEnd = currentProjection + 1;
                            if(currentOffset == nextOffset) {
                                while(currentProjectionEnd < pEnd && ((nextOffset = projection.getProjection(currentProjectionEnd).getOffset(level).getOffset())) == currentOffset) {
                                    if(REPORT) printf("traverse3_gotoJump: determined projection %u is in the same jump\n", currentProjectionEnd);
                                    currentProjectionEnd++;
                                }
                            }
                            traverse3_gotoJump(projection, currentProjection, currentProjectionEnd, level, currentLevel, currentLocalOffset, currentOffset, nextOffset, leftLength/2, chain, right, tb, ts, tbArgs...);
//                            printf("AFTER traverse3_gotoJump:\n");
//                            printf("      currentLevel          = %u:\n", currentLevel);
//                            printf("      chain[currentLevel]   = %zx:\n", chain[currentLevel]);
//                            printf("      lengths[currentLevel] = %u:\n", lengths[currentLevel]);
//                            printf("      leftLength            = %u:\n", leftLength);
//                            printf("      rightLength           = %u:\n", rightLength);
//                            printf("      currentProjection     = %u:\n", currentProjection);
//                            printf("      currentOffset         = %u:\n", currentOffset);
//                            printf("      currentLength         = %u:\n", currentLength);
//                            printf("      nextOffset            = %u:\n", nextOffset);
//                            printf("      nextLength            = %u:\n", nextLength);
                            uint32_t left = (uint32_t)deconstruct(chain[currentLevel], 0);
                            currentLevel--;
                            chain[currentLevel] = ((uint64_t)left) | (chain[currentLevel] & 0xFFFFFFFF00000000ULL);
                            currentProjection = currentProjectionEnd;
                            if(currentProjection >= pEnd) {
                                if(REPORT) {
                                    for(uint32_t l=level;l--;) printf("  ");
                                    printf("  no more projections %u\n", __LINE__);
                                }
                                currentOffset = length;
                                break;
                            }
                            currentOffset = nextOffset;
                            currentLength = projection.getProjection(currentProjection).getLength(level);
                            lengthToGo = currentLength;
                            if(currentProjection + 1 < pEnd) {
                                nextOffset = projection.getProjection(currentProjection + 1).getOffset(level).getOffset();
                                nextLength = projection.getProjection(currentProjection + 1).getLength(level);
                            } else {
                                nextOffset = length;
                            }
                            break;
                        } else {
                            touchedHere = std::min(leftLength - currentLocalOffset, lengthToGo);
                            uint64_t left = (this->*tb)(construct(chain[currentLevel-1] & 0xFFFFFFFFULL, 0), leftLength, currentLocalOffset, touchedHere, tbArgs...);
                            left = deconstruct(left, 0) & 0xFFFFFFFFULL;
                            chain[currentLevel-1] = left | (chain[currentLevel-1] & 0xFFFFFFFF00000000ULL);
                            lengthToGo -= touchedHere;
                            if(lengthToGo) {
                                currentOffset += touchedHere;
                                currentLength -= touchedHere;
                                if(REPORT) printf("  projection not done yet, remaining %u, offset %u\n", lengthToGo, currentOffset);
                            } else {
                                currentLevel--;
                                currentProjection++;
                                currentOffset = nextOffset;
                                currentLength = nextLength;
                                lengthToGo = currentLength;
                                if(currentProjection + 1 < pEnd) {
                                    nextOffset = projection.getProjection(currentProjection + 1).getOffset(level).getOffset();
                                    nextLength = projection.getProjection(currentProjection + 1).getLength(level);
                                    if(REPORT) printf("  projection %u done, setup current %u %u@%u and next %u@%u\n", currentProjection-1, currentProjection, currentLength, currentOffset, nextLength, nextOffset);
                                } else {
                                    nextOffset = length;
                                    if(REPORT) printf("  setup current %u %u@%u and next to finish\n", currentProjection-1, currentLength, currentOffset);
                                }
                                break;
                            }
                        }
                        if(rightLength == 1) {
                            currentLevel--;
                            uint64_t right = (this->*ts)(chain[currentLevel] >> 32, 1, 0, 1, tbArgs...);
                            right <<= 32;
                            chain[currentLevel] = right | (chain[currentLevel] & 0xFFFFFFFFULL);
                            lengthToGo = 0;
                            currentProjection++;
                            break;
                        }
                        globalOffset += leftLength;
                        currentLocalOffset = currentOffset - globalOffset;
                        lengths[currentLevel] = rightLength;
                        leftLength = 1 << lengthToLevel(rightLength);
                        rightLength -= leftLength;
                        chain[currentLevel] = construct(chain[currentLevel-1] >> 32, 0);
                        right[currentLevel] = 1;
                    }
                }

                // The left part is touched by currentProjection and at least one more
                else {
                    if(REPORT) {
                        for(uint32_t l=level;l--;) printf("  ");
                        printf("  left part touched by multiple projections, going there\n");
                    }
                    lengths[currentLevel] = leftLength;
                    leftLength /= 2;
                    rightLength = leftLength;
                    chain[currentLevel] = construct(chain[currentLevel-1] & 0xFFFFFFFFULL, 0);
                    right[currentLevel] = 0;
                }
                if(leftLength == 1) {
                    if(REPORT) {
                        for(uint32_t l=level;l--;) printf("  ");
                        printf("- lvl %u detected leftLength == 1, gloff %u, chain[currentLevel] = %zx\n", currentLevel + 1, globalOffset, chain[currentLevel]);
                    }
                    if(level < projection.getProjection(currentProjection).getLengthAndOffsets().getOffsets() - 1) {
                        if(REPORT) {
                            for(uint32_t l=level;l--;) printf("  ");
                            printf("  continues onto another tree\n");
                        }
                        uint32_t currentProjectionEnd = currentProjection + 1;
                        if(currentOffset == nextOffset) {
                            while(currentProjectionEnd < pEnd && ((nextOffset = projection.getProjection(currentProjectionEnd).getOffset(level).getOffset())) == currentOffset) currentProjectionEnd++;
                        }
//                        printf("BEFOR traverse3:\n");
//                        printf("      currentLevel          = %u:\n", currentLevel);
//                        printf("      chain[currentLevel]   = %zx:\n", chain[currentLevel]);
//                        printf("      lengths[currentLevel] = %u:\n", lengths[currentLevel]);
//                        printf("      leftLength            = %u:\n", leftLength);
//                        printf("      rightLength           = %u:\n", rightLength);
//                        printf("      currentProjection     = %u:\n", currentProjection);
//                        printf("      currentOffset         = %u:\n", currentOffset);
//                        printf("      currentLength         = %u:\n", currentLength);
//                        printf("      nextOffset            = %u:\n", nextOffset);
//                        printf("      nextLength            = %u:\n", nextLength);
                        IndexInserted newIdx = traverse3(chain[currentLevel], false, projection, currentProjection, currentProjectionEnd, level+1, tb, ts, tbArgs...);
                        chain[currentLevel] = newIdx.getState().getData();
                        currentProjection = currentProjectionEnd;
                        if(currentProjection >= pEnd) {
                            if(REPORT) {
                                for(uint32_t l=level;l--;) printf("  ");
                                printf("  no more projections %u\n", __LINE__);
                            }
                            currentOffset = length;
                            break;
                        }
                        currentOffset = nextOffset;
                        currentLength = projection.getProjection(currentProjection).getLength(level);
                        lengthToGo = currentLength;
                        if(currentProjection + 1 < pEnd) {
                            nextOffset = projection.getProjection(currentProjection + 1).getOffset(level).getOffset();
                            nextLength = projection.getProjection(currentProjection + 1).getLength(level);
                        } else {
                            nextOffset = length;
                        }
                    } else {
                        uint32_t touchedHere = std::min(leftLength + rightLength - currentLocalOffset, lengthToGo);
                        chain[currentLevel] = (this->*ts)(chain[currentLevel], leftLength + rightLength, currentLocalOffset, touchedHere, tbArgs...);
                        if(REPORT) printf("currentLocalOffset %u, nextOffset - globalOffset %u\n", currentLocalOffset, nextOffset - globalOffset);
                        if(nextOffset - globalOffset == 1) {
                            currentProjection++;
                            if(REPORT) printf("detected next projection in same leaf-node\n");
                            chain[currentLevel] = (this->*ts)(chain[currentLevel], leftLength + rightLength, 1, 1, tbArgs...);
                            currentLength = nextLength - 1;
                            if(currentLength) {
                                currentOffset = currentOffset + 2;
                                lengthToGo = currentLength;
                                if(REPORT) printf("  projection not done yet, remaining %u, offset %u\n", lengthToGo, currentOffset);
                            } else {
                                currentProjection++;
                                if(currentProjection >= pEnd) {
                                    if(REPORT) {
                                        for(uint32_t l = level; l--;) printf("  ");
                                        if(REPORT) printf("  projection %u done, no more projections %u\n", currentProjection-1, __LINE__);
                                    }
                                    currentOffset = length;
                                    break;
                                }
                                currentOffset = projection.getProjection(currentProjection).getOffset(level).getOffset();
                                currentLength = projection.getProjection(currentProjection).getLength(level);
                                lengthToGo = currentLength;
                            }
                            if(currentProjection + 1 < pEnd) {
                                nextOffset = projection.getProjection(currentProjection + 1).getOffset(level).getOffset();
                                nextLength = projection.getProjection(currentProjection + 1).getLength(level);
                                if(REPORT) printf("  setup current %u %u@%u and next %u@%u\n", currentProjection, currentLength, currentOffset, nextLength, nextOffset);
                            } else {
                                nextOffset = length;
                                if(REPORT) printf("  setup current %u %u@%u and next to finish\n", currentProjection, currentLength, currentOffset);
                            }
                        } else {
                            if(REPORT) printf("next projection is NOT in same leaf-node\n");
                            lengthToGo -= touchedHere;
                            if(lengthToGo) {
                                currentOffset += touchedHere;
                                currentLength -= touchedHere;
                                if(REPORT) printf("  projection not done yet, remaining %u, offset %u\n", lengthToGo, currentOffset);
                            } else {
                                currentProjection++;
                                currentOffset = nextOffset;
                                currentLength = nextLength;
                                lengthToGo = currentLength;
                                if(currentProjection + 1 < pEnd) {
                                    nextOffset = projection.getProjection(currentProjection + 1).getOffset(level).getOffset();
                                    nextLength = projection.getProjection(currentProjection + 1).getLength(level);
                                    if(REPORT) printf("  projection %u done, setup current %u %u@%u and next %u@%u\n", currentProjection-1, currentProjection, currentLength, currentOffset, nextLength, nextOffset);
                                } else {
                                    nextOffset = length;
                                    if(REPORT) printf("  projection %u done, setup current %u %u@%u and next to finish\n", currentProjection-1, currentProjection, currentLength, currentOffset);
                                }
                            }
                        }
                    }
                    break;
                }
            }

            uint32_t spansTo = globalOffset + lengths[currentLevel];

            if(REPORT) {
                for(uint32_t l=level;l--;) printf("  ");
                printf("- backtracking chain, spansTo %u+%u=%u, looking for offset %u\n", globalOffset, lengths[currentLevel], spansTo, currentOffset);
            }

            // If the currentOffset is not within the range of the current sub-tree
            if(spansTo <= currentOffset) {
                while(currentLevel > 0) {
                    uint32_t m = deconstruct(chain[currentLevel], 0);
                    if(REPORT) {
                        for(uint32_t l = level; l--;) printf("  ");
                        printf("  %2u:%x %zx %u\n", currentLevel, m, chain[currentLevel - 1], right[currentLevel]);
                    }
                    currentLevel--;
                    //                memcpy((uint32_t*)&chain[currentLevel - 1] + right[currentLevel], &m, sizeof(m));
                    chain[currentLevel] = right[currentLevel + 1] ? (((uint64_t) m) << 32) |
                                                                    (chain[currentLevel] & 0xFFFFFFFFULL)
                                                                  : (uint64_t) m |
                                                                    (chain[currentLevel] & 0xFFFFFFFF00000000ULL);
                    if(right[currentLevel + 1]) {
                        rightLength += leftLength;
                        leftLength = lengths[currentLevel] - rightLength;
                        globalOffset -= leftLength;
                    } else {
                        leftLength += rightLength;
                        rightLength = lengths[currentLevel] - leftLength;
                        spansTo += rightLength;
                        if(currentOffset < spansTo) {
                            if(REPORT) printf("  found path in chain to continue on, spansTo=%u\n", spansTo);
                            break;
                        }
                    }
                }
            }
        }
        if(REPORT) {
            for(uint32_t l = level; l--;) printf("  ");
            printf("need to backtrack left (we are right in the tree)\n");
        }
        while(currentLevel > 0) {
            uint32_t m = deconstruct(chain[currentLevel], 0);
            if(REPORT) {
                for(uint32_t l = level; l--;) printf("  ");
                printf("  %2u:%x %zx %u\n", currentLevel, m, chain[currentLevel - 1], right[currentLevel]);
            }
            currentLevel--;
            chain[currentLevel] = (((uint64_t) m) << 32) | (chain[currentLevel] & 0xFFFFFFFFULL);
        }
        if(REPORT) {
            for(uint32_t l=level;l--;) printf("  ");
            printf(">  returning %zx\n", chain[0]);
        }
        uint64_t result = deconstruct(chain[0], 0, length, isRoot && level == 0);
        checkForInsertedZeroes(result);
        return IndexInserted(result, length);
    }

    /**
     * Traverses the tree, calling traverseP2_construct on parts that are balanced
     * @param mapped
     * @param length
     * @param currentLocalOffset
     * @param currentLength
     * @param dest
     */
    template<typename TRAVERSE_BALANCED, typename TRAVERSE_SINGLE, typename... ARGS>
    __attribute__((always_inline))
    Index traverse4(Index idx, MultiProjection& projection, uint32_t pStart, uint32_t pEnd, uint32_t level, TRAVERSE_BALANCED&& tb, TRAVERSE_SINGLE&& ts, ARGS... tbArgs) {
//        uint64_t idxWithoutLength = idx.getID();
//        uint64_t mapped = construct(idxWithoutLength, 0, length, level == 0);
//        if(REPORT) printf("traverseP2_delta %zx %u %u\n", mapped, length, level);
//        uint32_t length = idx.getLength();
//        uint32_t lengthLevel = lengthToLevel(length);
//        uint32_t leftLength = 1 << lengthLevel;
//        uint32_t rightLength = length - leftLength;
//
//        uint64_t chain[lengthLevel+1];
//        bool right[lengthLevel+1];
//        uint32_t lengths[lengthLevel+1];
//        uint32_t currentLevel = 0;
//        chain[0] = mapped;
//        lengths[0] = length;
//        right[0] = 0;
//
//        //       _
//        // a b c d e f g h
//        //  1   2   3   4
//        //    5       6
//        //       (7)
//        //
//        // mappedOriginal = [5 6]
//        //
//        // chain[0] = [5 6]   left[0] = ?
//        // chain[1] = [1 2]   left[1] = 1
//        // chain[2] = [c D]   left[2] = 0
//        //
//
//        uint32_t currentProjection = pStart;
//        uint32_t currentProjectionOffset = projection.getProjection(currentProjection).getOffset(level);
//        uint32_t currentLocalLength = projection.getProjection(currentProjection).getLength(level);
//        uint32_t projections = projection.getProjections();
//
//        uint32_t currentLengthToGo = currentLocalLength;
//        uint32_t currentLocalOffset = currentProjectionOffset;
//
//            for(;leftLength > 1;) {
//                currentLevel++;
//                if(leftLength <= currentLocalOffset) {
//                    if(rightLength == 1) {
//                        //...
//                    }
//                    currentLocalOffset -= leftLength;
//                    chain[currentLevel] = rightLength;
//                    leftLength = 1 << lengthToLevel(rightLength);
//                    rightLength -= leftLength;
//                    chain[currentLevel] = construct(chain[currentLevel-1] >> 32, 0);
//                    right[currentLevel] = 1;
//                } else {
//                    chain[currentLevel] = leftLength;
//                    leftLength /= 2;
//                    rightLength = leftLength;
//                    chain[currentLevel] = construct(chain[currentLevel-1] & 0xFFFFFFFFULL, 0);
//                    right[currentLevel] = 0;
//                }
//            }
//            uint32_t left;
//            if(level < projection.getProjection(currentProjection).getOffsets() - 1) {
//                uint32_t currentProjectionEnd = currentProjection;
//                while(currentProjectionEnd < pEnd && (projection.getProjection(currentProjectionEnd).getOffset(level)) == projection.getProjection(0).getOffset(level)) currentProjectionEnd++;
//                chain[currentLevel] = traverse4(chain[currentLevel], projection, currentProjection, currentProjectionEnd, level+1, tb, ts, tbArgs...);
//                currentProjection = currentProjectionEnd;
//                currentLengthToGo = 0;
//                uint32_t nextProjectionOffset = projection.getProjection(currentProjection).getOffset(level);
//                currentLocalOffset += nextProjectionOffset - currentProjectionOffset;
//            } else {
//                uint32_t relevantLength = 1;
//                currentLengthToGo -= relevantLength;
//                currentLocalOffset += relevantLength;
//                if(currentLengthToGo == 0) {
//                    currentProjection++;
//                    uint32_t nextProjectionOffset = projection.getProjection(currentProjection).getOffset(level);
//                    currentLocalOffset += nextProjectionOffset - currentProjectionOffset;
//                }
//            }
//
//            if(currentProjection >= pEnd) {
//                // ...
//            }
//
//            while(currentLevel > 0) {
//                uint32_t m = deconstruct(chain[currentLevel], 0);
//                memcpy((uint32_t*) chain[currentLevel - 1] + right[currentLevel], &m, sizeof(m));
//                if(right[currentLevel]) {
//                    leftLength *= 2;
//                    rightLength = leftLength;
//                } else {
//                    leftLength += rightLength;
//                    rightLength = lengths[currentLevel - 1] - leftLength;
//                    currentLocalOffset += leftLength;
//                    if(currentLocalOffset) {
//
//                    }
//                }
//                currentLevel--;
//            }

    }
private:
    std::function<void(uint64_t, bool)> _handler_full;

};
