def timefromString(value):
    if len(value) == 19 :
        time = Time()
        time.yyyy = int(value[ :4])
        time.mm = int(value[5 : 7])
        time.md = int(value[8 : 10])
        time.hh = int(value[11 : 13])
        time.mm = int(value[14 : 16])
        time.ss = int(value[17 : 19])
            
        time.yyyy -= 1900
        time.md -= 1
            
        return True
    return False    

class Time:
    def __init__(self):
        self.yyyy = None
        self.mm = None
        self.md = None
        
        self.hh = None
        self.mm = None
        self.ss = None
