#pragma once

/** https://github.com/swansontec/map-macro
 *  
 *  The end goal is to create a MAP macro which works like this
 *  
 *     #define PRINT(a) printf(#a": %d", a)
 *      MAP(PRINT, a, b, c) // Apply PRINT to a, b, and c
**/

// Part 1: Basic recursion
// --------------------------------------------------------------------------------------------------------------------

/**
 *  Imagine we have the following macros:
 *  
 *     #define A(x) x B MAP_OUT (x)
 *     #define B(x) x A MAP_OUT (x)
 * 
 *   Evaluating the macro A (blah) produces the output text:
 * 
 *      blah B (blah)
 * 
 *   The preprocessor doesn't see any recursion yet. Feeding this back into the prepocessor expands the 
 *   call, producing the output:
 * 
 *       blah blah A (blah)
 * 
 *   Evaluating the output a third time expands the A (blah) macro, carrying the recursion full-circle. 
 *   The recursion continues as long as the caller continues to feed the output text back into the preprocessor.
**/

// To perform these repeated evaluations, the following EVAL macro passes its arguments down a tree of macro calls:
#define EVAL0(...) __VA_ARGS__
#define EVAL1(...) EVAL0(EVAL0(EVAL0(__VA_ARGS__)))
#define EVAL2(...) EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL3(...) EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL4(...) EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#define EVAL(...)  EVAL4(EVAL4(EVAL4(__VA_ARGS__)))
/**
 * Each level of the EVAL macro multiplies the effort of the previous level by 3, but also adds one evaluation of its own. 
 * Invoking the macro as a whole adds one more level, taking the total to:
 *     
 *       1 + (3 * (3 * (3 * (3 * (3 * (1) + 1) + 1) + 1) + 1) + 1) = 365
 * 
 * So we have a max stack depth of 365! This will fail if our list has >365 elements
**/

// Part 2: End detection
// --------------------------------------------------------------------------------------------------------------------

// The idea is to emit the following macro name instead of the normal recursive macro when its time to quit
#define MAP_END(...)
// Evaluating this macro does nothing, ending the recursion

#define MAP_OUT
#define MAP_COMMA ,

// To select b/t the 2 macros, the following macro compares one list item against the special end of list marker, ().
// The macro returns MAP_END if the item matches, or the next param if the item is anything else
#define MAP_GET_END2() 0, MAP_END
#define MAP_GET_END1(...) MAP_GET_END2
#define MAP_GET_END(...) MAP_GET_END1
// trick to prevent the preprocessor from evaluating the final result
#define MAP_NEXT0(test, next, ...) next MAP_OUT
#define MAP_NEXT1(test, next) MAP_NEXT0(test, next, 0)
#define MAP_NEXT(test, next)  MAP_NEXT1(MAP_GET_END test, next)
// Works by placing item test to the MAP_GET_END macro ^. If doing that forms a macro call, everything moves over a slot
// in the MAP_NEXT0 param list, changing the output. 

// Part 3: Put it together
// --------------------------------------------------------------------------------------------------------------------

// These macros apply operation f to the current list item x. They then examine the next list item, peek, to see if they
// should continue or not
#define MAP0(f, x, peek, ...) f(x) MAP_NEXT(peek, MAP1)(f, peek, __VA_ARGS__)
#define MAP1(f, x, peek, ...) f(x) MAP_NEXT(peek, MAP0)(f, peek, __VA_ARGS__)

#define MAP_LIST_NEXT1(test, next) MAP_NEXT0(test, MAP_COMMA next, 0)
#define MAP_LIST_NEXT(test, next)  MAP_LIST_NEXT1(MAP_GET_END test, next)

#define MAP_LIST0(f, x, peek, ...) f(x) MAP_LIST_NEXT(peek, MAP_LIST1)(f, peek, __VA_ARGS__)
#define MAP_LIST1(f, x, peek, ...) f(x) MAP_LIST_NEXT(peek, MAP_LIST0)(f, peek, __VA_ARGS__)

/**
 * Applies the function macro `f` to each of the remaining parameters.
 */
#define MAP(f, ...) EVAL(MAP1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

/**
 * Applies the function macro `f` to each of the remaining parameters and
 * inserts commas between the results.
 */
#define MAP_LIST(f, ...) EVAL(MAP_LIST1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))