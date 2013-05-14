#ifndef ISLPP_ISLPP_COMMON_H
#define ISLPP_ISLPP_COMMON_H

#ifdef __GNUC__
// Ignore #pragma region
//TODO:  #pragma GCC diagnostic pop
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif


// http://groups.google.com/group/comp.std.c/browse_thread/thread/77ee8c8f92e4a3fb/346fc464319b1ee5?pli=1
// http://stackoverflow.com/questions/16374776/macro-overloading
#define EXPAND(X) X
#define PP_NARG(...) \
    EXPAND( PP_NARG_(__VA_ARGS__, PP_RSEQ_N()) )
#define PP_NARG_(...) \
    EXPAND( PP_ARG_N(__VA_ARGS__) )
#define PP_ARG_N( \
     _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
    _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
    _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
    _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
    _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
    _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
    _61,_62,_63,  N, ...) N
#define PP_RSEQ_N() \
    63,62,61,60,                   \
    59,58,57,56,55,54,53,52,51,50, \
    49,48,47,46,45,44,43,42,41,40, \
    39,38,37,36,35,34,33,32,31,30, \
    29,28,27,26,25,24,23,22,21,20, \
    19,18,17,16,15,14,13,12,11,10, \
     9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define _CONCAT2(X,Y) X##Y
#define CONCAT2(X,Y) _CONCAT2(X,Y)
#define CONCAT3(X,Y,Z) CONCAT2(CONCAT2(X,Y),Z)
#define CONCAT4(X,...) CONCAT2(X,CONCAT3(__VA_ARGS__))
#define CONCAT(...) CONCAT2(CONCAT,PP_NARG(__VA_ARGS__))(__VA_ARGS__)

#define _NAMEUS2(X,Y) X##_##Y
#define NAMEUS2(X,Y) _NAMEUS2(X,Y)
#define NAMEUS3(X,Y,Z) NAMEUS2(X,NAMEUS2(Y,Z))
#define NAMEUS4(X,...) NAMEUS2(X,NAMEUS3(__VA_ARGS__))
#define NAMEUS(...) CONCAT2(NAMEUS,PP_NARG(__VA_ARGS__))(__VA_ARGS__)


#endif /* ISLPP_ISLPP_COMMON_H */
