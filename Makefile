CXX=g++

huffman: huffman.o bitchain.o
	$(CXX) -o $@ $^

huffman.o: huffman.cpp huffman.h
	$(CXX) -c $< -I MinHeap -I bitchain -DHC_TEST

bitchain.o: bitchain/bitchain.cpp
	$(CXX) -c $< -I bitchain

clean:
	rm -f *.o *.bin huffman