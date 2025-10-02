#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
using namespace std;

class JSONValidator {
private:
    enum State {
        START, OPEN_BRACE, EXPECT_PROP, AFTER_PROP, EXPECT_COLON,
        EXPECT_ARRAY, ARRAY_START, EXPECT_OBJ, OBJ_START, EXPECT_KEY,
        AFTER_KEY, EXPECT_COLON2, EXPECT_VALUE, AFTER_VALUE,
        EXPECT_COMMA_CLOSE, AFTER_OBJ, EXPECT_COMMA_ARRAY_CLOSE,
        AFTER_ARRAY, EXPECT_CLOSE, ACCEPT, REJECT
    };

    const char* input;
    int pos;
    State state;
    char buffer[64];
    int bufLen;

    inline char peek() {
        while (input[pos] && (input[pos] == ' ' || input[pos] == '\n' || 
               input[pos] == '\r' || input[pos] == '\t')) pos++;
        return input[pos];
    }

    inline char consume() {
        char c = peek();
        if (c) pos++;
        return c;
    }

    inline bool isAlpha(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    inline bool isAlphaNum(char c) {
        return isAlpha(c) || (c >= '0' && c <= '9');
    }

    bool readAlphaStr() {
        if (peek() != '"') return false;
        consume();
        if (!isAlpha(peek())) return false;
        
        bufLen = 0;
        buffer[bufLen++] = consume();
        while (isAlpha(peek()) && bufLen < 63) {
            buffer[bufLen++] = consume();
        }
        buffer[bufLen] = '\0';
        
        return peek() == '"' && (consume(), true);
    }

    bool readAlphaNumStr() {
        if (peek() != '"') return false;
        consume();
        if (!isAlpha(peek())) return false;
        
        bufLen = 0;
        buffer[bufLen++] = consume();
        while (isAlphaNum(peek()) && bufLen < 63) {
            buffer[bufLen++] = consume();
        }
        buffer[bufLen] = '\0';
        
        return peek() == '"' && (consume(), true);
    }

    inline bool match(const char* str) {
        for (int i = 0; str[i]; i++) {
            if (buffer[i] != str[i]) return false;
        }
        return buffer[bufLen] == '\0' && str[bufLen] == '\0';
    }

public:
    JSONValidator(const char* json) : input(json), pos(0), state(START) {}

    bool validate() {
        state = START;
        pos = 0;

        while (state != ACCEPT && state != REJECT) {
            char c = peek();
            
            switch (state) {
                case START:
                    state = (c == '{' && (consume(), true)) ? OPEN_BRACE : REJECT;
                    break;
                case OPEN_BRACE:
                    state = EXPECT_PROP;
                    break;
                case EXPECT_PROP:
                    state = (readAlphaStr() && match("employees")) ? AFTER_PROP : REJECT;
                    break;
                case AFTER_PROP:
                    state = EXPECT_COLON;
                    break;
                case EXPECT_COLON:
                    state = (c == ':' && (consume(), true)) ? EXPECT_ARRAY : REJECT;
                    break;
                case EXPECT_ARRAY:
                    state = (c == '[' && (consume(), true)) ? ARRAY_START : REJECT;
                    break;
                case ARRAY_START:
                    state = EXPECT_OBJ;
                    break;
                case EXPECT_OBJ:
                    if (c == '{') {
                        consume();
                        state = OBJ_START;
                    } else if (c == ']') {
                        consume();
                        state = AFTER_ARRAY;
                    } else {
                        state = REJECT;
                    }
                    break;
                case OBJ_START:
                    state = EXPECT_KEY;
                    break;
                case EXPECT_KEY:
                    state = (readAlphaNumStr() && (match("firstName") || match("lastName"))) 
                            ? AFTER_KEY : REJECT;
                    break;
                case AFTER_KEY:
                    state = EXPECT_COLON2;
                    break;
                case EXPECT_COLON2:
                    state = (c == ':' && (consume(), true)) ? EXPECT_VALUE : REJECT;
                    break;
                case EXPECT_VALUE:
                    state = readAlphaNumStr() ? AFTER_VALUE : REJECT;
                    break;
                case AFTER_VALUE:
                    state = EXPECT_COMMA_CLOSE;
                    break;
                case EXPECT_COMMA_CLOSE:
                    if (c == ',') {
                        consume();
                        state = EXPECT_KEY;
                    } else if (c == '}') {
                        consume();
                        state = AFTER_OBJ;
                    } else {
                        state = REJECT;
                    }
                    break;
                case AFTER_OBJ:
                    state = EXPECT_COMMA_ARRAY_CLOSE;
                    break;
                case EXPECT_COMMA_ARRAY_CLOSE:
                    if (c == ',') {
                        consume();
                        state = EXPECT_OBJ;
                    } else if (c == ']') {
                        consume();
                        state = AFTER_ARRAY;
                    } else {
                        state = REJECT;
                    }
                    break;
                case AFTER_ARRAY:
                    state = EXPECT_CLOSE;
                    break;
                case EXPECT_CLOSE:
                    state = (c == '}' && (consume(), !peek())) ? ACCEPT : REJECT;
                    break;
                default:
                    state = REJECT;
            }
        }

        return state == ACCEPT;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Uso: " << argv[0] << " <archivo.json>" << endl;
        return 1;
    }

    ifstream file(argv[1]);
    if (!file) {
        cerr << "Error: No se pudo abrir '" << argv[1] << "'" << endl;
        return 1;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string content = buffer.str();
    file.close();

    JSONValidator validator(content.c_str());
    
    if (validator.validate()) {
        cout << "VALIDO" << endl;
        return 0;
    } else {
        cout << "INVALIDO" << endl;
        return 1;
    }
}