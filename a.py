with open("big.txt", "w") as f:
    for i in range(1,10000):
        print(i,file=f)