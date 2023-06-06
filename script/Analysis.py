from lib import *
import argparse
from pathlib import Path

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
    
    test(infoFile, traceFile, memoryFile, heapFile)
        
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
    heapTrace = HeapTracer(heapFile)
    for heapOp in heapTrace[:10]:
        print(heapOp)

if __name__ == "__main__":
    main()
