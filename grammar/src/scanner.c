// SPDX-License-Identifier: MPL-2.0
// External scanner for tree-sitter-ziz: off-side (indentation) layout.
//
// Emits INDENT / DEDENT / NEWLINE from an indent stack. Explicit
// S-expressions are layout-insensitive as DESIGN.adoc requires: no external
// token is valid inside ( ) [ ] { }, so the scanner is never asked for one
// there, and grammar.js lists \n as an extra so it is plain whitespace.
//
// A line break that closes several blocks owes the parser an alternating
// run NEWLINE DEDENT NEWLINE DEDENT ... NEWLINE (each layout_block is followed
// by the _newline that ends its layout_list). The whitespace is consumed by
// the first token; the rest are emitted zero-width from pending_dedents /
// pending_newline.

#include "tree_sitter/parser.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum TokenType { INDENT, DEDENT, NEWLINE, ERROR_SENTINEL };

#define MAX_INDENTS 128

typedef struct {
  uint16_t indents[MAX_INDENTS];
  uint8_t  depth;            // indent stack height (indents[0] is always 0)
  uint8_t  pending_dedents;  // DEDENTs still owed for the current line break
  uint8_t  pending_newline;  // a NEWLINE is owed after the DEDENT just emitted
} Scanner;

// Push an indentation column onto the stack (silently capped at MAX_INDENTS).
static inline void push(Scanner *s, uint16_t col) {
  if (s->depth < MAX_INDENTS) s->indents[s->depth++] = col;
}

// Allocate a scanner whose indent stack holds only column 0.
void *tree_sitter_ziz_external_scanner_create(void) {
  Scanner *s = (Scanner *)calloc(1, sizeof(Scanner));
  push(s, 0);
  return s;
}

// Free a scanner created by tree_sitter_ziz_external_scanner_create.
void tree_sitter_ziz_external_scanner_destroy(void *p) { free(p); }

// Write the scanner state into buf; returns the number of bytes written.
unsigned tree_sitter_ziz_external_scanner_serialize(void *p, char *buf) {
  Scanner *s = (Scanner *)p;
  unsigned n = 0;
  buf[n++] = (char)s->depth;
  buf[n++] = (char)s->pending_dedents;
  buf[n++] = (char)s->pending_newline;
  for (unsigned i = 0; i < s->depth && n + 1 < TREE_SITTER_SERIALIZATION_BUFFER_SIZE; i++) {
    buf[n++] = (char)(s->indents[i] & 0xff);
    buf[n++] = (char)(s->indents[i] >> 8);
  }
  return n;
}

// Restore scanner state written by ..._serialize (empty buf = fresh state).
void tree_sitter_ziz_external_scanner_deserialize(void *p, const char *buf, unsigned len) {
  Scanner *s = (Scanner *)p;
  memset(s, 0, sizeof(*s));
  if (len < 3) { push(s, 0); return; }
  unsigned n = 0;
  uint8_t depth = (uint8_t)buf[n++];
  s->pending_dedents = (uint8_t)buf[n++];
  s->pending_newline = (uint8_t)buf[n++];
  for (unsigned i = 0; i < depth && n + 1 < len; i++) {
    s->indents[s->depth++] = (uint16_t)((uint8_t)buf[n] | ((uint8_t)buf[n + 1] << 8));
    n += 2;
  }
  if (s->depth == 0) push(s, 0);
}

// Advance past a ';' comment up to (not including) the newline.
static void skip_comment(TSLexer *lx) {
  while (!lx->eof(lx) && lx->lookahead != '\n') lx->advance(lx, true);
}

// Emit one owed DEDENT: pop a level and owe the NEWLINE that must follow it.
static bool emit_dedent(Scanner *s, TSLexer *lx) {
  if (s->pending_dedents > 0) s->pending_dedents--;
  if (s->depth > 1) s->depth--;
  s->pending_newline = 1;
  lx->result_symbol = DEDENT;
  return true;
}

// Emit a NEWLINE token.
static bool emit_newline(TSLexer *lx) {
  lx->result_symbol = NEWLINE;
  return true;
}

// Recognise INDENT / DEDENT / NEWLINE at a line break or end of input.
bool tree_sitter_ziz_external_scanner_scan(void *p, TSLexer *lx, const bool *valid) {
  Scanner *s = (Scanner *)p;

  if (valid[ERROR_SENTINEL]) return false;

  // Tokens still owed from an earlier line break, emitted zero-width.
  if (s->pending_newline && valid[NEWLINE]) {
    s->pending_newline = 0;
    return emit_newline(lx);
  }
  if (s->pending_dedents > 0 && valid[DEDENT]) return emit_dedent(s, lx);

  bool saw_newline = false;
  uint16_t col = 0;

  for (;;) {
    if (lx->lookahead == '\n') {
      saw_newline = true; col = 0;
      lx->advance(lx, true);
    } else if (lx->lookahead == '\r') {
      lx->advance(lx, true);
    } else if (lx->lookahead == ' ') {
      col++; lx->advance(lx, true);
    } else if (lx->lookahead == '\t') {
      col = (uint16_t)((col / 8 + 1) * 8); lx->advance(lx, true);
    } else if (lx->lookahead == ';') {
      skip_comment(lx);
    } else if (lx->lookahead == '\\') {
      lx->mark_end(lx);
      lx->advance(lx, true);
      if (lx->lookahead == '\r') lx->advance(lx, true);
      if (lx->lookahead == '\n') { lx->advance(lx, true); col = 0; continue; }
      return false;
    } else {
      break;
    }
  }

  bool at_eof = lx->eof(lx);
  if (!saw_newline && !at_eof) return false;   // mid-line: not a layout point
  if (at_eof) col = 0;                         // EOF closes every open block

  uint16_t top = s->indents[s->depth - 1];

  if (col > top) {
    if (valid[INDENT]) { push(s, col); lx->result_symbol = INDENT; return true; }
    if (valid[NEWLINE]) return emit_newline(lx);
    return false;
  }

  if (col < top) {
    uint8_t levels = 0;
    for (int i = s->depth - 1; i > 0 && s->indents[i] > col; i--) levels++;
    // The current line must end first; the DEDENTs follow zero-width.
    if (valid[NEWLINE]) { s->pending_dedents = levels; return emit_newline(lx); }
    if (valid[DEDENT])  { s->pending_dedents = levels; return emit_dedent(s, lx); }
    return false;
  }

  if (valid[NEWLINE]) return emit_newline(lx);
  return false;
}
