import time
from pyyd import *

class Listener:
    def __init__(self):
        self.has_caughtup=False

    def login(self, error, maxorderref, ismonitor):
        print(f"login {error}, {maxorderref}, {ismonitor}")

    def caughtup(self):
        self.has_caughtup = True
    
    def marketdata(self,m):
        print(m)

def main():
    listener = Listener()
    ydapi = YDApi(listener, "001", "001", "ydClient.ini")
    r = ydapi.start()
    print(f"ydapi.start()={r}")
    while not listener.has_caughtup:
        time.sleep(0.2)
    print(f"ydapi caughtup")

    ydapi.subscribe("a2203")
    time.sleep(5)
    ydapi.unsubscribe("a2203")

if __name__ == "__main__":
    main()

