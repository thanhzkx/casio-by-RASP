#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define MAX_ITER 100
#define MAX 100
#define EPSILON 1e-4 // Sai so 10^-4

#define LCD_ADDR 0x27  // Dia chi I2C cua LCD, neu khong hoat dong thi 0x3F
#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE 0x04
#define ACTION_SOLVE '!'   // Ky hieu dac biet cho Solve
#define ACTION_RESET '@'   // Ky hieu dac biet cho Reset

#define SDA_PIN 8  // Chon SDA
#define SCL_PIN 9  // Chon SCL

const int rowPins[4] = {5, 6, 7, 26}; // GPIO cho hang
const int colPins[6] = {0, 1, 2, 3, 4, 24};  // GPIO cho cot

// Ma tran ky tu cua keypad
char keys[4][6] = {
    {'=','7','4','1','+','D'},
    {'0','8','5','2','-','A'},
    {'^','9','6','3','*','S'},
    {'x','.','(',')','/','O'}
};

// Bat dau giao tiep I2C
void i2c_start() {
    pinMode(SDA_PIN, OUTPUT);
    pinMode(SCL_PIN, OUTPUT);
    digitalWrite(SDA_PIN, LOW);
    delay(1);  // Thay the usleep(5)
    digitalWrite(SCL_PIN, LOW);
}

// Dung giao tiep I2C
void i2c_stop() {
    pinMode(SDA_PIN, OUTPUT);
    digitalWrite(SDA_PIN, LOW);
    digitalWrite(SCL_PIN, HIGH);
    delay(1);  // Thay the usleep(5)
    digitalWrite(SDA_PIN, HIGH);
}

// Ghi byte qua I2C
void i2c_write_byte(unsigned char data) {
    for (int i = 0; i < 8; i++) {
        digitalWrite(SDA_PIN, (data & 0x80) ? HIGH : LOW);
        delay(1);  // Thay the usleep(5)
        digitalWrite(SCL_PIN, HIGH);
        delay(1);  // Thay the usleep(5)
        digitalWrite(SCL_PIN, LOW);
        data <<= 1;
    }
    pinMode(SDA_PIN, INPUT); // Nhan ACK
    delay(1);  // Thay the usleep(5)
    digitalWrite(SCL_PIN, HIGH);
    delay(1);  // Thay the usleep(5)
    digitalWrite(SCL_PIN, LOW);
    pinMode(SDA_PIN, OUTPUT);
}

// Gui lenh den LCD
void lcd_send_cmd(char cmd) {
    char data_high = cmd & 0xF0;
    char data_low = (cmd << 4) & 0xF0;
    
    i2c_start();
    i2c_write_byte(LCD_ADDR << 1);
    i2c_write_byte(data_high | LCD_BACKLIGHT | LCD_ENABLE);
    i2c_write_byte(data_high | LCD_BACKLIGHT);
    i2c_write_byte(data_low | LCD_BACKLIGHT | LCD_ENABLE);
    i2c_write_byte(data_low | LCD_BACKLIGHT);
    i2c_stop();
    delay(1);  // Thay the usleep(500)
}

// Gui du lieu den LCD
void lcd_send_data(char data) {
    char data_high = (data & 0xF0) | 0x01;
    char data_low = ((data << 4) & 0xF0) | 0x01;
    
    i2c_start();
    i2c_write_byte(LCD_ADDR << 1);
    i2c_write_byte(data_high | LCD_BACKLIGHT | LCD_ENABLE);
    i2c_write_byte(data_high | LCD_BACKLIGHT);
    i2c_write_byte(data_low | LCD_BACKLIGHT | LCD_ENABLE);
    i2c_write_byte(data_low | LCD_BACKLIGHT);
    i2c_stop();
    delay(1);  // Thay the usleep(500)
}

// Khoi tao LCD
void lcd_init() {
    lcd_send_cmd(0x33); // Khoi dong LCD 4-bit mode
    lcd_send_cmd(0x32);
    lcd_send_cmd(0x28); // Chi thi 2 dong, 5x8 font
    lcd_send_cmd(0x0C); // Bat man hinh, tat con tro
    lcd_send_cmd(0x06); // Chi thi nhap tu dong tang
    lcd_send_cmd(0x01); // Xoa man hinh
    delay(5);
}

// Dat con tro cho LCD
void lcd_set_cursor(int row, int col) {
    int offsets[] = {0x80, 0xC0};
    lcd_send_cmd(offsets[row] + col);
}

// In chuoi len LCD
void lcd_print(const char *str) {
    while (*str) {
        lcd_send_data(*str++);
    }
}

// Khoi tao keypad
void keypad_init() {
    for (int i = 0; i < 4; i++) { // 4 hang
        pinMode(rowPins[i], OUTPUT);
        digitalWrite(rowPins[i], HIGH); // Dat mac dinh HIGH
    }
    for (int j = 0; j < 6; j++) { // 5 cot
        pinMode(colPins[j], INPUT);
        pullUpDnControl(colPins[j], PUD_UP); // Su dung pull-up thay vi pull-down
    }
}

// Quet ban phim va tra ve ky tu nhan
char keypad_get_key() {
    for (int row = 0; row < 4; row++) { 
        for (int i = 0; i < 4; i++) 
            digitalWrite(rowPins[i], HIGH); // Dat tat ca hang ve HIGH
        
        digitalWrite(rowPins[row], LOW); // Kich hoat hang can kiem tra
        
        for (int col = 0; col < 6; col++) { 
            if (digitalRead(colPins[col]) == LOW) { // Neu phat hien nhan phim
                delay(20); // Chong rung phim (debounce)
                if (digitalRead(colPins[col]) == LOW) { // Kiem tra lai
                    while (digitalRead(colPins[col]) == LOW); // Cho thi phat
                    digitalWrite(rowPins[row], HIGH); // Reset hang
                    return keys[row][col]; // Tra ve ky tu nhan
                }
            }
        }
    }
    return '\0'; // Khong co phim nao duoc nhan
}

// Chuyen doi ky tu thanh ky tu hop le
char convert_key_to_char(char key) {
    switch (key) {
        case '0': return '0';
        case '1': return '1';
        case '2': return '2';
        case '3': return '3';
        case '4': return '4';
        case '5': return '5';
        case '6': return '6';
        case '7': return '7';
        case '8': return '8';
        case '9': return '9';
        case '+': return '+';
        case '-': return '-';
        case '*': return '*';
        case '/': return '/';
        case '=': return '=';
        case '(': return '(';
        case ')': return ')';
        case '.': return '.';
        case '^': return '^';
        case 'x': return 'x';
        default: return '\0'; // Ky tu khong hop le
    }
}

// Lay chuoi tu keypad
void keypad_get_string(char *str, int maxLen) {
    int idx = 0;
    int col = 0;
    char displayBuffer[17] = {0}; // Luu dang hien thi

    lcd_set_cursor(1, 0);

    while (idx < maxLen - 1) {
        char key = keypad_get_key();

        if (key != '\0') {
            // Xu ly cac phim chuc nang dac biet
            if (key == 'D') { // Xoa 1 ky tu
                if (idx > 0) {
                    idx--;
                    col = col > 0 ? col - 1 : 0;
                    displayBuffer[col] = '\0';
                    str[idx] = '\0';

                    lcd_set_cursor(1, 0);
                    for (int i = 0; i < 16; i++) {
                        lcd_send_data(displayBuffer[i] ? displayBuffer[i] : ' ');
                    }
                    lcd_set_cursor(1, col);
                }
            } else if (key == 'A') { // Xoa tat ca
                idx = 0;
                col = 0;
                str[0] = '\0';
                memset(displayBuffer, 0, sizeof(displayBuffer));
                lcd_set_cursor(1, 0);
                for (int i = 0; i < 16; i++) lcd_send_data(' ');
                lcd_set_cursor(1, 0);
            } else if (key == 'S') { // Giai phuong trinh
                str[idx] = ACTION_SOLVE;
                str[idx + 1] = '\0';
                return;
            } else if (key == 'O') { // Reset ban nhap
                str[idx] = ACTION_RESET;
                str[idx + 1] = '\0';
                return;
            } else {
                // Them ky tu hop le
                char convertedKey = convert_key_to_char(key);
                if (convertedKey != '\0') {
                    if (col >= 16) {
                        for (int i = 0; i < 15; i++) {
                            displayBuffer[i] = displayBuffer[i + 1];
                        }
                        displayBuffer[15] = convertedKey;
                    } else {
                        displayBuffer[col++] = convertedKey;
                    }

                    str[idx++] = convertedKey;
                    str[idx] = '\0';

                    lcd_set_cursor(1, 0);
                    for (int i = 0; i < 16; i++) {
                        lcd_send_data(displayBuffer[i] ? displayBuffer[i] : ' ');
                    }

                    lcd_set_cursor(1, col < 16 ? col : 15);
                }
            }
        }

        delay(100); // Chong rung
    }

    str[idx] = '\0';
}

// Dinh nghia cac trang thai trong qua trinh phan tich bieu thuc
typedef enum { S_START, S_OPERAND, S_OPERATOR, S_OPEN, S_CLOSE, S_ERROR, S_END } state_t;

// Dinh nghia loai token trong bieu thuc
typedef enum {
    OPERAND,  // So hang
    OPERATOR, // Toan tu
    VARIABLE  // Bien (x)
} TokenType;

// Cau truc luu tru token
typedef struct {
    TokenType type;
    union {
        float operand;   // Gia tri so
        char operator_;  // Toan tu
        float variable;  // Bien x
    } value;
} Token;

// Kiem tra ky tu co phai la toan tu hay khong
int isOperator(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '^');
}

// Xac dinh do uu tien cua toan tu
int precedence(char op) {
    switch (op) {
        case '+':
        case '-': return 1;
        case '*':
        case '/': return 2;
        case '^': return 3;
        default: return 0;
    }
}

// Tach phuong trinh thanh hai phan trai va phai
void splitEquation(char *expr, char *left, char *right) {
    char *equalSign = strchr(expr, '=');
    if (equalSign) {
        *equalSign = '\0';
        strcpy(left, expr);
        strcpy(right, equalSign + 1);
    } else {
        strcpy(left, expr);
        right[0] = '0';
        right[1] = '\0';
    }
}

// Chuyen bieu thuc infix sang postfix
Token *infixToPostfix(char* myFunction) {
    state_t current_state = S_START;
    Token *output = (Token *)malloc(MAX * sizeof(Token));
    int outputIndex = 0;
    char stack[MAX]; // Stack luu toan tu
    int stackTop = -1;

    while (1) {
        switch (current_state) {
           case S_START:
                if (*myFunction == '-') {  
                    output[outputIndex].type = OPERAND;
                    output[outputIndex].value.operand = 0;  // Them so 0 truoc dau tru
                    outputIndex++;
                    current_state = S_OPERATOR;
                } else if (isdigit(*myFunction) || *myFunction == '.' || *myFunction == 'x') {
                    current_state = S_OPERAND;
                } else if (*myFunction == '(') {
                    current_state = S_OPEN;
                } else if (*myFunction == 0) {
                    current_state = S_END;
                } else {
                    current_state = S_ERROR;
                }
                break;

            case S_OPERAND:
                if (*myFunction == 'x') {
                    output[outputIndex].type = VARIABLE;
                    output[outputIndex].value.variable = 0;
                    outputIndex++;
                    myFunction++;  
                } else {
                    float operand = 0.0;
                    int decimal_flag = 0;
                    float decimal_divisor = 1.0;
                    while (isdigit(*myFunction) || *myFunction == '.') {
                        if (*myFunction == '.') {
                            decimal_flag = 1;
                        } else {
                            if (decimal_flag == 0) {
                                operand = operand * 10 + (*myFunction - '0');
                            } else {
                                decimal_divisor *= 10;
                                operand = operand + (*myFunction - '0') / decimal_divisor;
                            }
                        }
                        myFunction++;
                    }
                    output[outputIndex].type = OPERAND;
                    output[outputIndex].value.operand = operand;
                    outputIndex++;
                }

                if (isOperator(*myFunction) || *myFunction == '-') {
                    current_state = S_OPERATOR;
                } else if (*myFunction == ')') {
                    current_state = S_CLOSE;
                } else if (*myFunction == 0) {
                    current_state = S_END;
                } else {
                    current_state = S_ERROR;
                }
                break;

            case S_OPERATOR:
                while (stackTop >= 0 && isOperator(stack[stackTop]) &&
                       ((precedence(stack[stackTop]) > precedence(*myFunction)) ||
                        (precedence(stack[stackTop]) == precedence(*myFunction) && *myFunction != '^'))) {  
                    output[outputIndex].type = OPERATOR;
                    output[outputIndex].value.operator_ = stack[stackTop];
                    outputIndex++;
                    stackTop--;
                }
                stack[++stackTop] = *myFunction;
                myFunction++;
                current_state = S_START;
                break;

            case S_OPEN:
                stack[++stackTop] = *myFunction;
                myFunction++;
                if (isdigit(*myFunction) || *myFunction == 'x' || *myFunction == '(') {  
                    current_state = S_START;
                } else if (*myFunction == '-') {  
                    output[outputIndex].type = OPERAND;
                    output[outputIndex].value.operand = 0;
                    outputIndex++;
                    current_state = S_OPERATOR;
                } else {
                    current_state = S_ERROR;
                }
                break;

            case S_CLOSE:
                while (stackTop >= 0 && stack[stackTop] != '(') {
                    output[outputIndex].type = OPERATOR;
                    output[outputIndex].value.operator_ = stack[stackTop];
                    outputIndex++;
                    stackTop--;
                }
                if (stackTop >= 0) stackTop--; // Bat dau '(' khoi stack
                myFunction++;

                if (isOperator(*myFunction)) {
                    current_state = S_OPERATOR;
                } else if (*myFunction == ')') {
                    current_state = S_CLOSE; // Neu co ngoac dong tiep, xu ly tiep
                } else if (*myFunction == 0) {
                    current_state = S_END;
                } else {
                    current_state = S_ERROR; // Neu gap so hoac bien ngay sau ')', loi
                }
                break;

            case S_END:
                while (stackTop >= 0) {
                    output[outputIndex].type = OPERATOR;
                    output[outputIndex].value.operator_ = stack[stackTop];
                    outputIndex++;
                    stackTop--;
                }
                output[outputIndex].type = OPERATOR;
                output[outputIndex].value.operator_ = 'E'; // Ky hieu ket thuc
                outputIndex++;
                return output;

            case S_ERROR:
                printf("Loi nhap bieu thuc!!!\n");
                return NULL;
        }
    }
}

// Ham xu ly co so am va mu khong nguyen
float handlePower(float base, float exponent) {
    int integerPart = (int)exponent;
    float decimalPart = exponent - integerPart;

    if (base > 0) {
        // Neu co so duong, tinh binh thuong
        return pow(base, exponent);
    } 
    else if (base == 0) {
        // Neu co so bang 0, tra ve 0 neu mu duong
        return (exponent > 0) ? 0 : NAN; // Khong xac dinh voi mu am
    }
    // Neu co so am, xu ly phan nguyen va phan thap phan cua mu
    else {
        // Xu ly phan nguyen cua mu
        float result = pow(base, integerPart);

        if (decimalPart != 0) {
            int n = round(1.0 / decimalPart);  // N gia tri dinh phan mu co dang 1/n

            // Kiem tra can chan hay le
            if (n % 2 == 0) {
                return NAN;
            } else {
                float root = pow(fabs(base), decimalPart); // Tinh can
                result *= (base < 0 && n % 2 != 0) ? -root : root; // Nhan voi ket qua truoc do
            }
        }
        return result;
    }
}

// Tinh gia tri cua bieu thuc hau to
float evaluatePostfix(Token *postfix, float x_value) {
    float stack[MAX];
    int top = -1;
    for (int i = 0; postfix[i].type != OPERATOR || postfix[i].value.operator_ != 'E'; i++) {
        if (postfix[i].type == OPERAND) {
            stack[++top]= postfix[i].value.operand;
        } else if (postfix[i].type == VARIABLE) {
            stack[++top] = x_value; // Gan gia tri bien
        } else {
            float b = stack[top--]; // Lay hai toan hang
            float a = stack[top--];
            switch (postfix[i].value.operator_) {
                case '+': stack[++top] = a + b; break; // Phép c?ng
                case '-': stack[++top] = a - b; break; // Phép tr?
                case '*': stack[++top] = a * b; break; // Phép nhân
                case '/': stack[++top] = a / b; break; // Phép chia
                case '^': stack[++top] = handlePower(a, b); break; // Luy th?a
            }
        }
    }
    return stack[top]; // Tr? v? k?t qu?
}

// Phuong phap chia doi (Bisection)
float bisection(Token *postfixLeft, Token *postfixRight, float a, float b) {
    float fa = evaluatePostfix(postfixLeft, a) - evaluatePostfix(postfixRight, a);
    float fb = evaluatePostfix(postfixLeft, b) - evaluatePostfix(postfixRight, b);
    
    if (fabs(fa) < EPSILON) return a; // N?u fa g?n 0, tr? v? a
    if (fabs(fb) < EPSILON) return b; // N?u fb g?n 0, tr? v? b

    if (fa * fb > 0) {
        return NAN; // Tr? v? giá tr? l?i
    }

    for (int i = 0; i < MAX_ITER; i++) {
        float c = (a + b) / 2; // Tính trung di?m
        float fc = evaluatePostfix(postfixLeft, c) - evaluatePostfix(postfixRight, c);

        if (fabs(fc) < EPSILON)
            return c; // N?u fc g?n 0, tr? v? c

        if (fa * fc < 0) {
            b = c; // Ch?n n?a bên trái
            fb = fc;
        } else {
            a = c; // Ch?n n?a bên ph?i
            fa = fc;
        }
    }
    return (a + b) / 2; // Tr? v? giá tr? trung bình
}

// Phuong phap lai (Hybrid Method)
float hybridMethod(Token *postfixLeft, Token *postfixRight, float a, float b, float x0) {
    float x = x0;
    for (int iter = 0; iter < MAX_ITER; iter++) {
        float f_left = evaluatePostfix(postfixLeft, x);
        float f_right = evaluatePostfix(postfixRight, x);
        float fx = f_left - f_right;

        if (fabs(fx) < EPSILON) return x; // N?u fx g?n 0, tr? v? x

        // Tính d?o hàm x?p x?
        float dfx = ((evaluatePostfix(postfixLeft, x + EPSILON) - evaluatePostfix(postfixRight, x + EPSILON)) - fx) / EPSILON;
        
        if (fabs(dfx) < EPSILON) {
            return bisection(postfixLeft, postfixRight, a, b); // N?u dfx g?n 0, dùng bisection
        }
        
        float x_new = x - fx / dfx; // C?p nh?t giá tr? x m?i

        if (x_new < a || x_new > b) {
            return bisection(postfixLeft, postfixRight, a, b); // N?u ra ngoài kho?ng [a, b], dùng bisection
        }

        x = x_new;
    }
    return bisection(postfixLeft, postfixRight, a, b); // Tr? v? k?t qu? bisection
}

int main() {
    char leftExpr[MAX] = "", rightExpr[MAX] = "", str[MAX];
    Token *outputLeft = NULL;
    Token *outputRight = NULL;

    if (wiringPiSetup() == -1) {
        printf("Khong the khoi tao wiringPi\n");
        return 1;
    }

    lcd_init(); // Khoi tao LCD
    keypad_init(); // Khoi tao keypad

    while (1) {
        lcd_send_cmd(0x01); // Xoa man hinh
        lcd_set_cursor(0, 0);
        char *msg = "Nhap bieu thuc:"; // Thong bao nhap bieu thuc
        for (int i = 0; msg[i] != '\0'; i++) {
            lcd_send_data(msg[i]);
        }

        keypad_get_string(str, MAX); // Lay chuoi tu keypad

        if (str[0] == ACTION_RESET) { // Neu nhan Reset
            if (outputLeft) {
                free(outputLeft);
                outputLeft = NULL;
            }
            if (outputRight) {
                free(outputRight);
                outputRight = NULL;
            }
            continue; // Quay lai nhap tu dau
        }

        if (str[0] == ACTION_SOLVE || strchr(str, ACTION_SOLVE)) {
            char exprOnly[MAX];
            strcpy(exprOnly, str);

            // Bo ACTION_SOLVE ra khoi chuoi
            if (exprOnly[0] == ACTION_SOLVE) {
                memmove(exprOnly, exprOnly + 1, strlen(exprOnly));
            }

            // Loai bo ACTION_SOLVE trong chuoi
            int j = 0;
            for (int i = 0; exprOnly[i] != '\0'; i++) {
                if (exprOnly[i] != ACTION_SOLVE)
                    exprOnly[j++] = exprOnly[i];
            }
            exprOnly[j] = '\0';

            // Tach bieu thuc
            splitEquation(exprOnly, leftExpr, rightExpr);

            if (outputLeft) free(outputLeft);
            if (outputRight) free(outputRight);
            outputLeft = infixToPostfix(leftExpr);
            outputRight = infixToPostfix(rightExpr);

            if (!outputLeft || !outputRight) {
                lcd_send_cmd(0x01);
                lcd_set_cursor(0, 0);
                char *errMsg = "Bieu thuc sai!"; // Thong bao loi
                for (int i = 0; errMsg[i] != '\0'; i++) {
                    lcd_send_data(errMsg[i]);
                }
                delay(2000);
                continue;
            }

            lcd_send_cmd(0x01);
            lcd_set_cursor(0, 0);
            char *gptMsg = "GPT"; // Thong bao GPT
            for (int i = 0; gptMsg[i] != '\0'; i++) {
                lcd_send_data(gptMsg[i]);
            }
            double a = -50, b = 50, x0;
            bool found = false;
            clock_t start_time = clock(); // Bat dau do thoi gian
            double root = hybridMethod(outputLeft, outputRight, a, b, x0);
            clock_t end_time = clock(); // Ket thuc do thoi gian
            double elapsed_time_ns = (double)(end_time - start_time) * 1e9 / CLOCKS_PER_SEC; // Thoi gian ns	
            
            if (!isnan(root)) {
                lcd_set_cursor(0, 5);
                char resStr[16];
                snprintf(resStr, 16, "x=%.2f", root); // Hien thi ket qua
                for (int i = 0; resStr[i] != '\0'; i++) {
                    lcd_send_data(resStr[i]);
                }
                lcd_set_cursor(1, 0);
                snprintf(resStr, 16, "Time: %.2fns", elapsed_time_ns); // Hien thi thoi gian
                for (int i = 0; resStr[i] != '\0'; i++) {
                    lcd_send_data(resStr[i]);
                }

                found = true;
            } 
            if (!found) {
                double start = -1e6, end = 1e6, step = 1000;
                for (double a = start; a < end; a += step) {
                    double b = a + step;
                    double fa = evaluatePostfix(outputLeft, a) - evaluatePostfix(outputRight, a);
                    double fb = evaluatePostfix(outputLeft, b) - evaluatePostfix(outputRight, b);
                    if (isnan(fa) || isnan(fb) || isinf(fa) || isinf(fb))
                        continue;
                    if (fa * fb < 0) {
                        x0 = (a + b) / 2;
                        // Bat dau do thoi gian
                        clock_t start_time = clock();
                        root = hybridMethod(outputLeft, outputRight, a, b, x0);
                        // Ket thuc do thoi gian
                        clock_t end_time = clock();
                        double elapsed_time_ns = (end_time - start_time) * 1e9 / CLOCKS_PER_SEC; // Thoi gian ns

                        if (!isnan(root)) {
                            lcd_set_cursor(0, 5);
                            char resStr[16];
                            snprintf(resStr, 16, "x=%.2f", root); // Hien thi ket qua
                            for (int i = 0; resStr[i] != '\0'; i++) {
                                lcd_send_data(resStr[i]);
                            }
                            // Dong 2: hien thi thoi gian
                            lcd_set_cursor(1, 0);
                            snprintf(resStr, 16, "Time: %.2fms", (int)elapsed_time_ns);
                            for (int i = 0; resStr[i] != '\0'; i++) {
                                lcd_send_data(resStr[i]);
                            }
                            found = true;
                            break;
                        }
                    }
                }
            }

            // Chi nhan RESET moi quay lai nhap bieu thuc
            while (1) {
                keypad_get_string(str, MAX);
                if (str[0] == ACTION_RESET) {
                    if (outputLeft) {
                        free(outputLeft);
                        outputLeft = NULL;
                    }
                    if (outputRight) {
                        free(outputRight);
                        outputRight = NULL;
                    }
                    break; // Thoat vong lap ch? reset -> quay lai nhap bieu thuc
                }
            }
        }
    }
}
