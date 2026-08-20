
from abc import ABC, abstractmethod


# TODO: whose responsibility is it to check the source? (e.g. FileSource with not solver log)


class Source(ABC):
    ''' Computative SAT solver events abstract source '''
    
    @abstractmethod
    def get_record(self) -> str: pass  # TODO: add concrete exception list

class FileSource(Source):
    ''' Computative SAT solver events from file '''
    
    def __init__(self, filename: str):
        self.filename = filename
        self.file = open(filename, 'r')
    
    def __del__(self):
        self.file.close()
    
    def get_record(self) -> str:
        
        record = self.file.readline().strip()
        if not record:
            raise EOFError('source exhausted')
        
        return record 
    
# TODO(future): class ProcessSource(Source): pass
