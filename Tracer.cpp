#include "pin.H"
#include <iostream>
#include <fstream>
#include <map>

using std::cerr;
using std::endl;
using std::string;
using std::map;

/* ================================================================== */
// Global variables
/* ================================================================== */

UINT64 insCount = 0;
UINT64 readCount = 0;
UINT64 writeCount = 0;

string bin;
VOID* loadedAddress;
VOID* codeAddress;
map<VOID*, string> disassembleCode;

string traceFile = "./output/trace.log";
std::ostream* trace = new std::ofstream(traceFile.c_str(), std::ios_base::out);
string memoryTraceFile = "./output/memoryTrace.log";
std::ostream* memoryTrace = new std::ofstream(memoryTraceFile.c_str(), std::ios_base::out);

/* ===================================================================== */
// Utilities
/* ===================================================================== */

INT32 Usage()
{
    cerr << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

VOID Image(IMG img, VOID* v)
{
    if (IMG_IsMainExecutable(img)) {
        bin = IMG_Name(img);
        loadedAddress = (VOID*)IMG_StartAddress(img);
        codeAddress = (VOID*)IMG_LowAddress(img);

        *trace << "fileName: " << bin << endl;
        *trace << "loadedAddress: " << loadedAddress << endl;
        *trace << "codeAddress: " << codeAddress << endl;
        *trace << "trace: " << endl;

        *memoryTrace << "fileName: " << bin << endl;
        *memoryTrace << "loadedAddress: " << loadedAddress << endl;
        *memoryTrace << "codeAddress: " << codeAddress << endl;
        *memoryTrace << "memoryTrace: " << endl;
    }
}

VOID RecordIns(VOID* address, string disassemble)
{
    insCount += 1;
    *trace << insCount << "\t" << address << ": " << disassembleCode[address] << endl;
}

VOID RecordMemRead(VOID* address, VOID* targetAddress, UINT32 size)
{
    readCount += 1;
    *memoryTrace << insCount << "\t" << address << " R " << targetAddress << " " << size << endl;
}

VOID RecordMemWrite(VOID* address, VOID* targetAddress, UINT32 size)
{
    writeCount += 1;
    *memoryTrace << insCount << "\t" << address << " W " << targetAddress << " " << size << endl;
}

VOID Trace(INS ins, VOID* v)
{
    IMG img = IMG_FindByAddress(INS_Address(ins));
    if (!IMG_Valid(img) || !IMG_IsMainExecutable(img)) {
        return;
    }

    VOID* address = (VOID*) INS_Address(ins);
    string insDisassemble = INS_Disassemble(ins);
    disassembleCode[address] = insDisassemble;

    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordIns, IARG_INST_PTR,  IARG_END);

    UINT32 memOperands = INS_MemoryOperandCount(ins);
    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
        // Read
        if (INS_MemoryOperandIsRead(ins, memOp)) {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, 
                (AFUNPTR)RecordMemRead, 
                IARG_INST_PTR, 
                IARG_MEMORYOP_EA, memOp,
                IARG_MEMORYREAD_SIZE,
                IARG_END);
        }
        // Write
        if (INS_MemoryOperandIsWritten(ins, memOp)) {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, 
                (AFUNPTR)RecordMemWrite, 
                IARG_INST_PTR, 
                IARG_MEMORYOP_EA, memOp,
                IARG_MEMORYWRITE_SIZE,
                IARG_END);
        }
    }
}

VOID Fini(INT32 code, VOID* v)
{
    cerr << "===============================================" << endl;
    cerr << "Tracer analysis results: " << endl;
    cerr << "FileName: " << bin << endl;
    cerr << "Loaded address: " << loadedAddress << endl;
    cerr << "Code address: " << codeAddress << endl;
    cerr << "Number of instructions: " << insCount << endl;
    cerr << "Number of read memory instructions: " << readCount << endl;
    cerr << "Number of write memory instructions: " << writeCount << endl;
    cerr << "===============================================" << endl;
}

int main(int argc, char* argv[])
{
    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid
    if (PIN_Init(argc, argv))
    {
        return Usage();
    }

    // Register function to be called when image load
     IMG_AddInstrumentFunction(Image, 0);

    // Register function to be called to instrument Instrumention
    INS_AddInstrumentFunction(Trace, 0);

    // Register function to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    cerr << "===============================================" << endl;
    cerr << "This application is instrumented by Tracer" << endl;
    cerr << "Trace File: " << traceFile << endl;
    cerr << "Memory Trace File: " << memoryTraceFile << endl;
    cerr << "===============================================" << endl;

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}
