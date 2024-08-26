/*
    WDL - turtleparse.h
    Copyright (C) 2021, Cockos Incorporated

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

    turtle RDF parser
*/

#ifndef _WDL_TURTLE_PARSE_H_
#define _WDL_TURTLE_PARSE_H_
#include "ptrlist.h"
#include "assocarray.h"
#include "wdlstring.h"
#include "wdlutf8.h"



class wdl_turtle_pair {
  enum { MODE_RESOURCE=0, MODE_LIST, MODE_COLLECTION, MODE_BLANK, MODE_LITERAL_TYPELESS };

  char *verb, *value; // verb can be NULL to indicate "a"
  UINT_PTR mode; // MODE_* or pointer to string which is @lang or type string (minus ^^)
public:
  wdl_turtle_pair(const char *v, const char *_res, const char *_lit, const char *_lit_type, int _list) // _list=2 for collection
  {
    WDL_ASSERT((!!_res) + (!!_lit) + (!!_list) <= 1);
    verb = v ? strdup(v) : NULL;
    if (_lit)
    {
      value = strdup(_lit);
      mode = _lit_type && *_lit_type ? (UINT_PTR) strdup(_lit_type) : MODE_LITERAL_TYPELESS;
    }
    else if (_res)
    {
      value = strdup(_res);
      mode = MODE_RESOURCE;
    }
    else if (_list)
    {
      value = (char *)new WDL_PtrList<wdl_turtle_pair>;
      mode = _list == 2 ? MODE_COLLECTION : MODE_LIST;
    }
    else
    {
      value = NULL;
      mode = MODE_BLANK;
    }
  }
  ~wdl_turtle_pair()
  {
    free(verb);
    if (mode > MODE_LITERAL_TYPELESS) free((void *)mode);

    WDL_PtrList<wdl_turtle_pair> *list = get_list();
    if (!list) list = get_collection();
    if (list) { list->Empty(true); delete list; }
    else free(value);
  }
  static void deletePair(wdl_turtle_pair *p) { delete p; }

  const char *get_verb() { return verb; } // NULL indicates "a"
  const char *get_resource() { return mode == MODE_RESOURCE ? value : NULL; } // object is a uri <http://foo>, returned as "http://foo", and _not_ URL-decoded (if resolving a path, caller should convert %20 etc)
  const char *get_literal() { return mode >= MODE_LITERAL_TYPELESS ? value : NULL; } // object is a literal "string" or """string""" or 1.2345 or true/false, etc
  const char *get_literal_type() { return mode > MODE_LITERAL_TYPELESS ? (const char *)mode : NULL; }
  WDL_PtrList<wdl_turtle_pair> *get_list() { return mode == MODE_LIST ? ( WDL_PtrList<wdl_turtle_pair>*) value : NULL; } // object is a [] list
  WDL_PtrList<wdl_turtle_pair> *get_collection() { return mode == MODE_COLLECTION ? ( WDL_PtrList<wdl_turtle_pair>*) value : NULL; } // object is a collection (),  ignore verb for these items!
};

class wdl_turtle_parser {
public:

  wdl_turtle_parser() :
     objects(true, wdl_turtle_pair::deletePair),
     m_prefixes(true, WDL_StringKeyedArray<char *>::freecharptr),
     m_error_ptr(NULL), m_error_msg(NULL) { }
  ~wdl_turtle_parser() { }

  // results generated by parse
  WDL_StringKeyedArray<wdl_turtle_pair*> objects; // will always be lists

  const char *m_error_ptr, *m_error_msg;

  void parse(const char *str, const char *str_end, const char *use_base="")
  {
    // note: specifically does not clear objects, so you can parse multiple files and get the union of them.
    m_prefixes.DeleteAll();
    m_base.Set(use_base);
    m_error_ptr = m_error_msg = NULL;
    WDL_FastString fs1,fs2;
    for (;;)
    {
      int tok_l;
      const char *tok = next_tok(&str,str_end, &tok_l);
      if (!tok) return;

      if (tok_l == 7 && !memcmp(tok,"@prefix",7))
      {
        tok = next_tok(&str,str_end,&tok_l);
        if (!tok) { on_err("@prefix missing name",str); return; }
        if (!tok_to_str(tok,tok_l,&fs1,false) || !fs1.GetLength() || fs1.Get()[fs1.GetLength()-1] != ':')
        {
          on_err("@prefix parameter must end in :",tok);
          return;
        }
        tok = next_tok(&str,str_end,&tok_l);
        if (!tok) { on_err("@prefix missing uriref",str); return; }
        if (tok_to_str(tok,tok_l,&fs2,true) != '<') { on_err("@prefix invalid uriref",tok); return; }

        m_prefixes.Insert(fs1.Get(),strdup(fs2.Get()));

        tok = next_tok(&str,str_end,&tok_l);
        if (!tok || tok_l != 1 || *tok != '.') { on_err("@prefix expected trailing .",tok?tok:str); return; }
      }
      else if (tok_l == 5 && !memcmp(tok,"@base",5))
      {
        tok = next_tok(&str,str_end,&tok_l);
        if (!tok) { on_err("@base missing uriref",str); return; }
        if (tok_to_str(tok,tok_l,&m_base,true) != '<') { on_err("@base invalid uriref",tok); return; }
        if (!tok || tok_l != 1 || *tok != '.') { on_err("@base expected trailing .",tok?tok:str); return; }
      }
      else
      {
        if (tok_to_str(tok,tok_l,&fs1,true) != '<') { on_err("expected IRI to begin triple",tok); return; }

        wdl_turtle_pair *list = objects.Get(fs1.Get());
        if (!list)
        {
          list = new wdl_turtle_pair(NULL,NULL,NULL,NULL,1);
          objects.Insert(fs1.Get(),list);
        }
        if (!parse_list(&str,str_end,list->get_list(),'.')) return;
      }
    }
  }

  static int is_number_tok(const char *p, const char *str_end)
  {
    int i = 0, f = 0; // 1=has number before e  2=has . 4=has e/E, 8=has number after e
    while (p+i < str_end)
    {
      if (p[i] >= '0' && p[i] <= '9') f |= f>3 ? 8 : 1;
      else if (p[i] == '.') { if (f>1) return 0; f|=2; }
      else if (p[i] == 'e' || p[i] == 'E')
      {
        if ((f&4) || !(f&1) || p+i+1 >= str_end) return 0;
        if (p[i+1] == '+' || p[i+1] == '-') i++;
        f|=4;
      }
      else if (p[i] == '+' || p[i] == '-') { if (i) return 0; }
      else break;
      i++;
    }
    if (!(f&1) || (f&(4|8)) == 4) return 0;
    return i;
  }


protected:
  WDL_FastString m_base; // @base
  WDL_StringKeyedArray<char *> m_prefixes; // @prefix

  static bool is_single_char_tok(int c) { return c == '.' || c == ',' || c == ';' || c == '(' || c == ')' || c == '[' || c == ']'; }
  static bool is_ws(int c) { return c == '\t' || c == '\r' || c == '\n' || c == ' '; }

  void on_err(const char *msg, const char *rdptr)
  {
    if (!m_error_msg && msg) m_error_msg = msg;
    if (!m_error_ptr && rdptr) m_error_ptr = rdptr;
  }

  const char *next_tok(const char **str, const char *str_end, int *toklen)
  {
    const char *tok_start = *str;
    for (;;)
    {
      while (tok_start < str_end && is_ws(*tok_start)) tok_start++;
      if (tok_start >= str_end) { *str = str_end; return NULL; }
      if (*tok_start != '#') break;
      while (tok_start < str_end && *tok_start != '\r' && *tok_start != '\n') tok_start++;
    }

    if (tok_start[0] == '"' || tok_start[0] == '\'')
    {
      const char term_c = tok_start[0], *p = tok_start + 1;
      if (tok_start+2 < str_end && tok_start[1] == term_c && tok_start[2] == term_c)
      {
        p+=2;
        while (p < str_end)
        {
          if (p+2 < str_end && p[0] == term_c && p[1] == term_c && p[2] == term_c)
          {
            *toklen = (int) ((*str = p+3) - tok_start);
            return tok_start;
          }
          if (decode_escape(&p,str_end,true) < 0)
          {
            on_err("invalid escape sequence in long quote", p);
            return NULL;
          }
        }
        on_err("unterminated long quote",tok_start);
        return NULL;
      }
      while (p < str_end)
      {
        if (p[0] == term_c)
        {
          *toklen = (int) ((*str = p+1) - tok_start);
          return tok_start;
        }
        if (p[0] == '\r' || p[0] == '\n' || decode_escape(&p,str_end,true) < 0)
        {
          on_err(p[0] == '\\' ? "invalid escape sequence in quote" : "invalid character in quote", p);
          return NULL;
        }
      }
      on_err("unterminated quote", tok_start);
      return NULL;
    }

    if (tok_start[0] == '<')
    {
      const char *p = tok_start + 1;
      while (p < str_end)
      {
        if (p[0] == '>')
        {
          *toklen = (int) ((*str = p+1) - tok_start);
          return tok_start;
        }

        if ((p[0] >= 0 && p[0] <= 0x20) || p[0] == '<' || p[0] == '|' ||
             p[0] == '"' || p[0] == '{' || p[0] == '}' || p[0] == '`' ||
             decode_escape(&p,str_end,false) < 0
             )
        {
          on_err(p[0] == '\\' ? "invalid escape sequence in IRI" : "invalid character in IRI", p);
          return NULL;
        }
      }
      on_err("unterminated IRI", tok_start);
      return NULL;
    }

    const char *p = tok_start;
    int nlen;
    bool had_colon = false;
    if ((nlen = is_number_tok(p,str_end))) p+=nlen;
    else if (is_single_char_tok(*p)) p++;
    else while (p < str_end && !is_ws(*p))
    {
      if (*p == '<') break;
      else if (*p == ':') had_colon = true;
      else if (had_colon && *p == '.' && (p+1) < str_end && !is_ws(p[1]) && !is_single_char_tok(p[1])) { }
      else if (is_single_char_tok(*p)) break;
      p++;
    }
    *toklen = (int)((*str = p) - tok_start);
    return tok_start;
  }

  static int decode_escape(const char **p, const char *endp, bool allow_echar)
    // if string is not beginning with '\\', returns unsigned raw character value, advances p by 1
    // if valid escape sequence, returns non-negative value (may be UTF-8 codepoint), advances p by length (2 or more)
    // if error occurs, returns -1
  {
    const char *rd = *p;
    if (*rd != '\\')
    {
      *p += 1;
      return (int) (unsigned char) *rd;
    }
    if (++rd >= endp) return -1;

    if (*rd == 'u' || *rd == 'U')
    {
      const int hexlen = *rd++ == 'u' ? 4 : 8;
      unsigned int rv = 0;
      if (rd + hexlen > endp) return -1;
      for (int x = 0; x < hexlen; x ++)
      {
        const char c = *rd++;
        if (c >= '0' && c <= '9') rv = rv*16 + (c - '0');
        else if (c >= 'A' && c <= 'F') rv = rv*16 + 10 + (c - 'A');
        else if (c >= 'a' && c <= 'f') rv = rv*16 + 10 + (c - 'a');
        else return -1;
      }
      if (rv >= 0x200000) return -1;
      *p += 2 + hexlen;
      return (int)rv;
    }

    if (!allow_echar) return -1;

    int rv;
    switch (*rd)
    {
      case 'b': rv = '\b'; break;
      case 'f': rv = '\f'; break;
      case 't': rv = '\t'; break;
      case 'n': rv = '\n'; break;
      case 'r': rv = '\r'; break;
      case '"': rv = '"'; break;
      case '\'': rv = '\''; break;
      case '\\': rv = '\\'; break;
      default: return -1;
    }
    *p += 2;
    return rv;
  }

  int tok_to_str(const char *tok, int toklen, WDL_FastString *fs, bool allow_decode)
    // returns '"' for string (regardless of whether it was long or single quoted),
    // or '<' for IRI
    // or 1 for unquoted literal
    // 0 for failure
  {
    int mode = 1, rv = 1;
    fs->Set("");
    if (allow_decode)
    {
      if (toklen >= 6 && (tok[0] == '"' || tok[0] == '\'') && tok[1] == tok[0] && tok[2] == tok[0])
      {
        rv = mode = tok[0];
        WDL_ASSERT(tok[toklen-1] == tok[0] && tok[toklen-2] == tok[0] && tok[toklen-3] == tok[0]);
        tok += 3;
        toklen -= 6;
      }
      else if (toklen >= 2 && (tok[0] == '"' || tok[0] == '\'' || tok[0] == '<'))
      {
        rv = mode = tok[0];
        WDL_ASSERT(tok[toklen-1] == (mode == '<' ? '>' : mode));
        toklen-=2;
        tok++;
      }
      else
      {
        int l;
        for (l = 0; l < toklen && tok[l] != ':'; l++);
        if (l < toklen)
        {
          char tmp[1024];
          lstrcpyn_safe(tmp, tok, wdl_min(sizeof(tmp), l+2));
          const char *pf = m_prefixes.Get(tmp);
          if (pf)
          {
            fs->Set(pf);
            toklen -= l+1;
            tok += l+1;
            rv = '<';
          }
        }
      }
    }

    const char *endp = tok+toklen;
    while (tok < endp)
    {
      if (mode > 1 && *tok == '\\')
      {
        const int cv = decode_escape(&tok,endp, mode!='<');
        if (cv < 0)
        {
          on_err(mode == '<' ? "invalid escape sequence in IRI" : "invalid escape sequence in quote", tok);
          return 0;
        }
        char tmp[8];
        const int tmpl = wdl_utf8_makechar(cv,tmp,sizeof(tmp));
        if (tmpl <= 0)
        {
          on_err(mode == '<' ? "invalid unicode codepoint in IRI" : "invalid unicode codepoint in quote", tok);
          return 0;
        }
        fs->Append(tmp,tmpl);
      }
      else
        fs->Append(tok++,1);
    }
    if (mode == '<' && m_base.GetLength())
    {
      const char *p = fs->Get();
      if (*p == '#' || *p == 0)
      {
        // <#foo> or <> are relative to base exactly
        fs->Insert(m_base.Get(),0);
      }
      else
      {
        while (*p && *p != ':' && *p != '#') p++;
        if (*p != ':') // relative path if no : found prior to any #
        {
          int baselen = m_base.GetLength();
          // <bar> is resolved relative to base minus base's filepart
#ifdef _WIN32
          while (baselen > 0 && !WDL_IS_DIRCHAR(m_base.Get()[baselen-1])) baselen--;
#else
          while (baselen > 0 && m_base.Get()[baselen-1] != '/') baselen--;
#endif
          if (WDL_NOT_NORMALLY(!baselen))
          {
            // base has no /, this is probably malformed
            fs->Insert("/",0);
          }
          fs->Insert(m_base.Get(),0,baselen);
        }
      }
    }
    if (rv == '\'') rv = '"';
    return rv;
  }

  bool parse_list(const char **str, const char *str_end, WDL_PtrList<wdl_turtle_pair> *listOut, int endchar)
  {
    if (WDL_NOT_NORMALLY(listOut == NULL)) return false;
    WDL_FastString verb_str, s, langtype;
    const char *has_verb = NULL;
    int state = 0, tok_l;
    for (;;)
    {
      const char *tok = next_tok(str,str_end,&tok_l);
      if (!tok) { on_err("unterminated list",*str); return false; }

      if (endchar == ')')
      {
        if (tok[0] == ')') return true;
        state=1;
      }
      else if (tok[0] == endchar)
      {
        if (state!=1) return true;
        on_err("premature list terminator",tok);
        return false;
      }
      if ((tok_l==1 && tok[0] == '.') || tok[0] == ']' || tok[0] == ')') { on_err("invalid character in list context",tok); return false; }

      switch (state)
      {
        case 0:
          if (tok_l == 1 && tok[0] == 'a') has_verb = NULL;
          else if (tok_to_str(tok,tok_l,&verb_str,true) == '<' && verb_str.GetLength()) has_verb = verb_str.Get();
          else { on_err("invalid verb in list",tok); return false; }
          state++;
        break;
        case 1:
          if (tok[0] == '[')
          {
            wdl_turtle_pair *list = new wdl_turtle_pair(has_verb, NULL,NULL,NULL,1);
            listOut->Add(list);
            if (!parse_list(str,str_end,list->get_list(),']')) return false;
          }
          else if (tok[0] == '(')
          {
            wdl_turtle_pair *list = new wdl_turtle_pair(has_verb, NULL,NULL,NULL,2);
            listOut->Add(list);
            if (!parse_list(str,str_end,list->get_collection(),')')) return false;
          }
          else
          {
            int t = tok_to_str(tok,tok_l,&s,true), has_langtype = 0;
            if (!t) { on_err("error parsing object",tok); return false; }
            if (t != '<')
            {
              if (t == '"')
              {
                const char *peek = *str;
                if (peek < str_end && peek[0] == '@')
                {
                  // this is a bit more permissive than the spec. include @ in string
                  tok = next_tok(str,str_end,&tok_l);
                  if (!tok || tok != peek) { on_err("literal has incorrect language specification",peek); return false; }
                  has_langtype++;
                  langtype.Set(tok,tok_l);
                }
                else if (peek+1 < str_end && peek[0] == '^' && peek[1] == '^')
                {
                  peek = (*str += 2); // do not include ^^ in token
                  tok = next_tok(str,str_end,&tok_l);
                  if (!tok || tok != peek || tok_to_str(tok,tok_l,&langtype,true) == '"') { on_err("literal has incorrect type specification",peek); return false; }
                  has_langtype++;
                }
              }
            }
            listOut->Add(new wdl_turtle_pair(has_verb, t == '<' ? s.Get() : NULL, t == '<' ? NULL : s.Get(), has_langtype ? langtype.Get() : NULL, 0));
          }
          state++;
        break;
        case 2:
          if (tok[0] == ';') state=0;
          else if (tok[0] == ',') state = 1;
          else { on_err("unknown token in list",tok); return false; }
        break;
      }
    }
  }
};


#endif
