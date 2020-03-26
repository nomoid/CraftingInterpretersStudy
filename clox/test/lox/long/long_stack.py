def main():
    s = "print 0"
    for i in range(1,1001):
        s += f" + {i}"
    s += "; // expect: 500500"
    print(s)

if __name__ == "__main__":
    main()