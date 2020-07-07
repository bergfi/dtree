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
            return _data & 0xFF000000;
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
    dtree(): Storage() {
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
        IndexInserted R(result, root.getLength());
        if(REPORT) {
            uint32_t buffer[R.getState().getLength()];
            get(R.getState(), buffer, isRoot);
            printBuffer("Inserted", buffer, idx.getLength(), result);
        }
        assert(root.getLength() > 0);
        return R;
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
    IndexInserted extend(Index idx, uint32_t alignment, uint32_t deltaLength, uint32_t* data, bool isRoot) {
        uint32_t length;
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
    IndexInserted extendAt(Index idx, uint32_t offset, uint32_t deltaLength, uint32_t* data, bool isRoot) {
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
        uint64_t newLength = length + (uint64_t)offset + (uint64_t)deltaLength;
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
        uint64_t idx = this->template storage_fop(v, level, length, isRoot);
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
        uint64_t idx = this->template storage_fop(v, level, 2, false);
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
        uint64_t idx = this->template storage_find(v, level, length, isRoot);
        return idx;
    }

    __attribute__((always_inline))
    uint64_t construct(uint64_t idx, uint32_t level, uint64_t& length, bool isRoot = false) {
//        if(idx == 0) return 0;
        uint64_t mapped = this->template storage_get(idx, level, length, isRoot);
//        assert(mapped != Storage::NotFound() && "Cannot construct: index is not in the map");
        if(REPORT) printf("Got %8zx(%u) -> %16zx (%s)\n", idx, 8, mapped, isRoot ? "root":"");
        return mapped;
    }

    __attribute__((always_inline))
    uint64_t construct(uint64_t idx, uint32_t level, bool isRoot = false) {
//        if(idx == 0) return 0;
        uint64_t g;
        uint64_t mapped = this->template storage_get(idx, level, g, isRoot);
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
            printf("\033[36mconstructSparse\033[0m(%zx, %u, %u, %p, [", idx, length, internalOffset >> 8, buffer);
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
            printf("\033[36mdeltaSparseApply\033[0m(%zx, %u, %u, %p, [", idx, length, internalOffset >> 8, delta);
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

        uint32_t level = lengthToLevel(extendTo);
        uint32_t leftLength = 1 << level;

        // If the left part is part of what we already have
        if(extendTo == length) {
            return idx;
        } else if(leftLength == length) {
            return deconstruct(idx, level, extendTo, toRoot);
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
private:
    std::function<void(uint64_t, bool)> _handler_full;

};
