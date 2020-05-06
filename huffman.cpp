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
}

#endif
