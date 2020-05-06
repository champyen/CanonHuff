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
            root = node;
        }
        makeHuffCode(root, 0, 0);

        // TODO - multi-level decode table
        {
            heap = make_shared<MinHeap<SymNode>>(numSymbols);
            for(int i = 0; i < numSymbols; i++){
                shared_ptr<SymNode> decNode = make_shared<SymNode>(symtab[i]->symbol, symtab[i]->code << (maxLen - symtab[i]->bits));
                decNode->bits = symtab[i]->bits;
                heap->insert(decNode);
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
        shared_ptr<SymNode> node = dectab[value];
        bc.skipbits(node->bits, err);
        symbol = node->symbol;
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
        bc.write_align();
    }

    {
        Bitchain bc("test.bin", true);
        for(int i = 0; i < 128; i++){
            uint64_t sym;
            hc.decode(sym, bc);
            if(sym != test_sym[i])
                printf("%d: d:%lu v:%lu\n", i, sym, test_sym[i]);
        }
    }
}

#endif
