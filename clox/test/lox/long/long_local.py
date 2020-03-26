def indent(s):
    return "    " + s

def main():
    print("{")
    print(indent("var v0 = 0;"))
    for i in range(1,1001):
        print(indent(f"var v{i} = v{i-1} + {i};"))
    print(indent("print v1000; // expect: 500500"))
    print("}")

if __name__ == "__main__":
    main()