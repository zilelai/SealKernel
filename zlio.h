#ifndef ZLIO_H
#define ZLIO_H

#include <stdarg.h>
#include <unistd.h>
#include <stdbool.h> 

#ifdef __cplusplus
extern "C" {
#endif

int out(const char *format, ...);

typedef enum {
    INPUTINT,
    INPUTDOUBLE,
    INPUTBOOL,
    INPUTSTR
} InputType;

typedef struct {
    InputType type;
    union {
        int intvalue;
        double doublevalue;
        bool boolvalue;
        char stringvalue[256];
    } data;
} InputValue;

InputValue inraw(const char *prompt);

void inint(const char *prompt, int *out_val);
void indouble(const char *prompt, double *out_val);
void inbool(const char *prompt, bool *out_val);
void instr(const char *prompt, char *out_val);


double math(const char *expr);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

inline void in(const char *prompt, int *val) { inint(prompt, val); }
inline void in(const char *prompt, double *val) { indouble(prompt, val); }
inline void in(const char *prompt, bool *val) { inbool(prompt, val); }
inline void in(const char *prompt, char *val) { instr(prompt, val); }

#else

#define in(prompt, var) _Generic((var), \
    int*: inint, \
    double*: indouble, \
    bool*: inbool, \
    char*: instr \
)(prompt, var)

#endif

#endif
