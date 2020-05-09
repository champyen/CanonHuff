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
        shared_ptr<SymNode> rnode = nullptr,
        uint64_t len = 0,
        uint64_t bin = 0
    ){
        symbol = sym;
        kval = freq;
        left = lnode;
        right = rnode;
        bits = len;
        code = bin;
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
    HuffCoder(int size) { 
        heap = make_shared<MinHeap<SymNode>>(size); 
        numSymbols = size;
        dummy = make_shared<SymNode>(0, 0);
        //dectab_0.fill(make_shared<SymNode>(0, 0));
    }
    ~HuffCoder() {};
    HuffCoder& insert(shared_ptr<SymNode>);
    uint32_t getMaxCodeLen(){ return maxLen; }

    HuffCoder& encode(uint64_t, Bitchain &, bool);
    HuffCoder& getVLC(uint64_t, uint64_t &, uint64_t&);
    HuffCoder& decode(uint64_t &, Bitchain &, bool);
    HuffCoder& getSymbol(uint64_t&, uint64_t, uint64_t &);

private:
    void makeHuffCode(shared_ptr<SymNode>, uint64_t, uint64_t);

    unordered_map<uint64_t, shared_ptr<SymNode>> symtab;
    shared_ptr<MinHeap<SymNode>> heap;
    shared_ptr<SymNode> root;
    shared_ptr<SymNode> dummy;
    //unordered_map<uint64_t, vector<shared_ptr<SymNode>>> dectab;
    vector<shared_ptr<SymNode>> dectab;

    array<shared_ptr<SymNode>, 256> dectab_0;
    unordered_map<uint64_t, shared_ptr<array<shared_ptr<SymNode>, 256>>> dectabs;

    int numSymbols = 0;
    int numInserted = 0;
    uint32_t maxLen = 0;
};

#endif //_CANONICAL_HUFFMAN_H_
