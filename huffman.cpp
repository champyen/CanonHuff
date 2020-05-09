#include <stdio.h>
#include "huffman.h"

HuffCoder& HuffCoder::insert(shared_ptr<SymNode> node)
{
    symtab[node->symbol] = node;
    heap->insert(node);
    numInserted++;

    if(numInserted == numSymbols){
        heap->makeHeap();

        int left = numSymbols;
        while(left){
            shared_ptr<SymNode> min0 = heap->getMin();
            left = heap->pop(nullptr) - 1;
            shared_ptr<SymNode> min1 = heap->getMin();
            shared_ptr<SymNode> node = make_shared<SymNode>(~0UL, min0->kval + min1->kval, min0, min1);
            if(left)
                heap->pop(node);
            else
                heap->pop(nullptr);
            root = node;
        }
        makeHuffCode(root, 0, 0);

        // Canonical Code
        {
            for(auto pair : symtab){
                shared_ptr<SymNode> node = pair.second;
                node->kval = node->bits;
                heap->insert(node);
            }
            heap->makeHeap();

            for(int i = 0, code = 0; i < numSymbols; i++){
                shared_ptr<SymNode> node = heap->getMin();
                int next_len = node->bits;
                if(heap->pop(nullptr)){
                    next_len = heap->getMin()->bits;
                }
                node->code = code;
                code = (code+1) << (next_len - node->bits);
            }
        }

        // TODO - multi-level decode table
        #ifdef ONE_DECODE_TAB
        {
            for(auto pair : symtab){
                shared_ptr<SymNode> node = pair.second;
                node->kval = node->code << (maxLen - node->bits);
                heap->insert(node);
            }
            heap->makeHeap();

            shared_ptr<SymNode> dec0 = make_shared<SymNode>(0, 0);
            shared_ptr<SymNode> dec1 = nullptr;
            for(int i = 0; i < numSymbols; i++){
                dec1 = heap->getMin();
                heap->pop(nullptr);
                for(uint64_t j = dec0->kval; j < dec1->kval; j++){
                    dectab.push_back(dec0);
                }
                dec0 = dec1;
            }
            for(uint64_t j = dec1->kval; j < (1 << maxLen); j++)
                dectab.push_back(dec1);
        }
        #else
        //make maxLen multiple of 8
        printf("setup multi-level decode table\n");
        maxLen = ((maxLen+7)) & ~(0x7);
        for(auto pair : symtab){
            shared_ptr<SymNode> node = pair.second;
            uint64_t bits = node->bits;
            uint64_t code = node->code << (maxLen - node->bits);

            uint64_t bits_ofs = 0;
            uint64_t tab_idx, idx, code_idx;
            while(bits > (bits_ofs+8)){
                /*
                 * Here the table index is built by INDEX_LEN + CODE_PREFIX
                 * For pointer-based multi-level table, it doesn't have the problem
                 * If code_prefix used as hash key, the problem is the value
                 * e.g.: 0x000001 and 0x0001 has the same value, but means different prefix.
                 * The cost is 8bit used for separating prefix space.
                 * This makes the largest number of bits for each codeword is 56 not 64.
                 */
                tab_idx = code >> (maxLen - bits_ofs);
                idx = (bits_ofs << 56) | tab_idx;
                if(dectabs.find(idx) == dectabs.end()){
                    dectabs[idx] = make_shared<array<shared_ptr<SymNode>,256>>();
                    dectabs[idx]->fill(dummy);
                }

                code_idx = (code >> (maxLen - (bits_ofs+8))) & 0xFF;
                (*dectabs[idx])[code_idx] = nullptr;
                bits_ofs += 8;
            }
            tab_idx = code >> (maxLen - bits_ofs);
            idx = (bits_ofs << 56) | tab_idx;
            if(dectabs.find(idx) == dectabs.end()){
                dectabs[idx] = make_shared<array<shared_ptr<SymNode>,256>>();
                dectabs[idx]->fill(dummy);
            }
            code_idx = (code >> (maxLen - (bits_ofs+8))) & 0xFF;
            (*dectabs[idx])[code_idx] = node;
        }

        for(auto pair : dectabs){
            shared_ptr<array<shared_ptr<SymNode>,256>> tab = pair.second;
            shared_ptr<SymNode> cur = (*tab)[0];
            for(int i = 1; i <= 255; i++){
                if((*tab)[i] == dummy)
                    (*tab)[i] = cur;
                else
                    cur = (*tab)[i];
            }
        }
        #endif
    }

    return *this;
}

void HuffCoder::makeHuffCode(shared_ptr<SymNode> node, uint64_t bits, uint64_t code)
{
    if(node != nullptr){
        if(bits > maxLen)
            maxLen = bits;
        node->bits = bits;
        node->code = code;
        makeHuffCode(node->left, bits+1, code << 1 );
        makeHuffCode(node->right, bits+1, (code << 1) | 1);
    }
}

HuffCoder& HuffCoder::encode(uint64_t symbol, Bitchain &bc, bool dep = true)
{
    if(dep){
        shared_ptr<SymNode> node = symtab[symbol];
        bc.write(node->code, node->bits);
    }
    return *this;
}

HuffCoder& HuffCoder::getVLC(uint64_t symbol, uint64_t &code, uint64_t &bits)
{
    shared_ptr<SymNode> node = symtab[symbol];
    code = node->code;
    bits = node->bits;
    return *this;
}

HuffCoder& HuffCoder::decode(uint64_t &symbol, Bitchain &bc, bool dep = true)
{
    if(dep){
        uint64_t value, err;
        bc.getbits(maxLen, value, err);
        shared_ptr<SymNode> node = nullptr;
        #ifdef ONE_DECODE_TAB
        node = dectab[value];
        #else
        uint64_t bits_ofs = 0;
        uint64_t tab_idx, idx, code_idx;
        while(node == nullptr){
            tab_idx = value >> (maxLen - bits_ofs);
            idx = (bits_ofs << 56) | tab_idx;
            code_idx = (value >> (maxLen - (bits_ofs+8))) & 0xFF;
            node = (*dectabs[idx])[code_idx];
            bits_ofs += 8;
        }
        #endif
        bc.skipbits(node->bits, err);
        symbol = node->symbol;
    }
    return *this;
}

HuffCoder& HuffCoder::getSymbol(uint64_t &symbol, uint64_t code, uint64_t &bits)
{
    if(code < dectab.size()){
        shared_ptr<SymNode> node = dectab[code];
        symbol = node->symbol;
        bits = node->bits;
    }
    return *this;
}

#ifdef HC_TEST
#include <stdlib.h>
#include <time.h>

#include <vector>

int main(void)
{
    printf("HuffCoder TEST\n");
    srand(time(NULL));
    int numSym = rand()&0xFF;
    if(numSym < 2) numSym += 0x80;
    printf("total %d symbols\n", numSym);

    HuffCoder hc(numSym);

    vector<shared_ptr<SymNode>> symlist;
    for(int i = 0; i < numSym; i++){
        //printf("%d\n", i);
        shared_ptr<SymNode> node = make_shared<SymNode>(i, rand()&0xFFFF);
        symlist.push_back(node);
        hc.insert(node);
    }
    printf("maximum length of code:%d\n", hc.getMaxCodeLen());

    for(int i = 0; i < numSym; i++){
        shared_ptr<SymNode> node = symlist[i];
        printf("sym:%lu, freq:%lu, code:%016lX, bits:%lu\n", node->symbol, node->kval, node->code, node->bits);
    }

#define TEST_SIZE   1024
    uint64_t test_sym[TEST_SIZE];
    {
        Bitchain bc("test.bin", false);
        for(int i = 0; i < TEST_SIZE; i++){
            test_sym[i] = rand() % numSym;
            hc.encode(test_sym[i], bc);
        }
        bc.write(0, hc.getMaxCodeLen());
    }

    {
        Bitchain bc("test.bin", true);
        for(int i = 0; i < TEST_SIZE; i++){
            uint64_t sym;
            hc.decode(sym, bc);
            if(sym != test_sym[i])
                printf("%d: d:%lu v:%lu\n", i, sym, test_sym[i]);
        }
    }
}

#endif
