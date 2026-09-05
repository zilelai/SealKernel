//COPYLEFT FROM ZILELAI/ZL PROJECTS 2026
//THIS FILE IS LICENSED BY GNU 3.0 LICENSE IN GITHUB

//UPDATE THE VERSION HERE!!!!!!!
char version [] = "SealKernel 5.9.2026";



//LIBRARIES
#include <iostream>
using namespace std;
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <float.h>
#include <sys/stat.h>
#include <unistd.h>
#include "math.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "compress.h"
#include "decompress.h"
#include <ctype.h>
#include "zlio.h"
#include <errno.h>
#include <termios.h>
#include <stdbool.h>
#include <cmath>
#include <fstream>
#include <ftw.h>



#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
    #define mkdir(path, ...) _mkdir(path)
    #define rmdir(dir) _rmdir(dir)
#else
    #include <sys/stat.h>
    #include <unistd.h>
    #include <dirent.h>
    inline int nvmkdir(const char* path, mode_t mode = 0777) {
        return ::mkdir(path, mode);
    }
    #if defined(mkdir)
        #undef mkdir
    #endif
    #define mkdir(...) nvmkdir(__VA_ARGS__)
#endif
#define MAX 10000
#define MIN 256

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define RESET   "\x1b[0m"


//ZLIO Implementations
void charoutput(char c) {
    write(STDOUT_FILENO, &c, 1);
}

void stroutput(const char *str, int *count) {
    if (!str) str = "(null)";
    while (*str) {
        charoutput(*str);
        if (count) (*count)++;
        str++;
    }
}

static void intoutput(int n, int *count) {
    char buf[32];
    int i = 0;

    if (n == 0) {
        charoutput('0');
        if (count) (*count)++;
        return;
    }

    long long num = n;
    if (num < 0) {
        charoutput('-');
        if (count) (*count)++;
        num = -num;
    }

    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }

    while (i > 0) {
        charoutput(buf[--i]);
        if (count) (*count)++;
    }
}

static void doubleoutput(double val, int *count) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%f", val);
    stroutput(buf, count);
}

int out(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int count = 0;

    while (*format) {
        if (*format == '%') {
            format++;

            if (*format == 'c') {
                char c = (char)va_arg(args, int);
                charoutput(c);
                count++;
            }
            else if (*format == 's') {
                char *s = va_arg(args, char *);
                stroutput(s, &count);
            }
            else if (*format == 'd' || *format == 'i') {
                int val = va_arg(args, int);
                intoutput(val, &count);
            }
            else if (*format == 'f') {
                double val = va_arg(args, double);
                doubleoutput(val, &count);
            }
            else if (*format == '%') {
                charoutput('%');
                count++;
            }
        } else {
            charoutput(*format);
            count++;
        }
        format++;
    }

    va_end(args);
    return count;
}

static void strw(const char *str) {
    if (str) {
        write(STDOUT_FILENO, str, strlen(str));
    }
}

static size_t rlen(char *buf, size_t max_len) {
    size_t i = 0;
    char c;
    while (i < max_len - 1) {
        ssize_t bytes = read(STDIN_FILENO, &c, 1);
        if (bytes <= 0 || c == '\n') break; 
        buf[i++] = c;
    }
    buf[i] = '\0';

    while (i > 0 && (buf[i - 1] == '\r' || isspace((unsigned char)buf[i - 1]))) {
        buf[--i] = '\0';
    }
    return i;
}

InputValue inraw(const char *prompt) {
    InputValue result;
    char buffer[256];

    strw(prompt);
    rlen(buffer, sizeof(buffer));

    if (buffer[0] == '\0') {
        result.type = INPUTSTR;
        result.data.stringvalue[0] = '\0';
        return result;
    }

    char *endptr;

    if (strchr(buffer, '.') != NULL) {
        double vald = strtod(buffer, &endptr);
        if (*endptr == '\0') {
            result.type = INPUTDOUBLE;
            result.data.doublevalue = vald;
            return result;
        }
    }

    long vall = strtol(buffer, &endptr, 10);
    if (*endptr == '\0') {
        result.type = INPUTINT;
        result.data.intvalue = (int)vall;
        return result;
    }

    double vald = strtod(buffer, &endptr);
    if (*endptr == '\0') {
        result.type = INPUTDOUBLE;
        result.data.doublevalue = vald;
        return result;
    }

    if (strcasecmp(buffer, "true") == 0 || strcmp(buffer, "1") == 0 || strcasecmp(buffer, "yes") == 0) {
        result.type = INPUTBOOL;
        result.data.boolvalue = true;
        return result;
    }
    if (strcasecmp(buffer, "false") == 0 || strcmp(buffer, "0") == 0 || strcasecmp(buffer, "no") == 0) {
        result.type = INPUTBOOL;
        result.data.boolvalue = false;
        return result;
    }

    if (strlen(buffer) > 1 && strpbrk(buffer, "+-*/") != NULL) {
        result.type = INPUTDOUBLE;
        result.data.doublevalue = math(buffer);
        return result;
    }

    result.type = INPUTSTR;
    strncpy(result.data.stringvalue, buffer, sizeof(result.data.stringvalue) - 1);
    result.data.stringvalue[sizeof(result.data.stringvalue) - 1] = '\0';

    return result;
}

void inint(const char *prompt, int *outval) {
    InputValue res = inraw(prompt);
    if (res.type == INPUTINT) *outval = res.data.intvalue;
    else if (res.type == INPUTDOUBLE) *outval = (int)res.data.doublevalue;
    else if (res.type == INPUTSTR) *outval = atoi(res.data.stringvalue);
    else *outval = 0;
}

void indouble(const char *prompt, double *outval) {
    InputValue res = inraw(prompt);
    if (res.type == INPUTDOUBLE) *outval = res.data.doublevalue;
    else if (res.type == INPUTINT) *outval = (double)res.data.intvalue;
    else if (res.type == INPUTSTR) *outval = atof(res.data.stringvalue);
    else *outval = 0.0;
}

void inbool(const char *prompt, bool *outval) {
    InputValue res = inraw(prompt);
    if (res.type == INPUTBOOL) *outval = res.data.boolvalue;
    else if (res.type == INPUTINT) *outval = res.data.intvalue != 0;
    else *outval = false;
}

void instr(const char *prompt, char *outval) {
    InputValue res = inraw(prompt);
    if (res.type == INPUTINT) {
        snprintf(outval, 256, "%d", res.data.intvalue);
    } else if (res.type == INPUTDOUBLE) {
        snprintf(outval, 256, "%f", res.data.doublevalue);
    } else if (res.type == INPUTBOOL) {
        strncpy(outval, res.data.boolvalue ? "true" : "false", 255);
        outval[255] = '\0';
    } else {
        strncpy(outval, res.data.stringvalue, 255);
        outval[255] = '\0';
    }
}


static double parse_expression(const char **str);

static double parse_factor(const char **str) {
    while (isspace((unsigned char)**str)) (*str)++;

    if (**str == '-') {
        (*str)++;
        return -parse_factor(str);
    }

    if (**str == '(') {
        (*str)++;
        double val = parse_expression(str);
        if (**str == ')') (*str)++;
        return val;
    }

    char *endptr;
    double val = strtod(*str, &endptr);
    *str = endptr;
    return val;
}

static double parse_term(const char **str) {
    double left = parse_factor(str);
    while (1) {
        while (isspace((unsigned char)**str)) (*str)++;
        char op = **str;
        if (op == '*' || op == '/') {
            (*str)++;
            double right = parse_factor(str);
            if (op == '*') left *= right;
            else if (op == '/') left = (right != 0.0) ? (left / right) : 0.0;
        } else {
            break;
        }
    }
    return left;
}

static double parse_expression(const char **str) {
    double left = parse_term(str);
    while (1) {
        while (isspace((unsigned char)**str)) (*str)++;
        char op = **str;
        if (op == '+' || op == '-') {
            (*str)++;
            double right = parse_term(str);
            if (op == '+') left += right;
            else if (op == '-') left -= right;
        } else {
            break;
        }
    }
    return left;
}

double math(const char *expr) {
    return parse_expression(&expr);
}


//tags to see if you are running in Linux or Windows or Other OS
#if defined(__linux__)
void tagged() {
    out("linux\n");
}
#elif defined(_WIN32) || defined(_WIN64)
void tagged() {
    out("windows\n");
}
#else
void tagged() {
    out("other OS\n");
}
#endif


//check filesize
void filesize(const char *label, const char *filename) {
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL) {
        perror(RED"errcode 3: file not provided\n" RESET);
        return; 
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    out("%s: %ld bytes\n", label, size);

    fclose(fp);
}

//package manager
int pkgdownload(const char *url) {
    const char *filename = strrchr(url, '/');
    if (filename) {
        filename++; 
    } else {
        filename = "downloadedlib.h";
    }

    

    char command[512];
    snprintf(command, sizeof(command), "curl -sL \"%s\" -o \"%s\"", url, filename);


    int result = system(command);

    if (result == 0) {
        out("installed %s\n", filename);
        return 0;
    } else {
        out(RED "errcode 4 : failed to install existing package\n" RESET);
        return 1;
    }
}

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;

    char *ptr = (char *)realloc(*(char **)userp, strlen(*(char **)userp) + realsize + 1);
    if(ptr == NULL) return 0; 
    
    *(char **)userp = ptr;
    
    strncat(*(char **)userp, (const char *)contents, realsize);
    return realsize;
}


//variables
char op [127];
double firstnum;
double secondnum;
double result;

char input_buf[50];
int loc = 0;
int notes_mode = 0;
char notes_name[64] = "notes.txt";
char notes_path[128];
int q = 1;
char w = 'A';
float e = 5.7293;
double r = 4.882823923923923823283;
long t = 832832832732;
int superior;
char users[128] = "seal";
int config;
char cwd[1024];
char memuser[128] = "seal";
char oripath[1024];

char cmd[20] = "";
char arg1[64] = "";
char arg2[64] = "";
char cursor = '$';

int cb(const char *fpth, const struct stat *sb, int typeflag, struct FTW *ftwbuf){
    int rmdir = remove(fpth);

    if(rmdir != 0){
        perror(fpth);
    }

    return rmdir;
}

int rmrf(const char *path){
    return nftw(path, cb, 64, FTW_DEPTH | FTW_PHYS);
}

//get where to put the file in
void get_current_path(int current_loc, const char* filename, char* out_path) {
    size_t max_len = 128;

    snprintf(out_path, max_len, "%s/%s", oripath, filename); 
}


//rock paper scissors game
int game(char player, char bot){
    if (player == bot){
        return -1;
    }

    if (player == 's' && bot == 'p'){
        return 0;
    }

    else if (player == 'p' && bot == 's'){
        return 1;
    }

    else if (player == 's' && bot == 'r'){
        return 1;
    }
    else if (player == 'r' && bot == 's'){
        return 0;
    }
    else if (player == 'r' && bot == 'p'){
        return 1;
    }
    else if (player == 'p' && bot == 'r'){
        return 0;
    }

    return -1;
}

//compressor implementation
int compressor(const char *inputfile, const char *outputfile)
{
    FILE *in = fopen(inputfile, "rb");
    if (!in)
    {
        perror(RED "errcode 5: file input failed\n" RESET);
        return 1;
    }

    FILE *out = fopen(outputfile, "wb");
    if (!out)
    {
        perror(RED "errcode 6: file output failed\n" RESET);
        fclose(in);
        return 1;
    }

    int prev = fgetc(in);
    if (prev == EOF)
    {
        fclose(in);
        fclose(out);
        return 0;
    }

    int count = 1;
    int curr;

    while ((curr = fgetc(in)) != EOF)
    {
        if (curr == prev)
        {
            count++;
        }
        else
        {
            fprintf(out, "%c%d", (char)prev, count);
            prev = curr;
            count = 1;
        }
    }

    fprintf(out, "%c%d", (char)prev, count);

    fclose(in);
    fclose(out);
    return 0;
}


//decompress implementation file
int decompress(const char *input_file, const char *output_file)
{
    FILE *in = fopen(input_file, "rb");
    if (!in)
    {
        perror(RED "errcode 5: file input failed\n" RESET);
        return 1;
    }

    FILE *out = fopen(output_file, "wb");
    if (!out)
    {
        perror(RED "errcode 6: file output failed\n" RESET);
        fclose(in);
        return 1;
    }

    int ch;
    while ((ch = fgetc(in)) != EOF)
    {
        int count = 0;
        int next_byte;


        while ((next_byte = fgetc(in)) != EOF && isdigit(next_byte))
        {
            count = count * 10 + (next_byte - '0');
        }


        if (count == 0)
        {
            count = 1;
        }

        for (int i = 0; i < count; i++)
        {
            fputc(ch, out);
        }

        if (next_byte != EOF)
        {
            ungetc(next_byte, in);
        }
    }

    fclose(in);
    fclose(out);
    return 0;
}

//first it checks where first to get the home location
void original_path(){
    if (getcwd(oripath, sizeof(oripath)) != NULL) {}
}



//commands
void process_system_command(char *input) {
    time_t currentTime;
    char content[256];
    FILE *file;

    

    input[strcspn(input, "\n")] = 0;
    if (strlen(input) == 0) return;



    //command variables
    
    int parsed_args = sscanf(input, "%s %s %s", cmd, arg1, arg2);



    //chmod
    if (strcmp(cmd, "chmod") == 0) {
        if (parsed_args < 3) {
            out(RED "errcode 7 : only 0 and 1 are allowed\n" RESET);
        } else if (strcmp(arg1, notes_name) != 0) {
            perror(RED "errcode 3: file not provided\n" RESET);
        } else {
            notes_mode = atoi(arg2);
            out("changed\n");
        }
    }


    //rm
    else if (strcmp(cmd, "rm") == 0) {
        if (parsed_args < 2) {
            out(RED "errcode 3: file not provided\n" RESET);
        } 
        
        else {
            if (strcmp(arg1, "tty1.cpp") == 0 || 
                strcmp(arg1, "tty2.cpp") == 0 || 
                strcmp(arg1, "qubabasdwiaisd.txt") == 0 || 
                strcmp(arg1, "compress.h") == 0 || 
                strcmp(arg1, "decompress.h") == 0 || 
                strcmp(arg1, "stb_image.h") == 0 || 
                strcmp(arg1, "zlio.h") == 0 || 
                strcmp(arg1, "root") == 0 || 
                strcmp(arg1, "text.text") == 0 || 
                strcmp(arg1, "seal.text") == 0 || 
                strcmp(arg1, "bootloader") == 0 || 
                strcmp(arg1, "tty1") == 0 || 
                strcmp(arg1, "tty2") == 0) {
                
                out(RED "errcode 24: access denied: doing such process is very dangerous\n" RESET);
            } 
            else {
                char target_path[128];
                get_current_path(loc, arg1, target_path);
    

                if (remove(target_path) == 0) {
                    if (strcmp(notes_name, arg1) == 0) {
                        strcpy(notes_name, "notes.txt");
                        notes_mode = 0;
                    }
                    out("changed\n");
                } else {

                    perror(RED "errcode 3: remove failed" RESET);
                }
            }
        }
    }


    //remve directory
    else if (strcmp(cmd, "rmdir") == 0) {
        if (parsed_args < 2) {
            out("errcode 3: file not provided\n");
        } 
        
        else {
            if (strcmp(arg1, "tty1.cpp") == 0 || 
                strcmp(arg1, "tty2.cpp") == 0 || 
                strcmp(arg1, "qubabasdwiaisd.txt") == 0 || 
                strcmp(arg1, "compress.h") == 0 || 
                strcmp(arg1, "decompress.h") == 0 || 
                strcmp(arg1, "stb_image.h") == 0 || 
                strcmp(arg1, "zlio.h") == 0 || 
                strcmp(arg1, "root") == 0 || 
                strcmp(arg1, "text.text") == 0 || 
                strcmp(arg1, "seal.text") == 0 || 
                strcmp(arg1, "bootloader") == 0 || 
                strcmp(arg1, "tty1") == 0 || 
                strcmp(arg1, "tty2") == 0) {
                
                out(RED "errcode 24: access denied: doing such process is very dangerous\n" RESET);
            } 
            else {
                char target_path[128];
                get_current_path(loc, arg1, target_path);
    

                if (rmrf(target_path) == 0) {
                    if (strcmp(notes_name, arg1) == 0) {
                        strcpy(notes_name, "notes.txt");
                        notes_mode = 0;
                    }
                    out("changed\n");
                } else {

                    perror(RED "errcode 3: remove failed" RESET);
                }
            }
        }
    }
    


    //mv
    else if (strcmp(cmd, "mv") == 0) {
        char o[128];
        char n[128];
        out("Key in your old file location: ");
        in("", o); 
        out("Key in your new file location: ");
        in("", n);
    
        if (rename(o, n) == 0) {
            out("File moved successfully.\n");
        } else {
            perror(RED "errcode 3: file not provided\n" RESET);
        }
    }


    //save

    else if (strcmp(cmd, "save") == 0) {
        if (parsed_args < 2) {
            out(RED "errcode 8 : url not provided\n" RESET);
        } else {
            if (strstr(arg1, ".seal") != NULL) {
                out("Script %s configuration saved successfully.\n", arg1);
            } else {
                out("Connecting to remote server...\n");
                out("Downloading resources from: %s\n", arg1);
                
                if (strstr(arg1, "codepad.app/pad/822052z5n") != NULL) {
                    FILE *dl = fopen("downloads/flag.txt", "w");
                    if (dl) {
                        fprintf(dl, "SEAL{c_strings_are_pointers_to_fun}\n");
                        fclose(dl);
                        out("Saved successfully to downloads/flag.txt\n");
                    }
                } else {
                    FILE *dl = fopen("downloads/index.html", "w");
                    if (dl) {
                        fprintf(dl, "\n<h1>SealOS Sandbox Landing</h1>\n");
                        fclose(dl);
                        out("Saved successfully to downloads/index.html\n");
                    }
                }
            }
        }
    }

    //exe-c
    else if (strcmp(cmd, "exe") == 0) {
        if (parsed_args >= 2) {
            strncpy(notes_name, arg1, sizeof(notes_name) - 1);
            notes_name[sizeof(notes_name) - 1] = '\0';
        } else {
            out(RED "errcode 3: file not provided\n" RESET);
            return; 
        }

        if (notes_mode == 0 && (strcmp(arg1, "code.txt") == 0 || strcmp(arg1, "code.c") == 0)) {
            out(RED "errcode 9 : permission denied \n" RESET);
        } else {
            char compile[512];
            snprintf(compile, sizeof(compile), "gcc %s -o main && ./main", arg1);
            system(compile);
            out("\n");
        }
    }

    //exe-c-math
    else if (strcmp(cmd, "exe-math") == 0) {
        if (parsed_args >= 2) {
            strncpy(notes_name, arg1, sizeof(notes_name) - 1);
            notes_name[sizeof(notes_name) - 1] = '\0';
        } else {
            out(RED "errcode 3: file not provided\n" RESET);
            return; 
        }

        if (notes_mode == 0 && (strcmp(arg1, "code.txt") == 0 || strcmp(arg1, "code.c") == 0)) {
            out(RED "errcode 9 : permission denied \n" RESET);
        } else {
            char compile[512];
            snprintf(compile, sizeof(compile), "gcc %s -o main -lm && ./main", arg1);
            system(compile);
            out("\n");
        }
    }


    //exe-cpp
    else if (strcmp(cmd, "exe-cpp") == 0) {
        if (parsed_args >= 2) {
            strncpy(notes_name, arg1, sizeof(notes_name) - 1);
            notes_name[sizeof(notes_name) - 1] = '\0';
        } else {
            out(RED "errcode 3: file not provided\n" RESET);
            return; 
        }

        if (notes_mode == 0 && (strcmp(arg1, "code.txt") == 0 || strcmp(arg1, "code.c") == 0)) {
            out(RED "errcode 9 : permission denied \n" RESET);
        } else {
            char compile[512];
            snprintf(compile, sizeof(compile), "g++ %s -o main && ./main", arg1);
            system(compile);
            out("\n");
        }
    }





    //exe-python
    else if (strcmp(cmd, "exe-python") == 0) {
        if (parsed_args >= 2) {
            strncpy(notes_name, arg1, sizeof(notes_name) - 1);
            notes_name[sizeof(notes_name) - 1] = '\0';
        } else {
            out(RED "errcode 3: file not provided\n" RESET);
            return; 
        }
        if (notes_mode == 0 && (strcmp(arg1, "code") == 0 || strcmp(arg1, "code.py") == 0)) {
            out(RED "errcode 9 : permission denied \n" RESET);
        } else {
            char compile[512];
            if (strstr(arg1, ".py") != NULL) {
                snprintf(compile, sizeof(compile), "python3 %s", arg1);
            } else {
                snprintf(compile, sizeof(compile), "python3 %s.py", arg1);
            }
            system(compile);
            out("\n");
        }
    }



    //exe-java
    else if (strcmp(cmd, "exe-java") == 0) {
        if (parsed_args >= 2) {
            strncpy(notes_name, arg1, sizeof(notes_name) - 1);
            notes_name[sizeof(notes_name) - 1] = '\0';
        } else {
            out(RED "errcode 3: file not provided\n" RESET);
            return; 
        }
        
        
        if (notes_mode == 0 && (strcmp(arg1, "code") == 0 || strcmp(arg1, "code.java") == 0)) {
            out(RED "errcode 9 : permission denied \n" RESET);
        } else {
            char compile[512];
           
            char class_name[256];
            strncpy(class_name, arg1, sizeof(class_name) - 1);
            class_name[sizeof(class_name) - 1] = '\0';

            char *ext = strstr(class_name, ".java");
            if (ext) *ext = '\0'; 

            snprintf(compile, sizeof(compile), "javac %s.java && java %s", class_name, class_name);
            system(compile);
            out("\n");
        }
    }



    //exe-asm x86-64
    else if (strcmp(cmd, "exe-asm") == 0) {
        if (parsed_args >= 2) {
            strncpy(notes_name, arg1, sizeof(notes_name) - 1);
            notes_name[sizeof(notes_name) - 1] = '\0';
        } else {
            out(RED "errcode 3: file not provided\n" RESET);
            return; 
        }
        
        if (notes_mode == 0 && (strcmp(arg1, "code") == 0 || strcmp(arg1, "code.asm") == 0)) {
            out(RED "errcode 9 : permission denied \n" RESET);
        } else {
            char compile[512];
            char basename[256];

            strncpy(basename, arg1, sizeof(basename) - 1);
            basename[sizeof(basename) - 1] = '\0';
            char *ext = strstr(basename, ".asm");
            if (ext) *ext = '\0';
            snprintf(compile, sizeof(compile), 
                     "nasm -f elf64 %s.asm -o %s.o && ld %s.o -o %s && ./%s", 
                     basename, basename, basename, basename, basename);
            
            system(compile);
            out("\n");
        }
    }



    //exe-shell
    else if (strcmp(cmd, "exe-shell") == 0) {
        if (parsed_args < 2) {
            out(RED "errcode 3: file not provided\n" RESET);
            return; 
        }
    
        strncpy(notes_name, arg1, sizeof(notes_name) - 1);
        notes_name[sizeof(notes_name) - 1] = '\0';
    
        if (notes_mode == 0 && (strcmp(arg1, "code.txt") == 0 || strcmp(arg1, "code.sh") == 0)) {
            out(RED "errcode 9: permission denied\n" RESET);
            return;
        }

        for (size_t i = 0; arg1[i] != '\0'; i++) {
            if (!isalnum((unsigned char)arg1[i]) && arg1[i] != '_' && arg1[i] != '-' && arg1[i] != '.') {
                out(RED "errcode 25: invalid filename characters\n" RESET);
                return;
            }
        }

        char compile[512];
        size_t len = strlen(arg1);
        if (len >= 3 && strcmp(arg1 + len - 3, ".sh") == 0) {
            snprintf(compile, sizeof(compile), "chmod +x %s && ./%s", arg1, arg1);
        } else {
            snprintf(compile, sizeof(compile), "chmod +x %s.sh && ./%s.sh", arg1, arg1);
        }
    
        system(compile);
        out("\n");
    }


    //rnm

    else if (strcmp(cmd, "rnm") == 0) { 
        if (parsed_args < 2) {
            out(RED "errcode 3: file not provided\n" RESET);
        } else {
            char old_path[128], new_path[128], new_name[64];
            sprintf(old_path, "documents/%s", arg1);
            
            FILE *check = fopen(old_path, "r");
            if (!check) {
                out(RED "errcode 3: file not provided\n" RESET);
            } else {
                fclose(check);
                out("rename: ");
                fflush(stdout);
                if (fgets(new_name, sizeof(new_name), stdin) != NULL) {
                    new_name[strcspn(new_name, "\n")] = 0;
                    sprintf(new_path, "documents/%s", new_name);

                    if (rename(old_path, new_path) == 0) {
                        if (strcmp(notes_name, arg1) == 0) {
                            strcpy(notes_name, new_name);
                        }
                        out("changed\n");
                    } else {
                        out(RED "errcode 10: name not provided\n" RESET);
                    }
                }
            }
        }
    }



    //mkdir
    else if (strcmp(cmd, "mkdir") == 0) { 
        if (parsed_args < 2) {
            out(RED "errcode 3: file not provided\n" RESET);
        } else {
            char folder_path[128];
            sprintf(folder_path, "documents/%s", arg1);
            
            if (mkdir(folder_path, 0777) == 0) {
                out("changed\n");
            } else {
                out(RED "errcode 10 : folder unable to generate\n" RESET);
            }
        }
    }
    else if (strcmp(cmd, "ls-t") == 0) {
        if(strcmp(arg1, "") == 0){
            DIR *dir;
            struct dirent *entry;
            char fpath[1024];
    
            dir = opendir(".");
            if (dir == NULL){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }

            while ((entry = readdir(dir)) != NULL){
                if (entry->d_type == DT_REG){
                    out("-: %s\n", entry->d_name);
                }
                else if (entry->d_type == DT_DIR){
                    out(BLUE"d: %s\n", entry->d_name);
                    out(RESET);
                }
           
            
            }

            if (closedir(dir) == -1){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
        }
        else{
            DIR *dir;
            struct dirent *entry;
            char fpath[1024];
    
            dir = opendir(".");
            if (dir == NULL){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }

            while ((entry = readdir(dir)) != NULL){
                if (entry->d_type == DT_REG){
                    out("-: %s\n", entry->d_name);
                }
                else if (entry->d_type == DT_DIR){
                    out(BLUE"d: %s\n", entry->d_name);
                    out(RESET);
                }
           
            
            }

            if (closedir(dir) == -1){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
        }
    }



    //ls-d
    else if (strcmp(cmd, "ls-d") == 0) {
        if(strcmp(arg1, "") == 0){
            DIR *dir;
            struct dirent *entry;
            char fpath[1024];
            char read;
            char write;
            char execute;
            char null;
    
            dir = opendir(".");
            if (dir == NULL){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }

            while ((entry = readdir(dir)) != NULL){
                snprintf(fpath, sizeof(fpath), "./%s", entry->d_name);

                struct stat pstd;
                if (stat(fpath, &pstd) != 0) {
                    perror(RED "errcode 11 : file status down" RESET);
                    continue;
                }

                if (S_ISREG(pstd.st_mode)){
                    int r = (access(fpath, R_OK) == 0);int w = (access(fpath, W_OK) == 0);int x = (access(fpath, X_OK) == 0);

                    if (r == 1){read = 'r';}
                    if (w == 1){write = 'w';}
                    if (x == 1){execute = 'x';}
                    if (r == 0){read = '-';}
                    if (w == 0){write = '-';}
                    if (x == 0){execute = '-';}
                    out("%c/%c/%c/%s\n", read,write,execute, entry->d_name);
                
                }

                else if (S_ISDIR(pstd.st_mode)){
                    int r = (access(fpath, R_OK) == 0);int w = (access(fpath, W_OK) == 0);int x = (access(fpath, X_OK) == 0);

                    if (r == 1){read = 'r';}
                    if (w == 1){write = 'w';}
                    if (x == 1){execute = 'x';}
                    if (r == 0){read = '-';}
                    if (w == 0){write = '-';}
                    if (x == 0){execute = '-';}
                    out(BLUE "%c/%c/%c/%s\n", read,write,execute, entry->d_name);
                    out(RESET);
                }
            }
            if (closedir(dir) == -1){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
        }
        else{
            DIR *dir;
            struct dirent *entry;
            char fpath[1024];
            char read;
            char write;
            char execute;
            char null;
    
            dir = opendir(arg1);
            if (dir == NULL){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }

            while ((entry = readdir(dir)) != NULL){
                snprintf(fpath, sizeof(fpath), "./%s", entry->d_name);

                struct stat pstd;
                if (stat(fpath, &pstd) != 0) {
                    perror(RED "errcode 11 : file status down" RESET);
                    continue;
                }

                if (S_ISREG(pstd.st_mode)){
                    int r = (access(fpath, R_OK) == 0);int w = (access(fpath, W_OK) == 0);int x = (access(fpath, X_OK) == 0);

                    if (r == 1){read = 'r';}
                    if (w == 1){write = 'w';}
                    if (x == 1){execute = 'x';}
                    if (r == 0){read = '-';}
                    if (w == 0){write = '-';}
                    if (x == 0){execute = '-';}
                    out("%c/%c/%c/%s\n", read,write,execute, entry->d_name);
                
                }

                else if (S_ISDIR(pstd.st_mode)){
                    int r = (access(fpath, R_OK) == 0);int w = (access(fpath, W_OK) == 0);int x = (access(fpath, X_OK) == 0);

                    if (r == 1){read = 'r';}
                    if (w == 1){write = 'w';}
                    if (x == 1){execute = 'x';}
                    if (r == 0){read = '-';}
                    if (w == 0){write = '-';}
                    if (x == 0){execute = '-';}
                    out(BLUE "%c/%c/%c/%s\n", read,write,execute, entry->d_name);
                    out(RESET);
                }
            }
            if (closedir(dir) == -1){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
        }
    }



    //ls-dt
    else if (strcmp(cmd, "ls-dt") == 0) {
        if(strcmp(arg1, "") == 0){
            DIR *dir;
            struct dirent *entry;
            char fpath[1024];
            char read;
            char write;
            char execute;
            char null;
    
            dir = opendir(".");
            if (dir == NULL){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
    
            while ((entry = readdir(dir)) != NULL) {
                snprintf(fpath, sizeof(fpath), "%s", entry->d_name); 
        
                struct stat pstd;
            
           
                if (stat(fpath, &pstd) != 0) {
                    continue; 
                }
        
                int r = (access(fpath, R_OK) == 0);
                int w = (access(fpath, W_OK) == 0);
                int x = (access(fpath, X_OK) == 0);
        
                if (r == 1){read = 'r';}
                if (w == 1){write = 'w';}
                if (x == 1){execute = 'x';}
                if (r == 0){read = '-';}
                if (w == 0){write = '-';}
                if (x == 0){execute = '-';}
            
                if (S_ISREG(pstd.st_mode)) {
                    out("-/%c/%c/%c/%s\n", read, write, execute, entry->d_name);
                } 
                else if (S_ISDIR(pstd.st_mode)) {
                    out(BLUE "d/%c/%c/%c/%s\n", read, write, execute, entry->d_name);
                    out(RESET);
                }
            }
    
            if (closedir(dir) == -1){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
        }
        else{
            DIR *dir;
            struct dirent *entry;
            char fpath[1024];
            char read;
            char write;
            char execute;
            char null;
    
            dir = opendir(arg1);
            if (dir == NULL){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
    
            while ((entry = readdir(dir)) != NULL) {
                snprintf(fpath, sizeof(fpath), "%s", entry->d_name); 
        
                struct stat pstd;
            
           
                if (stat(fpath, &pstd) != 0) {
                    continue; 
                }
        
                int r = (access(fpath, R_OK) == 0);
                int w = (access(fpath, W_OK) == 0);
                int x = (access(fpath, X_OK) == 0);
        
                if (r == 1){read = 'r';}
                if (w == 1){write = 'w';}
                if (x == 1){execute = 'x';}
                if (r == 0){read = '-';}
                if (w == 0){write = '-';}
                if (x == 0){execute = '-';}
            
                if (S_ISREG(pstd.st_mode)) {
                    out("-/%c/%c/%c/%s\n", read, write, execute, entry->d_name);
                } 
                else if (S_ISDIR(pstd.st_mode)) {
                    out(BLUE "d/%c/%c/%c/%s\n", read, write, execute, entry->d_name);
                    out(RESET);
                }
            }
    
            if (closedir(dir) == -1){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
        }
    }



    //ls-td (same as ls-dt)
    else if (strcmp(cmd, "ls-td") == 0) {
        if(strcmp(arg1, "") == 0){
            DIR *dir;
            struct dirent *entry;
            char fpath[1024];
            char read;
            char write;
            char execute;
            char null;
    
            dir = opendir(".");
            if (dir == NULL){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
    
            while ((entry = readdir(dir)) != NULL) {
                snprintf(fpath, sizeof(fpath), "%s", entry->d_name); 
        
                struct stat pstd;
            
           
                if (stat(fpath, &pstd) != 0) {
                    continue; 
                }
        
                int r = (access(fpath, R_OK) == 0);
                int w = (access(fpath, W_OK) == 0);
                int x = (access(fpath, X_OK) == 0);
        
                if (r == 1){read = 'r';}
                if (w == 1){write = 'w';}
                if (x == 1){execute = 'x';}
                if (r == 0){read = '-';}
                if (w == 0){write = '-';}
                if (x == 0){execute = '-';}
            
                if (S_ISREG(pstd.st_mode)) {
                    out("-/%c/%c/%c/%s\n", read, write, execute, entry->d_name);
                } 
                else if (S_ISDIR(pstd.st_mode)) {
                    out(BLUE "d/%c/%c/%c/%s\n", read, write, execute, entry->d_name);
                    out(RESET);
                }
            }
    
            if (closedir(dir) == -1){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
        }
        else{
            DIR *dir;
            struct dirent *entry;
            char fpath[1024];
            char read;
            char write;
            char execute;
            char null;
    
            dir = opendir(arg1);
            if (dir == NULL){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
    
            while ((entry = readdir(dir)) != NULL) {
                snprintf(fpath, sizeof(fpath), "%s", entry->d_name); 
        
                struct stat pstd;
            
           
                if (stat(fpath, &pstd) != 0) {
                    continue; 
                }
        
                int r = (access(fpath, R_OK) == 0);
                int w = (access(fpath, W_OK) == 0);
                int x = (access(fpath, X_OK) == 0);
        
                if (r == 1){read = 'r';}
                if (w == 1){write = 'w';}
                if (x == 1){execute = 'x';}
                if (r == 0){read = '-';}
                if (w == 0){write = '-';}
                if (x == 0){execute = '-';}
            
                if (S_ISREG(pstd.st_mode)) {
                    out("-/%c/%c/%c/%s\n", read, write, execute, entry->d_name);
                } 
                else if (S_ISDIR(pstd.st_mode)) {
                    out(BLUE "d/%c/%c/%c/%s\n", read, write, execute, entry->d_name);
                    out(RESET);
                }
            }
    
            if (closedir(dir) == -1){
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
        }
    }




    //w
    else if (strcmp(cmd, "w") == 0) { 
        if (parsed_args >= 2) {
            strcpy(notes_name, arg1);
        }
        if (notes_mode == 0 && parsed_args >= 2 && strcmp(arg1, "notes.txt") == 0) {
            out(RED "errcode 9 : permission denied \n" RESET);
        } else {
            get_current_path(loc, notes_name, notes_path);
            file = fopen(notes_path, "w");
            if (file) {
                if (strstr(notes_name, ".seal") != NULL) {
                    out("ZL SCRIPT EDITOR (%s)\n ", notes_name);
                } else {
                    out("ZL FILE EDITOR (%s)\nEnter text: ", notes_name);
                }
                fflush(stdout);
                fgets(content, sizeof(content), stdin);
                fprintf(file, "%s", content);
                fclose(file);
                out("Saved successfully.\n");
            } else {
                out(RED "errcode 12 : could not create file\n" RESET);
            }
        }
    }




    //pen
    else if (strcmp(cmd, "pen") == 0) { 
        if (parsed_args >= 2) {
            strcpy(notes_name, arg1);
        }
        if (notes_mode == 0 && parsed_args >= 2 && strcmp(arg1, "notes.txt") == 0) {
            out(RED "errcode 9 : permission denied \n" RESET);
        } else {
            get_current_path(loc, notes_name, notes_path);
            file = fopen(notes_path, "a+");
            if (file) {
                if (strstr(notes_name, ".seal") != NULL) {
                    out("ZL SCRIPT EDITOR (%s)\n ", notes_name);
                } else {
                    out("ZL FILE EDITOR (%s)\nEnter text: ", notes_name);
                }
                fflush(stdout);
                fgets(content, sizeof(content), stdin);
                fprintf(file, "%s", content);
                fclose(file);
                out("Saved successfully.\n");
            } else {
                out(RED "errcode 12 : could not create file\n" RESET);
            }
        }
    }




    //pad
    else if (strcmp(cmd, "pad") == 0) { 
        if (parsed_args >= 2) {
            strcpy(notes_name, arg1);
        }
        
        if (notes_mode == 0 && parsed_args >= 2 && strcmp(arg1, "code.txt") == 0) {
            out(RED "errcode 9 : permission denied \n" RESET);
        } else {
            out("PAD EDITOR (%s)\n", notes_name);
            out("Type exit to save and exit\n\n");
            char line_buffer[512]; 
            get_current_path(loc, notes_name, notes_path);
            file = fopen(notes_path, "a+");
            
            if (file) {
                fseek(file, 0, SEEK_SET); 
                while (fgets(line_buffer, sizeof(line_buffer), file) != NULL) {
                    out("%s", line_buffer);
                }
                while (1) {
                    fflush(stdout);
                    if (fgets(content, sizeof(content), stdin) == NULL) {
                        break; 
                    }
                    if (strcmp(content, "exit\n") == 0) {
                        break; 
                    }
                    fprintf(file, "%s", content);
                }
                fclose(file);
                out("saved\n");
            } else {
                out(RED "errcode 13 : could not open file\n" RESET);
            }
        }
    }


    //ow

    else if (strcmp(cmd, "ow") == 0){
        

        if (strcmp(arg1, "bootloader.cpp") == 0 || strcmp(arg1, "tty1.cpp") == 0 || strcmp(arg1, "tty2.cpp") == 0 || strcmp(arg1, "tty1") == 0 || strcmp(arg1, "tty2") == 0){
            out(RED "errcode 24: access denied: doing such process is very dangerous\n" RESET);
        }

        else{
            FILE *fptr;

            fptr = fopen(arg1, "w");
            fprintf(fptr, "");
            fclose(fptr);
        }

        
    }

    //ow-r

    else if (strcmp(cmd, "ow-r") == 0) { 
        if (parsed_args >= 2) {
            strcpy(notes_name, arg1);
        }
        
        if (notes_mode == 0 && parsed_args >= 2 && strcmp(arg1, "code.txt") == 0) {
            out(RED "errcode 9 : permission denied \n" RESET);
        } else {

            if (strcmp(arg1, "bootloader.cpp") == 0 || strcmp(arg1, "tty1.cpp") == 0 || strcmp(arg1, "tty2.cpp") == 0 || strcmp(arg1, "tty1") == 0 || strcmp(arg1, "tty2") == 0){
                out(RED "errcode 24: access denied: doing such process is very dangerous\n" RESET);
            }

            else{
                out("PAD EDITOR (%s)\n", notes_name);
                out("Type exit to save and exit\n\n");
                char line_buffer[512]; 
                get_current_path(loc, notes_name, notes_path);
                file = fopen(notes_path, "w");
            
                if (file) {
                    fseek(file, 0, SEEK_SET); 
                    while (fgets(line_buffer, sizeof(line_buffer), file) != NULL) {
                        out("%s", line_buffer);
                    }
                    while (1) {
                        fflush(stdout);
                        if (fgets(content, sizeof(content), stdin) == NULL) {
                            break; 
                        }
                        if (strcmp(content, "exit\n") == 0) {
                            break; 
                        }
                        fprintf(file, "%s", content);
                    }
                    fclose(file);
                    out("saved\n");
                }
                else {
                    out(RED "errcode 13 : could not open file\n" RESET);
                }
            }
        }
    }
    
    
    
    
    //r
    else if (strcmp(cmd, "r") == 0) {
        if (parsed_args < 2) {
            out(RED "errcode 3: file not provided\n" RESET);
        } else {
            if (strcmp(arg1, notes_name) == 0 && notes_mode == 0) {
                out(RED "errcode 9 : permission denied\n" RESET);
            } else {
                char read_path[128], line_buffer[256];
                get_current_path(loc, arg1, read_path);
                FILE *rf = fopen(read_path, "r");
                if (rf) {
                    out(" \n");
                    while (fgets(line_buffer, sizeof(line_buffer), rf) != NULL) {
                        out("%s", line_buffer);
                    }
                    out(" \n");
                    fclose(rf);
                } else {
                    out(RED "errcode 3: file not provided\n" RESET);
                }
            }
        }
    }



    //head
    else if (strcmp(cmd, "head") == 0) {
        if (parsed_args < 2) {
            out(RED "errcode 3: file not provided\n" RESET);
        } else {
            if (strcmp(arg1, notes_name) == 0 && notes_mode == 0) {
                out(RED "errcode 9 : permission denied\n" RESET);
            } else {
                int linesread = 10;
                out ("Input how many lines you want to see this file: ");
                in("", &linesread);
                
                char read_path[128], linebuffer[256];
                get_current_path(loc, arg1, read_path);
                FILE *rf = fopen(read_path, "r");
                if (rf) {
                    int linecount = 0;
                    while (linecount < linesread && fgets(linebuffer, sizeof(linebuffer), rf) != NULL) {
                        out(linebuffer);
                        linecount++;
                    }
                    fclose(rf);
                } else {
                    out(RED "errcode 3: file not provided\n" RESET);
                }
            }
        }
    }



    //tail
    else if (strcmp(cmd, "tail") == 0) {
        if (parsed_args < 2) {
            out(RED "errcode 3: file not provided\n" RESET);
        } else {
            if (strcmp(arg1, notes_name) == 0 && notes_mode == 0) {
                out(RED "errcode 9 : permission denied\n" RESET);
            } else {
                int linesread = 10;
                out ("Input how many lines you want to see this file: ");
                in("", &linesread);
                
                char read_path[128], linebuffer[256];
                get_current_path(loc, arg1, read_path);
                FILE *rf = fopen(read_path, "r");
                if (rf) {
                    int linecount = 0;
                    while (linecount < linesread && fgets(linebuffer, sizeof(linebuffer), rf) != NULL) {
                        out(linebuffer);
                        linecount--;
                    }
                    fclose(rf);
                } else {
                    out(RED "errcode 3: file not provided\n" RESET);
                }
            }
        }
    }


    //system stuff
    else if (strcmp(cmd, "info") == 0) {

        out("                 ..^~:::::::::::....              \n");
        out("            .::::.......:^::.. ..:..:::.          \n");
        out("       ...::..          .:.:.  .::.   .:^:        \n");
        out("      ::..              .: ~:. ::..      .::      \n");
        out("    ::.   ..^ :^..      .. . :. ...  :     :^.    \n");
        out("   .^    P@G:  7&#~      ...         ~       ^.   \n");
        out("  :^     ~!::!^.:!^  .   ::. ::::.^  ~. .    .^   \n");
        out("  ^:     ~^?B@@B!:~  ^     .:^..:::...:..     !^. \n");
        out(" ^:^    ^77GP5PPJ!^  ........ ...  .:   . ^: :^ ~ \n");
        out(" ^.^:   :...    ^^         ....... :^:^^~:: .^.:^ \n");
        out("  ..:::...:^:.  ^. .. .. .^.......^^..^:...:^:..  \n");
        out("       ....:::::^^^~~^^^:^~ : ^:::^^.......       \n");
        out("                   .......^:~^~~^^.               \n");
        out("\n");
        out("%s\n", version);
        out("Code Env.: VM\n");
        out("Code Env. 2: CodePad\n");
        out("Code: C, C++\n");
        out("Host: CodePad Server\n");
        out("PC Info: Virtual\n");
        out("Copyleft SealKernel from ZL Project\n");
        out("2026\n");
        out("QWERTYUIOPASDFGHJKLZXCVBNM1234567890\n");
            out("\n");
            time(&currentTime); 
            out("%s", ctime(&currentTime));
            tagged();
        
        }
        
        
        else if (strcmp(cmd, "about") == 0) {
            out("                 ..^~:::::::::::....              \n");
            out("            .::::.......:^::.. ..:..:::.          \n");
            out("       ...::..          .:.:.  .::.   .:^:        \n");
            out("      ::..              .: ~:. ::..      .::      \n");
            out("    ::.   ..^ :^..      .. . :. ...  :     :^.    \n");
            out("   .^    P@G:  7&#~      ...         ~       ^.   \n");
            out("  :^     ~!::!^.:!^  .   ::. ::::.^  ~. .    .^   \n");
            out("  ^:     ~^?B@@B!:~  ^     .:^..:::...:..     !^. \n");
            out(" ^:^    ^77GP5PPJ!^  ........ ...  .:   . ^: :^ ~ \n");
            out(" ^.^:   :...    ^^         ....... :^:^^~:: .^.:^ \n");
            out("  ..:::...:^:.  ^. .. .. .^.......^^..^:...:^:..  \n");
            out("       ....:::::^^^~~^^^:^~ : ^:::^^.......       \n");
            out("                   .......^:~^~~^^.               \n");
            out("\n");
        
            long size = 0;
            filesize("tty1.cpp", "tty1.cpp");
        
            
        
            out("%s\n", version);
            out("Code Env.: VM\n");
            out("Code Env. 2: CodePad\n");
            out("Code: C, C++\n");
            out("Host: CodePad Server\n");
            out("PC Info: Virtual\n");
            
            
        
            out("Copyleft SealKernel from ZL Project\n");
            out("2026\n");
            out("QWERTYUIOPASDFGHJKLZXCVBNM1234567890\n");
            out("\n");
            time(&currentTime); 
            out("%s", ctime(&currentTime));
            tagged();
        }


        //system info's versions
    else if (strcmp(cmd, "version-about") == 0){
        out("About Function - Version 1.5.0\n");
    }
    else if (strcmp(cmd, "version-info") == 0){
        out("About Function - Version 2.2.0\n");
    }
        
    //variable sizes    
    else if (strcmp(cmd, "sizeofint") == 0){
        out("%zu\n", sizeof(q));
    }
    else if (strcmp(cmd, "sizeofchar") == 0){
        out("%zu\n", sizeof(w));
    }
    else if (strcmp(cmd, "sizeoffloat") == 0){
        out("%zu\n", sizeof(e));
    }
    else if (strcmp(cmd, "sizeoflong") == 0){
        out("%zu\n", sizeof(t));
    }
    else if (strcmp(cmd, "sizeofdouble") == 0){
        out("%zu\n", sizeof(r));
    }
    else if (strcmp(cmd, "goto") == 0) {
        if (arg1[0] != '\0') {
            if (strcmp(arg1, "root") == 0){
                out(RED "errcode 26 : permission not available\n" RESET);
            }

            else {
                if (chdir(arg1) == 0) {}
                else {
                    perror(RED "errcode 3 : file not provided" RESET);
                }
            }
            
        }
    }



    //sudo stuff
    else if (strcmp(cmd, "sudo-on") == 0){

        if (strcmp(users, "seal") != 0){
            out(RED "errcode 14 : user doesn't have sudo power. Exit your user to either root or seal user\n" RESET);
        }
        else {
            superior = 1;
            char root_path[512];
            snprintf(root_path, sizeof(root_path), "%s/root", oripath);
            if (chdir(root_path) == 0) {
                out("sudo mode\n");
            } else {}
        }
    }
    else if (strcmp(cmd, "sudo-off") == 0){
        superior = 0;
        chdir("../");
        out("normal mode\n");
    }

    else if (strcmp(cmd, "exit") == 0){
        superior = 0;
        out("sudo-off");
        chdir("../");
        out("normal mode\n");
    }


    //go back up 1 dir
    else if (strcmp(cmd, "back") == 0) {
        chdir("../");
    }

    //see where you are
    else if (strcmp(cmd, "where") == 0) {
        
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            out("%s\n", cwd);
            if (strcmp(cwd, "/home/") == 0){
                out(YELLOW "Warning: some functions won't be available here, switch to your user directory to use those functions\n" RESET);
            } 
        }
        else {
            perror(RED "errcode 13 : could not open file" RESET);
        }
    }



    //version info
    else if (strcmp(cmd, "version") == 0) {
        out("%s\n", version);
    }




    //details for the latest version
    else if (strcmp(cmd, "release") == 0){
        out("SealKernel 10 - can check size of variable class and can check version and release.\n");out("SealKernel 11 - added curl to grab data from one site and added fast OS specification.\n");out("SealKernel 12 [BETA] - added tsastream command to check streaming marks for TSIS student\n");out("SealKernel 13 - Added calculator function andd improved tsastream\n"); out("SealKernel 14 [BETA] - added tic tac toe game\n");out("SealKernel 15 - added check storage in main.cpp\n");out("SealKernel 16 - added check storage in main.cpp inside quick and about and removed TIC TAC TOE for ROCK PAPER SCISSORS game\n");out("SealKernel 17 - fixed rock paper scissors game and added guess the number game\n"); out ("SealKernel 18 - changed file locations and changed space-main.cpp to space. Also fixed game1\n");out("SealKernel 19 - added game3 and game4 and also restricted exit command only for sudo user\n");out("SealKernel 20 - added dice feature\n");out("SealKernel 21 - fixed tsastream and removed quick and about for monthly cleaning (July)\n");    out("SealKernel 22 - Improved tsastream\n");out("SealKernel 23 - added luck program, element program and game5. Also improved pad function\n");out("SealKernel 24 - added speed reaction game\n");out("SealKernel 25 - Added conquer country game and also added bool. Also addded more space function (check out in help)\n");out("SealKernel 26 - changed entire ls family, changed file structure\n");out("SealKernel 27 - changed entire code structure of calculator\n");out("SealKernel 28 - added move function and changed execute function\n");out("SealKernel 16.7.2026 - changed version name from 28 to 16.7.2026 to indicate when was the version released, added more file for different purposes and also added image function. Finally, we also added file space for them\n");out("SealKernel 17.7.2026 - added words, phrase and essay function\n");out("SealKernel 19.7.2026 - prevent overflowing values for tsastream\n");out("SealKernel 19.7.2026 More - created users function\n");
        out("SealKernel 20.7.2026 - Changed input for user and also restricted normal user to root user so they cannot control the system and also added compress and decompress function\n");
        out("SealKernel 21.7.2026 - added own package manager from bpm and also fixed Trigraphs error and other warnings\n");
        out("SealKernel 22.7.2026 - added browser function to show HTML code in website\n");out("SealKernel 23.7.2026 - added bootloader and function clear\n");out("SealKernel 26.7.2026 - fixed space and about function and added error code for future purposes\n");
        out("SealKernel 27.7.2026 - added 2026 next term expectation in beta so students can see their marks and see which class they are going to be in and break their hopes and dreams\n");out("SealKernel 28.7.2026 - changed entire code structure for tsastream\n");out("SealKernel 29.7.2026 - added game8 and game9 is in progress\n");
        out("SealKernel 30.7.2026 - added game10, game9 in progress and tsastream new update.\n");out("SealKernel 31.7.2026 - changed stdio lib to zlio lib.\n");out("SealKernel 1.8.2026 - added date to info and about and also improved tsastream\n");out("SealKernel 5.8.2026 - improved help, bootloader changed, added another text editor, extend execute function to other language and added system function\n");out("SealKernel 9.8.2026 - added history and clear history function and also made remove function more secure\n");out("SealKernel 11.8.2026 - added talktoseal feature and added head and tail feature\n");out("SealKernel 12.8.2026 - changed how goto, back and where works and also changed user input stuff\n");out("SealKernel 13.8.2026 - making rm more restricted (so system files won't get deleted) and also fixed change directory/path issue\n");out("SealKernel 13.8.2026 More - changed path structure for SealKernel\n");out("SealKernel 14.8.2026 - updated all ls functions and also added webip-a function\n");out("SealKernel 16.8.2026 - added shell execution function\n");
        out("SealKernel 17.8.2026 - Added game11 and added remainder function\n");
        out("SealKernel 18.8.2026 - Added game12/varlab , added update-sudd and added cc functions\n");
        out("SealKernel 18.8.2026 More - Added memory allocations stuff (check help)\n");
        out("SealKernel 19.8.2026 - Added unit function\n");
        out("SealKernel 20.8.2026 - Added rmdir function, changed goto structure and also added exit function\n");out("SealKernel 21.8.2026 - Added minimum and maximum range for random, added sub commands for time, changed ZLIO.H, print what was the last command and finally added colour codes.\n");
        out("SealKernel 22.8.2026 - Added colour codes to ls, ls-d, ls-t, ls-td and ls-dt and improved game8\n");
        out("SealKernel 26.8.2026 - Added comments function\n");
        out("SealKernel 1.9.2026 - Added cursor function (beta) and overwrite function\n");
        out("SealKernel 3.9.2026 - Added previous command function and removed previous command from homescreen\n");
        out("SealKernel 5.9.2026 - Improved game8 and added chooser program\n");
        }




        //ls function
    else if (strcmp(cmd, "ls") == 0) {
        if (strcmp(arg1, "") == 0) {
            DIR *dir;
            struct dirent *entry;
    
            dir = opendir(".");
            if (dir == NULL) {
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
    
            while ((entry = readdir(dir)) != NULL) {
                struct stat st;
                if (stat(entry->d_name, &st) == 0) {
                    if (S_ISREG(st.st_mode)) {
                        out("%s\n", entry->d_name);
                    }

                    else if (S_ISDIR(st.st_mode)) {
                        out(BLUE"%s\n", entry->d_name);
                        out(RESET);
                    }
                }
            }
    
            if (closedir(dir) == -1) {
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
        }

        else{
            DIR *dir;
            struct dirent *entry;
    
            dir = opendir(".");
            if (dir == NULL) {
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
    
            while ((entry = readdir(dir)) != NULL) {
                struct stat st;
                if (stat(entry->d_name, &st) == 0) {
                    if (S_ISREG(st.st_mode)) {
                        out("%s\n", entry->d_name);
                    }

                    else if (S_ISDIR(st.st_mode)) {
                        out(BLUE"%s\n", entry->d_name);
                        out(RESET);
                    }
                }
            }
    
            if (closedir(dir) == -1) {
                out(RED "errcode 3: file not provided\n" RESET);
                return;
            }
        }
            
    }



    //help
    else if (strcmp(cmd, "help") == 0) {
        out(RED"SealKernel Functions:\n");
        out(RESET);
        out("1. System functions:\n");
        out(" \n");
        out("chmod - change file to either public(1) or private (0)\n");
        out("rm - remove a file\n");
        out("rmdir - remove a directory\n");
        out("mv - move a file to another location\n");
        out("save - save a file\n");
        out("exe - execute c/cpp file\n");
        out("exe-python - execute py3 file\n");
        out("exe-java - execute java file\n");
        out("exe-asm - execute assembly file\n");
        out("exe-shell - execute shell file\n");
        out("rnm - rename a file to another name\n");
        out("mkdir - make a directory\n");
        out("ls-t - list types of files\n");
        out("ls-d - list files in advance\n");
        out("ls-dt / ls-td - list types of files and look files in advance\n");
        out("w - write a file\n");
        out("pen - write a file but better than w\n");
        out("pad - write a file but better than w and pen\n");
        out("r - read a file\n");
        out("info - view system infomation\n");
        out("about - view system infomation but have more infomation\n");
        out("version-info - check info version\n");
        out("version-about - check about version\n");
        out("goto - go to a file/directory\n");
        out("sudo-on - turn on sudo mode\n");
        out("sudo-off - turn off sudo mode\n");
        out("exit - turn off sudo mode\n");
        out("back - go back to root / superior\n");
        out("where - tells you your location\n");
        out("version - tells you what version you are in\n");
        out("ls - list down files\n");
        out("echo - repeat what you key in\n");
        out("date - WHAT IS THE DATE NOW???\n");
        out("date-y - WHAT IS THE DATE NOW??? in year\n");
        out("date-m - WHAT IS THE DATE NOW??? in month\n");
        out("date-d - WHAT IS THE DATE NOW??? in day\n");
        out("date-h - WHAT IS THE DATE NOW??? in hour\n");
        out("date-min - WHAT IS THE DATE NOW??? in minutes\n");
        out("date-sec - WHAT IS THE DATE NOW??? in seconds\n");
        out("space - check space for tty1.cpp\n");
        out("space-documents - check space for documents directory\n");
        out("space-downloads - check space for downloads directory\n");
        out("space-system - check space for system directory\n");
        out("space-pictures - check space for pictures directory\n");
        out("space-videos - check space for videos directory\n");
        out("space-examples - check space for examples directory\n");
        out("space-music - check space for music directory\n");
        out("space-home - check space for home directory\n");
        out("space-others - check space for other directory/files\n");
        out("users - make a temporary user\n");
        out("users-seal - delete and go into seal user\n");
        out("whoami - print out who you are\n");
        out("available - print out active users\n");
        out("available-t - print out active users in detail\n");
        out("version-zlio - shows the version of SealKernel's own IO library\n");
        out("update-sudd - shows the version of SealKernel User Directory Design (SUDD)\n");
        out("system - shows what OS you are running\n");
        out("sudo-exit - shut down / exit this program\n");
        out("history - check previous commands\n");
        out("clear-history - delete previous commands\n");
        out("head - show the first (n) lines of a file\n");
        out("tail - show the last (n) lines of a file\n");
        out("webip-a - check your IP Adress in the web (DNS)\n");
        out("wc - check word count\n");
        out("cc - check character count\n");
        out("memcode - turns a string to memory address\n");
        out("strcode - turns a memory address to another memory address\n");
        out("ow - overwrite a file\n");
        out("ow-r - overwrite and replace with another text\n");
        out("!! - see previous command\n");
        
        

        out(" \n");
        out("2. Sizes\n");
        out(" \n");
        out("sizeofint - look at size of integer\n");
        out("sizeofchar - look at size of character\n");
        out("sizeoffloat - look at size of float\n");
        out("sizeofdouble - look at size of double\n");
        out("sizeoflong - look at size of long\n");

        out(" \n");
        out("3. Other apps\n");
        out(" \n");
        out("tsastream - look at your class and streaming marks for TSIS students\n");
        out("tsastream-free - look at your class and streaming marks for TSIS students\n");
        out("calc - calculator\n");
        out("version-calc - calculator version\n");
        out("random - gives out random numbers\n");
        out("elements - gives you a random element\n");
        out("bool - let the computer gives you 1 or 0\n");
        out("image - turns an image to ASCII art\n");
        out("phrase - gives you a random sentence\n");
        out("words - gives you a random word\n");
        out("essay - gives you a random essay\n");
        out("talktoseal - talk to the seal\n");
        out("talktoseal - talk to the seal but the output are in human language\n");
        out("comp - 'compress' a file\n");
        out("decomp - 'decompress' a file\n");
        out("pkgmgr add (URL) - download a single file\n");
        out("pkgmgr clone (URL) - run or use a download file (some repo cannot)\n");
        out("browser - show the code of a page\n");
        out("luck - shows ur luck\n");
        out("unit - checks units\n");
        out("cursor (beta) - change your cursor\n");
        out("chooser - let the program choose what to pick\n");

        out(" \n");
        out("4. ASCII Arts\n");
        out(" \n");
        out("lemon - lemon art\n");
        out("eggs - eggs art\n");

        out(" \n");
        out("5. Games\n");
        out(" \n");
        out("game1 - rock paper scissors game\n");
        out("game2 - guess the number game\n");
        out("game3 - capture the flag game\n");
        out("game4 - gun game\n");
        out("game5 - but guessing our number game\n");
        out("game6 - reaction time test game\n");
        out("game7 - country game\n");
        out("game8 - lamp guessing game\n");
        out("game9 - text sniping rpg game\n");
        out("game10 - avoid the chosen number game\n");
        out("game11 - Madlibs game\n");
        out("game12/varlab - variable game\n");
    }



    //echo
    else if (strcmp(cmd, "echo") == 0) {
        out("%s\n", input + (strlen(input) > 4 ? 5 : 0));
    }




    //tsastream
    else if (strcmp(cmd, "tsastream") == 0) {
        
        double math;
        double chemistry;
        double physics;
        double biology;
        double history;
        double geography;
        double sejarah;
        int input;
        char file1 [128];
        int file2;
        char class2025 [128];
        char class2026 [128];
        char classExpectation [128];
        int avg;
        char consumer [128];
        
        out("Which version do you want to choose?\n");
        out("1. Normal Version\n");
        out("2. New Version\n");
        in("", &input);

        out("Write your user (NOTE THAT WRITING YOUR REAL NAME IS NOT RECOMMENDED): ");
        in("", consumer);

        if (input == 1){
            out("Input your math marks: ");
            in("", &math);
            if (math < 0 || math >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your chemistry marks: ");
            in("", &chemistry);
            if (chemistry < 0 || chemistry >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your biology marks: ");
            in("", &biology);
            if (biology < 0 || biology >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your physics marks: ");
            in("", &physics);
            if (physics < 0 || physics >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your geography marks: ");
            in("", &geography);
            if (geography < 0 || geography >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your history marks: ");
            in("", &history);
            if (history < 0 || history >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }

            double math_pct = math * 0.55;
            double chem_pct = chemistry * 0.125;
            double bio_pct = biology * 0.1;
            double phy_pct = physics * 0.125;
            double geo_pct = geography * 0.05;
            double his_pct = history * 0.05;
            avg = math_pct+chem_pct+bio_pct+phy_pct+geo_pct+his_pct;

            out("Your streaming mark is: %d%%\n", avg);
            if (avg >= 90){
                strcpy(class2025, "Acacia");
            }
            else if (avg >= 80 && avg < 90){
                strcpy(class2025, "Aster");
            }
            else if (avg >= 70 && avg < 80){
                strcpy(class2025, "Begonia");
            }
            else if (avg >= 60 && avg < 70){
                strcpy(class2025, "Castanea");
            }
            else if (avg >= 50 && avg < 60){
                strcpy(class2025, "Juniper / MX Intensive 1");
            }
            else if (avg < 50){
                strcpy(class2025, "Magnolia / MX Intensive 2 / MX Intensive");
            }
            out("2025 streaming mark: %s\n", class2025);

            if (avg >= 70){
                strcpy(class2026, "Acacia");
            }
            else if (avg >= 60 && avg < 70){
                strcpy(class2026, "Aster");
            }
            else if (avg >= 50 && avg < 60){
                strcpy(class2026, "Begonia");
            }
            else if (avg >= 40 && avg < 50){
                strcpy(class2026, "Castanea");
            }
            else if (avg < 40){
                strcpy(class2026, "MX Intensive 1 & 2");
            }
            out("2026 streaming mark: %s\n", class2026);

            if (avg >= 80){
                strcpy(classExpectation, "Acacia");
            }
            else if (avg >= 70 && avg < 80){
                strcpy(classExpectation, "Aster");
            }
            else if (avg >= 60 && avg < 70){
                strcpy(classExpectation, "Begonia");
            }
            else if (avg >= 50 && avg < 60){
                strcpy(classExpectation, "Castanea");
            }
            else if (avg >= 40 && avg < 50){
                strcpy(classExpectation, "Juniper");
            }
            else if (avg >= 30 && avg < 40){
                strcpy(classExpectation, "Magnolia");
            }
            else if (avg >= 20 && avg < 30){
                strcpy(classExpectation, "MX Intensive 1");
            }
            else if (avg < 20){
                strcpy(classExpectation, "MX Intensive 2");
            }
            out("2026 New Term Expectation: %s\n", classExpectation);
        }

        else if (input == 2){
            out("Input your math marks: ");
            in("", &math);
            if (math < 0 || math >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your chemistry marks: ");
            in("", &chemistry);
            if (chemistry < 0 || chemistry >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your biology marks: ");
            in("", &biology);
            if (biology < 0 || biology >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your physics marks: ");
            in("", &physics);
            if (physics < 0 || physics >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your geography marks: ");
            in("", &geography);
            if (geography < 0 || geography >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your history marks: ");
            in("", &history);
            if (history < 0 || history >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }

            out("Input your sejarah marks: ");
            in("", &sejarah);
            if (sejarah < 0 || sejarah >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }

            if (input == 1){
                float math_pct = math * 0.55;
                float chem_pct = chemistry * 0.125;
                float bio_pct = biology * 0.1;
                float phy_pct = physics * 0.125;
                float geo_pct = geography * 0.05;
                float his_pct = history * 0.05;
                avg = math_pct+chem_pct+bio_pct+phy_pct+geo_pct+his_pct;
            }

            if (input == 2){
                float math_pct = math * 0.5;
                float chem_pct = chemistry * 0.125;
                float bio_pct = biology * 0.1;
                float phy_pct = physics * 0.125;
                float geo_pct = geography * 0.05;
                float his_pct = history * 0.05;
                float sej_pct = sejarah * 0.05;
                avg = math_pct+chem_pct+bio_pct+phy_pct+geo_pct+his_pct+sej_pct;
            }

            
            out("Your streaming mark is: %d%%\n", avg);
            if (avg >= 90){
                strcpy(class2025, "Acacia");
            }
            else if (avg >= 80 && avg < 90){
                strcpy(class2025, "Aster");
            }
            else if (avg >= 70 && avg < 80){
                strcpy(class2025, "Begonia");
            }
            else if (avg >= 60 && avg < 70){
                strcpy(class2025, "Castanea");
            }
            else if (avg >= 50 && avg < 60){
                strcpy(class2025, "Juniper / MX Intensive 1");
            }
            else if (avg < 50){
                strcpy(class2025, "Magnolia / MX Intensive 2 / MX Intensive");
            }
            out("2025 streaming mark: %s\n", class2025);

            if (avg >= 70){
                strcpy(class2026, "Acacia");
            }
            else if (avg >= 60 && avg < 70){
                strcpy(class2026, "Aster");
            }
            else if (avg >= 50 && avg < 60){
                strcpy(class2026, "Begonia");
            }
            else if (avg >= 40 && avg < 50){
                strcpy(class2026, "Castanea");
            }
            else if (avg < 40){
                strcpy(class2026, "MX Intensive 1 & 2");
            }
            out("2026 streaming mark: %s\n", class2026);

            if (avg >= 80){
                strcpy(classExpectation, "Acacia");
            }
            else if (avg >= 70 && avg < 80){
                strcpy(classExpectation, "Aster");
            }
            else if (avg >= 60 && avg < 70){
                strcpy(classExpectation, "Begonia");
            }
            else if (avg >= 50 && avg < 60){
                strcpy(classExpectation, "Castanea");
            }
            else if (avg >= 40 && avg < 50){
                strcpy(classExpectation, "Juniper");
            }
            else if (avg >= 30 && avg < 40){
                strcpy(classExpectation, "Magnolia");
            }
            else if (avg >= 20 && avg < 30){
                strcpy(classExpectation, "MX Intensive 1");
            }
            else if (avg < 20){
                strcpy(classExpectation, "MX Intensive 2");
            }
            out("2026 New Term Expectation: %s\n", classExpectation);
        }
        else {
            out("Only 1 and 2 allowed. Retype this function to try again.\n");
            return;
        }

        
        FILE *fptr = fopen("grades.txt", "a");
        if (fptr == NULL) {
            perror("Error opening file");
            return;
        }

        fprintf(fptr,"----------------------------------\n");
        fprintf(fptr, "User: %s\n", consumer);

        fprintf(fptr, "Math, %.2f\n", math);
        fprintf(fptr, "Physics, %.2f\n", physics);
        fprintf(fptr, "Chemistry, %.2f\n", chemistry);
        fprintf(fptr, "Biology, %.2f\n", biology);
        fprintf(fptr, "History, %.2f\n", history);
        fprintf(fptr, "Geography, %.2f\n", geography);
        fprintf(fptr, "Average, %d\n", avg);
        fprintf(fptr, "2025 Class, %s\n", class2025);
        fprintf(fptr, "2026 Class, %s\n", class2026);
        fprintf(fptr, "2026 Expectation Class, %s\n", classExpectation);

        fprintf(fptr,"----------------------------------\n");


        fclose(fptr);

        
        
    }



    //freedom of tsastream
    else if (strcmp(cmd, "tsastream-free") == 0) {
        
        double math;
        double chemistry;
        double physics;
        double biology;
        double history;
        double geography;
        double sejarah;
        int input;
        char file1 [128];
        int file2;
        char class2025 [128];
        char class2026 [128];
        char classExpectation [128];
        int avg;
        char consumer [128];
        
        out("Which version do you want to choose?\n");
        out("1. Normal Version\n");
        out("2. New Version\n");
        in("", &input);

        out("Write your user (NOTE THAT WRITING YOUR REAL NAME IS NOT RECOMMENDED): ");
        in("", consumer);

        if (input == 1){
            out("Input your math marks: ");
            in("", &math);
            if (math < 0 || math >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your chemistry marks: ");
            in("", &chemistry);
            if (chemistry < 0 || chemistry >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your biology marks: ");
            in("", &biology);
            if (biology < 0 || biology >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your physics marks: ");
            in("", &physics);
            if (physics < 0 || physics >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your geography marks: ");
            in("", &geography);
            if (geography < 0 || geography >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your history marks: ");
            in("", &history);
            if (history < 0 || history >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }

            double math_pct = math * 0.55;
            double chem_pct = chemistry * 0.125;
            double bio_pct = biology * 0.1;
            double phy_pct = physics * 0.125;
            double geo_pct = geography * 0.05;
            double his_pct = history * 0.05;
            avg = math_pct+chem_pct+bio_pct+phy_pct+geo_pct+his_pct;

            out("Your streaming mark is: %d%%\n", avg);
            if (avg >= 90){
                strcpy(class2025, "Acacia");
            }
            else if (avg >= 80 && avg < 90){
                strcpy(class2025, "Aster");
            }
            else if (avg >= 70 && avg < 80){
                strcpy(class2025, "Begonia");
            }
            else if (avg >= 60 && avg < 70){
                strcpy(class2025, "Castanea");
            }
            else if (avg >= 50 && avg < 60){
                strcpy(class2025, "Juniper / MX Intensive 1");
            }
            else if (avg < 50){
                strcpy(class2025, "Magnolia / MX Intensive 2 / MX Intensive");
            }
            out("2025 streaming mark: %s\n", class2025);

            if (avg >= 70){
                strcpy(class2026, "Acacia");
            }
            else if (avg >= 60 && avg < 70){
                strcpy(class2026, "Aster");
            }
            else if (avg >= 50 && avg < 60){
                strcpy(class2026, "Begonia");
            }
            else if (avg >= 40 && avg < 50){
                strcpy(class2026, "Castanea");
            }
            else if (avg < 40){
                strcpy(class2026, "MX Intensive 1 & 2");
            }
            out("2026 streaming mark: %s\n", class2026);

            if (avg >= 80){
                strcpy(classExpectation, "Acacia");
            }
            else if (avg >= 70 && avg < 80){
                strcpy(classExpectation, "Aster");
            }
            else if (avg >= 60 && avg < 70){
                strcpy(classExpectation, "Begonia");
            }
            else if (avg >= 50 && avg < 60){
                strcpy(classExpectation, "Castanea");
            }
            else if (avg >= 40 && avg < 50){
                strcpy(classExpectation, "Juniper");
            }
            else if (avg >= 30 && avg < 40){
                strcpy(classExpectation, "Magnolia");
            }
            else if (avg >= 20 && avg < 30){
                strcpy(classExpectation, "MX Intensive 1");
            }
            else if (avg < 20){
                strcpy(classExpectation, "MX Intensive 2");
            }
            out("2026 New Term Expectation: %s\n", classExpectation);
        }

        else if (input == 2){
            out("Input your math marks: ");
            in("", &math);
            if (math < 0 || math >100){
                printf("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your chemistry marks: ");
            in("", &chemistry);
            if (chemistry < 0 || chemistry >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your biology marks: ");
            in("", &biology);
            if (biology < 0 || biology >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your physics marks: ");
            in("", &physics);
            if (physics < 0 || physics >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your geography marks: ");
            in("", &geography);
            if (geography < 0 || geography >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }
            out("Input your history marks: ");
            in("", &history);
            if (history < 0 || history >100){
                out("errcode 16 : value must be less than 101 and more than -1\n");
                return;
            }

            if (input == 1){
                float math_pct = math * 0.55;
                float chem_pct = chemistry * 0.125;
                float bio_pct = biology * 0.1;
                float phy_pct = physics * 0.125;
                float geo_pct = geography * 0.05;
                float his_pct = history * 0.05;
                avg = math_pct+chem_pct+bio_pct+phy_pct+geo_pct+his_pct;
            }

            if (input == 2){
                float math_pct = math * 0.5;
                float chem_pct = chemistry * 0.125;
                float bio_pct = biology * 0.1;
                float phy_pct = physics * 0.125;
                float geo_pct = geography * 0.05;
                float his_pct = history * 0.05;
                float sej_pct = sejarah * 0.05;
                avg = math_pct+chem_pct+bio_pct+phy_pct+geo_pct+his_pct+sej_pct;
            }

            printf("Your streaming mark is: %d%%\n", avg);
            if (avg >= 90){
                strcpy(class2025, "Acacia");
            }
            else if (avg >= 80 && avg < 90){
                strcpy(class2025, "Aster");
            }
            else if (avg >= 70 && avg < 80){
                strcpy(class2025, "Begonia");
            }
            else if (avg >= 60 && avg < 70){
                strcpy(class2025, "Castanea");
            }
            else if (avg >= 50 && avg < 60){
                strcpy(class2025, "Juniper / MX Intensive 1");
            }
            else if (avg < 50){
                strcpy(class2025, "Magnolia / MX Intensive 2 / MX Intensive");
            }
            out("2025 streaming mark: %s\n", class2025);

            if (avg >= 70){
                strcpy(class2026, "Acacia");
            }
            else if (avg >= 60 && avg < 70){
                strcpy(class2026, "Aster");
            }
            else if (avg >= 50 && avg < 60){
                strcpy(class2026, "Begonia");
            }
            else if (avg >= 40 && avg < 50){
                strcpy(class2026, "Castanea");
            }
            else if (avg < 40){
                strcpy(class2026, "MX Intensive 1 & 2");
            }
            out("2026 streaming mark: %s\n", class2026);

            if (avg >= 80){
                strcpy(classExpectation, "Acacia");
            }
            else if (avg >= 70 && avg < 80){
                strcpy(classExpectation, "Aster");
            }
            else if (avg >= 60 && avg < 70){
                strcpy(classExpectation, "Begonia");
            }
            else if (avg >= 50 && avg < 60){
                strcpy(classExpectation, "Castanea");
            }
            else if (avg >= 40 && avg < 50){
                strcpy(classExpectation, "Juniper");
            }
            else if (avg >= 30 && avg < 40){
                strcpy(classExpectation, "Magnolia");
            }
            else if (avg >= 20 && avg < 30){
                strcpy(classExpectation, "MX Intensive 1");
            }
            else if (avg < 20){
                strcpy(classExpectation, "MX Intensive 2");
            }
            out("2026 New Term Expectation: %s\n", classExpectation);
        }
        else {
            out("Only 1 and 2 allowed. Retype this function to try again.\n");
            return;
        }

        out("Do YOU want to write into a file?\n");
        out("Yes - 1\n");
        out("No - Other numbers\n");
        in("", &file2);
        if (file2 == 1){
            out("What file do you want to write into? (example: filename.txt)\n");
            out("NOTE that if you key in an existing file it would not overwrite but instead append it\n");
            in("%127s", file1);
            FILE *fptr = fopen(file1, "a");
            if (fptr == NULL) {
                perror("Error opening file");
                return;
            }

            fprintf(fptr,"----------------------------------\n");
            fprintf(fptr, "User: %s\n", consumer);


            fprintf(fptr, "Math, %.2f\n", math);
            fprintf(fptr, "Physics, %.2f\n", physics);
            fprintf(fptr, "Chemistry, %.2f\n", chemistry);
            fprintf(fptr, "Biology, %.2f\n", biology);
            fprintf(fptr, "History, %.2f\n", history);
            fprintf(fptr, "Geography, %.2f\n", geography);
            fprintf(fptr, "Average, %d\n", avg);
            fprintf(fptr, "2025 Class, %s\n", class2025);
            fprintf(fptr, "2026 Class, %s\n", class2026);
            fprintf(fptr, "2026 Expectation Class, %s\n", classExpectation);

            fprintf(fptr,"----------------------------------\n");


            fclose(fptr);
            out("Writing saved as %s\n", file1);

        }
        
    }


    //date now
    else if (strcmp(cmd, "date") == 0) {
        time(&currentTime); 
        out("%s", ctime(&currentTime));
    }

    //date now - year
    else if (strcmp(cmd, "date-y") == 0) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        out("%d\n", t -> tm_year + 1900);
    }

    else if (strcmp(cmd, "date-m") == 0) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        out("%d\n", t -> tm_mon + 1);
    }

    else if (strcmp(cmd, "date-d") == 0) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        out("%d\n", t -> tm_mday);
    }

    else if (strcmp(cmd, "date-h") == 0) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        out("%d\n", t -> tm_hour);
    }

    else if (strcmp(cmd, "date-min") == 0) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        out("%d\n", t -> tm_min);
    }

    else if (strcmp(cmd, "date-s") == 0) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        out("%d\n", t -> tm_sec);
    }
    
    


    //random stuff
    else if (strcmp(cmd, "random") == 0) {
        int min;
        int max;
        out("What is your minimum range?: ");
        in("", &min);
        out("What is your maximum range?: ");
        in("", &max);
        out("%d\n", rand() % max + min);
    }


    //lemon ASCII art
    else if (strcmp(cmd, "lemon") == 0) {
        out(R"( 
            ..................................................
            ...........:::^^~~~~~~~~~^^::.....................
            .......::^~~~!!!!!!!!!!777777!~^::................
            ...::^~~~~~~~~~~~~!!!!!7777777??77!~:.............
            ..:~~~~~~~~~~~~~~~!!!!7777777777?????!^:..........
            .:^~~~~~~~~~~~~~!!!!!!!777777777?????J??~:........
            .:~~~~~~~~~~~~!~!!!!!77777777777???????JJ7^.......
            ..^~~~~~~~!!!!!!!!7777777777?????????????7~......
            ..:~~~~!!!!!!!77777???77???????????????????7^.....
            ..:^~~!!!!!!777777??????????????????????????~.....
            ...:~~~!!!!!7777?7????????JJJ?JJJJ????????JJ?^....
            ....^~~~!!!!77777777???????JJJJJ?????????J????7:..
            .....:~~!!!!!777777777?????????????????????JJJJ^..
            ......:^~~!!!!!!777777777??????????????????7!!^...
            ........::^~!!!!!7777777777??????????????!:.......
            ...........::^~!!!!777777777???????????!:.........
            )");
    }


    //eggs ASCII art
    else if (strcmp(cmd, "eggs") == 0) {
        out("                              \n"
               "            .::.              \n"
               "           :!~~~~  ...        \n"
               "          :77!~~7~~!~~~:      \n"
               "        ..^7YJ?JJY7!~!7!      \n"
               "       ^~~^^7JJ?7YYJJJ7^      \n"
               "     .:7?7!~~!~:^!?J?!^.      \n"
               "    ..^!?YYJJ7^..::^:..       \n"
               "     .:^~!77!^:...            \n");
    }


    //calc
    else if (strcmp(cmd, "calc") == 0) {
        char op[16];
        double firstnum = 0.0, secondnum = 0.0;
        double output = 0.0;
    
        out("Before proceeding, if u want to calculate roots, the big number is the first number and the root is the second number. Same applies to powers.\n");
        out("NOTE THAT YOUR CASE IF THE SYMBOLS DONT WORK YOU CAN TYPE IN THE SYMBOL'S FIRST 3 LINE (addition = add), except for root, where you just type in 'root'\n");
        out("Enter your case: (+, -, *, /, 'R' for roots, '^' for powers, and percentage symbol for remainders): ");
    
        in("", op);
    
        out("Enter your first number:\n");
        in("", &firstnum);
    
        if (op[0] != '%') {
            out("Enter your second number:\n");
            in("", &secondnum);
        }
    
        if (op[0] == '+' || strcmp(op, "add") == 0) {
            output = firstnum + secondnum;
            out("%f\n", output);
        }
        else if (op[0] == '-' || strcmp(op, "sub") == 0) {
            output = firstnum - secondnum;
            out("%f\n", output);
        }
        else if (op[0] == '*' || strcmp(op, "mul") == 0) {
            output = firstnum * secondnum;
            out("%f\n", output);
        }
        else if (op[0] == '/' || strcmp(op, "div") == 0) {
            if (secondnum != 0.0) {
                output = firstnum / secondnum;
                out("%f\n", output);
            } else {
                out(RED "Error: Division by zero\n" RESET);
            }
        }
        else if (op[0] == '^' || strcmp(op, "pow") == 0) {
            output = pow(firstnum, secondnum);
            out("%f\n", output);
        }
        else if (op[0] == 'R' || strcmp(op, "root") == 0) {
            if (secondnum != 0.0) {
                output = pow(firstnum, 1.0 / secondnum);
                out("%f\n", output);
            } else {
                out(RED "Error: Root degree cannot be zero\n" RESET);
            }
        }
        else if (op[0] == '%' || strcmp(op, "rem") == 0) {
            bool is_odd = fmod(firstnum, 2.0) != 0.0;
            if (!is_odd) {
                out("%.0f is an even number\n", firstnum);
            } else {
                out("%.0f is an odd number\n", firstnum);
            }
        }
        else {
            out(RED "errcode 15 : operator not found\n" RESET);
        }
    }



    //version of calculator function
    else if (strcmp(cmd, "version-calc") == 0){
        out("Calc Function by ZileLai - Version 2.2.0\n");
    }
    


    //spaces
    else if (strcmp(cmd, "space") == 0) {
        filesize("tty1.cpp", "tty1.cpp");
    }
    else if (strcmp(cmd, "space-documents") == 0) {
        filesize("documents", "documents");
    }
    else if (strcmp(cmd, "space-downloads") == 0) {
        filesize("downloads", "downloads");
    }
    else if (strcmp(cmd, "space-home") == 0) {
        filesize("home", "home");
    }
    else if (strcmp(cmd, "space-examples") == 0) {
        filesize("examples", "examples");
    }
    else if (strcmp(cmd, "space-videos") == 0) {
        filesize("videos", "videos");
    }
    else if (strcmp(cmd, "space-pictures") == 0) {
        filesize("pictures", "pictures");
    }
    else if (strcmp(cmd, "space-music") == 0) {
        filesize("music", "music"); 
    }
    else if (strcmp(cmd, "space-system") == 0) {
        filesize("system", "system");
    }
    else if (strcmp(cmd, "space-others") == 0) {
        char input[256];
        out("Prompt out your file/directory: ");
        if (scanf("%255s", input) == 1) {
            filesize(input, input);
        }
    }



    //game1 game
    else if (strcmp(cmd, "game1") == 0){
        int n = rand() % 3;
        char player, bot, result;
        srand(time(NULL));
        
        if(n == 0){
            bot = 's';
        }
        else if (n == 1){
            bot = 'p';
        }

        else{
            bot = 'r';
        }

        

        in("%c", &player);

        result = game(player, bot);

        if (result == -1) {
            out("YOU TIED! WUNDERBAR!\n");
        }
        else if (result == 0) {
            out("YOU WIN!\n");
        }
        else { 
            out("YOU LOST!\n");
        }
        out("You choose : %c and Bot choose : %c\n",player, bot);

    
    }


        //game2 game
    else if (strcmp(cmd, "game2") == 0){
        int n = rand() % 50;
        srand(time(NULL));
        int player;
        int bot;
        in("", &player);
        if (player == bot){
            out("You win! You guessed it\n");
        }
        else {
            out("Guess incorrect, guess again\n");
        }
    }



        //game3 game
    else if (strcmp(cmd, "game3") == 0){
        int input;
        out("Flag Capture\n");
        out("YOUR LOADOUT: 1. Healing Staff, 2. Sniper, 3. BHG, 4. Grenade Launcher\n");
        out("1 to 4 to change loadout\n");
        out("goal: grab the flag and put in your base\n");
        out("someone is attacking you\n");
        out("choose your weapon\n");
        out("he is having a sniper far away\n");

        in("", &input);
        if (input == 1){
            out("You die because of headshot\n");
        }
        else if (input == 2){
            out("Killed opponent, attacked 100 dmg, faced 50 dmg\n");

            out("GO AHEAD\n");
            sleep(100);
            out ("someone is attacking you\n");
            out ("he is having a shotgun\n");
            out("he is near\n");
            in("", &input);
            if (input == 1){
                out ("Killed opponent. Attacked 100dmg, dealt 90 damage.\n" );
                out("GO AHEAD\n");
                sleep(100);
                out("someone is attacking you\n");
                out ("he is having a rpg\n");
                out("he is far away\n");
                in("", &input);

                if (input == 1){
                    out("You die because of spam\n");
                }
                else if (input == 2){
                    out("Killed opponent. Attacked 100dmg, dealt 90 dmg\n");
                    out("GRABBED FLAG!!!\n");
                    sleep(100);
                    out("someone is attacking you\n");
                    out("he is having a BHG\n");
                    in("", &input);
                    if (input == 1){
                        out("You die because of proness\n");
                    }
                    else if (input == 2){
                        out("Killed opponent. Attacked 100dmg, dealt 70dmg\n");
                        out("GOAL ACHIEVED\n");
                        out("sucessfully put flag to your base\n");
                    }
                    else if (input == 3){
                        out("You die because of proness\n");
                    }
                    else if (input == 4){
                        out("You die because of proness\n");
                    }


                }
                else if (input == 3){
                    out("Killed opponent. Attacked 100dmg, dealt 60dmg\n");
                    out("GRABBED FLAG!!!\n");
                    sleep(100);
                    out("someone is attacking you\n");
                    out("he is having a BHG\n");
                    in("", &input);
                    if (input == 1){
                        out("You die because of proness\n");
                    }
                    else if (input == 2){
                        out("Killed opponent. Attacked 100dmg, dealt 70dmg\n");
                        out("GOAL ACHIEVED\n");
                        out("sucessfully put flag to your base\n");
                    }
                    else if (input == 3){
                        out("You die because of proness\n");
                    }
                    else if (input == 4){
                        out("You die because of proness\n");
                    }
                }
                else if (input == 4){
                    out("You die because you killed yourself in the wall\n");
                }

            }
            else if (input == 2){
                out("You die because of spam\n");
            }

            else if (input == 3){
                out("Killed opponent. Attacked 100dmg, dealt 67 damage\n");
                out("GO AHEAD\n");
                sleep(100);
                out("someone is attacking you\n");
                out ("he is having a rpg\n");
                out("he is far away\n");
                in("", &input);

                if (input == 1){
                    out("You die because of spam\n");
                }
                else if (input == 2){
                    out("Killed opponent. Attacked 100dmg, dealt 90 dmg\n");
                    out("GRABBED FLAG!!!\n");
                    sleep(100);
                    out("someone is attacking you\n");
                    out("he is having a BHG\n");
                    in("", &input);
                    if (input == 1){
                        out("You die because of proness\n");
                    }
                    else if (input == 2){
                        out("Killed opponent. Attacked 100dmg, dealt 70dmg\n");
                        out("GOAL ACHIEVED\n");
                        out("sucessfully put flag to your base\n");
                    }
                    else if (input == 3){
                        out("You die because of proness\n");
                    }
                    else if (input == 4){
                        out("You die because of proness\n");
                    }


                }
            }

            else if (input == 4){
                out("You die because of spam \n");
            }

            
        }
        else if (input == 3){
            out ("Killed opponent, attacked 100 dmg, faced 30 dmg\n");
            out("GO AHEAD\n");
            sleep(10);
            out ("someone is attacking you\n");
            out ("he is having a shotgun\n");
            out("he is near\n");
            in("", &input);
            if (input == 1){
                out ("Killed opponent. Attacked 100dmg, dealt 90 damage.\n" );
                out("GO AHEAD\n");
                sleep(10);
                out("someone is attacking you\n");
                out ("he is having a rpg\n");
                out("he is far away\n");
                in("", &input);

                if (input == 1){
                    out("You die because of spam\n");
                }
                else if (input == 2){
                    out("Killed opponent. Attacked 100dmg, dealt 90 dmg\n");
                    out("GRABBED FLAG!!!\n");
                    sleep(10);
                    out("someone is attacking you\n");
                    out("he is having a BHG\n");
                    in("", &input);
                    if (input == 1){
                        out("You die because of proness\n");
                    }
                    else if (input == 2){
                        out("Killed opponent. Attacked 100dmg, dealt 70dmg\n");
                        out("GOAL ACHIEVED\n");
                        out("sucessfully put flag to your base\n");
                    }
                    else if (input == 3){
                        out("You die because of proness\n");
                    }
                    else if (input == 4){
                        out("You die because of proness\n");
                    }


                }
                else if (input == 3){
                    printf("Killed opponent. Attacked 100dmg, dealt 60dmg\n");
                    printf("GRABBED FLAG!!!\n");
                    sleep(10);
                    printf("someone is attacking you\n");
                    printf("he is having a BHG\n");
                    in("", &input);
                    if (input == 1){
                        out("You die because of proness\n");
                    }
                    else if (input == 2){
                        out("Killed opponent. Attacked 100dmg, dealt 70dmg\n");
                        out("GOAL ACHIEVED\n");
                        out("sucessfully put flag to your base\n");
                    }
                    else if (input == 3){
                        out("You die because of proness\n");
                    }
                    else if (input == 4){
                        out("You die because of proness\n");
                    }
                }
                else if (input == 4){
                    out("You die because you killed yourself in the wall\n");
                }

            }
        }

        else if (input == 4){
            out("Killed opponent, attacked 140 dmg, faced 50 dmg\n");

            out("GO AHEAD\n");
            sleep(100);
            out ("someone is attacking you\n");
            out ("he is having a shotgun\n");
            out("he is near\n");
            in("", &input);
            if (input == 1){
                out ("Killed opponent. Attacked 100dmg, dealt 90 damage.\n" );
                out("GO AHEAD\n");
                sleep(10);
                out("someone is attacking you\n");
                out ("he is having a rpg\n");
                out("he is far away\n");
                in("", &input);

                if (input == 1){
                    out("You die because of spam\n");
                }
                else if (input == 2){
                    out("Killed opponent. Attacked 100dmg, dealt 90 dmg\n");
                    out("GRABBED FLAG!!!\n");
                    sleep(10);
                    out("someone is attacking you\n");
                    out("he is having a BHG\n");
                    in("", &input);
                    if (input == 1){
                        out("You die because of proness\n");
                    }
                    else if (input == 2){
                        out("Killed opponent. Attacked 100dmg, dealt 70dmg\n");
                        out("GOAL ACHIEVED\n");
                        out("sucessfully put flag to your base\n");
                    }
                    else if (input == 3){
                        out("You die because of proness\n");
                    }
                    else if (input == 4){
                        out("You die because of proness\n");
                    }


                }
                else if (input == 3){
                    out("Killed opponent. Attacked 100dmg, dealt 60dmg\n");
                    out("GRABBED FLAG!!!\n");
                    sleep(10);
                    out("someone is attacking you\n");
                    out("he is having a BHG\n");
                    in("", &input);
                    if (input == 1){
                        out("You die because of proness\n");
                    }
                    else if (input == 2){
                        out("Killed opponent. Attacked 100dmg, dealt 70dmg\n");
                        out("GOAL ACHIEVED\n");
                        out("sucessfully put flag to your base\n");
                    }
                    else if (input == 3){
                        out("You die because of proness\n");
                    }
                    else if (input == 4){
                        out("You die because of proness\n");
                    }
                }
                else if (input == 4){
                    out("You die because you killed yourself in the wall\n");
                }

            }
        }
        
    }



    //game4 game
    else if (strcmp(cmd, "game4") == 0) {
        int input;
        out("Gun Game\n");
        out("YOUR LOADOUT: 1. Knife, 2. Sniper, 3. Spamming Gun\n");
        out("1 to 3 to change loadout\n");
        out("ROUND START\n");
        out("The person is near to you by 1m\n");
        out("The person is using Rifle\n");
        out("Choose your weapon\n");
        in("%d", &input);
        if (input == 1){
            out("BACKSTAB! attacked: 100 damage, dealt 25 damage\n");
            sleep(5);
            out("ROUND 2\n");
            out("The person is near to you by 10m\n");
            out("The person is using RPG\n");
            out("Choose your weapon\n");
            in("", &input);
            if (input == 1){
                out("You lost lol!!!\n");
            }
            else if (input == 2){
                out("EASY HEADSHOT! attacked: 100 damage, dealt 25 damage\n");
                sleep(5);
                out("MATCH POINT\n");
                out("The person is near to you by 6m\n");
                out("The person is using Uzi\n");
                out("Choose your weapon\n");
                in("%d", &input);
                if (input == 1){
                    out("You lost lol!!!\n");
                }
                else if (input == 2){
                    out("You lost lol!!!\n");
                }
                else if (input == 3){
                    out("EASY TAKEBACK! attacked: 100 damage, dealt 30 damage\n");
                    out("YOU WIN!\n");
                }
            }
            else if (input == 3){
                out("You lost lol!!!\n");
            }
        }
        else if (input == 2){
            out("Killed! attacked: 100 damage, dealt 70 damage\n");
            
            sleep(5);
            out("ROUND 2\n");
            out("The person is near to you by 10m\n");
            out("The person is using RPG\n");
            out("Choose your weapon\n");
            in("", &input);
            if (input == 1){
                out("You lost lol!!!\n");
            }
            else if (input == 2){
                out("EASY HEADSHOT! attacked: 100 damage, dealt 25 damage\n");
                sleep(5);
                out("MATCH POINT\n");
                out("The person is near to you by 6m\n");
                out("The person is using Uzi\n");
                out("Choose your weapon\n");
                in("", &input);
                if (input == 1){
                    out("You lost lol!!!\n");
                }
                else if (input == 2){
                    out("You lost lol!!!\n");
                }
                else if (input == 3){
                    out("EASY TAKEBACK! attacked: 100 damage, dealt 30 damage\n");
                    out("YOU WIN!\n");
                }
            }
            else if (input == 3){
                out("You lost lol!!!\n");
            }
        }
        else if (input == 3){
            out("Killed! attacked: 100 damage, dealt 40 damage\n");
            sleep(5);
            out("ROUND 2\n");
            out("The person is near to you by 10m\n");
            out("The person is using RPG\n");
            out("Choose your weapon\n");
            in("", &input);
            if (input == 1){
                out("You lost lol!!!\n");
            }
            else if (input == 2){
                out("EASY HEADSHOT! attacked: 100 damage, dealt 25 damage\n");
                sleep(5);
                out("MATCH POINT\n");
                out("The person is near to you by 6m\n");
                out("The person is using Uzi\n");
                out("Choose your weapon\n");
                in("", &input);
                if (input == 1){
                    out("You lost lol!!!\n");
                }
                else if (input == 2){
                    out("You lost lol!!!\n");
                }
                else if (input == 3){
                    out("EASY TAKEBACK! attacked: 100 damage, dealt 30 damage\n");
                    out("YOU WIN!\n");
                }
            }
            else if (input == 3){
                out("You lost lol!!!\n");
            }
        }
        
    }



    //dice (0-6)
    else if (strcmp(cmd, "dice") == 0){
        int n = rand() % 6;
        srand(time(NULL));
        out ("%d\n", n);
    }



    //game6 game
    else if (strcmp(cmd, "game6") == 0) {
        srand(time(NULL));
        int n = rand() % 9; 
        float delay_time = 1.0f + ((float)rand() / (float)RAND_MAX) * 2.0f;
        int input;
        usleep((useconds_t)(delay_time * 1000000));
        out("Number: %d\n", n);
        in("", &input);
        if (input == n) {
            out("YOU WIN!\n");
            out("Your time reaction:%f\n", delay_time);
        } 
        else {
            out("YOU LOSE!!\n");
        }
    }


    //luck program
    else if (strcmp(cmd, "luck") == 0){
        int n = rand() % 11;
        srand(time(NULL));
        if (n == 0){
            out("NO LUCK\n");
        }
        else if (n == 1){
            out("NO LUCK\n");
        }
        else if (n == 2){
            out("YOU ARE LUCKY TODAY!!!!\n");
        }
        else if (n == 3){
            out("Somebody told you to sac the queen\n");
        }
        else if (n == 4){
            out("Somebody told you to sac the rook\n");
        }
        else if (n == 5){
            out("If you don't know how to solve a cube, twist the corner and it would automatically solve the cube\n");
        }
        else if (n == 6){
            out("YOU WONT GET 100 KILLS IN FLAG WARS (U SUCK BTW)\n");
        }
        else if (n == 7){
            out("YOU WONT HAVE 4 WINSTREAK IN RIVALS TODAY\n");
        }
        else if (n == 8){
            out("YOU WOULD GET 100 KILLS IN FLAG WARS\n");
        }
        else if (n == 9){
            out("YOU WOULD HAVE 4 WINSTREAK IN RIVALS TODAY\n");
        }
        else if (n == 10){
            out("I use arch btw...\n");
        }
    }



        //game5 game
    else if (strcmp(cmd, "game5") == 0){
        int input;
        srand(time(NULL));
        int bot = rand() % 100;
        in("", &input);
        if (input == bot){
            out("YOU LOSE. BOT's ANSWER IS SAME AS YOURS\n");
            out("BOT:%d\n", bot);
            out("BOT:%d\n", input);
            
        }
        else{
            out("YOU WIN. BOT's ANSWER ISNT SAME AS YOURS\n");
            out("BOT:%d\n", bot);
            out("YOU:%d\n", input);
            
        }
    }



        //game7 game
    else if (strcmp(cmd, "game7") == 0){
        int input;
        out("You are a country\n");
        out("You want to start a war\n");
        out("There are other countries surrounding you\n");
        out("Your strenght: 100\n");
        out("Other countries: \n");
        out("Country 1: 70 strenght, Country 2: 400 strenght, Country 3: 342 strenght, Country 4: 34 strenght, Country 5: 343 strenght, Country 6: 101 strenght\n");
        out("Which one do you choose? ");
        in("", &input);
        if (input == 1){
            out("YOU FIGHT WITH THEM!!! AND YOU GAINED TERRITORY\n");
            out("YOUR STRENGHT: 120\n");
            out("There are other countries surrounding you\n");
            out("Your strenght: 100\n");
            out("Other countries: \n");
            out("Country 2: 400 strenght, Country 3: 342 strenght, Country 5: 343 strenght, Country 6: 101 strenght\n");
            out("Which one do you choose? ");
            in("", &input);
            if (input == 2){
                out("YOU FIGHT WITH THEM!!! AND YOU DIED AND GET CONQUERED\n");
            }
            else if (input == 3){
                out("YOU FIGHT WITH THEM!!! AND YOU DIED AND GET CONQUERED\n");
            }
            else if (input == 5){
                out("YOU FIGHT WITH THEM!!! AND YOU DIED AND GET CONQUERED\n");
            }
            else if (input == 6){
                out("YOU FIGHT WITH THEM!!! AND YOU GAINED TERRITORY\n");
                out("YOUR STRENGHT: 350\n");
                out("There are other countries surrounding you\n");
  
                out("Other countries: \n");
                out("Country 2: 400 strenght, Country 3: 342 strenght, Country 5: 343 strenght\n");
                out("Which one do you choose? ");
                in("", &input);
                if (input == 2){
                    out("YOU FIGHT WITH THEM!!! AND YOU DIED AND GET CONQUERED\n");
                }
                else if (input == 3){
                    out("YOU FIGHT WITH THEM!!! AND YOU GAINED TERRITORY\n");
                    out("YOUR STRENGHT: 500\n");
                    out("There is only one country surrounding you\n");
                    
                    out("Other countries: \n");
                    out("Country 2: 400 strenght\n");
                    out("Which one do you choose? ");
                    in("", &input);
                }
                else if (input == 5){
                    out("YOU FIGHT WITH THEM!!! AND YOU GAINED TERRITORY\n");
                    out("YOUR STRENGHT: 499\n");
                    out("There is only one country surrounding you\n");
                    
                    out("Other countries: \n");
                    out("Country 2: 400 strenght, Country 3: 342 strenght, Country 5: 343 strenght\n");
                    out("Which one do you choose? ");
                    in("", &input);
                    if (input == 2){
                        out("YOU FIGHT WITH THEM!!! AND YOU GAINED TERRITORY\n");
                        out("YOU WIN!!\n");
                    }
                }
            }
        }
        else if (input == 2){
            out("YOU FIGHT WITH THEM!!! AND YOU DIED AND GET CONQUERED\n");
        }
        else if (input == 3){
            out("YOU FIGHT WITH THEM!!! AND YOU DIED AND GET CONQUERED\n");
        }
        else if (input == 4){
            out("YOU FIGHT WITH THEM!!! AND YOU GAINED TERRITORY\n");
            out("YOUR STRENGHT: 110\n");
            out("There are other countries surrounding you\n");
     
            out("Other countries: \n");
            out("Country 2: 400 strenght, Country 3: 342 strenght, Country 5: 343 strenght, Country 6: 101 strenght\n");
            out("Which one do you choose? ");
            in("%d", &input);
            if (input == 2){
                out("YOU FIGHT WITH THEM!!! AND YOU DIED AND GET CONQUERED\n");
            }
            else if (input == 3){
                out("YOU FIGHT WITH THEM!!! AND YOU DIED AND GET CONQUERED\n");
            }
            else if (input == 5){
                out("YOU FIGHT WITH THEM!!! AND YOU DIED AND GET CONQUERED\n");
            }
            else if (input == 6){
                out("YOU FIGHT WITH THEM!!! AND YOU GAINED TERRITORY\n");
                out("YOUR STRENGHT: 350\n");
                out("There are other countries surrounding you\n");
  
                out("Other countries: \n");
                out("Country 2: 400 strenght, Country 3: 342 strenght, Country 5: 343 strenght\n");
                out("Which one do you choose? ");
                in("", &input);
                if (input == 2){
                    out("YOU FIGHT WITH THEM!!! AND YOU DIED AND GET CONQUERED\n");
                }
                else if (input == 3){
                    out("YOU FIGHT WITH THEM!!! AND YOU GAINED TERRITORY\n");
                    out("YOUR STRENGHT: 500\n");
                    out("There is only one country surrounding you\n");
                    
                    out("Other countries: \n");
                    out("Country 2: 400 strenght\n");
                    out("Which one do you choose? ");
                    in("", &input);
                }
                else if (input == 5){
                    out("YOU FIGHT WITH THEM!!! AND YOU GAINED TERRITORY\n");
                    out("YOUR STRENGHT: 499\n");
                    out("There is only one country surrounding you\n");
                    
                    out("Other countries: \n");
                    out("Country 2: 400 strenght, Country 3: 342 strenght, Country 5: 343 strenght\n");
                    out("Which one do you choose? ");
                    in("", &input);
                    if (input == 2){
                        out("YOU FIGHT WITH THEM!!! AND YOU GAINED TERRITORY\n");
                        out("YOU WIN!!\n");
                    }
                }
            }
        }
        else if (input == 5){
            out("YOU FIGHT WITH THEM!!! AND YOU DIED AND GET CONQUERED\n");
        }
        else if (input == 6){
            out("YOU FIGHT WITH THEM!!! AND YOU DIED AND GET CONQUERED\n");
            
        }
        else {
            out("Invalid input, so the guard kick you out\n");
        }

    }




    //elements program

    else if (strcmp(cmd, "elements") == 0){
        srand(time(NULL));
        int n = rand() % 119;
        if (n == 1){out("Hydrogen");}
        else if (n == 2){out("Helium");}
        else if (n == 3){out("Lithium");}
        else if (n == 4){out("Beryllium");}
        else if (n == 5){out("Boron");}
        else if (n == 6){out("Carbon");}
        else if (n == 7){out("Nitrogen");}
        else if (n == 8){out("Oxygen");}
        else if (n == 9){out("Fluorine");}
        else if (n == 10){out("Neon");}
        else if (n == 11){out("Sodium");}
        else if (n == 12){out("Magnesium");}
        else if (n == 13){out("Aluminium");}
        else if (n == 14){out("Silicon");}
        else if (n == 15){out("Phosphorus");}
        else if (n == 16){out("Sulfur");}
        else if (n == 17){out("Chlorine");}
        else if (n == 18){out("Argon");}
        else if (n == 19){out("Potassium");}
        else if (n == 20){out("Calcium");}
        else if (n == 21){out("Scandium");}
        else if (n == 22){out("Titanium");}
        else if (n == 23){out("Vanadium");}
        else if (n == 24){out("Chromium");}
        else if (n == 25){out("Manganese");}
        else if (n == 26){out("Iron");}
        else if (n == 27){out("Cobalt");}
        else if (n == 28){out("Nickel");}
        else if (n == 29){out("Copper");}
        else if (n == 30){out("Zinc");}
        else if (n == 31){out("Gallium");}
        else if (n == 32){out("Germanium");}
        else if (n == 33){out("Arsenic");}
        else if (n == 34){out("Selenium");}
        else if (n == 35){out("Bromine");}
        else if (n == 36){out("Krypton");}
        else if (n == 37){out("Rubidium");}
        else if (n == 38){out("Strontium");}
        else if (n == 39){out("Yttrium");}
        else if (n == 40){out("Zirconium");}
        else if (n == 41){out("Niobidium");}
        else if (n == 42){out("Molybdenum");}
        else if (n == 43){out("Technetium");}
        else if (n == 44){out("Rutbenium");}
        else if (n == 45){out("Rhodium");}
        else if (n == 46){out("Palladium");}
        else if (n == 47){out("Silver");}
        else if (n == 48){out("Cadmium");}
        else if (n == 49){out("Indium");}
        else if (n == 50){out("Tin");}
        else if (n == 51){out("Antimony");}
        else if (n == 52){out("Tellurium");}
        else if (n == 53){out("Iodine");}
        else if (n == 54){out("Xenon");}
        else if (n == 55){out("Caesium");}
        else if (n == 56){out("Barium");}
        else if (n == 57){out("Lanthanium");}
        else if (n == 58){out("Cerium");}
        else if (n == 59){out("Praseodynium");}
        else if (n == 60){out("Neodynium");}
        else if (n == 61){out("Promethium");}
        else if (n == 62){out("Samarium");}
        else if (n == 63){out("Europium");}
        else if (n == 64){out("Gadolinium");}
        else if (n == 65){out("Terbium");}
        else if (n == 66){out("Dysprosium");}
        else if (n == 67){out("Holonium");}
        else if (n == 68){out("Erbium");}
        else if (n == 69){out("Thulium");}
        else if (n == 70){out("Yttbium");}
        else if (n == 71){out("Lutetium");}
        else if (n == 72){out("Hafnium");}
        else if (n == 73){out("Tantalum");}
        else if (n == 74){out("Tungsten");}
        else if (n == 75){out("Rhenium");}
        else if (n == 76){out("Osmium");}
        else if (n == 77){out("Iridium");}
        else if (n == 78){out("Platinium");}
        else if (n == 79){out("Gold");}
        else if (n == 80){out("Mercury");}
        else if (n == 81){out("Thallium");}
        else if (n == 82){out("Lead");}
        else if (n == 83){out("Bismuth");}
        else if (n == 84){out("Polonium");}
        else if (n == 85){out("Astatine");}
        else if (n == 86){out("Radon");}
        else if (n == 87){out("Francium");}
        else if (n == 88){out("Radium");}
        else if (n == 89){out("Actinium");}
        else if (n == 90){out("Thorium");}
        else if (n == 91){out("Protactinium");}
        else if (n == 92){out("Uranium");}
        else if (n == 93){out("Neptunium");}
        else if (n == 94){out("Plutonium");}
        else if (n == 95){out("Americium");}
        else if (n == 96){out("Curinum");}
        else if (n == 97){out("Berkelium");}
        else if (n == 98){out("Californium");}
        else if (n == 99){out("Einsteinium");}
        else if (n == 100){out("Fermium");}
        else if (n == 101){out("Mendelevium");}
        else if (n == 102){out("Nobelium");}
        else if (n == 103){out("Lawrencium");}
        else if (n == 104){out("Rutherforium");}
        else if (n == 105){out("Dubnium");}
        else if (n == 106){out("Seaborgium");}
        else if (n == 107){out("Seaborgium");}
        else if (n == 108){out("Hassium");}
        else if (n == 109){out("Meitnerium");}
        else if (n == 110){out("Damstadtium");}
        else if (n == 111){out("Roentgenium");}
        else if (n == 112){out("Copemicium");}
        else if (n == 113){out("Nihonium");}
        else if (n == 114){out("Flerovium");}
        else if (n == 115){out("Moscovium");}
        else if (n == 116){out("Livermorium");}
        else if (n == 117){out("Tenessine");}
        else if (n == 118){out("Oganesson");}

        out("\n");
    }



    //bool program
    else if (strcmp(cmd, "bool") == 0){
        srand(time(NULL));
        int n = rand() % 2;
        out("%d\n", n);
    }


    //image program

    else if (strcmp(cmd, "image") == 0) { 
        char chars[] = "`^\",:;Il!i~+_-?][}(1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao#MW&8%B@S";
        int charslen = strlen(chars);

        char input[128]; 
        int width, height, pixel;
    
        out("Type what file do you want to show in ASCII?\n");
    
        if (scanf("%127s", input) != 1) {
            out("errcode 17 : input failed to read\n");
            return; 
        }
        

        unsigned char *ImageData = stbi_load(input, &width, &height, &pixel, 0);
    
        if (ImageData) {
            int stepX = 4;  
            int stepY = 8;  
    
            for (int rownum = 0; rownum < height; rownum += stepY) 
            {
                for (int colnum = 0; colnum < width; colnum += stepX) 
                {
                    int pixelOffset = (rownum * width + colnum) * pixel;
                    unsigned char *pixelpointer = ImageData + pixelOffset;
                    
                    unsigned char R = pixelpointer[0];
                    unsigned char G = pixelpointer[1];
                    unsigned char B = pixelpointer[2];
                    
                    float Avg = (R + G + B) / 3.0f; 
                    int charnum = (int)(charslen * (Avg / 255.0f));
                    if (charnum >= charslen) charnum = charslen - 1;
                    
                    putchar(chars[charnum]);
                    putchar(chars[charnum]); 
                }
                putchar('\n');
            }
            stbi_image_free(ImageData);
    
        } else {
            out("errcode 18 : image loading failed, %s\n", stbi_failure_reason());
        }
    }



    //phrase program
    else if (strcmp(cmd, "phrase") == 0) {
        FILE *fptr = fopen("./text.text", "r");
        if (!fptr) {
            return ;
        }

        char **lines = (char **)malloc(MAX * sizeof(char *));
        int count = 0;
        char buffer[MAX];

        while(fgets(buffer, sizeof(buffer), fptr) && count < MAX){
            buffer[strcspn(buffer, "\n")] = '\0';
            lines[count] = strdup(buffer);
            count++;
        }
        fclose(fptr);
        

        if (count == 0){
            free(lines);
            return;
        }

        unsigned int seed;
        FILE *urandom = fopen("/dev/urandom", "r");

        if (urandom){
            if (fread(&seed, sizeof(seed), 1, urandom) != 1){
                seed = time(NULL);
            }
            fclose(urandom);
        }
        else{
            seed = time(NULL);
        }
        srand(seed);

        int target = (count < 20) ? count : 20;

        for (int i = 0; i < target; i++){
            int j = i + rand() % (count - i);
            char *temp = lines[i];
            lines[i] = lines[j];
            lines[j] = temp;
        }

        for (int i = 0; i < target; i++){
            out("%s", lines[i]);
            if(i < target - 1){
                out(" ");
            }
        }
        out("\n");

        for (int i = 0; i < count; i++){
            free(lines[i]);
        }

        free(lines);
    }



    //words program

    else if (strcmp(cmd, "words") == 0) {
        FILE *fptr = fopen("./text.text", "r");
        if (!fptr) {
            return ;
        }

        char **lines = (char **)malloc(MAX * sizeof(char *));
        int count = 0;
        char buffer[MAX];

        while(fgets(buffer, sizeof(buffer), fptr) && count < MAX){
            buffer[strcspn(buffer, "\n")] = '\0';
            lines[count] = strdup(buffer);
            count++;
        }
        fclose(fptr);
        

        if (count == 0){
            free(lines);
            return;
        }

        unsigned int seed;
        FILE *urandom = fopen("/dev/urandom", "r");

        if (urandom){
            if (fread(&seed, sizeof(seed), 1, urandom) != 1){
                seed = time(NULL);
            }
            fclose(urandom);
        }
        else{
            seed = time(NULL);
        }
        srand(seed);

        int target = (count < 1) ? count : 1;

        for (int i = 0; i < target; i++){
            int j = i + rand() % (count - i);
            char *temp = lines[i];
            lines[i] = lines[j];
            lines[j] = temp;
        }

        for (int i = 0; i < target; i++){
            out("%s", lines[i]);
            if(i < target - 1){
                out(" ");
            }
        }
        out("\n");

        for (int i = 0; i < count; i++){
            free(lines[i]);
        }

        free(lines);
    }


    //talktoseal english

    else if (strcmp(cmd, "talktoseal-human") == 0) {

        char input [128];

        out("What do you want to express to Seal ? ");

        in ("", input);

        out("Seal's Expression: ");


        FILE *fptr = fopen("./text.text", "r");
        if (!fptr) {
            return ;
        }

        char **lines = (char **)malloc(MAX * sizeof(char *));
        int count = 0;
        char buffer[MAX];

        while(fgets(buffer, sizeof(buffer), fptr) && count < MAX){
            buffer[strcspn(buffer, "\n")] = '\0';
            lines[count] = strdup(buffer);
            count++;
        }
        fclose(fptr);
        

        if (count == 0){
            free(lines);
            return;
        }

        unsigned int seed;
        FILE *urandom = fopen("/dev/urandom", "r");

        if (urandom){
            if (fread(&seed, sizeof(seed), 1, urandom) != 1){
                seed = time(NULL);
            }
            fclose(urandom);
        }
        else{
            seed = time(NULL);
        }
        srand(seed);

        int target = (count < 32) ? count : 32;

        for (int i = 0; i < target; i++){
            int j = i + rand() % (count - i);
            char *temp = lines[i];
            lines[i] = lines[j];
            lines[j] = temp;
        }

        for (int i = 0; i < target; i++){
            out("%s", lines[i]);
            if(i < target - 1){
                out(" ");
            }
        }
        out("\n");

        for (int i = 0; i < count; i++){
            free(lines[i]);
        }

        free(lines);
    }




    //talktoseal seal

    else if (strcmp(cmd, "talktoseal") == 0) {

        char input [128];

        out("What do you want to express to Seal ?");

        in ("", input);

        out("Seal's Expression: ");


        FILE *fptr = fopen("./seal.text", "r");
        if (!fptr) {
            return ;
        }

        char **lines = (char **)malloc(MAX * sizeof(char *));
        int count = 0;
        char buffer[MAX];

        while(fgets(buffer, sizeof(buffer), fptr) && count < MAX){
            buffer[strcspn(buffer, "\n")] = '\0';
            lines[count] = strdup(buffer);
            count++;
        }
        fclose(fptr);
        

        if (count == 0){
            free(lines);
            return;
        }

        unsigned int seed;
        FILE *urandom = fopen("/dev/urandom", "r");

        if (urandom){
            if (fread(&seed, sizeof(seed), 1, urandom) != 1){
                seed = time(NULL);
            }
            fclose(urandom);
        }
        else{
            seed = time(NULL);
        }
        srand(seed);

        int target = (count < 32) ? count : 32;

        for (int i = 0; i < target; i++){
            int j = i + rand() % (count - i);
            char *temp = lines[i];
            lines[i] = lines[j];
            lines[j] = temp;
        }

        for (int i = 0; i < target; i++){
            out("%s", lines[i]);
            if(i < target - 1){
                out(" ");
            }
        }
        out("\n");

        for (int i = 0; i < count; i++){
            free(lines[i]);
        }

        free(lines);
    }


    //essay program

    else if (strcmp(cmd, "essay") == 0) {
        FILE *fptr = fopen("./text.text", "r");
        if (!fptr) {
            return ;
        }

        char **lines = (char **)malloc(MAX * sizeof(char *));
        int count = 0;
        char buffer[MAX];

        while(fgets(buffer, sizeof(buffer), fptr) && count < MAX){
            buffer[strcspn(buffer, "\n")] = '\0';
            lines[count] = strdup(buffer);
            count++;
        }
        fclose(fptr);
        

        if (count == 0){
            free(lines);
            return;
        }

        unsigned int seed;
        FILE *urandom = fopen("/dev/urandom", "r");

        if (urandom){
            if (fread(&seed, sizeof(seed), 1, urandom) != 1){
                seed = time(NULL);
            }
            fclose(urandom);
        }
        else{
            seed = time(NULL);
        }
        srand(seed);

        int target = (count < 5000) ? count : 5000;

        for (int i = 0; i < target; i++){
            int j = i + rand() % (count - i);
            char *temp = lines[i];
            lines[i] = lines[j];
            lines[j] = temp;
        }

        for (int i = 0; i < target; i++){
            out("%s", lines[i]);
            if(i < target - 1){
                out(" ");
            }
        }
        out("\n");

        for (int i = 0; i < count; i++){
            free(lines[i]);
        }

        free(lines);
    }


    //users program
    else if (strcmp(cmd, "users") == 0) {
        char userpath[512];
        
        out("What user do you want to create and go into?\n");
        if (scanf("%127s", users) == 1) {
            snprintf(userpath, sizeof(userpath), "%s/%s", oripath, users);
    
            if (strcmp(memuser, "seal") == 0) {
                mkdir(userpath); 
                chdir(userpath); 
            }
            
            out("you are now logged in as %s\n", users);
            mkdir(userpath);
            chdir(userpath);
            strcpy(memuser, users); 
        }
        else {
            out("errcode 19 : user input failed\n");
        }
    }


    //users go back to seal user
    
    else if (strcmp(cmd, "users-seal") == 0) {
        char userpath[512];
        snprintf(userpath, sizeof(userpath), "%s/%s", oripath, memuser);
        if (rmdir(userpath) == 0) {
            out("Successfully removed user directory: %s\n", memuser);
        } else {
            out("Note: Could not remove directory (it may not exist or isn't empty).\n");
        }
        chdir("/home");
        strcpy(memuser, "guest"); 
    }


    //users delete function

    else if (strcmp(cmd, "users-remove") == 0) {
        if (parsed_args < 2) {
            out("errcode 3: file not provided\n");
        } 
        
        else {
            if (strcmp(arg1, "tty1.cpp") == 0 || 
                strcmp(arg1, "tty2.cpp") == 0 || 
                strcmp(arg1, "qubabasdwiaisd.txt") == 0 || 
                strcmp(arg1, "compress.h") == 0 || 
                strcmp(arg1, "decompress.h") == 0 || 
                strcmp(arg1, "stb_image.h") == 0 || 
                strcmp(arg1, "zlio.h") == 0 || 
                strcmp(arg1, "root") == 0 || 
                strcmp(arg1, "text.text") == 0 || 
                strcmp(arg1, "seal.text") == 0 || 
                strcmp(arg1, "bootloader") == 0 || 
                strcmp(arg1, "tty1") == 0 || 
                strcmp(arg1, "tty2") == 0) {
                
                out("errcode 24: access denied: doing such process is very dangerous\n");
            } 
            else {
                char target_path[128];
                get_current_path(loc, arg1, target_path);
    

                if (rmrf(target_path) == 0) {
                    if (strcmp(notes_name, arg1) == 0) {
                        strcpy(notes_name, "notes.txt");
                        notes_mode = 0;
                    }
                    out("changed\n");
                } else {

                    perror("errcode 3: remove failed");
                }
            }
        }
    }
    

    //who am i???

    else if (strcmp(cmd, "whoami") == 0) {
        out("%s\n", users);
    }

    //available users

    else if (strcmp(cmd, "available") == 0){
        out("root, ");
        out("seal, ");
        if (strcmp(users, "seal") == 0){
            out("\n");
        }
        else{
            out("%s \n", users);
        }
    }


    //list down available users

    else if (strcmp(cmd, "available-t") == 0){
        out("root - sudo power\n");
        out("seal - sudo power\n");
        if (strcmp(users, "seal") == 0){
            out("\n");
        }
        else{
            out("%s - normal user\n", users);
        }
    }

    //comp
    else if (strcmp(cmd, "comp") == 0){
        char input[256];
        char output[256];
        out("What file you want to compress? ");
        in("", input);
        out("What file do you want to compressed file go to (need to type .rle)? ");
        out("", output);
        compressor(input, output);
    }




    //decomp
    else if (strcmp(cmd, "decomp") == 0)
    {
        
        char input[256];
        char output[256];
        out("What file you want to decompress (need to type .rle)? ");
        in("", input);
        out("What file do you want to decompressed file go to? ");
        in("", output);
        decompress(input, output);
    }



//pkgmgr
    else if (strcmp(cmd, "pkgmgr") == 0) {
        char maincmd[32] = {0};
        char subcmd[32] = {0};
        char target[512] = {0};
    

        int count = sscanf(input, "%31s %31s %511s", maincmd, subcmd, target);
    
        
        if (strcmp(subcmd, "add") == 0) {
            #ifdef _WIN32
                char command[600];
                snprintf(command, sizeof(command), "curl -O \"%s\"", target);
                int status = system(command);
            #else
                char command[600];
                snprintf(command, sizeof(command), "curl -sLO \"%s\"", target);
                int status = system(command);
            #endif
    
            if (status == 0) {
            } else {
                out("errcode 20: internet connection lost (prob)\n");
            }
        } 

        else if (strcmp(subcmd, "clone") == 0) {
            
            char command[600];
            snprintf(command, sizeof(command), "git clone \"%s\"", target);
            
            int status = system(command);
            if (status == 0) {
            } else {
                out("errcode 21: git clone failed\n");
            }
        } 
    
        else if (strcmp(subcmd, "build") == 0) {
            
            char command[700];
            snprintf(command, sizeof(command), 
                     "cd \"%s\" && mkdir -p build && cd build && cmake .. && cmake --build .", target);
            
            int status = system(command);
            if (status == 0) {
                
            } else {
                out("errcode 22: cmake not installed or have problem\n");
            }
        } 
    }




    //browser
    else if (strcmp(cmd, "browser") == 0) {
        char url[128];
        char command[256];
    
        out("What website do you want to go to? ");
        fflush(stdout); 
    
        if (scanf("%127s", url) != 1) {
            return;
        }

        snprintf(command, sizeof(command), "curl -s -L '%s' | sed 's/<[^>]*>//g'", url);
    
        FILE *fp = popen(command, "r");
        if (!fp) {
            perror(RED "errcode 23 : failed to run curl" RESET);
            return;
        }
    
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            printf("%s", buffer);
        }
    
        pclose(fp);
    }



    //webip-a
    else if (strcmp(cmd, "webip-a") == 0) {
        char command[256];

        snprintf(command, sizeof(command), "curl -s -L 'http://ifconfig.me/' | sed 's/<[^>]*>//g'");
    
        FILE *fp = popen(command, "r");
        if (!fp) {
            perror(RED "errcode 23 : failed to find your ip adress" RESET);
            return;
        }
    
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            printf("%s", buffer);
        }
    
        pclose(fp);
    }


    //clear
    else if (strcmp(cmd, "clear") == 0){
        out("\033[H\033[J");
    }


    //game8 game
    else if (strcmp(cmd, "game8") == 0){
        srand(time(NULL));
        int guess = rand() % 23;
        char answer [128];
        out("Guess the Lamp\n");
        if (guess == 1){
            out("It is made by schreder, contains lots of types but mainly uses SON/HPS. Finally, it is the highest watts inside one if the most popular schreder lamps. ");
            in("", answer);
            if (strcmp(answer, "Schreder_Z3")== 0){
                out("Correct!!!\n");
            }
            else{
                out("try again bro\n");
            }
        }

        else if (guess == 2){
            out("Made by Thorn and I am the version 8 of a series");
            in("", answer);
            if (strcmp(answer, "Thorn_Alpha_8")== 0){
                out("Correct!!!\n");
            }
            else{
                out("try again bro\n");
            }
        }

        else if (guess == 3){
            out("Structure similar to Thorn Alpha 8, made by Philips. ");
            in("", answer);
            if (strcmp(answer, "SGS_304")== 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }

        else if (guess == 4){
            out("Another Schreder big hit and it is in Federal Highway. ");
            in("", answer);

            if (strcmp(answer, "Schreder_DZ60")== 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }

        else if (guess == 5){
            out("Schreder's big hit but instead of the highest watts it has the lowest watts. ");
            in("", answer);
            if (strcmp(answer, "Schreder_Z1") == 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }

        else if (guess == 6){
            out("Similar to schreder z2 but it has lower watts than z2 but similar to z1. ");
            in("", answer);
            if (strcmp(answer, "Schreder_Z18")== 0){
                out("Correct!!!\n");
            }
            else{
                out("try again bro\n");
            }
        }

        else if (guess == 7){
            out("Malaysia made lamp similar to Lunoida. ");
            in("", answer);
            if (strcmp(answer, "Nikkon_S419")== 0){
                out("Correct!!!\n");
            }
            else{
                out("try again bro\n");
            }
        }

        else if (guess == 8){
            out("Curved HPS lamp. ");
            in("", answer);
            if (strcmp(answer, "Lunoida")== 0){
                out("Correct!!!\n");
            }
            else {
                out("try again bro\n");
            }
        }

        else if (guess == 9){
            out("First schreder lamp. ");
            in("", answer);
            if (strcmp(answer, "Schreder_PQ")== 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }
        
        else if (guess == 10) {
            out("Schreder lamp that looks like Saturn. ");
            in("", answer);
            if (strcmp(answer, "Schreder_Saturn")== 0){
                out("Correct!!\n");
            }

            else{
                out("try again bro\n");
            }


        }

        else if (guess == 11){
            out("Made by Thorn and also can be seen in Salak Highway (maybe). ");
            in("", answer);
            if (strcmp(answer, "Thorn_Alpha_3")== 0){
                out ("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }

        else if (guess == 12){
            out("One of the thinnest street light that Thorn has ever made. ");

            in("", answer);
            if (strcmp(answer, "Thorn_Alpha_4")== 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }
        else if (guess == 13){
            out("Schreder lamp similar to Thorn Alpha 8 and SGS 203. ");
            in("", answer);
            if (strcmp(answer, "Schreder_MC")== 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }

        else if (guess == 14){
            out("A Philips street lamp and only have HPS and Mercury. ");
            in("", answer);
            if (strcmp(answer, "SGS_203")== 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }

        else if (guess == 0){
            out("A Schreder lamp that is really similar to SGS201. It's the only lamp in this game that has Cosmopolis and it is seal safe. ");
            in("", answer);
            if (strcmp(answer, "Schreder_Sapphire")== 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }

        else if (guess == 15){
            out("What is SOX? ");
            in("", answer);
            if (strcmp(answer, "Low_Pressure_Sodium")== 0 || strcmp(answer, "LPS")== 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }

        else if (guess == 16){
            out("What is SON? ");
            in("", answer);
            if (strcmp(answer, "HPS")== 0 || strcmp(answer, "High_Pressure_Sodium")== 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }

        else if (guess == 17){
            out("Which company added X in the word SOX? ");
            in("", answer);
            if (strcmp(answer, "Philips")== 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }

        else if (guess == 18){
            out("Which company created the trademark 'SON'");
            in("", answer);
            if (strcmp(answer, "Philips")== 0 || strcmp(answer, "Thorn_Lightning") == 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }

        else if (guess == 19){
            out("When was HPS/SON/SVL invented");
            in("", answer);
            if (strcmp(answer, "1963") == 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }

        else if (guess == 20){
            out("When SON/HPS lamp fires up, what colour would it glow?");
            in("", answer);
            if (strcmp(answer, "red") == 0 || strcmp(answer, "Red") == 0 || strcmp(answer, "Pink") == 0 || strcmp(answer, "pink") == 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }

        else if (guess == 21){
            out("What is the CRI Index of HPS? ");
            in("", answer);

            if (strcmp(answer, "20-30") == 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }

        else if (guess == 22){
            out("What is the CRI Index of LPS? ");
            in("", answer);

            if (strcmp(answer, "0-20") == 0){
                out("Correct!!!\n");
            }

            else{
                out("try again bro\n");
            }
        }
    }
//Guns: 

//Primary

//Sniper : 1
//Crossbow : 2

//Secondary

//Daggers : 1
//Revolver : 2

//Melee

//Scythe : 1
//Knife : 2

//Utilities

//MedKit : 1
//Jump Pad : 2



    //game9 game
    else if (strcmp(cmd, "game9") == 0){
        int primary;
        int secondary;
        int melee;
        int utility;
        int input;
        int input2;
        out("Welcome to sniping game\n");
        out("Pick your primary weapons: \n");
        out("Available primary: Sniper (1), Crossbow (2)\n");
        out("Pick your weapons by typing it with gun codes: ");
        in("", &primary);
        if (primary == 1){
            out("You picked sniper\n");
        }
        else if (primary == 2){
            out("You picked crossbow\n");
        }
        out("Pick your secondary weapons: \n");
        out("Available secondary: Daggers (1), Revolver (2)\n");
        out("Pick your weapons by typing it with gun codes: ");
        in("", &secondary);
        if (primary == 1){
            out("You picked daggers\n");
        }
        else if (primary == 2){
            out("You picked revolver\n");
        }
        out("Pick your melee: \n");
        out("Available melees: Scythe (1), Knife (2)\n");
        out("Pick your weapons by typing it with melee codes: ");
        in("", &melee);
        if (primary == 1){
            out("You picked scythe\n");
        }
        else if (primary == 2){
            out("You picked knife\n");
        }
        out("Pick your utility: \n");
        out("Available utilities: Medkit (1), Jump Pad (2)\n");
        out("Pick your weapons by typing it with utility codes: ");
        in("", &utility);
        if (primary == 1){
            out("You picked medkit\n");
        }
        else if (primary == 2){
            out("You picked jump pad\n");
        }
        out("So your main codes are: %d, %d, %d, %d,\n", primary, secondary, melee, utility);

        out("\n");
        out("ROUND 1/2\n");
        out("Your opponent placed a jump pad but still visible to you\n");
        out("What type of weapon do you equip? ");
        in("", &input);
        if (input == 1){
            if (input == 1){
                sleep(1);
                out("Headshot! Opponent had died\n");
                out("ROUND 2/2\n");
                out("Your opponent sniping you from far away\n");
                out("What type of weapon do you equip? ");
                in("", &input);
                if (input == 1){
                    sleep(1);
                    out("Headshot! Opponent had died\n");
                }
                else if (input == 2){
                    sleep(30);
                    out("What are you doing? Secondary is for close range. YOU DIED\n");
                }
                else if (input == 3){
                    sleep(10);
                    out("What are you doing? Melee is for close range. YOU DIED\n");
                }
                else if (input == 4){
                    sleep(4);
                    out("Worst decision BTW. YOU DIED\n");
                }
            }
            else if (input == 2){
                sleep(10);
                out("Headshot! Opponent had died\n");
            }
        }
        else if (input == 2){
            if (input == 1){
                sleep(30);
                out("It has been rough... You lost 70 of your HP, but you killed him!\n");
            }
            else if (input == 2){
                sleep(20);
                out("It has been rough... You lost 40 of your HP, but you killed him!\n");
            }
        }

        else if (input == 3){
            if (input == 1){
                sleep(30);
                out("You dashed but your opponent found and killed you. YOU ARE OUT OF THE MATCH!\n");
            }
            else if (input == 2){
                sleep(10);
                out("You sneak attack and backstabbed him! Good work!!!\n");
            }
        }

        else if (input == 4){
            if (input == 1){
                sleep(30);
                out("Worst decision ever yet!!! Now opponent found you and sniped. YOU ARE OUT OF THE MATCH\n");
            }
            else if (input == 2){
                sleep(10);
                out("Worst decision ever yet!!! Now opponent found you and sniped. YOU ARE OUT OF THE MATCH\n");
            }
        }

    }



        //game10 game
    else if (strcmp(cmd, "game10") == 0){
        int r1 = rand() % 11;
        int r2 = rand() % 11;
        int r3 = rand() % 11;
        int r4 = rand() % 11;
        int r5 = rand() % 11;
        int input;

        out("avoid the chosen number (from 0 to 10)\n");
        out("Guess a number: ");
        in("", &input);
        if (input == r1){
            out("you are out.\n");
        }
        else{
            out("Guess a number: ");
            in("", &input);
            if (input == r2 || input == r1){
                out("you are out.\n");
            }
            else{
                out("Guess a number: ");
                in("", &input);
                if (input == r3 || input == r2 || input == r1){
                    out("you are out.\n");
                }
                else{
                    out("Guess a number: ");
                    in("", &input);
                    if (input == r4 || input == r3 || input == r2 || input == r1){
                        out("you are out.\n");
                    }
                    else{
                        out("Guess a number: ");
                        in("", &input);
                        if (input == r5 || input == r4 || input == r3 || input == r2 || input == r1){
                            out("you are out.\n");
                        }
                        else{
                            out("You pass all round.\n");
                        }
                    }
                }
            }
        }


    }


    //zlio version
    else if (strcmp(cmd, "version-zlio") == 0){
        out("ZLIO Library - Version 2.1.0\n");
    }

    //sudd version
    else if (strcmp(cmd, "version-sudd") == 0){
        out("SealKernel User Directory Design (SUDD) - Update 3\n");
    }



    //see your os 
    else if (strcmp(cmd, "system") == 0){
        tagged();
    }


    //history

    else if (strcmp(cmd, "history") == 0){
        FILE *fptr;
        fptr = fopen("qubabasdwiaisd.txt", "r");

        char history [100];

        if (fptr != NULL){
            while(fgets(history, 100, fptr)){
                out("%s", history);
            }
        }

        fclose(fptr);

        out("\n");
    }



    //clear history

    else if (strcmp(cmd, "clear-history") == 0){
        FILE *fptr;
        fptr = fopen("qubabasdwiaisd.txt", "w");

        fprintf(fptr, "");
        fclose(fptr);
    }



    //wc
    else if (strcmp(cmd, "wc") == 0) {
        if (parsed_args < 2) {
            out(RED "errcode 3: file not provided\n" RESET);
        } else {
            if (strcmp(arg1, notes_name) == 0 && notes_mode == 0) {
                out(RED "errcode 9: permission denied\n" RESET);
            } else {
                char read_path[128];
                get_current_path(loc, arg1, read_path);
                FILE *rf = fopen(read_path, "r");
                if (rf) {
                    int count = 0;
                    int inword = 0;
                    int ch;
    
                    while ((ch = fgetc(rf)) != EOF) {
                        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
                            inword = 0;
                        } else if (!inword) {
                            inword = 1;
                            count++;
                        }
                    }
                    fclose(rf);
    
                    char outbuffer[64];
                    snprintf(outbuffer, sizeof(outbuffer), "%d\n", count);
                    out(outbuffer);
                } else {
                    out(RED "errcode 3: file not provided\n" RESET);
                }
            }
        }
    }


    //cc
    else if (strcmp(cmd, "cc") == 0) {
        FILE *fptr;
        fptr = fopen(arg1, "r");
        
        if (fptr != nullptr) { 
            char buffer[100]; 
            
            if (std::fgets(buffer, sizeof(buffer), fptr) != nullptr) {
                std::string line_buffer(buffer);
                
                size_t len = line_buffer.length(); 

                std::cout << len << std::endl;
            }
            
            fclose(fptr); 
        }
    }
    else if (strcmp(cmd, "find") == 0) {
        std::ifstream file(arg1);
        if (!file.is_open()) {
            out(RED);
            std::cout << "errcode 3: file not provided" << arg1 << std::endl;
            out(RESET);
            return; 
        }
    
        out("What do you want to find inside %s ? ", arg1);
        std::string search_target;
        std::cin >> search_target;
    
        std::string line;
        int line_number = 1;
        bool found = false;
    
        while (std::getline(file, line)) {
            size_t char_pos = line.find(search_target);

            if (char_pos != std::string::npos) {
                std::cout << "Line: " << line_number 
                          << ", At: " << char_pos << std::endl;
                found = true;
            }
            line_number++;
        }
    
        if (!found) {
            out(RED);
            std::cout << "errcode25: the word you search for doesn't exist in this file" << std::endl;
            out(RESET);
        }
       
    }
    

    //varlab / game12

    else if (strcmp(cmd, "varlab") == 0 || strcmp(cmd, "game12") == 0) {
        int input;
        int input2;
        out("Welcome to Variable Labs!\n");
        out("Choose one variable here:\n");
        
        do {
            out("1. Integer\n");
            out("2. Long Integer\n");
            out("3. Floating Point\n");
            out("4. Double Floating Point\n");
            in("", &input);
        } while(input < 1 || input > 4); 
    
        if (input == 1) {
            int num1;
            out("Type a number (integer): ");
            in("", &num1);
            out("%d\n", num1);
            out("Now choose a different variable: \n");
            
            do {
                out("2. Long Integer\n");
                out("3. Floating Point\n");
                out("4. Double Floating Point\n");
                in("", &input2);
            } while(input2 < 2 || input2 > 4);
            
            if (input2 == 2) {
                long num2 = (long)num1;
                out("%ld\n", num2);
            } else if (input2 == 3) {
                float num2 = (float)num1;
                out("%f\n", num2);
            } else if (input2 == 4) {
                double num2 = (double)num1;
                out("%lf\n", num2);
            }
        }
        else if (input == 2) {
            long num1;
            out("Type a number (long integer): ");
            std::cin >> num1;
            out("%ld\n", num1);
            out("Now choose a different variable: \n");
            
            do {
                out("1. Integer\n");
                out("3. Floating Point\n");
                out("4. Double Floating Point\n");
                in("", &input2);
            } while(input2 != 1 && input2 != 3 && input2 != 4);
            
            if (input2 == 1) { int num2 = (int)num1; out("%d\n", num2); }
            else if (input2 == 3) { float num2 = (float)num1; out("%f\n", num2); }
            else if (input2 == 4) { double num2 = (double)num1; out("%lf\n", num2); }
        }
        else if (input == 3) {
            float num1;
            out("Type a number (floating point): "); 
            std::cin >> num1;
            out("%f\n", num1); 
            out("Now choose a different variable: \n");
            
            do {
                out("1. Integer\n");
                out("2. Long Integer\n");
                out("4. Double Floating Point\n");
                in("", &input2);
            } while(input2 != 1 && input2 != 2 && input2 != 4);
            
            if (input2 == 1){ 
                int num2 = (int)num1; out("%d\n", num2); 
            }
            else if (input2 == 2) { 
                long num2 = (long)num1; out("%ld\n", num2); 
            }
            else if (input2 == 4) { 
                double num2 = (double)num1; out("%lf\n", num2); 
            }
        }
        else if (input == 4) {
            double num1;
            out("Type a number (double floating point): ");
            in("", &num1);
            out("%lf\n", num1);
            out("Now choose a different variable: \n");
            
            do {
                out("1. Integer\n");
                out("2. Long Integer\n");
                out("3. Floating Point\n");
                in("", &input2);
            } while(input2 < 1 || input2 > 3);
            
            if (input2 == 1) { 
                int num2 = (int)num1; out("Converted: %d\n", num2); 
            }
            else if (input2 == 2) { 
                long num2 = (long)num1; out("Converted: %ld\n", num2); 
            }
            else if (input2 == 3) {
                float num2 = (float)num1; out("Converted: %f\n", num2); 
            }
        }
    }
    
    
//game11

    else if (strcmp(cmd, "game11") == 0){
        char one[128];
        char two[128];
        char three[128];
        char four[128];
        char five[128];
        char six[128];
        char seven[128];
        char eight[128];
        char nine[128];
        char ten[128];
        char eleven[128];
        char twelve[128];
        out("Madlibs Game!!\n");
        out("By Rubixmaster123 on CodePad, written in Python\n");
        out("This version in SealKernel was written in C with ZLIO functions\n");
        out("Now, Do the following input: \n");
        out("Name: ");
        in("", one);
        out("Animal: ");
        in("", two);
        out("Colour: ");
        in("", three);
        out("Verb that is a past tense (ed ending): ");
        in("", four);
        out("A noun: ");
        in("", five);
        out("A type of building: ");
        in("", six);
        out("Location in %s: ", six);
        in("", seven);
        out("A verb that is present or past continous (ing ending): ");
        in("", eight);
        out("Biome: ");
        in("", nine);
        out("Animal: ");
        in("", ten);
        out("Name: ");
        in("", eleven);
        out("Food: ");
        in("", twelve);

        out("So you and your friend %s are going on an adventure. You start your adventure by a near by forest, where you find a %s wandering around. For some reason the %s suddenly turned %s! How weird! The %s %s came up to you and %s, then suddenly %s away. You and %s wanted to investigate, so you followed the weird %s when suddenly you tripped over a %s. When you got up, you saw the %s go into a %s, and when you and %s got into the %s you saw the %s go into the %s. You and %s went into the %s and saw the %s %s outside through a open window! You and %s crawled through the open window and the %s was running towards a %s. You and %s followed it into the %s and saw it, but then something weird happened. The %s suddenly trasformed into a %s! suddenly the %s that used to be a %s spoke in english and said it's name is %s. Suddenly, with a snap of it's fingers, %s made %s appear out of nowhere and then started eating it! What a weird adventure!\n",one, two, two, three, three, two, one, four, one, two, five, two, six, one, six, two, seven, one, seven, two, eight, one, two, nine, one, nine, two, ten, ten, two, eleven, eleven, twelve);

    }

//1, 2, 2, 3, 3, 2, 1, 4, 1, 2, 5, 2, 6, 1, 6, 2, 7, 1, 7, 2, 8, 1, 2, 9, 1, 9, 2, 10, 10, 2, 11, 11, 12
//name
//animal
//colour
//ed
//noun
//building
//6
//ing
//biome
//animal
//name
//food
//So you and your friend {one} are going on an adventure. You start your adventure by a near by forest, where you find a {two} wandering around. For some reason the {two} suddenly turned {three}! How wierd! The {three} {two} came up to you and {one}, then suddenly {four} away. You and {one} wanted to investigate, so you followed the wierd {two} when suddenly you tripped over a {five}. When you got up, you saw the {two} go into a {six}, and when you and {one} got into the {six} you saw the {two} go into the {seven}. You and {one} went into the {seven} and saw the {two} {eight} outside through a open window! You and {one} crawled through the open window and the {two} was running towards a {nine}. You and {one} followed it into the {nine} and saw it, but then something wierd happened. The {two} suddenly trasformed into a {ten}! suddenly the {ten} that used to be a {two} spoke in english and said it's name is {eleven}. Suddenly, with a snap of it's fingers, {eleven} made {twelve} appear out of nowhere and then started eating it! What a wierd adventure!




    //memory adress stuff

    else if (strcmp(cmd, "memcode") == 0){
        std::string input;
        
        
        out("Type something... ");
        std::cin >> input;
        
        std::string *pointer = &input;
        
        std::cout << pointer << std::endl; 
    }


    //strcode
    else if (strcmp(cmd, "strcode") == 0){
        int input;
        int *ptr = &input;


        out("Type something... ");
        in("", &input);
    
        char addrstr[32]; 
        snprintf(addrstr, sizeof(addrstr), "%p", (void*)ptr);

        out("%s\n", addrstr);

    }


    //unit function
    else if (strcmp(cmd, "unit") == 0){
        int input;
        double unit;
        double unit2;
        out("This is the unit program\n");
        out("It is used to change unit\n");
        out("Now select what unit changer you want\n");
        do {
            out("1. cm to m\n");
            out("2. mm to cm\n");
            out("3. kg to g\n");
            out("4. m to km\n");
            out("5. F to C\n");
            out("6. ml to l\n");
            out("7. m to cm\n");
            out("8. cm to mm\n");
            out("9. g to kg\n");
            out("10. km to m\n");
            out("11. C to F\n");
            out("12. l to ml\n");
            in("", &input);
        } while(input < 1 || input > 12);

        if(input == 1){
            out("type your value in cm: ");
            in("", &unit);
            unit2 = unit / 100;
            printf("%lf", unit2);
        }
        else if(input == 2){
            out("type your value in mm: ");
            in("", &unit);
            unit2 = unit / 10;
            printf("%lf", unit2);
        }
        else if(input == 3){
            out("type your value in kg: ");
            in("", &unit);
            unit2 = unit * 1000;
            printf("%lf", unit2);
        }
        else if(input == 4){
            out("type your value in m: ");
            in("", &unit);
            unit2 = unit / 1000;
            printf("%lf", unit2);
        }
        else if(input == 5){
            out("type your value in F: ");
            in("", &unit);
            unit2 = (unit - 32) * (5.0 / 9.0);
            printf("%lf", unit2);
        }

        //out("1. cm to m\n");
        //out("2. mm to cm\n");
        //out("3. kg to g\n");
        //out("4. m to km\n");
        //out("5. F to C\n");
        //out("6. ml to l\n");
        //out("7. m to cm\n");
        //out("8. cm to mm\n");
        //out("9. g to kg\n");
        //out("10. km to m\n");
        //out("11. C to F\n");
        //out("12. l to ml\n");
        else if(input == 6){
            out("type your value in ml: ");
            in("", &unit);
            unit2 = unit / 1000;
            printf("%lf", unit2);
        }
        else if(input == 7){
            out("type your value in m: ");
            in("", &unit);
            unit2 = unit * 100;
            printf("%lf", unit2);
        }
        else if(input == 8){
            out("type your value in cm: ");
            in("", &unit);
            unit2 = unit * 10;
            printf("%lf", unit2);
        }
        else if(input == 9){
            out("type your value in g: ");
            in("", &unit);
            unit2 = unit / 1000;
            printf("%lf", unit2);
        }
        else if(input == 10){
            out("type your value in km: ");
            in("", &unit);
            unit2 = unit * 1000;
            printf("%lf", unit2);
        }
        else if(input == 11){
            out("type your value in C: ");
            in("", &unit);
            unit2 = (unit * 9.0 / 5.0) + 32;
            printf("%lf", unit2);
        }
        else if(input == 12){
            out("type your value in l: ");
            in("", &unit);
            unit2 = unit * 1000;
            printf("%lf", unit2);
        }

    }

    //comments
    else if (strcmp(cmd, "*") == 0){}


    //cursor changer
    else if (strcmp(cmd, "cursor") == 0){
        in("Type your cursor (only 1 character): ", &cursor);
        out("changed\n");
    }


    //view last command
    else if (strcmp(cmd, "!!") == 0){
        FILE *fptr;

        fptr = fopen("qubabasdwiaisd.txt", "r");

        if(fptr == NULL){
            out(RED "errcode 3: file not provided\n" RESET);
        }

        char buffer[MAX];
        char lastline [MAX] = "";

        while(fgets(buffer, sizeof(buffer), fptr) != NULL){
            snprintf(lastline, sizeof(lastline), "%s", buffer);
        }

        out(GREEN "%s", buffer);
        out(RESET);

    }

    //similar to bool program


    else if (strcmp(cmd, "chooser") == 0){
        int chooser = rand() % 2;
        srand(time(NULL));
        char opt1 [124];
        char opt2 [124];
        in("Give your first option: ", opt1);

        in("Give your second option: ", opt2);

        if(chooser == 1){
            out("%s\n", opt2);
        }

        else if (chooser == 0){
            out("%s\n", opt1);
        }
    }
    
    

    
//exit program
    
    else if (strcmp(cmd, "sudo-exit") == 0) {
        if (superior == 1){exit(0);}
        else {out(RED "errcode 2 : sudo user undetected, switch to sudo user\n" RESET);}
        
    }

    else {
        out(RED "errcode 1 : command '%s' not found\n", cmd);
        out(RESET);
    }


    //where it saves command to history
    FILE *fptr = fopen("qubabasdwiaisd.txt", "a+");
    if (fptr != nullptr) {
        fprintf(fptr, "%s %s\n", cmd, arg1);
        fclose(fptr);
    } 
    else {
    }

}


//main function

int main() {
    system("clear");
    srand(time(NULL));
    original_path();
    char input[100];
    FILE *file;

    file = fopen("system/sys.txt", "w");
    if (file != NULL) {
        fprintf(file, "https://codepad.app/pad/822052z5n");
        fclose(file);
    }
    out(YELLOW "%s\n", version);
    out("A project by ZileLai\n");
    out("ZL Projects' Website : https://zilelai.lab26.my/\n");
    out("if don't know any command, use 'help'\n" RESET);


    
    char root[512]; 

    snprintf(root, sizeof(root), "%s/root", oripath);

    if (mkdir(root) == 0) {} 
    else {
        if (errno == EEXIST) {} 
        else {}
    }

    

    while (1){
        if (strcmp(users, "seal") == 0 && superior == 0) {
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                if (cursor == '$') {
                    out(BLUE "%s $ ", cwd);
                } 
                
                else {
                    out(BLUE "%s %c ", cwd, cursor);
                }
            }
        } 
        
        else if (strcmp(users, "seal") != 0 && superior == 0) {
            if (cursor == '$') {
                out(BLUE "%s $ ", cwd);
            } 
            
            else {
                out(BLUE "%s %c ", cwd, cursor);
            }
        } 
        
        else if (superior == 1) {
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                out(BLUE "%s $ ", cwd);
            }
        }
    
        out("%s", RESET);
        fflush(stdout);
    
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        
        if (strcmp(input, "curl\n") == 0) {
            #ifdef LIBCURL_AVAILABLE
                out("running curl...\n");
                break;
            #else
                out("err: curl not supported here\n");
                continue;
            #endif
        }
        
        process_system_command(input);
        
        fflush(stdout);
    }


    #ifdef LIBCURL_AVAILABLE
        CURL *curl;
        CURLcode res;
        char *response_string = malloc(1); 
        if (response_string == NULL) return 1;
        response_string[0] = '\0';

        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init(); 

        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, "https://google.com");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

            res = curl_easy_perform(curl);

            if(res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            } else {
                out("%s\n", response_string);
            }

            curl_easy_cleanup(curl);
        }
        
        free(response_string);
        curl_global_cleanup();
    #endif

    return 0;
}
