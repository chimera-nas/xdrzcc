
enum Options {
    OPTION1 = 1,
    OPTION2 = 2,
    OPTION3 = 3
};

struct MyOption1 {
    unsigned int value;
};

struct MyOption2 {
    string value;
};

union MyMsg switch (Options opt) {
 case OPTION1:
    MyOption1 option1;
 case OPTION2:
    MyOption2  option2;
 default:
    void;
};

