from lib import *
import argparse
from pathlib import Path

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
        heapInfo[address] = heapInfo.get(address, [])
        heapInfo[address].append([traceId, size])
        
    heapInfo = sorted(heapInfo.items(), key=lambda v: v[0])
    heapInfo = dict(heapInfo)
    return heapInfo

def processMemoryTrace(memoryTrace: MemoryTrace, heapTrace: HeapTrace):
    heapInfo = processHeapTrace(heapTrace)
    max_base_address = list(heapInfo.keys())[-1]
    max_address = max_base_address + max(heapInfo[max_base_address], key=lambda v: v[1])[1]
    min_address = list(heapInfo.keys())[0]
    
    memoryInfo = [memoryTrace[0]]
    for memoryOp in memoryTrace[1:]:
        targetAddress = memoryOp["targetAddress"]
        if targetAddress > max_address or targetAddress < min_address:
            continue
        
        if memoryOp["op"] == memoryInfo[-1]["op"] and targetAddress == (memoryInfo[-1]["targetAddress"] + memoryInfo[-1]["size"]):
            memoryInfo[-1]["size"] += memoryOp["size"]
        else:
            memoryInfo.append(memoryOp)
    return memoryInfo

def checkVulnerability(trace: dict):    
    memoryInfo = {}
    heapInfo = {}
    
    for op in trace:
        if type(op) == HeapOp:
            if op["func"] == "free":
                address = op["args"][0]
                if address not in heapInfo:
                    print(f"[-] {hex(address)} not malloc!")
                    exit(0)
                size = heapInfo[address]
                for i in range(size // 16):
                    if memoryInfo[address - 0x10 + 0x10 * i] == [0] * 16:
                        print(f"[!] {hex(address)} double free!")
                        print(op)
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
                heapInfo[address] = size
                for i in range(size // 16):
                    memoryInfo[address - 0x10 + 0x10 * i] = [1] * 16
        elif type(op) == MemoryOp:
            address = op["targetAddress"]
            size = op["size"]
            if (address - address % 16) not in memoryInfo:
                continue
            
            for baseAddress in heapInfo:
                targetSize = heapInfo[baseAddress]
                if baseAddress <= address < baseAddress + targetSize:
                    if (baseAddress + targetSize - address) < size:
                        print(f"[!] {hex(address)} out of bound")
                        print(op)
                        print()
                        pass
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
    
    memoryTrace = MemoryTrace(memoryFile)
    heapTrace = HeapTrace(heapFile)

    minMemoryTrace = processMemoryTrace(memoryTrace, heapTrace)
    allTrace = heapTrace + minMemoryTrace
    allTrace.sort(key=lambda v: v["id"])
    
    checkVulnerability(allTrace)
    
    # test(infoFile, traceFile, memoryFile, heapFile)
        
def test(infoFile, traceFile, memoryFile, heapFile):
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
        
def testCalculateSize():
    assert calculateSize(0x10) == 0x20
    assert calculateSize(0x8) == 0x20
    assert calculateSize(0x18) == 0x20
    assert calculateSize(0x19) == 0x30

if __name__ == "__main__":
    main()
