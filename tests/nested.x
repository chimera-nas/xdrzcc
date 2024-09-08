struct MyInner1 {
    unsigned int value;
};

struct MyInner2 {
    unsigned int value;
};

struct MyMsg {
    unsigned int    value;
    MyInner1        inner1;
    MyInner2        inner2;
};
