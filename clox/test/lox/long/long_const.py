def main():
    print("var x = 0;")
    for i in range(1,1001):
        print(f"x = x + {i};")
    print("print x; // expect: 500500")

if __name__ == "__main__":
    main()