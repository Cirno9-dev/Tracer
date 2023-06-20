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
[!] 0x5599f45bbe80 Out Of Bound!
0x5599f45bbe40 2872
offset: 64
33215	684343	0x7f93b29a0c4a W 0x5599f45bbe80 5991
[+] backtrace:
0x000000000002222e: JBIGDecode 于 /root/tiff/tiff-4.0.9/libtiff/tif_jbig.c:102
0x000000000001227d: TIFFReadEncodedStrip 于 /root/tiff/tiff-4.0.9/libtiff/tif_read.c:539
0x00000000000037bd: main 于 <PATH>/tiff/poc.c:18 (discriminator 3)
0x000029f9bfe90d90: ?? ??:0
0x000029f9bfe90e40: ?? ??:0
0x0000000000003655: _start 于 ??:?
```

Of course there may be some false positives.
