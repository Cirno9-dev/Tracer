
infoFile = "./output/info.log"
traceFile = "./output/trace.log"
memoryFile = "./output/memoryTrace.log"
heapFile = "./output/heapTrace.log"

class Info:
    def __init__(self, filename: str) -> None:
        self._parseFromFile(filename)
        
    def _parseFromFile(self, filename: str) -> None:
        with open(filename) as f:
            line = f.readline()
            while line:
                exec("self."+line.replace(": ", " = ").strip())
                line = f.readline()
    
    def __str__(self) -> str:
        return self.__repr__()
    
    def __repr__(self) -> str:
        info = ""
        for (name, value) in vars(self).items():
            info += f"{name}: {value}\n"
        return info.strip()
    
class Instruction(dict):
    def __init__(self, id: int, address: int, ins: str) -> None:
        super().__init__({
            "id": id,
            "address": address,
            "ins": ins
        })
        
    def __str__(self) -> str:
        return self.__repr__()
    
    def __repr__(self) -> str:
        id = self["id"]
        address = self["address"]
        ins = self["ins"]
        return f"{id}\t{hex(address)}: {ins}"

class Trace(list):
    def __init__(self, filename) -> None:
        super().__init__([])
        self._parseFromFile(filename)
    
    def _parseFromFile(self, filename: str) -> None:
        with open(filename) as f:
            line = f.readline()
            while line:
                line = line.strip()
                id = line.split("\t")[0]
                address = int(line.split("\t")[1].split(": ")[0], 16)
                ins = line.split("\t")[1].split(": ")[1]
                self.append(Instruction(id, address, ins))
                line = f.readline()
                
    def __str__(self) -> str:
        return self.__repr__()
    
    def __repr__(self) -> str:
        trace = ""
        for ins in self:
            trace += str(ins) + "\n"
        return trace.strip()
        
def main() -> None:
    print("Analysis script for Tarcer output!")
    print("\nInfo:")
    info = Info(infoFile)
    print(info)
    print("\nTrace:")
    trace = Trace(traceFile)
    for ins in trace[:10]:
        print(ins)

if __name__ == "__main__":
    main()
