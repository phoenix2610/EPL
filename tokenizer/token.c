#include <stdio.h>
#include <string.h>
#include "token.h"

/* ─────────────────────────────────────────────
   EPL Tokenizer — token.c
   Utility functions for creating, naming,
   and printing tokens.
   ───────────────────────────────────────────── */


/* ─────────────────────────────────────────────
   make_token
   Constructs a Token struct from its parts.
   Safe string copy — never overflows.
   ───────────────────────────────────────────── */
Token make_token(TokenType type, const char *value, int line, int col) {
    Token t;
    t.type = type;
    t.line = line;
    t.col  = col;

    /* Safe copy — truncate if somehow over limit */
    strncpy(t.value, value, MAX_TOKEN_LEN - 1);
    t.value[MAX_TOKEN_LEN - 1] = '\0';

    return t;
}


/* ─────────────────────────────────────────────
   token_type_name
   Returns a human-readable string for any
   TokenType. Used in error messages + debug.
   ───────────────────────────────────────────── */
const char *token_type_name(TokenType type) {
    switch (type) {

        /* Literals */
        case TOKEN_NUMBER:          return "NUMBER";
        case TOKEN_STRING:          return "STRING";
        case TOKEN_BOOLEAN:         return "BOOLEAN";
        case TOKEN_NULL:            return "NULL";

        /* Identifier */
        case TOKEN_IDENTIFIER:      return "IDENTIFIER";

        /* Core keywords */
        case TOKEN_SET:             return "SET";
        case TOKEN_UPDATE:          return "UPDATE";
        case TOKEN_AS:              return "AS";
        case TOKEN_TO:              return "TO";
        case TOKEN_BY:              return "BY";
        case TOKEN_FROM:            return "FROM";
        case TOKEN_WITH:            return "WITH";
        case TOKEN_AND:             return "AND";
        case TOKEN_OR:              return "OR";
        case TOKEN_NOT:             return "NOT";
        case TOKEN_IN:              return "IN";
        case TOKEN_OF:              return "OF";

        /* Control flow */
        case TOKEN_IF:              return "IF";
        case TOKEN_THEN:            return "THEN";
        case TOKEN_OTHERWISE:       return "OTHERWISE";
        case TOKEN_ELSE:            return "ELSE";
        case TOKEN_WHILE:           return "WHILE";
        case TOKEN_REPEAT:          return "REPEAT";
        case TOKEN_UNTIL:           return "UNTIL";
        case TOKEN_FOR:             return "FOR";
        case TOKEN_EACH:            return "EACH";
        case TOKEN_BREAK:           return "BREAK";
        case TOKEN_CONTINUE:        return "CONTINUE";
        case TOKEN_RETURN:          return "RETURN";

        /* Loop variants */
        case TOKEN_TIMES:           return "TIMES";
        case TOKEN_INDEFINITELY:    return "INDEFINITELY";
        case TOKEN_DOWN:            return "DOWN";

        /* Arithmetic */
        case TOKEN_PLUS:            return "PLUS";
        case TOKEN_MINUS:           return "MINUS";
        case TOKEN_TIMES_OP:        return "TIMES(OP)";
        case TOKEN_DIVIDED_BY:      return "DIVIDED_BY";
        case TOKEN_MODULO:          return "MODULO";
        case TOKEN_POWER_OF:        return "POWER_OF";

        /* Comparison */
        case TOKEN_EQUALS:          return "EQUALS";
        case TOKEN_IS:              return "IS";
        case TOKEN_NOT_EQUALS:      return "NOT_EQUALS";
        case TOKEN_IS_NOT:          return "IS_NOT";
        case TOKEN_GREATER_THAN:    return "GREATER_THAN";
        case TOKEN_LESS_THAN:       return "LESS_THAN";
        case TOKEN_AT_LEAST:        return "AT_LEAST";
        case TOKEN_AT_MOST:         return "AT_MOST";

        /* Advanced operators */
        case TOKEN_IS_ANY_OF:       return "IS_ANY_OF";
        case TOKEN_IS_NONE_OF:      return "IS_NONE_OF";
        case TOKEN_FOR_ALL:         return "FOR_ALL_VALUES";
        case TOKEN_FOR_ANY:         return "FOR_ANY_VALUE";
        case TOKEN_FOR_EVERY:       return "FOR_EVERY_VALUE";
        case TOKEN_RESPECTIVELY:    return "RESPECTIVELY";
        case TOKEN_LENGTH_OF:       return "LENGTH_OF";
        case TOKEN_SIZE_OF:         return "SIZE_OF";
        case TOKEN_ITEM:            return "ITEM";
        case TOKEN_ITEMS:           return "ITEMS";

        /* Output / Input */
        case TOKEN_SHOW:            return "SHOW";
        case TOKEN_DISPLAY:         return "DISPLAY";
        case TOKEN_PROVIDE:         return "PROVIDE";
        case TOKEN_WAIT:            return "WAIT";
        case TOKEN_USER:            return "USER";
        case TOKEN_INPUT:           return "INPUT";
        case TOKEN_ENTRY:           return "ENTRY";

        /* Activity keywords */
        case TOKEN_DESCRIBE:        return "DESCRIBE";
        case TOKEN_CHECK:           return "CHECK";
        case TOKEN_THAT:            return "THAT";
        case TOKEN_ATTEMPT:         return "ATTEMPT";
        case TOKEN_IF_FAILS:        return "IF_FAILS";
        case TOKEN_GIVEN_THAT:      return "GIVEN_THAT";

        /* Collection */
        case TOKEN_ADD:             return "ADD";
        case TOKEN_EMPTY:           return "EMPTY";

        /* System */
        case TOKEN_EXECUTE:         return "EXECUTE";
        case TOKEN_SYSTEM:          return "SYSTEM";
        case TOKEN_COMMAND:         return "COMMAND";
        case TOKEN_WINDOWS:         return "WINDOWS";

        /* Delimiters */
        case TOKEN_COMMA:           return "COMMA";
        case TOKEN_LPAREN:          return "LPAREN";
        case TOKEN_RPAREN:          return "RPAREN";
        case TOKEN_LBRACKET:        return "LBRACKET";
        case TOKEN_RBRACKET:        return "RBRACKET";

        /* Structure */
        case TOKEN_TERMINATOR:      return "TERMINATOR(!)";
        case TOKEN_NEWLINE:         return "NEWLINE";
        case TOKEN_COMMENT:         return "COMMENT";
        case TOKEN_MULTICOMMENT:    return "MULTICOMMENT";
        case TOKEN_EOF:             return "EOF";

        /* Error */
        case TOKEN_UNKNOWN:         return "UNKNOWN";

        default:                    return "UNRECOGNIZED";
    }
}


/* ─────────────────────────────────────────────
   print_token
   Prints a token in a clean debug format:
   [TYPE] "value"  (line X, col Y)
   ───────────────────────────────────────────── */
void print_token(Token token) {

    /* Comments and newlines clutter debug output
       so we print them more compactly           */
    if (token.type == TOKEN_NEWLINE) {
        printf("[NEWLINE] (line %d)\n", token.line);
        return;
    }

    if (token.type == TOKEN_COMMENT || token.type == TOKEN_MULTICOMMENT) {
        printf("[%s] \"%s\" (line %d)\n",
            token_type_name(token.type),
            token.value,
            token.line);
        return;
    }

    if (token.type == TOKEN_EOF) {
        printf("[EOF] (line %d)\n", token.line);
        return;
    }

    /* Standard token print */
    printf("[%-20s] \"%s\"  (line %d, col %d)\n",
        token_type_name(token.type),
        token.value,
        token.line,
        token.col);
}
