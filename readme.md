
An implementation of Byte Pair Encoding:
https://en.wikipedia.org/wiki/Byte_pair_encoding

In brief:
Input is divided into blocks of the largest valid size that leaves NUMPASSES byte values unused. In each block, the most frequent pairs of bytes are replaced with bytes that do not occur in that block.
To decode, the bytes are replaced with the respective pairs, the replacements being done in reverse order.

Two basic approaches are implemented: the more straightforward one just provides a full replacement table for each block, the other uses a byte pair table for the full input and only a byte key table for each block.

This algorithm is good at reducing long runs to a few replacements and has a few interesting properties that make it useful for embedded systems and hardware decoding. Decoding requires only a table of byte pairs and substitution values, totaling 3 bytes for each pass: a 32 pass decoder needs only 96 B for the substitution tables. More, each decoding stage only needs 3 bytes to describe the substitution and at most one byte of input for every byte of output, making it trivial to pipeline. A pipelined FPGA decoder can produce one decoded byte every clock cycle, with latency proportional to the number of stages.

