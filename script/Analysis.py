from lib import *
import argparse
from pathlib import Path
import os

def calculateSize(size):
    size = size - 8
    size = max(0x20, size+0x10)
    size = size + 0xf
    size = size - (size % 0x10)
    return size

def processHeapTrace(heapTrace: HeapTrace) -> dict:
    heapInfo = {}
    
    for heapOp in heapTrace:
        if heapOp["func"] == "malloc":
            size = heapOp["args"][0]
        elif heapOp["func"] == "calloc":
            size = heapOp["args"][0] * heapOp["args"][1]
        
        traceId = heapOp["id"]
        address = heapOp["retAddress"]
        size = calculateSize(size)
        heapInfo[address - 0x10] = heapInfo.get(address, [])
        heapInfo[address - 0x10].append([traceId, size])
        
    heapInfo = sorted(heapInfo.items(), key=lambda v: v[0])
    heapInfo = dict(heapInfo)
    return heapInfo

def processMemoryTrace(memoryTrace: MemoryTrace, heapTrace: HeapTrace):
    heapInfo = processHeapTrace(heapTrace)
    max_base_address = list(heapInfo.keys())[-1]
    max_address = max_base_address + max(heapInfo[max_base_address], key=lambda v: v[1])[1]
    min_address = list(heapInfo.keys())[0]
    
    writeInfo = []
    readInfo = []
    for memoryOp in memoryTrace[1:]:
        targetAddress = memoryOp["targetAddress"]
        if targetAddress > max_address or targetAddress < min_address:
            continue
        
        if memoryOp["op"] == "W":
            if len(writeInfo) == 0:
                writeInfo.append(memoryOp)
                continue
            if targetAddress == (writeInfo[-1]["targetAddress"] + writeInfo[-1]["size"]):
                writeInfo[-1]["size"] += memoryOp["size"]
            else:
                writeInfo.append(memoryOp)
        if memoryOp["op"] == "R":
            if len(readInfo) == 0:
                readInfo.append(memoryOp)
                continue
            if targetAddress == (readInfo[-1]["targetAddress"] + readInfo[-1]["size"]):
                readInfo[-1]["size"] += memoryOp["size"]
            else:
                readInfo.append(memoryOp)
    
    memoryInfo = writeInfo + readInfo
    memoryInfo.sort(key=lambda v: v["opId"])
    return memoryInfo

def getBacktraceStr(info: Info, trace: Trace, backtrace: Backtrace, op: MemoryOp | HeapOp):
    baseAddress = info.codeAddress
    bin = info.fileName
    
    id = op["id"] - 1
    if trace[id]["ins"].startswith("jmp qword ptr [rip+"):
        id = id - 1
    
    btAddress = backtrace[id]
    for address in btAddress:
        offset = address - baseAddress
        output = os.popen(f"addr2line -a {hex(offset)} -e {bin} -f -C -i -p").readlines()
        print("".join(output), end="")

def checkVulnerability(info: Info, trace: Trace, backtrace: Backtrace, allTrace: dict):    
    memoryInfo = {}
    heapInfo = {}
    lastId = 0
    
    for op in allTrace:
        if type(op) == HeapOp:
            lastId = op["id"]
            if op["func"] == "free":
                address = op["args"][0]
                if (address - 0x10) not in heapInfo:
                    print(f"[-] {hex(address)} not malloc!")
                    print(op)
                    exit(0)
                size = heapInfo[address - 0x10]
                for i in range(size // 16):
                    if memoryInfo[address - 0x10 + 0x10 * i] == [0] * 16:
                        print(f"[!] {hex(address)} Double Free!")
                        print(op)
                        print("[+] backtrace:")
                        getBacktraceStr(info, trace, backtrace, op)
                        print()
                        break
                    memoryInfo[address - 0x10 + 0x10 * i] = [0] * 16
            else:
                if op["func"] == "malloc":
                    size = op["args"][0]
                elif op["func"] == "calloc":
                    size = op["args"][0] * op["args"][1]
                    
                address = op["retAddress"]
                size = calculateSize(size)
                heapInfo[address - 0x10] = size
                for i in range(size // 16):
                    memoryInfo[address - 0x10 + 0x10 * i] = [1] * 16
        elif type(op) == MemoryOp:
            if op["id"] == lastId:
                continue
            
            address = op["targetAddress"]
            size = op["size"]
            if (address - address % 16) not in memoryInfo:
                continue
            
            if memoryInfo[address - address % 16] == [0] * 16:
                print(f"[!] {hex(address)} Use After Free!")
                print(op)
                print("[+] backtrace:")
                getBacktraceStr(info, trace, backtrace, op)
                print()
                continue
            
            for baseAddress in heapInfo:
                targetSize = heapInfo[baseAddress]
                if baseAddress <= address < baseAddress + targetSize:
                    if (baseAddress + targetSize - address) < size:
                        print(f"[!] {hex(address)} Out Of Bound!")
                        print(hex(baseAddress), targetSize)
                        print(f"offset: {address - baseAddress}")
                        print(op)
                        print("[+] backtrace:")
                        getBacktraceStr(info, trace, backtrace, op)
                        print()
                    break

def main() -> None:
    parser = argparse.ArgumentParser(description="Analysis script for Tarcer output!")
    parser.add_argument("-d", "--dir", dest="dir", default="./output", type=str, help="output dir. Default: ./output")
    args = parser.parse_args()
    
    outputDir = Path(args.dir)
    if not outputDir.exists():
        print("[-] output dir does not exist!\n")
        parser.print_help()
        exit(0)
    
    infoFile = outputDir.joinpath("info.log")
    traceFile = outputDir.joinpath("trace.log")
    memoryFile = outputDir.joinpath("memoryTrace.log")
    heapFile = outputDir.joinpath("heapTrace.log")
    backtraceFile = outputDir.joinpath("backtrace.log")
    
    info = Info(infoFile)
    trace = Trace(traceFile)
    memoryTrace = MemoryTrace(memoryFile)
    heapTrace = HeapTrace(heapFile)
    backtrace = Backtrace(backtraceFile)

    minMemoryTrace = processMemoryTrace(memoryTrace, heapTrace)
    allTrace = heapTrace + minMemoryTrace
    allTrace.sort(key=lambda v: v["opId"])
    
    checkVulnerability(info, trace, backtrace, allTrace)
    
    # test(infoFile, traceFile, memoryFile, heapFile, backtraceFile)
        
def test(infoFile, traceFile, memoryFile, heapFile, backtraceFile):
    print("\nInfo:")
    info = Info(infoFile)
    print(info)
    
    print("\nTrace:")
    trace = Trace(traceFile)
    for ins in trace[:10]:
        print(ins)

    print("\nMemoryTrace:")
    memoryTrace = MemoryTrace(memoryFile)
    for memOp in memoryTrace[:10]:
        print(memOp)
        
    print("\nHeapTrace:")
    heapTrace = HeapTrace(heapFile)
    for heapOp in heapTrace[:10]:
        print(heapOp)
        
    print("\nBacktrace:")
    backtrace = Backtrace()
    print(backtrace)
        
def testCalculateSize():
    assert calculateSize(0x10) == 0x20
    assert calculateSize(0x8) == 0x20
    assert calculateSize(0x18) == 0x20
    assert calculateSize(0x19) == 0x30

if __name__ == "__main__":
    main()
