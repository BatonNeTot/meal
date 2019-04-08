#include "meal/print.h"

#include "meal/platform.h"
#include "meal/macros.h"
#include "meal/math.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>


static const char *lowerChars = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char *upperChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

#define ARRAY_INT_BUFFER 65

static char bufferArray[ARRAY_INT_BUFFER] = {'\0'};

#define CHECK_RADIX(radix) \
if (radix > 36 || radix < 1)\
    return NULL;\

#define UTOS(val, radix, isUpper, lengthPtr)\
char *buffer = bufferArray + ARRAY_INT_BUFFER;\
uint32_t length = 0;\
const char *chars = (const char *)TERN(isUpper, (WST)upperChars, (WST)lowerChars);\
\
do {\
    buffer--;\
    *buffer = chars[val % radix];\
    length++;\
    val /= radix;\
} while (val);\
\
if (lengthPtr)\
    *lengthPtr = length;\
\
return buffer;

const char *itos(int64_t val, uint32_t radix, bool isUpper, uint32_t *lengthPtr) {
    CHECK_RADIX(radix)

    const int64_t mask = val >> 63;
    val += mask;
    val ^= mask;

    UTOS(val, radix, isUpper, lengthPtr)
}

const char *utos(uint64_t val, uint32_t radix, bool isUpper, uint32_t *lengthPtr) {
    CHECK_RADIX(radix)

    UTOS(val, radix, isUpper, lengthPtr)
}

static char *dtos(double val, int ndigit, int *dec, int *sign) {
    return fcvt(val, ndigit, dec, sign);
}

typedef struct {
    unsigned isSpecial:1;

    unsigned hasMinus:1;
    unsigned hasPlus:1;
    unsigned hasSpace:1;
    unsigned hasSharp:1;
    unsigned hasZero:1;
    unsigned hasDot:1;

    unsigned hasWidth:1;
    unsigned hasPrecision:1;

    char letter;

    uint32_t width;
    uint32_t precision;
} printInfo;

#define MY_WRITER(buffer,count) do {\
                const int32_t result = writer(buffer, count, data);\
                if (result == -1)\
                    return -1;\
                totalCount += result;\
} while (0)

static inline int printArg(printInfo *info, writer_f writer, void *data, va_list *argsPtr) {
    int32_t totalCount = 0;
    const char *buffer = NULL;

    //Some data

    struct {
        uint32_t strLength;
        uint32_t fullLength;
        int sign;

        union {
            struct {
                uint32_t radix;
                bool isUpper;
            } i;
            struct {
                uint32_t dotOffset;
                uint32_t numbOffset;
                bool isNumber;
            } f;
        };
    } arg = {};

    union {
        int i;
        unsigned int u;
        double f;
        void *p;
    } value;

    //Getting value
    switch (info->letter) {
        case 'i':case 'I':
        case 'd':case 'D':
        case 'c':{
            value.i = va_arg(*argsPtr, int);
            break;
        }
        case 'b':case 'B':
        case 'u':case 'U':
        case 'o':case 'O':
        case 'x':case 'X':{
            value.u = va_arg(*argsPtr, unsigned int);
            break;
        }
        case 'p':case 'P':
        case 's':{
            value.p = va_arg(*argsPtr, void *);
            break;
        }
        case 'f':case 'F':{
            value.f = va_arg(*argsPtr, double);
            break;
        }
        default:break;
    }

    //Setup radix
    switch (info->letter) {
        case 'b':case 'B':{
            arg.i.radix = 2;
            break;
        }
        case 'o':case 'O':{
            arg.i.radix = 8;
            break;
        }
        case 'i':case 'I':
        case 'd':case 'D':
        case 'u':case 'U':{
            arg.i.radix = 10;
            break;
        }
        case 'x':case 'X':
        case 'p':case 'P':{
            arg.i.radix = 16;
            break;
        }
        default:break;
    }

    //Setup upperCase
    switch (info->letter) {
        case 'i':case 'd':
        case 'b':case 'u':
        case 'o':case 'x':
        case 'p':{
            arg.i.isUpper = 0;
            break;
        }
        case 'I':case 'D':
        case 'B':case 'U':
        case 'O':case 'X':
        case 'P':{
            arg.i.isUpper = 1;
            break;
        }
        default:break;
    }

    //Format
    switch (info->letter) {
        case 'i':case 'I':
        case 'd':case 'D':{
            buffer = itos(value.i, arg.i.radix, arg.i.isUpper, &arg.fullLength);
            break;
        }
        case 'b':case 'B':
        case 'u':case 'U':
        case 'o':case 'O':
        case 'x':case 'X':{
            buffer = utos(value.u, arg.i.radix, arg.i.isUpper, &arg.fullLength);
            break;
        }
        case 'p':case 'P':{
            buffer = utos((uint32_t)value.p, arg.i.radix, arg.i.isUpper, &arg.fullLength);
            break;
        }
        case 'f':case 'F':{
            if (isinf(value.f)) {
                buffer = "Inf";
                arg.fullLength = 3;
                arg.f.isNumber = 0;
            } else if (isnan(value.f)) {
                buffer = "NaN";
                arg.fullLength = 3;
                arg.f.isNumber = 0;
            } else {
                const int precision = TERN(info->hasDot, info->precision, 6);
                int dotPos;
                buffer = dtos(value.f, precision, &dotPos, &arg.sign);

                arg.f.dotOffset = (uint32_t) IF(dotPos > 0, dotPos);
                arg.f.numbOffset = (uint32_t) IF(dotPos <= 0, 1 - dotPos);
                arg.strLength = (uint32_t) (precision + dotPos);

                if (info->hasSharp) {
                    const char *done = buffer + arg.f.dotOffset;
                    const char *tail = buffer + arg.strLength;
                    while (done != tail && tail[-1] == '0') {
                        tail--;
                    }
                    arg.strLength = tail - buffer;
                }

                arg.fullLength = (value.f < 0 || info->hasPlus || info->hasSpace) +
                               IF(arg.strLength > 0, arg.f.numbOffset + arg.strLength) +
                               IF(arg.f.dotOffset < arg.f.numbOffset + arg.strLength, 1);
                arg.f.isNumber = 1;
            }
            break;
        }
        case 'c':{
            static char character;
            character = (char) value.i;
            buffer = &character;
            arg.fullLength = 1;
            break;
        }
        case 's': {
            buffer = value.p;

            if (buffer) {
                arg.fullLength = strlen(buffer);
                arg.fullLength -= IF(info->hasDot && info->precision < arg.fullLength, arg.fullLength - info->precision);
            } else {
                buffer = "(null)";
                arg.fullLength = MIN(6u, info->precision);
            }

            break;
        }
        default:break;
    }

    //After Format
    switch (info->letter) {
        case 'i':case 'I':
        case 'd':case 'D':{
            arg.strLength = arg.fullLength;
            arg.sign = 0;
            arg.fullLength += IF(info->precision > arg.fullLength, info->precision - arg.fullLength);
            break;
        }
        case 'b':case 'B':
        case 'u':case 'U':
        case 'o':case 'O':
        case 'x':case 'X':
        case 'p':case 'P':{
            arg.strLength = arg.fullLength;
            arg.sign = value.i < 0;
            arg.fullLength += IF(info->precision > arg.fullLength, info->precision - arg.fullLength);
            break;
        }
        default:break;
    }

    //After after Format
    switch (info->letter) {
        case 'i':case 'I':
        case 'd':case 'D':{
            arg.fullLength += (value.i < 0 || info->hasPlus || info->hasSpace);
            break;
        }
        case 'o':case 'O':{
            arg.fullLength += IF(info->hasSharp, 1);
            break;
        }
        case 'b':case 'B':
        case 'u':case 'U':
        case 'x':case 'X':
        case 'p':case 'P':{
            arg.fullLength += IF(info->hasSharp, 2);
            break;
        }
        default:break;
    }

    //Printing before indent
    switch (info->letter) {
        case 'i':case 'I':
        case 'd':case 'D':
        case 'b':case 'B':
        case 'u':case 'U':
        case 'o':case 'O':
        case 'x':case 'X':
        case 'p':case 'P': {
            if (!info->hasMinus && info->width > arg.fullLength) {
                const char *space = (const char *) TERNP(info->hasZero && !info->hasDot, "0", " ");
                do {
                    MY_WRITER(space, 1);
                    info->width--;
                } while (info->width > arg.fullLength);
            }
            break;
        }
        case 'f':case 'F':
        case 'c':case 's': {
            if (!info->hasMinus && info->width > arg.fullLength) {
                const char *space = (const char *) TERNP(info->hasZero, "0", " ");
                do {
                    MY_WRITER(space, 1);
                    info->width--;
                } while (info->width > arg.fullLength);
            }
            break;
        }
        default:break;
    }

    //Printing sign or header
    switch (info->letter) {
        case 'i':case 'I':
        case 'd':case 'D':
        case 'f':case 'F': {
            if (arg.sign) {
                MY_WRITER("-", 1);
            } else if (info->hasPlus) {
                MY_WRITER("+", 1);
            } else if (info->hasSpace) {
                MY_WRITER(" ", 1);
            }
            break;
        }
        default:break;
    }

    if (info->hasSharp) {
        switch (info->letter) {
            case 'b':case 'B': {
                MY_WRITER("0b", 2);
                break;
            }
            case 'u':case 'U': {
                MY_WRITER("0d", 2);
                break;
            }
            case 'o':case 'O': {
                MY_WRITER("0", 1);
                break;
            }
            case 'x':case 'X':
            case 'p':case 'P': {
                MY_WRITER("0x", 2);
                break;
            }
            default:break;
        }
    }

    //Printing "0" indent
    switch(info->letter) {
        case 'i':case 'I':
        case 'd':case 'D':
        case 'b':case 'B':
        case 'u':case 'U':
        case 'o':case 'O':
        case 'x':case 'X':
        case 'p':case 'P': {
            while (info->precision > arg.strLength) {
                MY_WRITER("0", 1);
                info->precision--;
            }
            break;
        }
        default:break;
    }

    //Printing body
    switch(info->letter) {
        case 'i':case 'I':
        case 'd':case 'D':
        case 'b':case 'B':
        case 'u':case 'U':
        case 'o':case 'O':
        case 'x':case 'X':
        case 'p':case 'P':{
            MY_WRITER(buffer, arg.strLength);
            break;
        }
        case 'c':case 's': {
            MY_WRITER(buffer, arg.fullLength);
            break;
        }
        case 'f':case 'F':{
            if (arg.f.isNumber) {
                if (arg.f.numbOffset || !arg.strLength) {
                    MY_WRITER("0", 1);
                } else {
                    MY_WRITER(buffer, arg.f.dotOffset);
                }

                if (arg.f.dotOffset >= arg.f.numbOffset + arg.strLength || (!arg.strLength && info->hasSharp))
                    break;

                arg.f.numbOffset -= IF(arg.f.numbOffset > 0, 1);
                MY_WRITER(".", 1);

                while (arg.f.numbOffset--) {
                    MY_WRITER("0", 1);
                }

                MY_WRITER(buffer + arg.f.dotOffset, arg.strLength - arg.f.dotOffset);
            } else {
                MY_WRITER(buffer, arg.fullLength);
            }
            break;
        }
        default:break;
    }

    //Printing after indent
    if (info->hasMinus && info->width > arg.fullLength) {
        do {
            MY_WRITER(" ", 1);
            info->width--;
        } while (info->width > arg.fullLength);
    }

    return totalCount;
}

int wprintv(writer_f writer, void *data, const char *frmt, va_list args) {
    size_t totalCount = 0;

    const char *carret = frmt;

    const printInfo defFlags = {
            isSpecial: 0,

            hasZero: 0,
            hasDot: 0,
            hasWidth: 0,
            hasPrecision: 0,

            width: 0,
            precision: 0
    };

    printInfo flags = defFlags;

    while (*frmt != '\0') {
        if (!flags.isSpecial) {
            if (*frmt == '%') {
                uint32_t diff = frmt - carret;
                MY_WRITER(carret, diff);
                carret = frmt;

                flags.isSpecial = 1;
            }
        } else {
            switch (*frmt) {
                case '-': {
                    if (flags.hasDot || flags.hasMinus)
                        flags = defFlags;
                    else
                        flags.hasMinus = 1;
                    break;
                }
                case '+': {
                    if (flags.hasDot || flags.hasPlus)
                        flags = defFlags;
                    else
                        flags.hasPlus = 1;
                    break;
                }
                case ' ': {
                    if (flags.hasDot || flags.hasSpace)
                        flags = defFlags;
                    else
                        flags.hasSpace = 1;
                    break;
                }
                case '#': {
                    if (flags.hasDot || flags.hasSharp)
                        flags = defFlags;
                    else
                        flags.hasSharp = 1;
                    break;
                }
                case '0': {
                    if (!flags.hasDot && !flags.hasZero) {
                        flags.hasZero = 1;
                        break;
                    }
                }
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9': {
                    if (flags.hasDot) {
                        if (flags.hasPrecision) {
                            flags = defFlags;
                        } else {
                            char *next;
                            flags.precision = strtoul(frmt, &next, 10);
                            flags.hasPrecision = 1;
                            frmt = next - 1;
                        }
                    } else {
                        if (flags.hasWidth) {
                            flags = defFlags;
                        } else {
                            char *next;
                            flags.width = strtoul(frmt, &next, 10);
                            flags.hasWidth = 1;
                            frmt = next - 1;
                        }
                    }
                    break;
                }
                case '.': {
                    if (flags.hasDot) {
                        flags = defFlags;
                        break;
                    }
                    flags.hasDot = 1;
                    break;
                }
                case '*': {
                    if (flags.hasDot) {
                        if (flags.hasPrecision) {
                            flags = defFlags;
                        } else {
                            flags.precision = va_arg(args, unsigned long);
                            flags.hasPrecision = 1;
                        }
                    } else {
                        if (flags.hasWidth) {
                            flags = defFlags;
                        } else {
                            flags.width = va_arg(args, unsigned long);
                            flags.hasWidth = 1;
                        }
                    }
                    break;
                }
                case 'i':case 'I':
                case 'd':case 'D':
                case 'b':case 'B':
                case 'u':case 'U':
                case 'o':case 'O':
                case 'x':case 'X':
                case 'p':case 'P':
                case 'f':case 'F':
                case 'c':case 's': {
                    flags.letter = *frmt;
                    int result = printArg(&flags, writer, data, &args);
                    if (result == -1)
                        return -1;
                    totalCount += result;
                    carret = frmt + 1;
                    flags = defFlags;
                    break;
                }
                case 'n': {
                    int *ptr = va_arg(args, int *);
                    *ptr = totalCount;
                    carret = frmt + 1;
                    flags = defFlags;
                    break;
                }
                case '%': {
                    MY_WRITER("%", 1);
                    carret = frmt + 1;
                    flags = defFlags;
                    break;
                }
                default:break;
            }
        }
        frmt++;
    }
    MY_WRITER(carret, frmt - carret);
    return totalCount;
}

static int32_t writeToStream(const char *buffer, uint32_t count, void *data) {
    FILE *stream = data;
    return fwrite(buffer, sizeof(char), count, stream);
}

int32_t fprintv(FILE *stream, const char *frmt, va_list args) {
    if (!stream)
        return -1;
    return wprintv(writeToStream, stream, frmt, args);
}

static int32_t writeToStr(const char *buffer, uint32_t count, void *data) {
    char *str = data;
    strncpy(str, buffer, count);
    return count;
}

int32_t sprintv(char *str, const char *frmt, va_list args) {
    if (!str)
        return -1;
    return wprintv(writeToStr, str, frmt, args);
}

typedef struct {
    char *buffer;
    size_t size;
} bufferInfo;

static int32_t writeToBuffer(const char *buffer, uint32_t count, void *data) {
    bufferInfo *target = data;

    size_t minSize = MIN(target->size, count);
    if (minSize == 0)
        return 0;

    strncpy(target->buffer, buffer, minSize);
    target->size -= minSize;
    return minSize;
}

int32_t snprintv(char *buffer, uint32_t size, const char *frmt, va_list args) {
    if (!buffer)
        return -1;
    if (size == 0)
        return 0;
    bufferInfo info = {buffer: buffer, size: size};
    return wprintv(writeToBuffer, &info, frmt, args);
}

int32_t wprint(writer_f writer, void *data, const char *frmt, ...) {
    if (!writer || !frmt)
        return -1;
    va_list args;
    va_start(args, frmt);
    const int32_t result = wprintv(writer, data, frmt, args);
    va_end(args);
    return result;
}

int32_t fprint(FILE *stream, const char *frmt, ...) {
    if (!stream)
        return -1;
    va_list args;
    va_start(args, frmt);
    const int32_t result = fprintv(stream, frmt, args);
    va_end(args);
    return result;
}

int32_t sprint(char *str, const char *frmt, ...) {
    if (!str)
        return -1;
    va_list args;
    va_start(args, frmt);
    const int32_t result = sprintv(str, frmt, args);
    va_end(args);
    return result;
}

int32_t snprint(char *buffer, uint32_t size, const char *frmt, ...) {
    if (!buffer)
        return -1;
    if (size == 0)
        return 0;
    va_list args;
    va_start(args, frmt);
    const int32_t result = snprintv(buffer, size, frmt, args);
    va_end(args);
    return result;
}

