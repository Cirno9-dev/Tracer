# Tracer

The tracer implemented using intel pin. Tracer keeps track of heap block requests, releases, reads and writes, and program execution traces.

---

## Usage

**download**

```bash
git clone https://github.com/Cirno9-dev/Tracer.git
```

**build & run**

32 Architecture:

```bash
cd Tracer
PIN_ROOT=<YOUR PIN ROOT> make Target=ia32
pin -t ./obj-ia32/Tracer.so -- /bin/true
```

64 Architecture:

```bash
cd Tracer
PIN_ROOT=<YOUR PIN ROOT> make Target=intel64
pin -t ./obj-intel64/Tracer.so -- /bin/true
```

**result**

output/info.log

```
fileName: /usr/bin/true
loadedAddress: 0x7f249023d000
codeAddress: 0x56193c649000
insCount: 99
readCount: 41522
writeCount: 15450
mallocCount: 0
callocCount: 0
freeCount: 0
```

output/trace.log

```
 <ID>   <Address>: <instruction>
```

output/backtrace.log

```
<ID>	[<Backtrace Address>]
```

output/memoryTrace.log

```
<ID>    <OP ID> <Address> R/W <TargetAddress> <Size>
```

output/heapTrace.log

```
<ID>    <OP ID> <FunName>[<ARG>] <RETURN>
```

## Analysis

Analyze the program for possible vulnerabilities based on the output of Tracer.

```bash
python3 ./script/Analysis.py -d ./output
```

**output**

```bash
[!] <Address> Out Of Bound!
<Heap Address> <size>
offset: <offset>
<op>
[+] backtrace:
<backtrace>

[!] <Address> Use After Free!
<op>
[+] backtrace:
<backtrace>

[!] <Address> Double Free!
<op>
[+] backtrace:
<backtrace>
```

## Example

CVE-2018-18557

https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2018-18557

**Tracer**

```
pin -t ../Tracer/obj-intel64/Tracer.so -- ./poc ./image.tif
```

```
===============================================
This application is instrumented by Tracer
Info File: ./output/info.log
Trace File: ./output/trace.log
Backtrace File: ./output/backtrace.log
Memory Trace File: ./output/memoryTrace.log
Heap Trace File: ./output/heapTrace.log
===============================================
TIFFReadDirectoryCheckOrder: Warning, Invalid TIFF directory; tags are not sorted in ascending order.
it will crash,because heap space has been overflow:

free(): invalid next size (normal)

===============================================
Tracer analysis results: 
FileName: <PATH>/tiff/poc
Loaded address: 0x7f6341f49000
Code address: 0x55b30079b000
Number of instructions: 33269
Number of read memory instructions: 381781
Number of write memory instructions: 314752
Number of malloc: 31
Number of calloc: 0
Number of free: 25
===============================================
[1]    147880 IOT instruction (core dumped)  pin -t ../../Tracer/obj-intel64/Tracer.so -- ./poc ./image.tif

```

**Analysis**

```
[!] 0x55b3020736dc Out Of Bound!
0x55b302073290 1104
offset: 1100
156	41	0x7f634139f16b W 0x55b3020736dc 8
[+] backtrace:
0x00000000000105b0: TIFFClientOpen 于 /root/tiff/tiff-4.0.9/libtiff/tif_open.c:125
0x0000000000015735: TIFFFdOpen 于 /root/tiff/tiff-4.0.9/libtiff/tif_unix.c:216
0x000000000001580d: TIFFOpen 于 /root/tiff/tiff-4.0.9/libtiff/tif_unix.c:251
0x0000000000003777: main 于 <PATH>/tiff/poc.c:12
0x000029b040a8ed90: ?? ??:0
0x000029b040a8ee40: ?? ??:0
0x0000000000003655: _start 于 ??:?

[!] 0x55b302073798 Out Of Bound!
0x55b302073700 176
offset: 152
579	126	0x7f63413a0a59 W 0x55b302073798 32
[+] backtrace:
0x00000000000092b9: TIFFFetchDirectory 于 /root/tiff/tiff-4.0.9/libtiff/tif_dirread.c:4774
0x000000000000e7d4: TIFFReadDirectory 于 /root/tiff/tiff-4.0.9/libtiff/tif_dirread.c:3532
0x0000000000010aea: TIFFClientOpen 于 /root/tiff/tiff-4.0.9/libtiff/tif_open.c:466
0x0000000000015735: TIFFFdOpen 于 /root/tiff/tiff-4.0.9/libtiff/tif_unix.c:216
0x000000000001580d: TIFFOpen 于 /root/tiff/tiff-4.0.9/libtiff/tif_unix.c:251
0x0000000000003777: main 于 <PATH>/tiff/poc.c:12
0x000029b040a8ed90: ?? ??:0
0x000029b040a8ee40: ?? ??:0
0x0000000000003655: _start 于 ??:?

[!] 0x55b3020737ac Out Of Bound!
0x55b302073700 176
offset: 172
895	257	0x55b3007a3da5 R 0x55b3020737ac 12
[+] backtrace:
0x0000000000008d9d: TIFFFetchDirectory 于 /root/tiff/tiff-4.0.9/libtiff/tif_dirread.c:4817
0x000000000000e7d4: TIFFReadDirectory 于 /root/tiff/tiff-4.0.9/libtiff/tif_dirread.c:3532
0x0000000000010aea: TIFFClientOpen 于 /root/tiff/tiff-4.0.9/libtiff/tif_open.c:466
0x0000000000015735: TIFFFdOpen 于 /root/tiff/tiff-4.0.9/libtiff/tif_unix.c:216
0x000000000001580d: TIFFOpen 于 /root/tiff/tiff-4.0.9/libtiff/tif_unix.c:251
0x0000000000003777: main 于 <PATH>/tiff/poc.c:12
0x000029b040a8ed90: ?? ??:0
0x000029b040a8ee40: ?? ??:0
0x0000000000003655: _start 于 ??:?

[!] 0x55b302073e80 Out Of Bound!
0x55b302073e40 2864
offset: 64
33215	684343	0x7f63413a0c4a W 0x55b302073e80 5991
[+] backtrace:
0x000000000002222e: JBIGDecode 于 /root/tiff/tiff-4.0.9/libtiff/tif_jbig.c:102
0x000000000001227d: TIFFReadEncodedStrip 于 /root/tiff/tiff-4.0.9/libtiff/tif_read.c:539
0x00000000000037bd: main 于 <PATH>/tiff/poc.c:18 (discriminator 3)
0x000029b040a8ed90: ?? ??:0
0x000029b040a8ee40: ?? ??:0
0x0000000000003655: _start 于 ??:?

[!] 0x55b302077740 Out Of Bound!
0x55b302077700 6048
offset: 64
33215	684344	0x7f63413a0c4a R 0x55b302077740 5991
[+] backtrace:
0x000000000002222e: JBIGDecode 于 /root/tiff/tiff-4.0.9/libtiff/tif_jbig.c:102
0x000000000001227d: TIFFReadEncodedStrip 于 /root/tiff/tiff-4.0.9/libtiff/tif_read.c:539
0x00000000000037bd: main 于 <PATH>/tiff/poc.c:18 (discriminator 3)
0x000029b040a8ed90: ?? ??:0
0x000029b040a8ee40: ?? ??:0
0x0000000000003655: _start 于 ??:?
```

Of course there are some false positives.
