#ifndef _HUFF_CODER_H_
#define _HUFF_CODER_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include <stdint.h>

#include "min_heap.h"
#include "bitchain.hpp"

class SymNode
{
public:
    SymNode(
        uint64_t sym,
        uint64_t freq,
        shared_ptr<SymNode> lnode = nullptr,
        shared_ptr<SymNode> rnode = nullptr
    ){
        symbol = sym;
        kval = freq;
        left = lnode;
        right = rnode;
    }

    ~SymNode() {};

    uint64_t symbol = 0;
    uint64_t kval = 0;       // as freq
    uint64_t bits = 0;
    uint64_t code = 0;
    shared_ptr<SymNode> left;
    shared_ptr<SymNode> right;
};

class HuffCoder
{
public:
    HuffCoder(int size) { heap = make_shared<MinHeap<SymNode>>(size); numSymbols = size; }
    ~HuffCoder() {};
    HuffCoder& insert(shared_ptr<SymNode>);
    uint32_t getMaxCodeLen(){ return maxLen; }

    HuffCoder& encode(uint64_t, Bitchain &, bool);
    HuffCoder& getVLC(uint64_t, uint64_t &, uint64_t&);
    //HuffCoder& decode(uint32_t &sym, uint32_t code, uint32_t &bits, bool dep = true);

private:
    void makeHuffCode(shared_ptr<SymNode>, uint64_t, uint64_t);

    unordered_map<uint64_t, shared_ptr<SymNode>> symtab;
    shared_ptr<MinHeap<SymNode>> heap;
    shared_ptr<SymNode> root;
    int numSymbols = 0;
    int numInserted = 0;
    int maxLen = 0;
};

#endif //_CANONICAL_HUFFMAN_H_
